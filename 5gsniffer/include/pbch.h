#ifndef PBCH_H
#define PBCH_H

#include <cstdint>
#include <vector>
#include <complex>
#include <memory>
#include <srsran/srsran.h>
#include <span>
#include "worker.h"
#include "phy.h"

using namespace std;

/**
 * Class for decoding PBCH.
 */
namespace nr {
  class pbch : public worker {
    public:
      pbch(shared_ptr<nr::phy> phy);
      virtual ~pbch();
      std::function<void(srsran_mib_nr_t&, bool)> on_mib_found = [](srsran_mib_nr_t& mib, bool found) {};
      void process(shared_ptr<vector<symbol>>& symbols, int64_t metadata) override;
      float get_ibar_ssb_snr(uint8_t i_ssb, uint8_t n_hf, vector<symbol> symbols);
      float channel_estimate(uint8_t i_ssb, uint8_t n_hf, vector<symbol>& symbols);
      static vector<uint64_t> get_dmrs_indices(uint8_t ofdm_symbol_number, uint16_t cell_id);
      static vector<uint64_t> get_data_indices(uint8_t ofdm_symbol_number, uint16_t cell_id);
      void initialize_dmrs_seq();
      // Tables for optimization
      std::unordered_map<string, std::vector<std::complex<float>> > dmrs_seq_table;
      std::unordered_map<string, std::vector<uint64_t> > dmrs_sc_indices_table;

    private:
      void decode(vector<complex<float>>& soft_symbols);
      
      shared_ptr<nr::phy> phy;
  };
}

#endif // PBCH_H