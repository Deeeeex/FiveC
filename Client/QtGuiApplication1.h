#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "GameModel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

protected:
    // ����
    void paintEvent(QPaintEvent* event);
    // ��������ƶ��������������
    void mouseMoveEvent(QMouseEvent* event);
    // ʵ������
    void mouseReleaseEvent(QMouseEvent* event);

private:
    GameModel* game; // ��Ϸָ��
    GameType game_type; // �洢��Ϸ����
    int clickPosRow, clickPosCol; // �洢�������λ��
    int netPosRow=1, netPosCol=1;//�洢server������λ��
    void initGame();
    //void checkGame(int y, int x);

private slots:
    void chessOneByPerson(); // ��ִ��
    void chessOneByNet();//����server��������Ϣ������
    void chessOneByAI(); // AI����

    void initPVPGame();
    void initPVP_NETGame();
    void initPVEGame();
};

#endif // MAINWINDOW_H