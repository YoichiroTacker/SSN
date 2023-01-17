#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iostream>

#define N 10
#define NUM_THREAD 2

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

int array[N];

/*typedef struct
{
    int readset;
    int writeset;
    int transaction_status;  0=in-flight, 1=commit, 2=abort
} Transaction;
*/

enum class txop
{
    r,
    w,
};

class txdefinition
{
public:
    txop operation;
    int dataitem;

    txdefinition(void);

    txop getoperation() { return operation; }
    int getdataitem() { return dataitem; }
};

txdefinition::txdefinition(void)
{
    operation = txop::r;
    dataitem = 0;
}

/*int makeDB()
{
    int array[N] = (int *)malloc(sizeof(int *) * N);
    for (int i = 0; i < N; i++)
    {
        array[i] = NULL;
    }
}*/

std::vector<txdefinition> generate_transaction()
{
    std::vector<txdefinition> transaction;
    // transaction(txop x, int y);
    txdefinition txdefinition;

    return transaction;
}

void begin_transaction()
{
}

void write(int dataitem, int key)
{
    array[dataitem] = key;
}

void read(int dataitem)
{
    int key = array[dataitem];
    return;
}

void ssn_read(int t, int v)
{
}

/*Transaction commit(Transaction t)
{
    if (t.transaction_status = 0)
    {
        t.transaction_status = 1;
    }
    return t;
}*/

void end_transaction()
{
}

void show_array()
{
    for (int i = 0; i < N; i++)
    {
        printf("%d ", array[i]);
    }
}

int main()
{
    // int array[] = makeDB();

    for (int i = 0; i < N; i++)
    {
        array[i] = 0;
    }

    std::vector<txdefinition> t1;
    t1 = generate_transaction();
    // t1.emplace_back(txop::r, 1);

    for (size_t i = 0; i < t1.size(); ++i)
    {
        std::cout << t1.getoperation(i) << t1.getdataitem(i) << std::endl;
    }

    /*begin_transaction();
    Transaction t;

    write(5, 3);
    write(3, 4);
    write(1, 1);
    read(3);
    read(1);*/

    // commit(t);

    show_array();
}
