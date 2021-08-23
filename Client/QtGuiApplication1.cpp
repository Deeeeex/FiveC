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
#include "QtGuiApplication1.h"


#define CHESS_ONE_SOUND ":/res/sound/chessone.wav"
#define WIN_SOUND ":/res/sound/win.wav"
#define LOSE_SOUND ":/res/sound/lose.wav"


const int kBoardMargin = 30; // ���̱�Ե��϶
const int kRadius = 12; // ���Ӱ뾶
const int kMarkSize = 6; // ���ӱ�Ǳ߳�
const int kBlockSize = 40; // ���ӵĴ�С
const int kPosDelta = 20; // �������ģ����������
//const int kBoardSizeNum = 18;
//���ǵ��ж���Ϸ״̬��Ҫ�õ����̳ߴ罫��һ�з���GameModel.h�ļ���
const int kAIDelay = 300; // AI�����˼��ʱ��


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // �������̴�С
    setFixedSize(kBoardMargin * 2 + kBlockSize * kBoardSizeNum, kBoardMargin * 2 + kBlockSize * kBoardSizeNum);
    //setStyleSheet("background-color:grey;");
    //��������������ɫ���

        // �������hover���ܣ�������һ��Ҫ����window��
    setMouseTracking(true);
    //    centralWidget()->setMouseTracking(true);

        // ��ʼ���˵���
    QMenu* gameMenu = menuBar()->addMenu(tr("Model")); // menuBarĬ���Ǵ��ڵģ�ֱ�ӼӲ˵��Ϳ�����
    QMenu* gameAction = menuBar()->addMenu(tr("Action"));//��Ӳ�����
    //Model��Ŀ����Ŀ�ѡģʽ
    QAction* actionPVE = new QAction("Person VS Computer", this);
    connect(actionPVE, SIGNAL(triggered()), this, SLOT(initPVEGame()));
    gameMenu->addAction(actionPVE);
    
    QAction* actionPVP = new QAction("Person VS Person(local)", this);
    connect(actionPVP, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    gameMenu->addAction(actionPVP);

    QAction* actionPVP_Network = new QAction("Person VS Person(Network)", this);
    connect(actionPVP_Network, SIGNAL(triggered()), this, SLOT(initPVP_NETGame()));
    gameMenu->addAction(actionPVP_Network);

    //Action��Ŀ����Ŀ�ѡ����
    QAction* actionUndo = new QAction("Undo", this);
    //connect(actionUndo, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    gameAction->addAction(actionUndo);

    QAction* actionSurrender = new QAction("Surrender", this);
    //connect(actionSurrender, SIGNAL(triggered()), this, SLOT(initPVPGame()));
    gameAction->addAction(actionSurrender);

    // ��ʼ��Ϸ
    initGame();
}

MainWindow::~MainWindow()//�ر���Ϸ
{
    if (game)
    {
        delete game;
        game = nullptr;
    }
}

