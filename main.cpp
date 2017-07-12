#include <iostream>
#include "jsoncpp/json.h"
#include <signal.h>
#include <unistd.h>
#include <csetjmp>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>


using namespace std;

#define gridsize 8
#define black 1
#define white 0
#define scoremax 50
#define scoremin -50

int dx[24] = { -1,-1,-1,0,0,1,1,1,-2,-2,-2,-2,-2,-1,-1,0,0,1,1,2,2,2,2,2 };
int dy[24] = { -1,0,1,-1,1,-1,0,1,-2,-1,0,1,2,-2,2,-2,2,-2,2,-2,-1,0,1,2 };

long long near[64] = {0LL};
long long between[64][64] = {0LL};

inline bool inMap(int x, int y)
{
    return x >= 0 && x <gridsize && y >= 0 && y < gridsize;
}

void initialize()
{
    for (int i = 0; i < gridsize; ++i)
    {
        for (int j = 0; j < gridsize; ++j)
        {
            long long temp = 0;
            for (int k = 0; k < 8; ++k)
            {
                if (i + dx[k]<0 || i + dx[k] >= gridsize || j + dy[k]<0 || j + dy[k] >= gridsize)
                    continue;
                temp |= (1LL << (((i + dx[k]) << 3) | (j + dy[k])));
            }
            near[i << 3 | j] |= temp;
        }
    }
    for (int i = 0; i < gridsize; i++){
        for (int j = 0; j < gridsize; j++){
            for (int dire = 0; dire < 8; dire++){
                for (int k = 1; k < gridsize; k++){
                    int a = i + k * dx[dire], b = j + k * dy[dire];
                    if (!inMap(a, b))
                        break;
                    long long temp = 0LL;
                    for (int l = 1; l < k; l++){
                        temp |= (1LL << (((i + l * dx[dire]) << 3 ) | (j + l * dy[dire])));
                    }
                    between[i << 3 | j][a << 3 | b] |= temp;
                }
            }
        }
    }
}

class bitboard
{
private:
    long long state[2] = {0LL};

public:
    bool gameover(){
        return !(~(state[black] | state[white]));
    }
    void set_cancel(int pos, int color){
        state[color] ^= (1LL << pos);

    }
    bool currstate(int x, int y, int color){
        return (bool)((state[color] >> (x << 3 | y)) & 1LL);
    }
    long long validpos(int color)
    {
        long long blankstate = blank();
        //cout << "blankstate" << blankstate << endl;
        long long ret = 0LL;
        int pos = 0;
        for (long long temp = state[color ^ 1]; temp; temp ^= (1LL << pos)){
            pos = __builtin_ctzll(temp);
            ret |= (near[pos] & blankstate);
        }
        //cout << "ret" << ret << endl;
        return ret;
    }
    long long blank(){
        return ~(state[black] | state[white]);
    }
    int score(int color)
    {
        int mobility = __builtin_popcountll(validpos(color)) - __builtin_popcountll(validpos(color ^ 1));
        //TODO
        int stability = 0;
        int disc = 0;
        //TODO
        return mobility + stability + disc;
    }
};

class bot
{
private:
    int color;
    bitboard board;
    int outpos;
public:
    void botinput()
    {
        board.set_cancel((3 << 3 | 4), black);
        board.set_cancel((4 << 3 | 3), black);
        board.set_cancel((3 << 3 | 3), white);
        board.set_cancel((4 << 3 | 4), white);
        string str;
        getline(cin, str);
        Json::Reader reader;
        Json::Value input;
        reader.parse(str, input);
        int turnID = input["responses"].size();
        color = input["requests"][(Json::Value::UInt) 0]["x"].asInt() < 0 ? black : white;
        int x, y;
        for (int i = 0; i < turnID; i++)
        {
            // 根据这些输入输出逐渐恢复状态到当前回合
            x = input["requests"][i]["x"].asInt();
            y = input["requests"][i]["y"].asInt();
            if (x >= 0)
                board.set_cancel((x << 3 | y), color^1); // 模拟对方落子
            x = input["responses"][i]["x"].asInt();
            y = input["responses"][i]["y"].asInt();
            if (x >= 0)
                board.set_cancel((x << 3 | y), color); // 模拟己方落子
        }
        x = input["requests"][turnID]["x"].asInt();
        y = input["requests"][turnID]["y"].asInt();
        if (x >= 0)
            board.set_cancel((x << 3 | y), color^1); // 模拟对方落子
    }
    inline void changecolor(){
        color ^= 1;
    }
    int alphabeta(int alpha, int beta, int depth, bool out = false){
        if (depth == 0)
            return board.score(color);
        long long valid = board.validpos(color);
        //cout << valid << endl;
        if (__builtin_popcountll(valid) == 0){
            if (board.gameover())
                return board.score(color);
            changecolor();
            int value =  -alphabeta(-beta, -alpha, depth-1);
            changecolor();
            return value;
        }
        int pos = 0;
        for (long long temp = valid; temp; temp ^= (1LL << pos)){
            pos = __builtin_ctzll(temp);
            board.set_cancel(pos,color);
            changecolor();
            int value = -alphabeta(-beta, -alpha, depth-1);
            if (value >= beta)
                return beta;
            if (value > alpha){
                alpha = value;
                if (out)
                    outpos = pos;
            }
            board.set_cancel(pos,color);
            changecolor();
        }
        return alpha;
    }
    void debug()
    {
        for (int i = 0; i < gridsize; ++i){
            for (int j = 0; j < gridsize; ++j){
                if (board.currstate(i, j, black))
                    cout << "b ";
                else if (board.currstate(i, j, white))
                    cout << "w ";
                else
                    cout << ". ";
            }
            cout << "\n";
        }
    }
    void botoutput(){
        Json::Value ret;
        ret["response"]["x"] = outpos >> 3;
        ret["response"]["y"] = outpos & 7;
        Json::FastWriter writer;
        cout << outpos;
        cout << writer.write(ret) << endl;
    }
};

int main() {
    initialize();
    bot syys;
    syys.botinput();
    syys.debug();
    syys.alphabeta(scoremin, scoremax, 2, true);
    //syys.debug();
    syys.botoutput();
    return 0;
}