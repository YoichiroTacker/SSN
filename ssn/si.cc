#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <sys/time.h>
#include <ctime>
#include <thread>
#include <mutex>

#define N 10 // databaseのサイズ
#define M 3  // 1threadあたりにtransactionが繰り返す回数

int counter = 0;      // timestamp用のcounter
std::mutex countmtx_; // timestampのlock

std::mutex showmtx_; // transaction表示用のlock

std::vector<std::vector<class nodedef>> database(N); // database
std::mutex dbmtx_;                                   // database用のlock

class tsstoredef
{
public:
    int startTs;
    int commitTs;
};

std::vector<class tsstoredef> Tsstore; // timestamp store
std::mutex tsstoremtx_;                // Tsstore用lock

// timestamp
int gettimestamp(void)
{
    std::lock_guard<std::mutex> lock(countmtx_); // counterのlockを取得
    counter++;
    return counter;
}

enum class txop
{
    READ,
    WRITE,
};

class nodedef
{
public:
    int nversion;
    int key;
};

class workernodedef
{
public:
    int nversion;
    int key;
    int dataitem;
};

class ope
{
public:
    txop operation; // READ or WRITE
    int dataitem;   // writeしたいデータの位置
    int key;        // writeしたいデータの値
};

class txdef
{
public:
    std::vector<ope> txinfo;
    int status; // inflight=0, commit=1, abort=2
    int startTs;
    int commitTs;
    std::vector<workernodedef> worker;
};

// databaseを生成して初期化
void generate_database(void)
{
    nodedef tmp;
    for (int i = 0; i < N; i++)
    {
        tmp.key = 0;
        tmp.nversion = 0;
        database.at(i).push_back(tmp);
    }
    return;
}

// min_valからmax_valの範囲で乱数の生成
uint64_t get_rand(uint64_t min_val, uint64_t max_val)
{
    static std::mt19937_64 mt64(time(NULL));
    std::uniform_int_distribution<uint64_t> get_rand_uni_int(min_val, max_val);
    return get_rand_uni_int(mt64);
}

// transactionの生成
txdef generate_transaction()
{
    txdef transaction;
    int operation_size = get_rand(1, 10);
    transaction.startTs = gettimestamp();
    transaction.status = 0;

    for (int i = 0; i < operation_size; i++)
    {
        ope tmp;
        if (get_rand(0, 100) % 2 == 0)
        {
            tmp.operation = txop::READ;
        }
        else
        {
            tmp.operation = txop::WRITE;
            tmp.key = get_rand(0, 100);
        }
        tmp.dataitem = get_rand(0, N - 1);
        transaction.txinfo.emplace_back(tmp);
    }
    return transaction;
}

// write operation
void write(std::vector<workernodedef> &worker, int key, int dataitem, int startTs)
{
    workernodedef tmp;
    tmp.key = key;
    tmp.nversion = startTs;
    tmp.dataitem = dataitem;
    worker.push_back(tmp);
    return;
}

// read operation
void read(int dataitem)
{
    // int key = array[dataitem];
    return;
}

// transactionの実行
void execution(txdef *tx)
{
    for (auto p = tx->txinfo.begin(); p != tx->txinfo.end(); p++)
    {
        if (p->operation == txop::READ)
        {
            read(p->dataitem);
        }
        if (p->operation == txop::WRITE)
        {
            write(tx->worker, p->key, p->dataitem, tx->startTs);
        }
    }
}

// commit phase
void commit(txdef *tx)
{
    std::lock_guard<std::mutex> lock(dbmtx_); // databaseのlockを取得
    for (auto p = tx->worker.begin(); p != tx->worker.end(); p++)
    {
        nodedef tmp;
        tmp.key = p->key;
        tmp.nversion = p->nversion;
        database.at(p->dataitem).push_back(tmp);
    }
}

