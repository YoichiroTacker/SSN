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

// timestamp counterのlock
int counter = 0;
std::mutex countmtx_;

// Tsstore用lock
std::mutex tsstoremtx_;

// transaction表示用のlock
std::mutex showmtx_;

/*void *count(void *arg)
{
    struct counter *a = (struct counter *)arg;

    pthread_mutex_lock(a->lock);
    for (int i = 0; i < 1000 * 1000; i++)
    {
        (*(int *)a->num)++;
    }
    pthread_mutex_unlock(a->lock);

    return NULL;
}*/

// database
std::vector<std::vector<class nodedef>> database(N);
// database用mutex
std::mutex dbmtx_;

class concurrenttx_idetifier
{
public:
    int startTs;
    int commitTs;
};

std::vector<class concurrenttx_idetifier> Tsstore; // timestamp store

// timestamp
int gettimestamp(void)
{
    // counterのlockを取得
    std::lock_guard<std::mutex> lock(countmtx_);
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
    // std::vector<std::vector<class nodedef>> database(N);
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

void setstartTs(const txdef *tx)
{
    std::lock_guard<std::mutex> lock(tsstoremtx_);
    concurrenttx_idetifier tmp;
    tmp.startTs = tx->startTs;
    Tsstore.push_back(tmp);
}

// transactionの生成
txdef generate_transaction()
{
    txdef transaction;
    int operation_size = get_rand(1, 10);
    transaction.startTs = gettimestamp();
    transaction.status = 0;
    // TsstoreにstartTsを追加する
    setstartTs(&transaction);

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
    return;
}

// commit phase
void commit(txdef *tx, std::vector<std::vector<class nodedef>> &database)
{
    // databaseのlockを取得
    std::lock_guard<std::mutex> lock(dbmtx_);
    for (auto p = tx->worker.begin(); p != tx->worker.end(); p++)
    {
        nodedef tmp;
        tmp.key = p->key;
        tmp.nversion = p->nversion;
        database.at(p->dataitem).push_back(tmp);
    }
    return;
}

// database(vector)の中身を表示
void show_database(void)
{
    for (int i = 0; i < N; i++)
    {
        printf("%d ", database[i][database.at(i).size() - 1].key);
        // std::cout << data[i][data.at(i).size() - 1] << std::endl;
    }
}

void setcommitTs(txdef *tx)
{
    // commit timestampを設定
    tx->commitTs = gettimestamp();
    // Tsstoreのlockを取得
    std::lock_guard<std::mutex> lock(tsstoremtx_);
    // 判定を行うためにTsstoreにcommitTsを追加する
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        if (p->startTs == tx->startTs)
        {
            p->commitTs = tx->commitTs;
            break;
        }
    }
}

// validation phase
void validation(txdef *tx, std::vector<std::vector<class nodedef>> &database)
{
    setcommitTs(tx);

    //   判定
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        // first committers wins
        if (p->startTs < tx->startTs && tx->startTs < p->commitTs)
        {
            // abort
            tx->status = 2;
            return;
        }
    }
    // commit
    tx->status = 1;
    commit(tx, database);
    return;
}

void show_identifier(void)
{
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        std::cout << p->startTs << std::endl;
        std::cout << p->commitTs << std::endl;
    }
}

// transactionの中身を表示
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
    // transaction t1の実行
    txdef t1 = generate_transaction();

    execution(&t1);
    validation(&t1, database);
    show_transaction(&t1);
}

void ThreadB()
{
    // transaction t2の実行
    txdef t2 = generate_transaction();

    execution(&t2);
    validation(&t2, database);
    show_transaction(&t2);
}

void ThreadC()
{
    // transaction t3の実行
    txdef t3 = generate_transaction();

    execution(&t3);
    validation(&t3, database);
    show_transaction(&t3);
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

    show_identifier();

    // databaseの状態を表示
    show_database();

    return 0;
}