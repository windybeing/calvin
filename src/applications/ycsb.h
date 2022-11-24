// Author: Alexander Thomson (thomson@cs.yale.edu)
// Author: Kun Ren (kun@cs.yale.edu)
//
// A YCSB application that reads all elements of the read_set, does
// some trivial computation, and writes to all elements of the write_set.

#ifndef _DB_APPLICATIONS_YCSB_H_
#define _DB_APPLICATIONS_YCSB_H_

#include <set>
#include <string>

#include "applications/application.h"

using std::set;
using std::string;

class YCSB : public Application {
 public:
  enum TxnType {
    INITIALIZE = 0,
    MICROTXN_SP = 1,
    MICROTXN_MP = 2,
  };

  YCSB(int nodecount, int hotcount) {
    nparts = nodecount;
    hot_records = hotcount;
  }

  virtual ~YCSB() {}

  virtual TxnProto* NewTxn(int64 txn_id, int txn_type, string args,
                           Configuration* config = NULL, int r_pct = 0) const;
  virtual int Execute(TxnProto* txn, StorageManager* storage) const;

  TxnProto* InitializeTxn();
  TxnProto* MicroTxnSP(int64 txn_id, int part);
  TxnProto* MicroTxnMP(int64 txn_id, int part1, int part2);

  int nparts;
  int hot_records;
  static const int kRWSetSize = 10;  // MUST BE EVEN
  static const int kDBSize = 1000000;


  virtual void InitializeStorage(Storage* storage, Configuration* conf) const;

 private:
  void GetRandomKeys(set<int>* keys, int num_keys, int key_start,
                     int key_limit, int part);
  YCSB() {}
};

#endif  // _DB_APPLICATIONS_YCSB_H_