void setTimestamp(txdef *tx) // timestampを設定
{
    std::lock_guard<std::mutex> lock(tsstoremtx_);
    tsstoredef tmp;
    tmp.startTs = tx->startTs;
    tmp.commitTs = tx->commitTs;
    Tsstore.push_back(tmp);
}

void validation(txdef *tx);

void rollback(txdef *tx)
{
    tx->status = 0;
    tx->startTs = gettimestamp();
    tx->commitTs = 0;
    tx->worker.clear();
    execution(tx);
    validation(tx);
}

void Tscomparison(txdef *tx)
{
    std::lock_guard<std::mutex> lock(tsstoremtx_);
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        // first committers wins
        if ((p->startTs <= tx->startTs && tx->startTs < p->commitTs) || (tx->startTs <= p->startTs && p->startTs < tx->commitTs))
        {
            // abort
            tx->status = 2;
            return;
        }
    }
    return;
}

// validation phase
void validation(txdef *tx)
{
    //   read only transactionの場合、automarically commit
    bool isEmpty = tx->worker.empty();
    if (isEmpty == true)
    {
        tx->status = 1;
        tx->commitTs = 0;
        return;
    }
    else
    {
        tx->commitTs = gettimestamp();
        Tscomparison(tx); // 判定

        if (tx->status == 2)
        {
            // abort
            rollback(tx);
        }
        else if (tx->status == 0)
        {
            // commit
            setTimestamp(tx);
            tx->status = 1;
            commit(tx);
        }
    }
}

void show_database(void)
{
    std::cout << " " << std::endl;
    std::cout << "-------database--------" << std::endl;
    for (int i = 0; i < N; i++)
    {
        printf("%d ", database[i][database.at(i).size() - 1].key);
    }
}

void show_Tsstore(void)
{
    std::cout << "----保存されているtimestamp----" << std::endl;
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        printf("(%d,%d)", p->startTs, p->commitTs);
    }
}

void show_transaction(txdef *tx)
{
    std::lock_guard<std::mutex> lock(showmtx_);
    std::cout << "--------NEW TRANSACTION--------" << std::endl;
    if (tx->status == 1)
    {
        std::cout << "transaction status: committed" << std::endl;
    }
    else if (tx->status == 2)
    {
        std::cout << "transaction status: aborted" << std::endl;
    }
    else
    {
        std::cout << "transaction status:" << tx->status << std::endl;
    }
    std::cout << "startTs:" << tx->startTs << " commitTs:" << tx->commitTs << std::endl;

    for (auto p = tx->txinfo.begin(); p != tx->txinfo.end(); p++)
    {
        if (p->operation == txop::READ)
        {
            std::cout << "READ vector["
                      << p->dataitem << "]" << std::endl;
        }
        else
        {
            std::cout << "WRITE vector[" << p->dataitem << "] " << p->key << std::endl;
        }
    }
    return;
}

void ThreadA()
{
    // transaction txの実行
    for (int i = 0; i < M; i++)
    {
        txdef *tx = (txdef *)malloc(sizeof(txdef));
        *tx = generate_transaction();
        execution(tx);
        validation(tx);
        show_transaction(tx);
        free(tx);
    }
}

void ThreadB()
{
    // transaction txの実行
    for (int i = 0; i < M; i++)
    {
        txdef *tx = (txdef *)malloc(sizeof(txdef));
        *tx = generate_transaction();
        execution(tx);
        validation(tx);
        show_transaction(tx);
        free(tx);
    }
}

void ThreadC()
{
    // transaction txの実行
    for (int i = 0; i < M; i++)
    {
        txdef *tx = (txdef *)malloc(sizeof(txdef));
        *tx = generate_transaction();
        execution(tx);
        validation(tx);
        show_transaction(tx);
        free(tx);
    }
}

int main()
{
    generate_database();

    std::thread th_a(ThreadA);
    std::thread th_b(ThreadB);
    std::thread th_c(ThreadC);

    th_a.join();
    th_b.join();
    th_c.join();

    show_Tsstore();  // Tsstoreを表示
    show_database(); // databaseを表示
}