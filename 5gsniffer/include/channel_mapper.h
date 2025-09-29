#ifndef CHANNEL_MAPPER_H
#define CHANNEL_MAPPER_H

#include <cstdint>
#include <vector>
#include <memory>
#include "symbol.h"
#include "worker.h"
#include "pdcch.h"
#include "config.h"

namespace nr {
  class phy;
}

using namespace std;

/**
 * Mapper that given a list of symbols can determine the channel to which these
 * symbols belong.
 */
class channel_mapper : public worker {
  public:
    channel_mapper(shared_ptr<nr::phy> phy, pdcch_config pdcch_config);
    virtual ~channel_mapper();
    void process(shared_ptr<vector<symbol>>& symbols, int64_t metadata) override;

    shared_ptr<nr::phy> phy;
    
    // Sublayers
    nr::pdcch pdcch;
};

#endif // CHANNEL_MAPPER_H