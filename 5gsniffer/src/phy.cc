#include <cmath>
#include <cstdint>
#include "exceptions.h"
#include "phy.h"
#include "spdlog/spdlog.h"
#include "phy_params_common.h"
#include <iostream>


using namespace std;

namespace nr {
  /** 
  * Constructor for phy.
  */
  phy::phy() {
    in_synch = false;
  }

  uint16_t phy::get_cell_id() const {
    return (3*nid1) + nid2;
  }

  shared_ptr<bandwidth_part> phy::get_initial_dl_bandwidth_part() {
    if(bandwidth_parts.size() > 0) {
      return bandwidth_parts.at(0);
    } else {
      throw sniffer_exception("Tried to retrieve initial DL bandwidth part before receiving MIB.");
    }
  }
}