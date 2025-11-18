#ifndef FLOW_POOL_H
#define FLOW_POOL_H

#include <cstdint>
#include <vector>
#include <memory>
#include <zmq.hpp>
#include <semaphore>
#include "flow.h"
#include <string>

using namespace std;

/**
 * A flow_pool is a collection of processing flows (i.e. threads) that can be
 * reserved by a worker to execute a certain flow graph.
 */
namespace nr {
  class flow_pool : public worker {
    public:
      // MHZ - Pass zmq_address
      flow_pool(uint64_t max_flows, const std::string& zmq_address);
      virtual ~flow_pool();
      void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
      shared_ptr<flow> acquire_flow();
      void release_flows();
    private:
      vector<shared_ptr<flow>> pool;
      vector<shared_ptr<flow>> acquired_flows;
      zmq::context_t main_ctx;
      zmq::socket_t zmq_socket;
      shared_ptr<counting_semaphore<>> available_flows;
      uint64_t max_flows;
  };
}

#endif // FLOW_POOL_H