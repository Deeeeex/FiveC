# 服务器端程序设计文档

## 系统概述

为了实现广域网五子棋连接对战，即在不同局域网下的机器互相连接，且在不考虑内网穿透等实现手段的情况下，我们需要一个服务器端，用于对战双方数据的转发，服务器端程序部署于云服务器，用于示例的服务器IP地址为``101.34.252.176``，服务提供商为[腾讯云](https://cloud.tencent.com/)。

## 运行环境

用于示例的云服务器系统环境为``CentOS 7.6 64bit``

## 功能设计

在服务器端程序开启的情况下，会监听三个端口，``CONF_PORT``，``HOST_PORT``，``GUEST_PORT``。其中，``CONF_PORT``用于接收客户端程序的申请，另外两个端口用于建立连接，实现信息的转发。具体过程如下：

1. 客户端向服务器端发出申请，连接到``CONF_PORT``。
2. 服务器端向``CONF_PORT``发送出当前的游戏状态``Game_Status``，分为``VACANT``，``WAITING``，``ONGOING``，分别表示空闲，已有玩家等待中，以及游戏进行中。
   * 若当前为空闲，则当前申请的客户端将与``HOST_PORT``建立连接，等待另一位玩家进入游戏，并将游戏状态``Game_Status``修改为``WAITING``。
   * 若当前为等待，则当前申请的客户端将与``GUEST_PORT``建立连接，开始游戏，并将并将游戏状态``Game_Status``修改为``ONGOING``。
   * 若当前为游戏中，则断开连接，并返回信息。

3. 当双方玩家都分别建立连接后，开始进行对战，服务器同时监听双方的信息，并不作修改地转发。
4. 当一局游戏结束之后，玩家断开连接，将游戏状态``Game_Status``修改为``VACANT``，并且还原为最初监听状态。

## 关键算法设计

### Socket

由于需要实现与客户端的信息交互，因此使用``Socket``进行信息的传递。为了避免信息的丢失，保证消息传递的可靠，使用``TCP``协议，流式传输信息。

分别在不同子进程中使用``Socket``，和不同端口进行消息的接收与发送。以``HOST_PORT``为例，主要代码如下所示：

1. 申请``Socket``（``fd``表示文件描述符，用于接收``socket()``函数的返回值）

   ```c
   int socket_fd_Guest = socket(AF_INET, SOCK_STREAM, 0);//Apply for a socket
   ```

2. 本地地址初始化及绑定Socket：

   ```c
   struct sockaddr_in  servaddr_Host;
   memset(&servaddr_Host, 0, sizeof(servaddr_Host));
   servaddr_Host.sin_family = AF_INET;
   servaddr_Host.sin_addr.s_addr = htonl(INADDR_ANY);	//Listen to all Addresses
   servaddr_Host.sin_port = htons(HOST_PORT);			//Set listening port as default
   bind(socket_fd_Host, (struct sockaddr*)&servaddr_Host, sizeof(servaddr_Host)；//Bind
   ```

   将Socket设置为对所有IP地址进行监听。

3. 在子进程中开启对于申请的监听：

   ```c
   listen(socket_fd_Host, 10);
   int connect_fd_Host = accept(socket_fd_Host, (struct sockaddr*)NULL, NULL);
   ```

   新建一个描述符``connnect_fd_Host``用于接收``accept()``传递回的Socket描述符。

4. 对消息的读取和发送：

   ```c
   recv(connect_fd_Guest, buff_H, 4096, 0);
   send(connect_fd_Guest, buff_G, length, 0);
   ```

   其中``buff_H``和``buff_G``分别是接收和发送消息的缓冲区，为``char*``类型。

### 多进程

为了实现对三个端口的同时监听，以及互不干扰的并行运行，选择使用``MultiProcess``实现这项功能。当服务器处于正常监听状态时，进程树如下：

```c
Server(4257)─┬─Server(15329)//Listening to CONF_PORT
             ├─Server(15330)//Listening to HOST_PORT
             └─Server(15331)//Listening to GUEST_PORT
```

如图，主进程``Server（4257）``有三个子进程，分别实现对于三个端口的监听。

当``CONF_PORT``收到来自客户端的申请时，会从``Game_Status``文件获取当前的游戏状态，并将其发送给发出申请的客户端。

当游戏成功建立的时候，监听``HOST_PORT``和``GUEST_PORT``的两个进程将分别``fork()``一个子进程，用于向对应端口发送信息，实现发送和监听的分离。在这种状态下，进程树如图所示：

```c
Server(4257)─┬─Server(15329)//Listening to CONF_PORT
             ├─Server(15330)-Server(15897)//Listening to HOST_PORT
             └─Server(15331)-Server(16372)//Listening to GUEST_PORT
```

### 进程间通信

由于程序涉及到多个进程，而不同进程之间的变量是不共享的，且不同进程之间有通信的需求，因此需要使用一些方法，实现进程之间的通信。这里主要用了三种方法：共享文件，管道通信，以及信号机制。

#### 共享文件

关于游戏状态``Game_Status``的存储和读取，选择使用文件，在目录下建立一个``Game_Status``文件，用于存储当前的游戏状态。服务器程序每次运行的时候，都会将其初始化为``VACANT``，以供后续进程读取和修改。

读取过程如代码所示：

```c
Game_Status_fd = open("Game_Status", O_RDONLY);
read(Game_Status_fd, &Game_Status, sizeof(int));
close(Game_Status_fd);
```

其中``Game_Status_fd``和``Game_Status``均为``int``型变量，``Game_Status_fd``用作文件描述符，``Game_Status``用于读取文件中存储的单个``int``型数据，表示不同状态，如下：

```c
#define VACANT 0
#define WAITING 1
#define ONGOING 2
```

当``HOST``玩家建立连接时，将会对``Game_Status``文件进行修改，修改过程如代码所示：

```c
Game_Status = WAITING;
Game_Status_fd = open("Game_Status", O_WRONLY);
write(Game_Status_fd, &Game_Status, sizeof(int));
close(Game_Status_fd);
```

由此，可以实现对游戏状态的共享访存。

#### 管道通信

在双方都建立连接时，总共有四个进程，分别实现双方消息的接收和发送，这就需要在不同进程之间互相传递信息，才能实现消息的实时转发，这里选择使用匿名管道``pipe()``来实现此项功能。

1. 主进程中初始化两个管道：

   ```c
   if(pipe(fd_1) == -1)
   {
       perror("pipe");
       return -1;
   }
   int *write_H2G = &fd_1[1];
   int *read_H2G = &fd_1[0];//H2G:Host to Guest
   
   if(pipe(fd_2) == -1)
   {
       perror("pipe");
       return -1;
   }
   int *write_G2H = &fd_2[1];
   int *read_G2H = &fd_2[0];//G2H:Guest to Host
   ```

   其中``fd_1``和``fd_2``均为``int[2]``数组，而``write_H2G``，``read_H2G``，``write_G2H``，``read_G2H``是为了后面调用方便而初始化的别名。

2. 初始化管道之后，主进程分别``fork()``两个子进程用于实现信息交流，并在``fork()``结束之后关闭自己的四个读写端口：

   ```c
   close(*write_H2G);
   close(*read_G2H);
   close(*write_G2H);
   close(*read_H2G);
   ```

   两个子进程分别关闭与对方对应的读写端口：

   * ``Host``进程：

     ```c
     close(*write_G2H);
     close(*read_H2G);
     ```

   * ``Client``进程：

     ```c
     close(*write_H2G);
     close(*read_G2H);
     ```

3. 这样实现消息的实时转发（以``Host``进程为例）：

   * 每当Socket传来消息的时候，都将消息不作改动地传递至管道的写端：

     ```c
     while(1)
     {
         n = recv(connect_fd_Host, buff_H, 4096, 0);
         if (n == -1 || n == 0)
         {//when connexion shut down
             kill(My_Child, SIGKILL);
             close(connect_fd_Host);
             exit(0);
         }
         write(*write_H2G, buff_H, n);
     }
     ```

     每当没有新消息时，便阻塞于``recv()``函数处。

   * 新开一个子进程，实现：每当管道另一端有新消息传递来，将消息不做改动地经有Socket发送出去：

     ```c
     while(1)
     {
         length = read(*read_G2H, buff_G, 4096);
         send(connect_fd_Host, buff_G, length, 0);
     }
     ```

由此，可以实现消息的实时转发

#### 信号机制

通过以上几种方式，已经可以实现消息的转发，但还存在一些问题。如在程序终止时，主进程的结束不会让各个子进程也结束，以及子进程没有良好的结束和回收机制，因此会造成僵尸进程和孤儿进程的存在。这是需要解决的，我们希望在主进程结束的时候，子进程也能同步结束，以及当有某一方断开连接的时候，各个子进程能够结束，同时让主进程回到最原初的监听状态。

这里选择使用Linux系统的信号（``SIGNAL``）机制来解决这些问题。

* 在主进程中，为了在接收外界信号的同时能够杀死所有的子进程，定义了这样的处理函数：

  ```c
  void handler(int signum)
  {
      int i;
      for (i = 0; i < 3; i++)
          kill(pid[i], SIGUSR1);
      exit(0);
  }
  ```

  其中``pid[3]``为``pid_t``型数组，存储了三个子进程的``pid``。

  同时在主进程中将信号``SIGINT``和``SIGTERM``与之连接：

  ```c
  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  ```

  由此可以实现，主进程在收到``ctrl+C``的信号以及系统``kill``指令时，可以向所有的子进程发送一个自定义信号``SIGUSR1``。

* 在子进程中，为了在接收主进程信号的同时能够杀死对应的子进程，定义了这样的处理函数：

  ```c
  void Be_Killed_Handler(int signum)
  {
      kill(My_Child, SIGKILL);
      exit(0);
  }
  ```

  其中``My_Child``为``pid_t``型变量，存储了各自对应的子进程``pid``。

  并在子进程中将信号``SIGUSR1``与之相连：

  ```c
  signal(SIGUSR1, Be_Killed_Handler);
  ```

  由此可以实现，当子进程收到来自主进程的``SIGUSR1``时，可以杀死孙进程，然后自我结束。

* 在孙进程中，为了防止变成孤儿进程，定义了这样的处理函数：

  ```c
  void Pipe_Handler(int signum)
  {
      exit(0);
  }
  ```

  并在子进程中将信号``SIGPIPE``与之相连：

  ```c
  signal(SIGPIPE, Pipe_Handler);
  ```

  由此可以实现，在管道另一端关闭时，孙进程自己结束，而不会在对应子进程结束后成为孤儿进程。

#### 其他

除了以上三种方式之外，还在主进程循环的末尾有这样的语句：

```c
int Exit_Pid = wait(NULL);

int i;
for (i = 0; i < 3; i++)
	kill(pid[i], SIGUSR1);
```

主进程会阻塞在这里，等待着第一个子进程的结束，一旦有一个子进程结束，主进程将会结束所有的子进程，然后回到循环的开头，回到等待连接的状态。

## 部署与运行

服务器端代码命名为``Server.c``，在``Linux``服务器上编译运行的指令如下：

```
$ gcc Server.c -o Server
$ nohup ./Server &
```

这里使用``nohup``以及``&``是为了让程序能够在后台运行，并且关闭终端也能持续运行。

## 心得与展望

服务端程序的编写涉及到了很多Linux操作系统以及计算机网络的的内容，涉及范围较广，对于个人能力提升较大。后期考虑添加账户系统，以及排名系统，使之作为联网游戏更加完整。

