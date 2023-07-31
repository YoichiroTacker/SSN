#include <stdlib.h>
#include <time.h>
#include <random>
#include <chrono>
#include <sys/time.h>
#include <ctime>
#include <thread>
#include <algorithm>
#include <math.h>
#include <pthread.h>
#include <atomic>
#include <iostream>
#include <array>
#include <stdint.h>

//#include "result.hh"
#include "transaction.hh"
#include "util.hh"
#include "common.hh"
#include "ermia_op_element.hh"
#include "garbage_collection.hh"
#include "lock.hh"
#include "transaction_status.hh"
#include "transaction_table.hh"
#include "tuple.hh"
#include "version.hh"

using namespace std;

#define N 5          // databaseのサイズ
#define NUM_THREAD 2 // Thread数


void displayDB() {
  Tuple *tuple;
  Version *version;

  for (unsigned int i = 0; i < FLAGS_tuple_num; ++i) {
    tuple = &Table[i];
    cout << "------------------------------" << endl;  // - 30
    cout << "key: " << i << endl;

    version = tuple->latest_.load(std::memory_order_acquire);
    while (version != NULL) {
      cout << "val: " << version->val_ << endl;

      switch (version->status_) {
        case VersionStatus::inFlight:
          cout << "status:  inFlight";
          break;
        case VersionStatus::aborted:
          cout << "status:  aborted";
          break;
        case VersionStatus::committed:
          cout << "status:  committed";
          break;
      }
      cout << endl;

      cout << "cstamp:  " << version->cstamp_ << endl;
      cout << "pstamp:  " << version->psstamp_.pstamp_ << endl;
      cout << "sstamp:  " << version->psstamp_.sstamp_ << endl;
      cout << endl;

      version = version->prev_;
    }
    cout << endl;
  }
}


void makeDB() {
  if (posix_memalign((void **) &Table, PAGE_SIZE, (FLAGS_tuple_num) * sizeof(Tuple)) !=
      0)

  size_t maxthread = decideParallelBuildNumber(FLAGS_tuple_num);
  std::vector<std::thread> thv;
  for (size_t i = 0; i < maxthread; ++i)
    thv.emplace_back(partTableInit, i, i * (FLAGS_tuple_num / maxthread),
                     (i + 1) * (FLAGS_tuple_num / maxthread) - 1);
  for (auto &th : thv) th.join();
}

int main(int argc, char *argv[]) try {
    makeDB();
    displayDB();
}
