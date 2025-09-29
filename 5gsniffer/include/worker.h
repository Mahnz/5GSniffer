#ifndef WORKER_H
#define WORKER_H

#include <spdlog/spdlog.h>
#include <vector>
#include <complex>
#include <memory>
#include "symbol.h"
#include "exceptions.h"

using namespace std;

/**
 * Generic worker for producing and processing samples or symbols. Can pass work
 * on to a list of subsequent workers if needed.
 */
class worker {
  public:
    std::function<void(void)> on_end = [](){};  ///< Callback triggered when the worker has no work to do anymore.

    worker();
    virtual ~worker();
    virtual void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) { throw sniffer_exception("Tried to call worker::process directly"); };
    virtual void process(shared_ptr<vector<symbol>>& symbols, int64_t metadata) { throw sniffer_exception("Tried to call worker::process directly"); };
    virtual shared_ptr<vector<complex<float>>> produce_samples(size_t num_samples);
    virtual shared_ptr<vector<symbol>> produce_symbols(size_t num_symbols);
    virtual void finish();

    void work(size_t num_samples);
    void connect(shared_ptr<worker> w);
    void disconnect(shared_ptr<worker> w);
    void disconnect_all();
    void disconnect_finished();
    void finish_next_workers();
    const size_t num_next_workers();
    
    /** 
     * Work function executed by processing workers.
     *
     * @param samples shared_ptr to input buffer to process
     */
    template<class T>
    void work(shared_ptr<vector<T>>& inputs, int64_t metadata = 0) {
      this->process(inputs, metadata);
    }
  protected:
    /** 
     * Helper function to distribute work to the next workers.
     *
     * @param inputs shared_ptr to input buffer to pass on to the next workers
     */
    template<class T>
    void send_to_next_workers(shared_ptr<vector<T>> inputs) {
      // Send work to next workers TODO parallelize
      for (const auto& worker : this->next_workers) {
        worker->work(inputs, 0);
      }
    }
    template<class T>
    void send_to_next_workers(shared_ptr<vector<T>> inputs, int64_t metadata) {
      // Send work to next workers TODO parallelize
      for (const auto& worker : this->next_workers) {
        worker->work(inputs,metadata);
      }
    }

    bool finished;
    int64_t total_produced_samples;
  private:
    vector<shared_ptr<worker>> next_workers;
};

#endif