# 程序设计文档

### 创意来源

目前在Windows端多见本地的棋类游戏，联机的棋类游戏大多为各大平台所有，而且大多掺杂很多广告，游戏体验一般，于是希望尝试用自己的所学知识做一个基于Qt的联机版五子棋游戏（网络间实时对战功能主要基于TCP协议实现），同时支持人机对战、人人对战（本地）和人人对战（联机）三种游戏模式，每种模式也各自支持投降和悔棋功能，为玩家提供更稳定、更舒适的游戏体验。

### 运行逻辑&游戏框架

整体游戏分为客户端和服务器端，其中客户端负责整个棋盘的布局和操作等，服务器端用于联网模式下双方对战信息的转发。客户端的框架建立在Qt上，版本为5.12.11，`IDE`使用`Qt Creater 4.15.0`，使用`qmake`编译生成可执行文件。服务器端程序部署于云服务器，用于示例的服务器`IP`地址为`101.34.252.176`，服务提供商为腾讯云，系统环境为`CentOS 7.6 64bit`。

为了减轻服务器端的负担以及传送信息的复杂度，联网模式采取的策略是：将落子的逻辑判断全部放在本地的客户端进行，每一次玩家的操作在本地客户端的棋盘上更新的同时向服务器发送相应的操作，服务器收到信息后转发给另一端的玩家，对方收到后会对本地的棋盘同步更新，并开始自己的回合。

整个游戏可以拆分为三个部分：**负责与玩家交互的`MainWindow`部分，负责对棋盘进行更新和逻辑判断的`GameModel`部分，负责判断当前已有游戏人数和转发操作信息的`server`部分。**

### 游戏规则&功能设计

**本地的人机对战和人人对战**直接在菜单栏开始即可，在这里不做具体的规则介绍；

**联机模式功能设计：**

在服务器端程序开启的情况下，会监听三个端口，``CONF_PORT``，``HOST_PORT``，``GUEST_PORT``。其中，``CONF_PORT``用于接收客户端程序的申请，另外两个端口用于建立连接，实现信息的转发。具体过程如下：

1. 客户端向服务器端发出申请，连接到``CONF_PORT``。
2. 服务器端向``CONF_PORT``发送出当前的游戏状态``Game_Status``，分为``VACANT``，``WAITING``，``ONGOING``，分别表示空闲，已有玩家等待中，以及游戏进行中。
   * 若当前为空闲，则当前申请的客户端将与``HOST_PORT``建立连接，等待另一位玩家进入游戏，并将游戏状态``Game_Status``修改为``WAITING``。
   * 若当前为等待，则当前申请的客户端将与``GUEST_PORT``建立连接，开始游戏，并将并将游戏状态``Game_Status``修改为``ONGOING``。
   * 若当前为游戏中，则断开连接，并返回信息。

3. 当双方玩家都分别建立连接后，开始进行对战，服务器同时监听双方的信息，并不作修改地转发。
4. 当一局游戏结束之后，玩家断开连接，将游戏状态``Game_Status``修改为``VACANT``，并且还原为最初监听状态。

联机的人人模式需要进入一个login_dialog的窗口，输入对应服务器端的`Address`和`Port`即可根据已有游戏人数确定先后手并直接连接到相应的端口，在演示过程我们使用的`IP`地址`101.34.252.176`，端口为`10900`。在连接到`CONF_PORT`以后，服务器端会将当前的`Game_Status`传送到本地，本地也根据`GameStatus`确定自己的先后手，同时以弹窗方式提示"You are the Host"、”You are the Guest“、"Sorry, You can't join now".

**落子提示功能：**游戏开始以后，每次对方落子以后，本地暂时以红色表示对方的棋子（目的是为了标记对方落子的位置以及提醒轮到自己），在本方落子以后对方之前的落子会恢复本来的黑色，本方的棋子会以白色表示。

**投降功能：**没有轮到自己是不可以执行投降操作的，轮到自己落子时可以在菜单栏的`Action`执行投降操作，这样本方会弹出"You lose"的提示信息，对方也会同步弹出”You win“d的提示信息，无论是某一方投降或是某一方连成五子，点击提示信息框的OK都可以直接进入下一局（网络连接不断）。

