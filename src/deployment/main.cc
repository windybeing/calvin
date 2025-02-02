// Author: Kun Ren (kun.ren@yale.edu)
// Author: Alexander Thomson (thomson@cs.yale.edu)
//
// Main invokation of a single node in the system.

#include <csignal>
#include <cstdio>
#include <cstdlib>

#include "applications/microbenchmark.h"
#include "applications/ycsb.h"
#include "applications/tpcc.h"
#include "common/configuration.h"
#include "common/connection.h"
#include "backend/simple_storage.h"
#include "backend/fetching_storage.h"
#include "backend/collapsed_versioned_storage.h"
#include "scheduler/serial_scheduler.h"
#include "scheduler/deterministic_scheduler.h"
#include "sequencer/sequencer.h"
#include "proto/tpcc_args.pb.h"
#ifdef PAXOS
# include "paxos/paxos.h"
#endif

#define HOT 10000

map<Key, Key> latest_order_id_for_customer;
map<Key, int> latest_order_id_for_district;
map<Key, int> smallest_order_id_for_district;
map<Key, Key> customer_for_order;
unordered_map<Key, int> next_order_id_for_district;
map<Key, int> item_for_order_line;
map<Key, int> order_line_number;

vector<Key>* involed_customers;

pthread_mutex_t mutex_;
pthread_mutex_t mutex_for_item;
ZipfianGenerator *zipfianGenerator;
// Microbenchmark load generation client.
class MClient : public Client {
 public:
  MClient(Configuration* config, int mp)
      : microbenchmark(config->all_nodes.size(), HOT), ycsb(config->all_nodes.size(), HOT), config_(config),
        percent_mp_(mp) {
  }
  virtual ~MClient() {}
  virtual void GetTxn(TxnProto** txn, int txn_id) {
    *txn = ycsb.ZipfianTxn(txn_id, config_);
    // if (config_->all_nodes.size() > 1 && rand() % 100 < percent_mp_) {
    //   // Multipartition txn.
    //   int other;
    //   do {
    //     other = rand() % config_->all_nodes.size();
    //   } while (other == config_->this_node_id);
    //   // *txn = microbenchmark.MicroTxnMP(txn_id, config_->this_node_id, other);
    //     *txn = ycsb.MicroTxnMP(txn_id, config_->this_node_id, other);
    // } else {
    //   // Single-partition txn.
    //   // *txn = microbenchmark.MicroTxnSP(txn_id, config_->this_node_id);
      // *txn = ycsb.MicroTxnSP(txn_id, config_->this_node_id);
    // }
  }

 private:
  Microbenchmark microbenchmark;
  YCSB ycsb;
  Configuration* config_;
  int percent_mp_;
};

// TPCC load generation client.
class TClient : public Client {
 public:
  TClient(Configuration* config, int mp) : config_(config), percent_mp_(mp) {}
  virtual ~TClient() {}
  virtual void GetTxn(TxnProto** txn, int txn_id) {
    TPCC tpcc;
    TPCCArgs args;

    args.set_system_time(GetTime());
    // if (rand() % 100 < percent_mp_)
    //   args.set_multipartition(true);
    // else
    args.set_multipartition(false);

    string args_string;
    args.SerializeToString(&args_string);


    // New order txn
   int random_txn_type = rand() % 100;
    // New order txn
  #ifdef STANDARD_MIX
    if (random_txn_type < 45)  {
      *txn = tpcc.NewTxn(txn_id, TPCC::NEW_ORDER, args_string, config_, percent_mp_);
    } else if(random_txn_type < 88) {
      *txn = tpcc.NewTxn(txn_id, TPCC::PAYMENT, args_string, config_, percent_mp_ == 0 ? 0 : 15);
    } else if(random_txn_type < 92) {
      *txn = tpcc.NewTxn(txn_id, TPCC::ORDER_STATUS, args_string, config_, 0);
      args.set_multipartition(false);
    } else if(random_txn_type < 96){
      *txn = tpcc.NewTxn(txn_id, TPCC::DELIVERY, args_string, config_, 0);
      args.set_multipartition(false);
    } else {
      *txn = tpcc.NewTxn(txn_id, TPCC::STOCK_LEVEL, args_string, config_, 0);
      args.set_multipartition(false);
    }
  #endif
    if (random_txn_type < 50)  {
      *txn = tpcc.NewTxn(txn_id, TPCC::NEW_ORDER, args_string, config_, percent_mp_);
    } else {
      *txn = tpcc.NewTxn(txn_id, TPCC::PAYMENT, args_string, config_, percent_mp_ == 0 ? 0 : 15);
    }

  }

