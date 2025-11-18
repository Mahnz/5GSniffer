#ifndef SNIFFER_H
#define SNIFFER_H

#include "worker.h"
#include "syncer.h"
#include "phy.h"

using namespace std;

/**
 * 5G sniffer main class
 */
class sniffer {
  public:
    // MHZ - Pass zmq_address
    sniffer(uint64_t sample_rate, uint64_t frequency, string rf_args, uint16_t ssb_numerology, const std::string& zmq_address); ///< Create a sniffer for an SDR source.
    sniffer(uint64_t sample_rate, string path, uint16_t ssb_numerology, const std::string& zmq_address); ///< Create a sniffer for a file source.
    virtual ~sniffer();
    void start();
    void stop();

    uint64_t sample_rate;
    uint16_t ssb_numerology;
    unique_ptr<worker> device;
  private:
    void init();
    bool running;
    
    std::string zmq_address_;
};

#endif // SNIFFER_H