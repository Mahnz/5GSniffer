#ifndef PHY_H
#define PHY_H

#include <cstdint>
#include <vector>
#include <memory>
#include "bandwidth_part.h"
#include "channel_mapper.h"

using namespace std;

/**
 * Class for holding 5G NR physical-layer properties.
 */
namespace nr {
  class phy {
    public:
      uint8_t nid1;
      uint16_t nid2;
      uint8_t i_ssb;
      uint8_t n_hf;
      bool in_synch;
      shared_ptr<bandwidth_part> ssb_bwp;                 ///< Special bandwidth part used for the SSB only
      vector<shared_ptr<bandwidth_part>> bandwidth_parts;
      vector<shared_ptr<channel_mapper>> channel_mappers;
      std::array<std::array<std::complex<float>,sss_length>,nid_max+1> ssss;

      phy();
      uint16_t get_cell_id() const;
      shared_ptr<bandwidth_part> get_initial_dl_bandwidth_part();
  };
}

#endif // PHY_H