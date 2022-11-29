// Author: Kun Ren (kun.ren@yale.edu)
// Author: Alexander Thomson (thomson@cs.yale.edu)
//
// TODO(scw): remove iostream, use cstdio instead

#include "applications/ycsb.h"

#include <iostream>

#include "backend/storage.h"
#include "backend/storage_manager.h"
#include "common/utils.h"
#include "common/configuration.h"
#include "proto/txn.pb.h"

// #define PREFETCHING
#define COLD_CUTOFF 990000

// Fills '*keys' with num_keys unique ints k where
// 'key_start' <= k < 'key_limit', and k == part (mod nparts).
// Requires: key_start % nparts == 0
void YCSB::GetRandomKeys(set<int>* keys, int num_keys, int key_start,
                                   int key_limit, int part) {
  assert(key_start % nparts == 0);
  keys->clear();
  for (int i = 0; i < num_keys; i++) {
    // Find a key not already in '*keys'.
    int key;
    do {
      key = key_start + part +
            nparts * (rand() % ((key_limit - key_start)/nparts));
    } while (keys->count(key));
    keys->insert(key);
  }
}

TxnProto* YCSB::InitializeTxn() {
  // Create the new transaction object
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(0);
  txn->set_txn_type(INITIALIZE);

  // Nothing read, everything written.
  for (int i = 0; i < kDBSize; i++)
    txn->add_write_set(IntToString(i));

  return txn;
}

TxnProto* YCSB::ZipfianTxn(int64 txn_id) {
  TxnProto* txn = new TxnProto();
  txn->set_txn_id(txn_id);
  txn->set_txn_type(MICROTXN_SP);
  int key[kRWSetSize];
  bool update[kRWSetSize];
  set<int> keys;
  for (int i = 0; i < kRWSetSize; i++) {
    int k;
    do {
      k = zipfianGenerator->nextValue();
      keys.insert(k);
    } while (keys.size() != (size_t)i + 1);
    key[i] = k;
  }
  // key[0] = 0; key[1] = 1;
  for(int i=0; i< kRWSetSize; i++){
    update[i] = false;
  }
  uint writeKey = kRWSetSize / 2;
  while(writeKey > 0){
    int w = rand() % kRWSetSize;
    if(!update[w]){
      update[w] = true;
      writeKey--;
    }
  }
  // update[0] = true;
  for (int i = 0; i < kRWSetSize; ++i) {
    if (update[i]) {
      txn->add_write_set(IntToString(key[i]));
    } else {
      txn->add_read_set(IntToString(key[i]));
    }
  }
  return txn;
}

// Create a non-dependent single-partition transaction
TxnProto* YCSB::MicroTxnSP(int64 txn_id, int part) {
  // Create the new transaction object
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(txn_id);
  txn->set_txn_type(MICROTXN_SP);

  // Add one hot key to read/write set.
  int hotkey = part + nparts * (rand() % hot_records);
  txn->add_read_write_set(IntToString(hotkey));

  // Insert set of kRWSetSize - 1 random cold keys from specified partition into
  // read/write set.
  set<int> keys;
  GetRandomKeys(&keys,
                kRWSetSize - 1,
                nparts * hot_records,
                nparts * kDBSize,
                part);
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));

  return txn;
}

// Create a non-dependent multi-partition transaction
TxnProto* YCSB::MicroTxnMP(int64 txn_id, int part1, int part2) {
  assert(part1 != part2 || nparts == 1);
  // Create the new transaction object
  TxnProto* txn = new TxnProto();

  // Set the transaction's standard attributes
  txn->set_txn_id(txn_id);
  txn->set_txn_type(MICROTXN_MP);

  // Add two hot keys to read/write set---one in each partition.
  int hotkey1 = part1 + nparts * (rand() % hot_records);
  int hotkey2 = part2 + nparts * (rand() % hot_records);
  txn->add_read_write_set(IntToString(hotkey1));
  txn->add_read_write_set(IntToString(hotkey2));

  // Insert set of kRWSetSize/2 - 1 random cold keys from each partition into
  // read/write set.
  set<int> keys;
  GetRandomKeys(&keys,
                kRWSetSize/2 - 1,
                nparts * hot_records,
                nparts * kDBSize,
                part1);
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));
  GetRandomKeys(&keys,
                kRWSetSize/2 - 1,
                nparts * hot_records,
                nparts * kDBSize,
                part2);
  for (set<int>::iterator it = keys.begin(); it != keys.end(); ++it)
    txn->add_read_write_set(IntToString(*it));

  return txn;
}

// The load generator can be called externally to return a transaction proto
// containing a new type of transaction.
TxnProto* YCSB::NewTxn(int64 txn_id, int txn_type,
                                 string args, Configuration* config, int r_pct) const {
  return NULL;
}

int YCSB::Execute(TxnProto* txn, StorageManager* storage, Configuration* config) const {
  // Read all elements of 'txn->read_set()', add one to each, write them all
  // back out.
  #ifdef YCSB10
  for (int i = 0; i < kRWSetSize / 2; i++) {
    Value* val;
    
    if (config->LookupPartition(txn->read_set(i)) == config->this_node_id)
      val = storage->ReadObject(txn->read_set(i));
    
    if (config->LookupPartition(txn->write_set(i)) == config->this_node_id) {
      // val = storage->ReadObject(txn->write_set(i));
      // *val = IntToString(StringToInt(*val) + 1);
      storage->PutObject(txn->write_set(i), val);
    }
  #else
  for (int i = 0; i < kRWSetSize; i++) {
    Value* val = storage->ReadObject(txn->read_write_set(i));
    *val = IntToString(StringToInt(*val) + 1);
  #endif
    // Not necessary since storage already has a pointer to val.
    //   storage->PutObject(txn->read_write_set(i), val);

    // The following code is for YCSB "long" transaction, uncomment it if for "long" transaction
    /**int x = 1;
      for(int i = 0; i < 1100; i++) {
        x = x*x+1;
        x = x+10;
        x = x-2;
      }**/

  }
  return 0;
}

void YCSB::InitializeStorage(Storage* storage,
                                       Configuration* conf) const {
  for (int i = 0; i < nparts*kDBSize; i++) {
    if (conf->LookupPartition(IntToString(i)) == conf->this_node_id) {
      storage->PutObject(IntToString(i), new Value(IntToString(i)));

    }
  }
}

