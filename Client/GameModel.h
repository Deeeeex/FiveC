#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <vector>

enum GameType
{
    PERSON,
    BOT,
    PERSON_NET
};

// ��Ϸ״̬
enum GameStatus
{
    PLAYING,
    WIN,//�˴�WINָ��ʤ��һ��
    DEAD//�˴�DEADָ����
};
const int kBoardSizeNum = 18;//���̳ߴ�Ϊ19*19

class GameModel
{
public:
    GameModel();

public:
    std::vector<std::vector<int>> gameMapVec; // �洢��ǰ��Ϸ���̺����ӵ����,�հ�Ϊ0������1������-1
    std::vector<std::vector<int>> scoreMapVec; // �洢������λ�������������ΪAI��������
    bool playerFlag; // ��ʾ���巽/�Ⱥ���
    GameType gameType; // ��Ϸģʽ
    GameStatus gameStatus; // ��Ϸ״̬

    void startGame(GameType type); // ��ʼ��Ϸ
    void calculateScore(); // ��������
    void actionByPerson(int row, int col); // ��ִ������
    void actionByAI(int& clickRow, int& clickCol); // ����ִ������
    void updateGameMap(int row, int col); // ÿ�����Ӻ������Ϸ����
    bool isWin(int row, int col); // �ж���Ϸ�Ƿ�ʤ��
    bool isDeadGame(); // �ж��Ƿ����
};

#endif // GAMEMODEL_H#pragma once
