#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/wait.h>
#include<time.h>
#define HOST_PORT 10888
#define GUEST_PORT 10889
#define CONF_PORT 10900
#define MAXLINE 4096
#define VACANT 10
#define WAITING 11
#define ONGOING 12

int     socket_fd_Host, connect_fd_Host, 
        socket_fd_Guest, connect_fd_Guest, 
        socket_fd_Conf, connect_fd_Conf;
struct sockaddr_in  servaddr_Host, 
                    servaddr_Guest, 
                    servaddr_Conf;
int     buff_H[2], buff_G[2];
int     n;
int     Game_Status = VACANT, Game_Status_fd;
pid_t   pid[3];//to tag different child processes
int     fd_1[2];
int     fd_2[2];
int     length;
int     My_Child;
time_t  now;
struct tm *ptm;

void handler(int signum);
void Pipe_Handler(int signum);
void Be_Killed_Handler(int signum);

int main(void)
{
    {
    //Socket Application
    if((socket_fd_Host = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( (socket_fd_Guest = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( (socket_fd_Conf = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    //Initialization for Host Socket
    memset(&servaddr_Host, 0, sizeof(servaddr_Host));
    servaddr_Host.sin_family = AF_INET;
    servaddr_Host.sin_addr.s_addr = htonl(INADDR_ANY);//Listen to all Addresses
    servaddr_Host.sin_port = htons(HOST_PORT);//Set listening port as default

    //Initilization for Guest Socket
    memset(&servaddr_Guest, 0, sizeof(servaddr_Guest));
    servaddr_Guest.sin_family = AF_INET;
    servaddr_Guest.sin_addr.s_addr = htonl(INADDR_ANY);//Listen to all Addresses
    servaddr_Guest.sin_port = htons(GUEST_PORT);//Set listening port as default

    //Initilization for Confirmation Socket
    memset(&servaddr_Conf, 0, sizeof(servaddr_Conf));
    servaddr_Conf.sin_family = AF_INET;
    servaddr_Conf.sin_addr.s_addr = htonl(INADDR_ANY);//Listen to all Addressess
    servaddr_Conf.sin_port = htons(CONF_PORT);//Set listening port as default
    
    //Bind
    if( bind(socket_fd_Host, (struct sockaddr*)&servaddr_Host, sizeof(servaddr_Host)) == -1)
    {
        printf("bind Host socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( bind(socket_fd_Guest, (struct sockaddr*)&servaddr_Guest, sizeof(servaddr_Guest)) == -1)
    {
        printf("bind Guest socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    if( bind(socket_fd_Conf, (struct sockaddr*)&servaddr_Conf, sizeof(servaddr_Conf)) == -1)
    {
        printf("bind Conf socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    }

    
    while(1)
    {
        //Game_Status Initialization
        Game_Status_fd = open("Game_Status", O_RDWR);
        write(Game_Status_fd, &Game_Status, sizeof(int));
        close(Game_Status_fd);

        time (&now);
        ptm = localtime (&now);
        printf("%s======waiting for Host's request======\n", asctime(ptm));

        //Here runs the Confirmation child process
        if((pid[0] = fork()) < 0)
        {
            printf("child process fork error: %s(errno: %d)",strerror(errno),errno);
        }
        else if(pid[0] == 0)
        {
            if( listen(socket_fd_Conf, 10) == -1)
            {
                printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
                exit(0);
            }
            while(1)
            {
                //Blocked until Guest connect
                if( (connect_fd_Conf = accept(socket_fd_Conf, (struct sockaddr*)NULL, NULL)) == -1)
                {
                    printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                    continue;
                }

                time (&now);
                ptm = localtime (&now);
                printf("%sCONFIRMATION accepted\n\n",asctime(ptm));

                Game_Status_fd = open("Game_Status", O_RDONLY);
                read(Game_Status_fd, &Game_Status, sizeof(int));
                close(Game_Status_fd);
                //send data to Client
                if(!fork())
                {
                    if(send(connect_fd_Conf, &Game_Status, sizeof(int), 0) == -1)
                        perror("send error");

                    time (&now);
                    ptm = localtime (&now);
                    printf("%sGAME_STATUS sent:%d\n\n", asctime(ptm), Game_Status);

                    close(connect_fd_Conf);
                    exit(0);
                }
                wait(NULL);
            }
            exit(0);
        }


        //Init Pipe
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

        //Here runs the Host child process
        if((pid[1] = fork()) < 0)
        {
            printf("child process fork error: %s(errno: %d)",strerror(errno),errno);
        }
        else if(pid[1] == 0)
        {
            close(*write_G2H);
            close(*read_H2G);
            if( listen(socket_fd_Host, 10) == -1)
            {
                printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
                exit(0);
            }
            while(1)
            {
                //Block until connexion established
                if( (connect_fd_Host = accept(socket_fd_Host, (struct sockaddr*)NULL, NULL)) == -1)
                {
                    printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                    continue;
                }

                //update Game Status when Host connect
                Game_Status = WAITING;
                Game_Status_fd = open("Game_Status", O_WRONLY);
                write(Game_Status_fd, &Game_Status, sizeof(int));
                close(Game_Status_fd);

                
                My_Child = fork();
                if (My_Child == 0)
                {//send data to Host
                    signal(SIGPIPE, Pipe_Handler);
                    while(1)
                    {
                        length = read(*read_G2H, buff_G, 4096);
                        send(connect_fd_Host, buff_G, length, 0);
                    }
                }
                signal(SIGUSR1, Be_Killed_Handler);
                //listen to data from host
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
            }
            exit(0);
        }


        //Here runs the Guest child process
        if((pid[2] = fork()) < 0)
        {
            printf("child process fork error: %s(errno: %d)",strerror(errno),errno);
        }
        else if(pid[2] == 0)
        {
            close(*write_H2G);
            close(*read_G2H);
            if( listen(socket_fd_Guest, 10) == -1)
            {
                printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
                exit(0);
            }
            while(1)
            {
                //Block until connexion established
                if( (connect_fd_Guest = accept(socket_fd_Guest, (struct sockaddr*)NULL, NULL)) == -1)
                {
                    printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
                    continue;
                }

                //update Game Status when Guest connect
                Game_Status = ONGOING;
                Game_Status_fd = open("Game_Status", O_WRONLY);
                write(Game_Status_fd, &Game_Status, sizeof(int));
                close(Game_Status_fd);
                
                //send data to Guest
                My_Child = fork();
                if (My_Child == 0)
                {
                    signal(SIGPIPE, Pipe_Handler);
                    while(1)
                    {
                        length = read(*read_H2G, buff_G, 4096);
                        send(connect_fd_Guest, buff_G, length, 0);
                    }
                }
                signal(SIGUSR1, Be_Killed_Handler);
                //listen to data from Guest
                while(1)
                {
                    n = recv(connect_fd_Guest, buff_H, 4096, 0);
                    if(n == 0 || n == -1)
                    {//when connexion shut down
                        kill(My_Child, SIGKILL);
                        close(connect_fd_Guest);
                        exit(0);
                    }
                    write(*write_G2H, buff_H, n);
                }
            }
            exit(0);
        }

        signal(SIGINT, handler);
        signal(SIGTERM, handler);

        close(*write_H2G);
        close(*read_G2H);
        close(*write_G2H);
        close(*read_H2G);
        
        int Exit_Pid = wait(NULL);

        time (&now);
        ptm = localtime (&now);
        printf("%s%d Exited\n", asctime(ptm), Exit_Pid);

        int i;
        for (i = 0; i < 3; i++)
            kill(pid[i], SIGUSR1);

        while((Exit_Pid = wait(NULL)) != -1)
        {
            time (&now);
            ptm = localtime (&now);
            printf("%s%d Exited\n", asctime(ptm), Exit_Pid);
        }
    }

    exit(0);
}

void handler(int signum)
{
    int i;
    for (i = 0; i < 3; i++)
        kill(pid[i], SIGUSR1);
    if (signum == SIGINT)
        printf("SIGINT\n");
    else
        printf("SIGTERM\n");
    printf("OK\n");
    exit(0);
}

void Pipe_Handler(int signum)
{
    exit(0);
}

void Be_Killed_Handler(int signum)
{
    kill(My_Child, SIGKILL);
    exit(0);
}