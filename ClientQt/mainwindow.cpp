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


#define CHESS_ONE_SOUND ":/res/sound/chessone.wav"
#define WIN_SOUND ":/res/sound/win.wav"
#define LOSE_SOUND ":/res/sound/lose.wav"


const int kBoardMargin = 30; // 棋盘边缘空隙
const int kRadius = 12; // 棋子半径
const int kMarkSize = 6; // 落子标记边长
const int kBlockSize = 40; // 格子的大小
const int kPosDelta = 20; // 鼠标点击的模糊距离上限
//const int kBoardSizeNum = 18;
//考虑到判断游戏状态需要用到棋盘尺寸将上一行放在GameModel.h文件中
const int kAIDelay = 300; // AI下棋的思考时间


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 设置棋盘大小
    setFixedSize(kBoardMargin * 2 + kBlockSize * kBoardSizeNum, kBoardMargin * 2 + kBlockSize * kBoardSizeNum);
    //setStyleSheet("background-color:grey;");
    //可以设置棋盘颜色风格

        // 开启鼠标hover功能，这两句一般要设置window的
    setMouseTracking(true);
    //    centralWidget()->setMouseTracking(true);

        // 初始化菜单栏
    QMenu* gameMenuLocal = menuBar()->addMenu(tr("Local")); // menuBar默认是存在的，直接加菜单就可以了
    QMenu* gameMenuNet = menuBar()->addMenu(tr("Networking"));
    QMenu* gameAction = menuBar()->addMenu(tr("Action"));//添加操作栏
    //Model栏目下面的可选模式
    QAction* actionPVE = new QAction("Person VS Computer", this);
    connect(actionPVE, SIGNAL(triggered()), this, SLOT(initPVEGame()));
    gameMenuLocal->addAction(actionPVE);

    QAction* actionPVP = new QAction("Person VS Person(local)", this);
    connect(actionPVP, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    gameMenuLocal->addAction(actionPVP);

    //QAction* actionPVP_Network = new QAction("Person VS Person(Network)", this);
    //connect(actionPVP_Network, SIGNAL(triggered()), this, SLOT(initPVP_NETGame()));
    //gameMenuNet->addAction(actionPVP_Network);

    QAction* actionHost = new QAction("Host a game", this);
    connect(actionHost, SIGNAL(triggered()), this, SLOT(initPVP_NETGameHost()));
    gameMenuNet->addAction(actionHost);

    QAction* actionGuest = new QAction("Join a game", this);
    connect(actionGuest, SIGNAL(triggered()), this, SLOT(initPVP_NETGameGuest()));
    gameMenuNet->addAction(actionGuest);

    //Action栏目下面的可选操作
    QAction* actionUndo = new QAction("Undo", this);
    connect(actionUndo, SIGNAL(triggered()), this, SLOT(Undo()));
    gameAction->addAction(actionUndo);

    QAction* actionSurrender = new QAction("Surrender", this);
    connect(actionSurrender, SIGNAL(triggered()), this, SLOT(Surrender()));
    gameAction->addAction(actionSurrender);

    // 开始游戏
    initGame();
}

MainWindow::~MainWindow()//关闭游戏
{
    if (game)
    {
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

void MainWindow::initPVP_NETGameHost()
{
    game_type = PERSON_NET;
    game->isHost = true;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
}

void MainWindow::initPVP_NETGameGuest()
{
    game_type = PERSON_NET;
    game->isHost = false;
    game->gameStatus = PLAYING;
    game->startGame(game_type);
    update();
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
    QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Sorry",  "You lose!");
    if (btnValue == QMessageBox::Ok)
    {
        game->startGame(game_type);
        game->gameStatus = PLAYING;
    }
    //if(game_type==PERSON_NET)此处发送信息(-2,-2)表示投降
    //rw_socket->write(text.toUtf8)
    update();
}

void MainWindow::Undo()
{


}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    // 绘制棋盘
    painter.setRenderHint(QPainter::Antialiasing, true); // 抗锯齿
//    QPen pen; // 调整线条宽度
//    pen.setWidth(2);
//    painter.setPen(pen);
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
                brush.setColor(Qt::black);
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
    if (game_type == PERSON_NET && !game->playerFlag)//把chessOneByNet()放到mouseMoveEvent里，略显笨拙地解决了点击才能更新的问题
    {
        chessOneByNet();
    }
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
    /*//此处是原有的落子逻辑（仅支持Person VS Person(local)和Person VS Computer）
    if (!(game_type == BOT && !game->playerFlag))
    {
        chessOneByPerson();
        // 如果是人机模式，需要调用AI下棋
        if (game->gameType == BOT && !game->playerFlag)
        {
            // 用定时器做一个延迟
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
    }
    */
    if (game_type == PERSON_NET && !game->playerFlag)
    {
        chessOneByNet();
    }
    else
    {
        chessOneByPerson();
        if (game_type == BOT && !game->playerFlag)
        {
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
        else if (game_type == PERSON_NET && !game->playerFlag)
        {
            chessOneByNet();

            //补充判断此时的输赢
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
            netPosCol++;
            netPosRow++;
        }
    }
}

void MainWindow::chessOneByPerson()//玩家下棋的操作
{
    // 根据当前存储的坐标下子
    // 只有有效点击才下子，并且该处没有子
    if (clickPosRow != -1 && clickPosCol != -1 && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        game->actionByPerson(clickPosRow, clickPosCol);
        //QSound::play(CHESS_ONE_SOUND);
        update();
    }
}

void MainWindow::chessOneByNet()//接受服务器传来的信息并落子
{
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
        update();
    }
    else if (netPosRow!= -1 && netPosCol != -1 && game->gameMapVec[netPosRow][netPosCol] == 0)//接收到落子信号
    {
        game->actionByPerson(netPosRow, netPosCol);
        //QSound::play(CHESS_ONE_SOUND);
        update();
    }
}

void MainWindow::chessOneByAI()//AI下棋的操作
{
    game->actionByAI(clickPosRow, clickPosCol);
    //QSound::play(CHESS_ONE_SOUND);
    update();
}
