#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GameModel.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

public slots:
    //void connectToServer();
    //void startListening();

protected:
    // 绘制
    void paintEvent(QPaintEvent* event);
    // 监听鼠标移动情况，方便落子
    void mouseMoveEvent(QMouseEvent* event);
    // 实际落子
    void mouseReleaseEvent(QMouseEvent* event);

private:
    //QTcpServer* listen_socket;
    //QTcpSocket* rw_socket;
    GameModel* game; // 游戏指针
    GameType game_type; // 存储游戏类型
    int clickPosRow, clickPosCol; // 存储将点击的位置
    int netPosRow=1, netPosCol=1;//存储server传来的位置
    void initGame();
    //void checkGame(int y, int x);


private slots:
    void chessOneByPerson(); // 人执行
    void chessOneByNet();//接受server传来的信息并落子
    void chessOneByAI(); // AI下棋

    void initPVPGame();
    void initPVP_NETGameHost();
    void initPVP_NETGameGuest();
    void initPVEGame();

    void Surrender();
    void Undo();

    //负责Network的函数
    //void connectedToHost();
    //void acceptConnection();
    //void recvMessage();
    //void get_ready();
};

#endif // MAINWINDOW_H
