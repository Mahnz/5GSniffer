#ifndef SDR_H
#define SDR_H

#include <vector>
#include <complex>
#include <memory>
#include "srsran/phy/rf/rf.h"
#include "worker.h"

using namespace std;

/**
 * Implementation of a SDR.
 *
 * The SDR class produces samples for other workers to process, using the srsRAN
 * RF libraries.
 */
class sdr : public worker {
  public:
    sdr(
      double sample_rate = 23'040'000,
      double frequency = 627'750'000,
      string rf_args = "",
      double rx_gain = 40.0,
      double tx_gain = 0.0
    );
    virtual ~sdr();
    shared_ptr<vector<complex<float>>> produce_samples(size_t num_samples) override;

  private:
    double sample_rate;
    double frequency;
    double rx_gain;
    double tx_gain;
    srsran_rf_t rf;

    shared_ptr<vector<complex<float>>> receive(size_t num_samples);

    time_t secs_prev;
    double frac_secs_prev;

};

#endif
