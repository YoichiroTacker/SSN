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
#include <algorithm>
#include <math.h>
#include <pthread.h>

#define N 10 // databaseのサイズ
#define NUM_THREAD 3

int counter = 0;           // timestamp用のcounter
pthread_mutex_t countmtx_; // timestampのlock
pthread_mutex_t showmtx_;  // transaction表示用のlock

std::vector<std::vector<class nodedef>> database(N); // database
pthread_mutex_t dbmtx_;                              // database用のlock

class txtabledef
{
public:
    int t_sstamp;
    int t_cstamp;
};

std::vector<class txtabledef> transaction_table; // timestamp store
pthread_mutex_t txtablemtx_;                     // transaction_table用lock

// timestamp
int gettimestamp(void)
{
    pthread_mutex_lock(&countmtx_);
    counter++;
    pthread_mutex_unlock(&countmtx_);
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
    int v_cstamp;    // version creation stamp
    int v_etastamp;  // version access stamp(mada tukattenai=0)
    int v_pistamp;   // version successor stamp(mada tukattenai=00)
    nodedef *v_prev; // pointer to the overritten version
    int key;
    int tuple;
};

class readsetdef
{
public:
    int v_etastamp;
    int v_pistamp;
    int key;
    int tuple;
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

class txdef
{
public:
    std::vector<ope> txinfo;
    txstatus status; // inflight, committed, aborted
    int t_sstamp;    // transaction start timestamp
    int t_cstamp;    // transaction commit timestamp
    int t_etastamp;  // predecessor high watermark η(T)
    int t_pistamp;   // successor low watermark π(T)
    std::vector<nodedef> wworker;
    std::vector<readsetdef> rworker;
};

std::vector<class txdef> tx_showtable; // transaction表示用vector

// databaseを生成して初期化
void generate_database(void)
{
    nodedef tmp;
    for (int i = 0; i < N; i++)
    {
        tmp.v_etastamp = 0;
        tmp.v_pistamp = 50000;
        tmp.v_prev = NULL;
        tmp.tuple = i;
        tmp.key = 0;
        tmp.v_cstamp = 0;
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
    txdef tx;
    int operation_size = get_rand(1, 10);
    // tx.t_sstamp = gettimestamp();
    tx.t_sstamp = 0;
    tx.status = txstatus::inflight;
    tx.t_pistamp = 5000;

    tx.t_cstamp = 0;
    tx.t_etastamp = 0;

    for (int i = 0; i < operation_size; i++)
    {
        ope tmp;
        if (get_rand(0, 100) % 2 == 0)
        {
            tmp.operation = txop::READ;
            tmp.key = 0;
        }
        else
        {
            tmp.operation = txop::WRITE;
            tmp.key = get_rand(0, 100);
        }
        tmp.tuple = get_rand(0, N - 1);
        tx.txinfo.emplace_back(tmp);
    }
    return tx;
}

void start_transaction(txdef *tx) // transactionのstart timestampを取得
{
    tx->t_sstamp = gettimestamp();
}

nodedef *specify_latestnodeversion(txdef *tx, int openum) // readを行うnode, overwriteするnodeを取得
{
    nodedef *version = &database.at(tx->txinfo.at(openum).tuple).front();
    for (int v = 0; v < database.at(tx->txinfo.at(openum).tuple).size(); v++)
    {
        if (database.at(tx->txinfo.at(openum).tuple).at(v).v_cstamp < tx->t_sstamp)
        {
            version = &database.at(tx->txinfo.at(openum).tuple).at(v);
        }
    }
    return version;
}

void verify_exclusion_or_abort(txdef *tx) // exclusion window testing
{
    if (tx->t_pistamp <= tx->t_etastamp)
    {
        tx->status = txstatus::aborted;
    }
}

void ssn_write(txdef *tx, nodedef *version, int i) // 論文に準拠
{
    // if v not in t.writes:
    // update \eta(t) with w:r edge
    tx->t_etastamp = std::max<int>(tx->t_etastamp, version->v_etastamp);
    // t.writes.add(v)
    nodedef tmp;
    tmp.key = tx->txinfo.at(i).key;
    tmp.v_cstamp = tx->t_sstamp;
    tmp.tuple = tx->txinfo.at(i).tuple;
    tmp.v_prev = version;
    tmp.v_etastamp = tx->t_etastamp;
    tmp.v_pistamp = tx->t_pistamp;
    tx->wworker.push_back(tmp);

    // t.reads.discard(v); avoid false positive
    for (auto p = tx->rworker.begin(); p != tx->rworker.end(); p++)
    {
        if (p->key == version->key && p->tuple == version->tuple)
        {
            tx->rworker.erase(p);
        }
    }
    // verify_exclusion_or_abort(t)
    verify_exclusion_or_abort(tx);
}

void ssn_read(txdef *tx, nodedef *version, int num) // 論文に準拠
{

    // if v not in t.writes:(read関数で行う)
    //  update \eta(t) with w:r edges
    tx->t_etastamp = std::max(tx->t_etastamp, version->v_cstamp);

    if (version->v_pistamp == INFINITY)
    {
        // t.reads.add(v);
        readsetdef tmp;
        tmp.key = version->key;
        tmp.tuple = version->tuple;
        tmp.v_pistamp = version->v_pistamp;
        tx->rworker.push_back(tmp);
        // tx->txinfo.at(num).key = version->key;
    }
    else
    {
        // update \pi(t) with r:w edge
        tx->t_pistamp = std::min<int>(tx->t_pistamp, version->v_pistamp);
    }
    // verify_exclusion_or_abort(t)
    verify_exclusion_or_abort(tx);
}

void ssn_execution(txdef *tx) // transactionの実行
{
    nodedef *version;

    for (int i = 0; i < tx->txinfo.size(); i++)
    {
        version = specify_latestnodeversion(tx, i);
        if (tx->txinfo.at(i).operation == txop::READ)
        {
            // read(tx, version);
            ssn_read(tx, version, i);
        }
        else if (tx->txinfo.at(i).operation == txop::WRITE)
        {
            // write(tx, version, i);
            ssn_write(tx, version, i);
        }
    }
}

void commit(txdef *tx) // transactionをcommit
{
    pthread_mutex_lock(&dbmtx_);
    for (auto p = tx->wworker.begin(); p != tx->wworker.end(); p++)
    {
        database.at(p->tuple).push_back(*p);
    }
    pthread_mutex_unlock(&dbmtx_);
}

void setTimestamp(txdef *tx) // timestampを設定
{
    txtabledef tmp;
    tmp.t_sstamp = tx->t_sstamp;
    tmp.t_cstamp = tx->t_cstamp;
    transaction_table.push_back(tmp);
}

void SIcomparison(txdef *tx)
{
    if (tx->status == txstatus::aborted)
    {
        return;
    }
    else
    {
        //   read only transactionの場合、automarically commit
        bool isEmpty = tx->wworker.empty();
        if (isEmpty == true)
        {
            tx->status = txstatus::committed;
            return;
        }
        else
        {
            pthread_mutex_lock(&txtablemtx_);
            isEmpty = transaction_table.empty();
            if (isEmpty == false)
            {
                for (auto p = transaction_table.begin(); p != transaction_table.end(); p++)
                {
                    // first committers wins
                    if (((p->t_sstamp < tx->t_sstamp && tx->t_sstamp < p->t_cstamp) || (tx->t_sstamp < p->t_sstamp && p->t_sstamp < tx->t_cstamp)) && p->t_cstamp < tx->t_cstamp)
                    {
                        tx->status = txstatus::aborted; // abort
                    }
                    else
                    {
                        tx->status = txstatus::committed; // commit
                    }
                }
            }
            else // isEmpty == true(transaction_tableが空なら)
            {
                tx->status = txstatus::committed;
            }

            if (tx->status == txstatus::committed)
            {
                setTimestamp(tx); // transaction_tableに追加
            }

            pthread_mutex_unlock(&txtablemtx_);
        }
    }
}

void ssn_commit(txdef *tx) // 論文に準拠
{
    if (tx->status == txstatus::inflight)
    {
        tx->t_cstamp = gettimestamp();
    }

    // finalize \pi(T)
    tx->t_pistamp = std::min<int>(tx->t_pistamp, tx->t_cstamp);
    for (auto p = tx->rworker.begin(); p != tx->rworker.end(); p++)
    {
        tx->t_pistamp = std::min<int>(tx->t_pistamp, p->v_pistamp);
    }

    // finalize \eta(T)
    for (auto p = tx->wworker.begin(); p != tx->wworker.end(); p++)
    {
        tx->t_etastamp = std::max<int>(tx->t_etastamp, p->v_prev->v_etastamp);
    }

    // verify_exclusion_or_abort(t)
    verify_exclusion_or_abort(tx); // exclusion window
    SIcomparison(tx);              // SI verification
    tx_showtable.push_back(*tx);
    if (tx->status == txstatus::committed) // post-commit begins
    {

        // update \eta(v)
        for (auto p = tx->rworker.begin(); p != tx->rworker.end(); p++)
        {
            p->v_etastamp = std::max<int>(p->v_etastamp, tx->t_cstamp);
        }
        // update \pi(v)
        for (auto p = tx->wworker.begin(); p != tx->wworker.end(); p++)
        {
            p->v_prev->v_pistamp = tx->t_pistamp;
            // initialize new version
            p->v_cstamp = p->v_etastamp = tx->t_cstamp;
        }
        commit(tx);
    }
}

void show_database(void)
{
    std::cout << " " << std::endl;
    std::cout << "-------database--------" << std::endl;
    for (int i = 0; i < N; i++)
    {
        printf("vector[");
        printf("%d", i);
        printf("] ");
        for (int j = 0; j < database.at(i).size(); j++)
        {
            printf("%d ", database[i][j].key);
        }
        std::cout << " " << std::endl;
    }
}
void show_txtable(void)
{
    std::cout << "----保存されているtimestamp----" << std::endl;
    for (auto p = transaction_table.begin(); p != transaction_table.end(); p++)
    {
        printf("(%d,%d)", p->t_sstamp, p->t_cstamp);
    }
}
void show_transaction(void)
{
    for (auto p = tx_showtable.begin(); p != tx_showtable.end(); p++)
    {
        std::cout << "--------NEW TRANSACTION--------" << std::endl;
        if (p->status == txstatus::committed)
        {
            std::cout << "transaction status: committed" << std::endl;
        }
        else if (p->status == txstatus::aborted)
        {
            std::cout << "transaction status: aborted" << std::endl;
        }
        else if (p->status == txstatus::inflight)
        {
            std::cout << "transaction status:inflight" << std::endl;
        }

        std::cout << "startTimestamp:" << p->t_sstamp << " commitTimestamp:" << p->t_cstamp << std::endl;
        std::cout << "pistamp:" << p->t_pistamp << " etastamp:" << p->t_etastamp << std::endl;

        for (auto q = p->txinfo.begin(); q != p->txinfo.end(); q++)
        {
            if (q->operation == txop::READ)
            {
                std::cout << "READ vector[" << q->tuple << "]" << /*q->key <<*/ std::endl;
            }
            else
            {
                std::cout << "WRITE vector[" << q->tuple << "] " << q->key << std::endl;
            }
        }
    }
}

void *function(void *arg)
{
    std::vector<txdef> *tx = (std::vector<txdef> *)arg;

    //  transaction txの実行
    for (auto p = tx->begin(); p != tx->end(); p++)
    {
        start_transaction(&(*p));
        ssn_execution(&(*p));
        ssn_commit(&(*p));
    }

    return NULL;
}

int main()
{
    pthread_mutex_init(&countmtx_, NULL);
    pthread_mutex_init(&showmtx_, NULL);
    pthread_mutex_init(&dbmtx_, NULL);

    generate_database();

    pthread_t thread[NUM_THREAD];

    // transactionの生成
    std::vector<std::vector<class txdef>> txtx(NUM_THREAD, std::vector<txdef>(0));
    for (int i = 0; i < NUM_THREAD; i++)
    {
        int NUM_transaction = get_rand(1, 5);
        for (int j = 0; j < NUM_transaction; j++)
        {
            txdef tmp;
            tmp = generate_transaction();
            txtx.at(i).push_back(tmp);
        }
    }

    for (int i = 0; i < NUM_THREAD; i++)
    {
        pthread_create(&thread[i], NULL, function, &txtx[i]);
    }

    for (int i = 0; i < NUM_THREAD; i++)
    {
        pthread_join(thread[i], NULL);
    }

    show_transaction(); // 　transactionの表示
    show_txtable();     // transaction tableを表示
    show_database();    // databaseを表示

    pthread_mutex_destroy(&countmtx_);
    pthread_mutex_destroy(&showmtx_);
    pthread_mutex_destroy(&dbmtx_);
}