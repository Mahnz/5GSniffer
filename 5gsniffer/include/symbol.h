#ifndef SYMBOL_H
#define SYMBOL_H

#include <cstdint>
#include <vector>
#include <complex>
#include <memory>
#include <span>
#include <liquid/liquid.h>
#include <spdlog/spdlog.h>
#include "phy_params_common.h"

using namespace std;

/**
 * Class representing an OFDM symbol.
 */
class symbol {
  public:
    symbol();
    virtual ~symbol();
    symbol(const symbol& other);
    symbol(symbol&& other);
    symbol& operator=(const symbol& other);
    symbol& operator=(symbol&& other);

    void swap(symbol& other);

    // Debugging
    uint64_t sample_index;

    // Resource elements
    vector<complex<float>> samples;
    vector<complex<float>> samples_eq;
    vector<complex<float>> noise;
    uint8_t symbol_index;
    uint8_t slot_index;
    span<complex<float>> get_res(size_t start_index, size_t end_index);

    // Equalization
    bool is_equalized;
    vector<complex<float>> channel_filter;
    void channel_estimate(const vector<complex<float>>& dmrs_reference, const vector<uint64_t>& dmrs_indices, uint64_t subcarrier_start, uint64_t subcarrier_end);
    float get_average_noise_magnitude() const;
    float get_average_magnitude() const;
    float get_average_channel_magnitude() const;
    complex<float> get_average_channel() const;
    void normalize();
};

#endif // SYMBOL_H