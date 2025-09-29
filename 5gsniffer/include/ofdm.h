#ifndef OFDM_H
#define OFDM_H

#include <cstdint>
#include <memory>
#include <vector>
#include <complex>
#include <liquid/liquid.h>
#include "worker.h"
#include "bandwidth_part.h"

using namespace std;

/**
 * Class for OFDM modulation and demodulation.
 */
class ofdm : public worker {
  public:
    uint8_t symbol_index; ///< Index of the current symbol in the subframe
    uint8_t slot_index;   ///< Index of the current slot in the frame
    
    ofdm(shared_ptr<bandwidth_part> bwp, float cyclic_prefix_fraction = 0.5);
    virtual ~ofdm();
    void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
    vector<complex<float>> modulate(vector<symbol>& symbols);
  private:
    shared_ptr<bandwidth_part> bwp;
    float cyclic_prefix_fraction;
    uint64_t samples_processed;
    std::vector<std::complex<float>> leftover_samples;


    string get_symbol_dump_path_name();
};


#endif // OFDM_H