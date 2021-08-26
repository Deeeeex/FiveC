#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <vector>

enum GameType
{
    PERSON,
    BOT,
    PERSON_NET
};

// 游戏状态
enum GameStatus
{
    PLAYING,
    WIN,//此处WIN指有胜利一方
    DEAD//此处DEAD指死局
};
const int kBoardSizeNum = 18;//棋盘尺寸为19*19

class GameModel
{
public:
    GameModel();

public:
    std::vector<std::vector<int>> gameMapVec; // 存储当前游戏棋盘和棋子的情况,空白为0，白子1，黑子-1
    std::vector<std::vector<int>> scoreMapVec; // 存储各个点位的评分情况，作为AI下棋依据
    bool playerFlag; // 标示下棋方/先后手
    bool isHost;//根据谁是房间的host决定先后手
    GameType gameType; // 游戏模式
    GameStatus gameStatus; // 游戏状态

    void startGame(GameType type); // 开始游戏
    void calculateScore(); // 计算评分
    void actionByPerson(int row, int col); // 人执行下棋
    void actionByAI(int& clickRow, int& clickCol); // 机器执行下棋
    void actionUndo(int row,int col);
    void updateGameMap(int row, int col); // 每次落子后更新游戏棋盘
    bool isWin(int row, int col); // 判断游戏是否胜利
    bool isDeadGame(); // 判断是否和棋
};

#endif // GAMEMODEL_H#pragma once