 private:
  Configuration* config_;
  int percent_mp_;
};

void stop(int sig) {
#ifdef PAXOS
 StopZookeeper(ZOOKEEPER_CONF);
#endif
  exit(sig);
}

int main(int argc, char** argv) {
  // TODO(alex): Better arg checking.
  if (argc < 5) {
    fprintf(stderr, "Usage: %s <node-id> <m[icro]|t[pcc]> <percent_mp>\n",
            argv[0]);
    exit(1);
  }
  bool useFetching = false;
  if (argc > 5 && argv[5][0] == 'f')
    useFetching = true;
  // Catch ^C and kill signals and exit gracefully (for profiling).
  signal(SIGINT, &stop);
  signal(SIGTERM, &stop);

  // Build this node's configuration object.
  Configuration config(StringToInt(argv[1]), "deploy-run.conf");

  // Build connection context and start multiplexer thread running.
  ConnectionMultiplexer multiplexer(&config);
  config.distributed_ratio = StringToDouble(argv[5]);
#ifdef ENABLE_DISTRIBUTED_TXN
  zipfianGenerator = new ZipfianGenerator(0, YCSB::kDBSize - 1, StringToDouble(argv[4]));
  
  std::cout << "This node id: " << config.this_node_id << " Use distributed txn, distributed ratio: " << config.distributed_ratio << " zipf theta: " << StringToDouble(argv[4]) <<std::endl;
#else 
  zipfianGenerator = new ZipfianGenerator(0, config.all_nodes.size() * YCSB::kDBSize - 1, StringToDouble(argv[4]));
#endif
  // Artificial loadgen clients.
  Client* client = (argv[2][0] == 'm') ?
      reinterpret_cast<Client*>(new MClient(&config, atoi(argv[3]))) :
      reinterpret_cast<Client*>(new TClient(&config, atoi(argv[3])));

#ifdef PAXOS
 StartZookeeper(ZOOKEEPER_CONF);
#endif
pthread_mutex_init(&mutex_, NULL);
pthread_mutex_init(&mutex_for_item, NULL);
involed_customers = new vector<Key>;

  Storage* storage;
  if (!useFetching) {
    storage = new SimpleStorage();
  } else {
    storage = FetchingStorage::BuildStorage();
  }
storage->Initmutex();
  if (argv[2][0] == 'm') {
    // Microbenchmark(config.all_nodes.size(), HOT).InitializeStorage(storage, &config);
    YCSB(config.all_nodes.size(), HOT).InitializeStorage(storage, &config);
  } else {
    TPCC().InitializeStorage(storage, &config);
  }

  // Initialize sequencer component and start sequencer thread running.
  Sequencer sequencer(&config, multiplexer.NewConnection("sequencer"), client,
                      storage);

  // Run scheduler in main thread.
  if (argv[2][0] == 'm') {
    DeterministicScheduler scheduler(&config,
                                     multiplexer.NewConnection("scheduler_"),
                                     storage,
                                     new YCSB(config.all_nodes.size(), HOT));
  } else {
    DeterministicScheduler scheduler(&config,
                                     multiplexer.NewConnection("scheduler_"),
                                     storage,
                                     new TPCC());
  }

  Spin(180);
  return 0;
}