void MainWindow::initGame()
{
    // ��ʼ����Ϸģ��
    game = new GameModel;//������ϷʱĬ��Ϊ�˻���սģʽ
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
    game_type = PERSON_NET;
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

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    // ��������
    painter.setRenderHint(QPainter::Antialiasing, true); // �����
//    QPen pen; // �����������
//    pen.setWidth(2);
//    painter.setPen(pen);
    for (int i = 0; i <= kBoardSizeNum+1; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i, kBoardMargin, kBoardMargin + kBlockSize * i, size().height() - kBoardMargin);
        painter.drawLine(kBoardMargin, kBoardMargin + kBlockSize * i, size().width() - kBoardMargin, kBoardMargin + kBlockSize * i);
    }

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    // �������ӱ��(��ֹ������Խ��)
    if (clickPosRow >= 0 && clickPosRow <= kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol <= kBoardSizeNum &&
        game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        if (game->playerFlag)//�ֵ��Լ�������ʾ���ӱ��
            brush.setColor(Qt::white);
        else
            brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.drawRect(kBoardMargin + kBlockSize * clickPosCol - kMarkSize / 2, kBoardMargin + kBlockSize * clickPosRow - kMarkSize / 2, kMarkSize, kMarkSize);
    }

    // �������� 
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

    // �ж���Ӯ
    if (clickPosRow >= 0 && clickPosRow <= kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol <= kBoardSizeNum &&
        (game->gameMapVec[clickPosRow][clickPosCol] == 1 ||
            game->gameMapVec[clickPosRow][clickPosCol] == -1))
    {
        if (game->isWin(clickPosRow, clickPosCol) && game->gameStatus == PLAYING)
        {
            qDebug() << "win";
            game->gameStatus = WIN;
            QSound::play(WIN_SOUND);
            QString str;
            if (game->gameMapVec[clickPosRow][clickPosCol] == 1)
                str = "White player";
            else if (game->gameMapVec[clickPosRow][clickPosCol] == -1)
                str = "Black player";
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Congratulations", str + " win!");

            // ������Ϸ״̬������������ѭ��
            if (btnValue == QMessageBox::Ok)
            {
                game->startGame(game_type);
                game->gameStatus = PLAYING;
            }
        }
    }


    // �ж�����
    if (game->isDeadGame())
    {
        QSound::play(LOSE_SOUND);
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
    // ͨ������hoverȷ�����ӵı��
    int x = event->x();
    int y = event->y();

    // ���̱�Ե��������
    if (x > kBoardMargin - kBlockSize / 2 &&
        x < size().width() - kBoardMargin &&
        y > kBoardMargin - kBlockSize / 2 &&
        y < size().height() - kBoardMargin)
    {
        // ��ȡ��������Ͻǵĵ�
        int col = x / kBlockSize;
        int row = y / kBlockSize;

        int leftTopPosX = kBoardMargin + kBlockSize * col;
        int leftTopPosY = kBoardMargin + kBlockSize * row;

        // ���ݾ���������ʵĵ��λ��,һ���ĸ��㣬���ݰ뾶����ѡ�����
        clickPosRow = -1; // ��ʼ�����յ�ֵ
        clickPosCol = -1;
        int len = 0; // �������ȡ���Ϳ�����

        // ȷ��һ������ڷ�Χ�ڵĵ㣬��ֻ����ȷ��һ������
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

    // ���������ҲҪ�ػ�
    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)//����Ƿ�����������ط�Ҳ��Ҫ��
{
    // �����壬���Ҳ�������������
    /*
    if (!(game_type == BOT && !game->playerFlag))
    {
        chessOneByPerson();
        // ������˻�ģʽ����Ҫ����AI����
        if (game->gameType == BOT && !game->playerFlag)
        {
            // �ö�ʱ����һ���ӳ�
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
    }
    */
    ///*
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
            
            //�����жϴ�ʱ����Ӯ
            if (game->isWin(netPosRow, netPosCol) && game->gameStatus == PLAYING)
            {
                qDebug() << "win";
                game->gameStatus = WIN;
                QSound::play(WIN_SOUND);
                QString str;
                str = "You";
                QMessageBox::StandardButton btnValue = QMessageBox::information(this, "Congratulations", str + " lose!");

                // ������Ϸ״̬������������ѭ��
                if (btnValue == QMessageBox::Ok)
                {
                    game->startGame(game_type);
                    game->gameStatus = PLAYING;
                }
            }
            if (game->isDeadGame())
            {
                QSound::play(LOSE_SOUND);
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
    //*/
}

void MainWindow::chessOneByPerson()//�������Ĳ���
{
    // ���ݵ�ǰ�洢����������
    // ֻ����Ч��������ӣ����Ҹô�û����
    if (clickPosRow != -1 && clickPosCol != -1 && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        game->actionByPerson(clickPosRow, clickPosCol);
        QSound::play(CHESS_ONE_SOUND);
        update();
    }
}

void MainWindow::chessOneByNet()//���ܷ�������������Ϣ������
{
    if (netPosRow!= -1 && netPosCol != -1 && game->gameMapVec[netPosRow][netPosCol] == 0)
    {
        game->actionByPerson(netPosRow, netPosCol);
        QSound::play(CHESS_ONE_SOUND);
        update();
    }
}

void MainWindow::chessOneByAI()//AI����Ĳ���
{
    game->actionByAI(clickPosRow, clickPosCol);
    QSound::play(CHESS_ONE_SOUND);
    update();
}