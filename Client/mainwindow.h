#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GameModel.h"
#include <stack>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

namespace Ui{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

protected:
    // 绘制
    void paintEvent(QPaintEvent* event);
    // 监听鼠标移动情况，方便落子
    void mouseMoveEvent(QMouseEvent* event);
    // 实际落子
    void mouseReleaseEvent(QMouseEvent* event);

private:
    QTcpSocket *client;
    QTcpSocket *clientTry;//socket对象
    GameModel* game; // 游戏指针
    GameType game_type; // 存储游戏类型
    int clickPosRow, clickPosCol; // 存储将点击的位置
    int netPosRow=-1, netPosCol=-1;//存储server传来的位置,正常情况下置为(-1,-1)
    int playerExisted;
    QHostAddress host_addr;
    int Port;
    quint16 PortForward;//看返回值确定下一步去哪个端口
    int stackOperation[2][2];//记录前两次操作的栈
    int counterFlag=0;//用于标记下次操作在栈中的位置
    void initGame();

private slots:
    void chessOneByPerson(); // 人执行
    void chessOneByNet();//接受server传来的信息并落子
    void chessOneByAI(); // AI下棋
    void chessUndo(int row,int col);
    void initPVPGame();
    void initPVP_NETGame();
    void recvMessage();
    void initPVEGame();
    void Surrender();
    void Undo();

};

#endif // MAINWINDOW_H
