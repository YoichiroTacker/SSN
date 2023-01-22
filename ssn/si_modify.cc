#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <sys/time.h>
#include <ctime>

#define N 10

class concurrenttx_idetifier
{
public:
    int startTs;
    int commitTs;
};

std::vector<class concurrenttx_idetifier> Tsstore;

// timestamp
int gettimestamp(void)
{
    int millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return millisec_since_epoch;
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
    int data;
    int dataitem;
};

class ope
{
public:
    txop operation;              // READ or WRITE
    int dataitem;                // writeしたいデータの位置
    int key;                     // writeしたいデータの値
};

class txdef
{
public:
    std::vector<ope> txinfo;
    int status = 0; // inflight=0, commit=1, abort=2
    int startTs;
    int commitTs;
    std::vector<nodedef> worker;
};

// databaseを生成して初期化
std::vector<std::vector<class nodedef>> generate_database(void)
{
    std::vector<std::vector<class nodedef>> database(N);
    nodedef tmp;
    for (int i = 0; i < N; i++)
    {
        tmp.data = 0;
        tmp.nversion = 0;
        database.at(i).push_back(tmp);
    }
    return database;
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
    // TsstoreにstartTsを追加する
    concurrenttx_idetifier tmp;
    tmp.startTs = transaction.startTs;
    Tsstore.push_back(tmp);

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
        transaction.txinfo.push_back(tmp);
    }
    return transaction;
}

// write operation
nodedef write(txdef transaction, int key, int dataitem)
{
    nodedef tmp;
    tmp.data = key;
    tmp.nversion = transaction.startTs;
    tmp.dataitem = dataitem;
    transaction.worker.push_back(tmp);
    return tmp;
}

// read operation
void read(int dataitem)
{
    // int key = array[dataitem];
    return;
}

// transactionの実行
txdef execution(txdef tx)
{
    for (auto p = tx.txinfo.begin(); p != tx.txinfo.end(); p++)
    {
        if (p->operation == txop::READ)
        {
            read(p->dataitem);
        }
        if (p->operation == txop::WRITE)
        {
           write(tx, p->key, p->dataitem);
        }
    }
    return tx;
}

//commit phase
std::vector<std::vector<class nodedef>> commit(txdef tx, std::vector<std::vector<class nodedef>>database){
for (auto p=tx.worker.begin(); p !=tx.worker.end(); p++){
    database.at(p->dataitem).emplace_back(p);
}
return database;
}


// database(vector)の中身を表示
void show_database(std::vector<std::vector<class nodedef>> database)
{
    for (int i = 0; i < N; i++)
    {
        printf("%d ", database[i][database.at(i).size() - 1].data);
        // std::cout << data[i][data.at(i).size() - 1] << std::endl;
    }
}

// validation phase
std::vector<std::vector<class nodedef>> validation(txdef tx, std::vector<std::vector<class nodedef>>database)
{
    // commit timestampを設定
    tx.commitTs = gettimestamp();
    // 判定を行うためにTsstoreにcommitTsを追加する
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        if (p->startTs == tx.startTs)
        {
            p->commitTs = tx.commitTs;
            break;
        }
    }
    // 判定
    for (auto p = Tsstore.begin(); p != Tsstore.end(); p++)
    {
        // first committers wins
        if (p->startTs <= tx.startTs && tx.startTs <= p->commitTs)
        {
            tx.status = 2;
            return database;
        }
    }
    // commit
        tx.status = 1;
        database= commit(tx, database);
    return database;
}

// transactionの中身を表示
void show_transaction(txdef transaction)
{
    std::cout << "new transaction" << std::endl;
    for (auto p = transaction.txinfo.begin(); p != transaction.txinfo.end(); p++)
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
}

int main()
{
    std::vector<std::vector<class nodedef>> database = generate_database();

    // transaction t1の実行
    txdef t1 = generate_transaction();
    t1 = execution(t1);
    // show_transaction(t1);
    database = validation(t1,database);
    // std::cout << t1.begin()->status << std::endl;

    // transaction t2の実行
    txdef t2 = generate_transaction();
    t2 = execution(t2);
    // show_transaction(t2);
    database = validation(t2,database);
    // std::cout << t2.begin()->status << std::endl;

    // databaseの状態を表示
    show_database(database);
}