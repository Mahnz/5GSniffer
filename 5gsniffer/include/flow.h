#ifndef FLOW_H
#define FLOW_H

#include <fstream>
#include <string>
#include <vector>
#include <complex>
#include <memory>
#include <thread>
#include <zmq.hpp>
#include "worker.h"

using namespace std;

/**
 * A flow is a worker that copies all input samples to a threaded processing
 * flow graph.
 */
class flow : public worker {
  public:
    flow(uint64_t flow_id, zmq::socket_ref send_socket, shared_ptr<counting_semaphore<>> available_flows);
    virtual ~flow();
    // void process(shared_ptr<vector<complex<float>>>& samples) override;
    void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
    void finish() override;
    void handle_messages();
    void set_available();

    bool available;
    bool sniffer_finished;
    uint64_t flow_id;
  private:
    void wait_for_start_message_ack(zmq::socket_ref send_socket);
    void wait_for_start_message(zmq::socket_ref receive_socket);

    thread t;
    zmq::socket_ref send_socket;
    std::string routing_id;
    int64_t metadata;
    shared_ptr<counting_semaphore<>> available_flows;
};

#endif // FLOW_H