#include <QPainter>
#include <QTimer>
#include <QSound>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <math.h>
#include "mainwindow.h"
#include "login_dialog.h"
#include <QHostAddress>
#include "ui_mainwindow.h"

const int kBoardMargin = 40; // 棋盘边缘空隙
const int kRadius = 15; // 棋子半径
const int kMarkSize = 8; // 落子标记边长
const int kBlockSize = 45; // 格子的大小
const int kPosDelta = 35; // 鼠标点击的模糊距离上限
//const int kBoardSizeNum = 18;
//考虑到判断游戏状态需要用到棋盘尺寸将上一行放在GameModel.h文件中
const int kAIDelay = 300; // AI下棋的思考时间

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 设置棋盘大小
    setFixedSize(kBoardMargin * 2 + kBlockSize * kBoardSizeNum, kBoardMargin * 2 + kBlockSize * kBoardSizeNum);
    //setStyleSheet("background-color:grey;");
    setAutoFillBackground(true);//必须有这条语句
    setPalette(QPalette(QColor(213,176,146)));
    //可以设置棋盘颜色风格

        // 开启鼠标hover功能，这两句一般要设置window的
    setMouseTracking(true);
    //    centralWidget()->setMouseTracking(true);

        // 初始化菜单栏
    QMenu* gameMenuModel = menuBar()->addMenu(tr("Model")); // menuBar默认是存在的，直接加菜单就可以了
    QMenu* gameMenuAction = menuBar()->addMenu(tr("Action"));
    //QMenu* gameSurrender = menuBar()->addMenu(tr("投降"));//添加操作栏
    //Model栏目下面的可选模式
    QAction* actionPVE = new QAction(tr("人机对战"), this);
    connect(actionPVE, SIGNAL(triggered()), this, SLOT(initPVEGame()));
    gameMenuModel->addAction(actionPVE);

    QAction* actionPVP = new QAction(tr("人人对战（本地）"), this);
    connect(actionPVP, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    gameMenuModel->addAction(actionPVP);

    QAction* actionPVP_Network = new QAction(tr("人人对战（联网）"), this);
    connect(actionPVP_Network, SIGNAL(triggered()), this, SLOT(initPVP_NETGame()));
    gameMenuModel->addAction(actionPVP_Network);

    //Action栏目下面的可选操作
    QAction* actionUndo = new QAction(tr("悔棋"), this);
    connect(actionUndo, SIGNAL(triggered()), this, SLOT(Undo()));
    gameMenuAction->addAction(actionUndo);

    QAction* actionSurrender = new QAction(tr("投降"), this);
    connect(actionSurrender, SIGNAL(triggered()), this, SLOT(Surrender()));
    gameMenuAction->addAction(actionSurrender);

    // 开始游戏
    initGame();
}

MainWindow::~MainWindow()//关闭游戏
{
    if (game)
    {
        client->abort();
        delete game;
        game = nullptr;
    }
}

void MainWindow::initGame()
{
    // 初始化游戏模型
    game = new GameModel;//进入游戏时默认为人机对战模式
    initPVEGame();
}

void MainWindow::initPVPGame()
{
    game_type = PERSON;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
}

void MainWindow::initPVP_NETGame()
{
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
}

//用于接受10900端口传来的playerexisted
void MainWindow::recvMessage()
{
    if(Port==10900)
    {
       //QTextStream os(clientTry);
       //os>>playerExisted;
       clientTry->read((char*)(&playerExisted),sizeof(int));
       if(playerExisted<=11)
       {
           //接收到10说明是host,接收到11说明是guest,接收到12表示房间已满
           game->isHost=11-playerExisted;
           QMessageBox::information(this, "Connected", "Connected to port 10900!");
           if(playerExisted==11)
           {
               //收到11表示是后手，去连接10889端口进行游戏
               QMessageBox::information(this, "Connected", "You are the Guest!");
               QString str="10889";
               PortForward=str.toUInt();
           }
           else if(playerExisted==10)
           {
               //收到10表示是先手，去连接10888端口进行游戏
               QMessageBox::information(this, "Connected", "You are the Host!");
               QString str="10888";
               PortForward=str.toUInt();
           }
           else if(playerExisted==0)
           QMessageBox::information(this, "Sorry", "No Message Received!");
       }
       else if(playerExisted==12)
       {
            QMessageBox::information(this, "Sorry", "Game has begun!");
       }
       clientTry->abort();
     }
    client=new QTcpSocket();
    connect(client,SIGNAL(readyRead()),this,SLOT(chessOneByNet()));
    client->connectToHost(host_addr,PortForward);
    if(playerExisted==10||playerExisted==11)
    {
        QMessageBox::information(this, "Connected", "Connected to server!");
        game_type = PERSON_NET;
        game->gameStatus = PLAYING;
        game->startGame(game_type);
        update();
    }
}