**悔棋功能：**玩家可以在菜单栏的`Action`执行悔棋操作，点击以后自己和对方的棋盘都会回到一回合前的状态。同时考虑到游戏的公平性，不支持连续悔棋。

### 关键算法设计

本游戏设计的难点主要为不同局域网下设备间的通信，下面主要就服务器端和客户端两部分介绍主要的算法设计。

#### 一.TCP通信的构建(服务器端)

##### Socket

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

##### 多进程

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

##### 进程间通信

由于程序涉及到多个进程，而不同进程之间的变量是不共享的，且不同进程之间有通信的需求，因此需要使用一些方法，实现进程之间的通信。这里主要用了三种方法：共享文件，管道通信，以及信号机制。

##### 共享文件

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

##### 管道通信

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

##### 信号机制

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

##### 其他

除了以上三种方式之外，还在主进程循环的末尾有这样的语句：

```c
int Exit_Pid = wait(NULL);

int i;
for (i = 0; i < 3; i++)
	kill(pid[i], SIGUSR1);
```

主进程会阻塞在这里，等待着第一个子进程的结束，一旦有一个子进程结束，主进程将会结束所有的子进程，然后回到循环的开头，回到等待连接的状态。

### 二.TCP通信的构建（客户端）

客户端需要建立两个`QTcpSocket`

```c++
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
QTcpSocket *client;
QTcpSocket *clientTry;//socket对象
```

其中`clientTry`用于与服务器的`CONF_PORT`进行连接以获得`game_status`

```c++
	login_dialog *dlg;
    dlg= new login_dialog();
    if(dlg->exec()!=QDialog::Accepted)
        return;
    if(host_addr.setAddress(dlg->getAddress()))
    {
        clientTry = new QTcpSocket();
        Port=dlg->getPort();
        connect(clientTry,SIGNAL(readyRead()),this,SLOT(recvMessage()));
        clientTry->connectToHost(host_addr,dlg->getPort());
    }
    else
    {
        QMessageBox::warning(this,"Error","Error:Invalid address!");
    }
```

其中`recvMessage()`用`clientTry->read()`的形式去获取对应的`game_status`

得到`game_status`以后，客户端会自动去连接对应`host/guest`的端口并将接收到信息的信号连接至本地更新棋盘的函数`chessOneByNet()`

```c++
client=new QTcpSocket();
connect(client,SIGNAL(readyRead()),this,SLOT(chessOneByNet()));
client->connectToHost(host_addr,PortForward);
```

- 玩家如果执行落子操作，会将自己的落子位置`(clickPosRow,clickPosCol)`发送给服务器端

  ```c++
  QString text;
  text+=QString(" %1 %2").arg(QString::number(clickPosRow),QString::number(clickPosCol));
  client->write(text.toUtf8());
  ```

- 玩家如果执行投降操作，则发送`(-2,-2)`表示投降信号；
- 玩家如果执行悔棋操作，则发送`(-3,-3)`表示悔棋信号；

`recvMessage()`部分负责对传来的信号进行相应处理

```c++
QTextStream os(client);
os>>netPosRow>>netPosCol;
```

- 如果传来有效坐标，则同步至本地棋盘；
- 如果传来(-2,-2)，则直接提示对方投降；
- 如果传来(-3,-3)，则将操作栈中的上一个回合的操作恢复；

### 部署与运行

**服务器端**代码命名为``Server.c``，在``Linux``服务器上编译运行的指令如下：

```
$ gcc Server.c -o Server
$ nohup ./Server &
```

这里使用``nohup``以及``&``是为了让程序能够在后台运行，并且关闭终端也能持续运行。

**客户端**采用`windeployqt`的方式形成可打包文件，解压后点击exe文件即可运行。

### 心得与展望

**服务端程序**的编写涉及到了很多`Linux`操作系统以及计算机网络的的内容，涉及范围较广，对于个人能力提升较大。后期考虑添加账户系统，以及排名系统，使之作为联网游戏更加完整。

**客户端程序**的编写需要自学`Qt`的相关内容，同时熟悉了`Qt Creater`的使用，后期考虑对棋盘界面进行进一步的优化，同时希望可以在游戏本身之外加入一个小型的聊天室供双方玩家交流。

