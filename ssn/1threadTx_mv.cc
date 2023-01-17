#include <stdio.h>
#include <stdlib.h>
// #include <time.h>
#include <vector>
#include <random>
#include <iostream>

#define N 10

// database→N✖︎0の行列
std::vector<std::vector<int>> data(N);

enum class txop
{
    READ,
    WRITE,
};

class txdefinition
{
public:
    txop operation;
    int dataitem;
    int key;
    int status = 0; // inflight=0, commit=1, abort=2

    txop getoperation()
    {
        return operation;
    }

    int getdataitem()
    {
        return dataitem;
    }
    int getkey()
    {
        return key;
    }

    int getstatus()
    {
        return status;
    }
};

void reset_data()
{
    for (int i = 0; i < N; i++)
    {
        data.at(i).push_back(0);
    }
}

// min_valからmax_valの範囲で乱数の生成
uint64_t get_rand(uint64_t min_val, uint64_t max_val)
{
    static std::mt19937_64 mt64(time(NULL));
    std::uniform_int_distribution<uint64_t> get_rand_uni_int(min_val, max_val);
    return get_rand_uni_int(mt64);
}

// transactionの生成
std::vector<txdefinition> generate_transaction()
{
    std::vector<txdefinition> transaction;
    int operation_size = get_rand(1, 10);
    txdefinition tmp;
    for (int i = 0; i < operation_size; i++)
    {
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
        transaction.push_back(tmp);
    }
    return transaction;
}

// write operation
void write(int dataitem, int key)
{
    data.at(dataitem).push_back(key);
}

// read operation
void read(int dataitem)
{
    // int key = array[dataitem];
    return;
}

// commit operation
void commit(std::vector<txdefinition> transaction)
{
    if (transaction.begin()->getstatus() == 0)
        transaction.begin()->status = 1;
    return;
}

// database(vector)の状態を表示
void show_data()
{
    for (int i = 0; i < N; i++)
    {
        printf("%d ", data[i][data.at(i).size() - 1]);
    }
}

// transactionの実行
void execution(std::vector<txdefinition> transaction)
{
    for (auto p = transaction.begin(); p != transaction.end(); p++)
    {
        if (p->getoperation() == txop::READ)
        {
            read(p->getdataitem());
        }
        if (p->getoperation() == txop::WRITE)
        {
            write(p->getdataitem(), p->getkey());
        }
    }
}

// transactionの中身を表示
void show_transaction(std::vector<txdefinition> transaction)
{
    std::cout << "new transaction" << std::endl;
    for (auto p = transaction.begin(); p != transaction.end(); p++)
    {
        if (p->getoperation() == txop::READ)
        {
            std::cout << "READ array["
                      << p->getdataitem() << "]" << std::endl;
        }
        else
        {
            std::cout << "WRITE array[" << p->getdataitem() << "] " << p->getkey() << std::endl;
        }
    }
}

int main()
{
    // databaseを初期化
    reset_data();

    // transaction t1の実行
    std::vector<txdefinition> t1 = generate_transaction();
    execution(t1);
    // show_transaction(t1);
    commit(t1);

    // transaction t2の実行
    std::vector<txdefinition> t2 = generate_transaction();
    execution(t2);
    // show_transaction(t2);
    commit(t2);

    // databaseの状態を表示
    show_data();
}