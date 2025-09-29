#ifndef SSB_MAPPER_H
#define SSB_MAPPER_H

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <liquid/liquid.h>
#include "symbol.h"
#include "worker.h"
#include "phy.h"
#include "pss.h"
#include "pbch.h"

using namespace std;

/**
 * Mapper for the SSB. Assumes that the SSB is in the first 4 symbols received.
 */
class ssb_mapper : public worker {
  public:
    // Callbacks
    std::function<void(uint16_t)> on_sss_found = [](uint16_t nid1) {}; ///< Default callback when SSS is found: do nothing
    std::function<void(void)> on_sss_not_found = [](){}; ///< Callback when SSS is not found

    ssb_mapper(shared_ptr<nr::phy> phy);
    virtual ~ssb_mapper();
    void process(shared_ptr<vector<symbol>>& symbols, int64_t metadata) override;

    // Sublayers
    nr::pbch pbch;
  private:
    shared_ptr<nr::phy> phy;
};

#endif // SSB_MAPPER_H