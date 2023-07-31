#ifndef SUB_H
#define SUB_H
//------------------------------------------------
#include <stdio.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <limits>

//------------------------------------------------
//  マクロ定義(Macro definition)
//------------------------------------------------
#define MAX_NUM_TRANSACTION 10 // 1スレッドで処理する最大トランザクション数
#define MIN_NUM_TRANSACTION 10 // 1スレッドで処理する最小トランザクション数
#define MAX_NUM_OPERATION 10   // 1トランザクションあたりの最大オペレーション数
#define MIN_NUM_OPERATION 10   // 1トランザクションあたりの最小オペレーション数
// read:write = 1:1

//------------------------------------------------
//  型定義(Type definition)
//------------------------------------------------
enum class txop
{
    READ,
    WRITE,
};

class ope
{
public:
    txop operation; // READ or WRITE
    int tuple;      // writeしたいデータの位置
    int key;        // writeしたいデータの値
};

enum class txstatus
{
    inflight,
    committed,
    aborted
};

class nodedef
{
public:
    int v_cstamp = 0;                                         // version creation stamp
    int v_etastamp = 0;                                       // version access stamp(mada tukattenai=0)
    float v_pistamp = std::numeric_limits<float>::infinity(); // version successor stamp(mada tukattenai=00)
    nodedef *v_prev = NULL;                                   // pointer to the overritten version
    int key = 0;
    int tuple = 0;
    int initialized = 0;
    // nodedef() {}
};

/*class readsetdef
{
public:
    int v_etastamp;
    int v_pistamp;
    int key;
    int tuple;
};*/

class txdef
{
public:
    int tid;
    std::vector<ope> txinfo;
    txstatus status; // inflight, committed, aborted
    int t_sstamp;    // transaction start timestamp
    int t_cstamp;    // transaction commit timestamp
    int t_etastamp;  // predecessor high watermark η(T)
    int t_pistamp;   // successor low watermark π(T)
    std::vector<nodedef> wworker;
    std::vector<nodedef> rworker;
};

//------------------------------------------------
//  プロトタイプ宣言(Prototype declaration)
//------------------------------------------------

//------------------------------------------------
#endif