void MainWindow::initPVEGame()
{
    game_type = BOT;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
}

void MainWindow::Surrender()
{
    if (!game->playerFlag)
    {
        QMessageBox::information(this, tr("Sorry"), "You cannot surrender now!");
        return;
    }
    qDebug() << "win";
    game->gameStatus = WIN;
    //QSound::play(LOSE_SOUND);
    if(game_type==PERSON_NET)//此处发送信息(-2,-2)表示投降
    {
        QString text;
        text+=QString(" %1 %2").arg(QString::number(-2),QString::number(-2));
        client->write(text.toUtf8());
    }
    QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Sorry",  "You lose!");
    if (btnValue == QMessageBox::Ok)
    {
        game->startGame(game_type);
        game->gameStatus = PLAYING;
    }
    update();
}

void MainWindow::Undo()
{
    chessUndo(stackOperation[0][0],stackOperation[0][1]);
    chessUndo(stackOperation[1][0],stackOperation[1][1]);
    //发送信号(-3,-3);
    if(game_type==PERSON_NET)
    {
        QString text;
        text+=QString(" %1 %2").arg(QString::number(-3),QString::number(-3));
        client->write(text.toUtf8());
    }
    update();
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    // 绘制棋盘
    painter.setRenderHint(QPainter::Antialiasing, true); // 抗锯齿
    for (int i = 0; i <= kBoardSizeNum+1; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i, kBoardMargin, kBoardMargin + kBlockSize * i, size().height() - kBoardMargin);
        painter.drawLine(kBoardMargin, kBoardMargin + kBlockSize * i, size().width() - kBoardMargin, kBoardMargin + kBlockSize * i);
    }

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    // 绘制落子标记(防止鼠标出框越界)
    if (clickPosRow >= 0 && clickPosRow <= kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol <= kBoardSizeNum &&
        game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        if (game->playerFlag)//轮到自己才能显示落子标记
            brush.setColor(Qt::white);
        else
            brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.drawRect(kBoardMargin + kBlockSize * clickPosCol - kMarkSize / 2, kBoardMargin + kBlockSize * clickPosRow - kMarkSize / 2, kMarkSize, kMarkSize);
    }

    // 绘制棋子
    for (int i = 0; i <= kBoardSizeNum; i++)
        for (int j = 0; j <= kBoardSizeNum; j++)
        {
            if (game->gameMapVec[i][j] == 1)
            {
                brush.setColor(Qt::white);
                painter.setBrush(brush);
                painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius, kBoardMargin + kBlockSize * i - kRadius, kRadius * 2, kRadius * 2);
            }
            else if (game->gameMapVec[i][j] == -1)
            {

                if(game_type==PERSON_NET&&i==netPosRow&&j==netPosCol) brush.setColor(Qt::red);
                else brush.setColor(Qt::black);
                painter.setBrush(brush);
                painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius, kBoardMargin + kBlockSize * i - kRadius, kRadius * 2, kRadius * 2);
            }
        }

    // 判断输赢
    if (clickPosRow >= 0 && clickPosRow <= kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol <= kBoardSizeNum &&
        (game->gameMapVec[clickPosRow][clickPosCol] == 1 ||
            game->gameMapVec[clickPosRow][clickPosCol] == -1))
    {
        if (game->isWin(clickPosRow, clickPosCol) && game->gameStatus == PLAYING)
        {
            qDebug() << "win";
            game->gameStatus = WIN;
            //QSound::play(WIN_SOUND);
            QString str;
            if (game->gameMapVec[clickPosRow][clickPosCol] == 1)
                str = "White player";
            else if (game->gameMapVec[clickPosRow][clickPosCol] == -1)
                str = "Black player";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Congratulations", str + " win!");

            // 重置游戏状态
            if (btnValue == QMessageBox::Ok)
            {
                game->startGame(game_type);
                game->gameStatus = PLAYING;
            }
        }
    }

    // 判断死局
    if (game->isDeadGame())
    {
        //QSound::play(LOSE_SOUND);
        QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Oops", "Dead game!");
        if (btnValue == QMessageBox::Ok)
        {
            game->startGame(game_type);
            game->gameStatus = PLAYING;
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    // 通过鼠标的hover确定落子的标记
    int x = event->x();
    int y = event->y();

    // 棋盘边缘不能落子
    if (x > kBoardMargin - kBlockSize / 2 &&
        x < size().width() - kBoardMargin &&
        y > kBoardMargin - kBlockSize / 2 &&
        y < size().height() - kBoardMargin)
    {
        // 获取最近的左上角的点
        int col = x / kBlockSize;
        int row = y / kBlockSize;

        int leftTopPosX = kBoardMargin + kBlockSize * col;
        int leftTopPosY = kBoardMargin + kBlockSize * row;

        // 根据距离算出合适的点击位置,一共四个点，根据半径距离选最近的
        clickPosRow = -1; // 初始化最终的值
        clickPosCol = -1;
        int len = 0; // 计算完后取整就可以了

        // 确定一个误差在范围内的点，且只可能确定一个出来
        len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta)
        {
            clickPosRow = row;
            clickPosCol = col;
        }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta)
        {
            clickPosRow = row;
            clickPosCol = col + 1;
        }
        len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta)
        {
            clickPosRow = row + 1;
            clickPosCol = col;
        }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta)
        {
            clickPosRow = row + 1;
            clickPosCol = col + 1;
        }
    }
    // 存了坐标后也要重绘
    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)//此处是落子的基本逻辑顺序
{
    if (game_type == PERSON_NET && !game->playerFlag)
    {
    }
    else
    {
        chessOneByPerson();
        if (game_type == BOT && !game->playerFlag)
        {
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
    }
}

void MainWindow::chessOneByPerson()//玩家下棋的操作
{
    if (clickPosRow != -1 && clickPosCol != -1 && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        game->actionByPerson(clickPosRow, clickPosCol);
        stackOperation[counterFlag][0]=clickPosRow;
        stackOperation[counterFlag][1]=clickPosCol;
        //发送位置坐标给另一方
        if(game_type==PERSON_NET)
        {
            netPosCol=netPosRow=-1;
            QString text;
            text+=QString(" %1 %2").arg(QString::number(clickPosRow),QString::number(clickPosCol));
            client->write(text.toUtf8());
            //添加把状态改成轮到对手
        }
        counterFlag=1-counterFlag;
        //QSound::play(CHESS_ONE_SOUND);
        update();
    }
}

void MainWindow::chessOneByNet()//接受服务器传来的信息并落子
{
    QTextStream os(client);
    os>>netPosRow>>netPosCol;
    if (netPosCol == -2 && netPosRow == -2)//接收到投降信号
    {
        qDebug() << "win";
        game->gameStatus = WIN;
        //QSound::play(WIN_SOUND);
        QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Congratulations", +"You win!");
        if (btnValue == QMessageBox::Ok)
        {
            game->startGame(game_type);
            game->gameStatus = PLAYING;
        }
        //netPosCol=netPosRow=-1;
        update();
    }
    else if(netPosCol==-3&&netPosRow==-3)//接到Undo信号，把栈里的两次操作复原
    {
        chessUndo(stackOperation[0][0],stackOperation[0][1]);
        chessUndo(stackOperation[1][0],stackOperation[1][1]);
        //netPosCol=netPosRow=-1;
        update();
    }
    else if (netPosRow!= -1 && netPosCol != -1 && game->gameMapVec[netPosRow][netPosCol] == 0)//接收到落子信号
    {
        game->actionByPerson(netPosRow, netPosCol);
        stackOperation[counterFlag][0]=netPosRow;
        stackOperation[counterFlag][1]=netPosCol;
        //netPosCol=netPosRow=-1;//使用完置为-1
        counterFlag=1-counterFlag;
        //QSound::play(CHESS_ONE_SOUND);
        update();
        //为了提升同步性，对方落子后立即判断输赢
        if (game->isWin(netPosRow, netPosCol) && game->gameStatus == PLAYING)
        {
            qDebug() << "win";
            game->gameStatus = WIN;
            //QSound::play(WIN_SOUND);
            QString str;
            str = "You";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Sorry", str + " lose!");

            // 重置游戏状态，否则容易死循环
            if (btnValue == QMessageBox::Ok)
            {
                game->startGame(game_type);
                game->gameStatus = PLAYING;
            }
        }
        if (game->isDeadGame())
        {
            //QSound::play(LOSE_SOUND);
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Oops", "Dead game!");
            if (btnValue == QMessageBox::Ok)
            {
                game->startGame(game_type);
                game->gameStatus = PLAYING;
            }
        }
    }
}

void MainWindow::chessOneByAI()//AI下棋的操作
{
    game->actionByAI(clickPosRow, clickPosCol);
    stackOperation[counterFlag][0]=clickPosRow;
    stackOperation[counterFlag][1]=clickPosCol;
    counterFlag=1-counterFlag;
    //QSound::play(CHESS_ONE_SOUND);
    update();
}

void MainWindow::chessUndo(int row,int col)
{
    game->actionUndo(row,col);
}
