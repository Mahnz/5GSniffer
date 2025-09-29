#include <complex>
#include <cstddef>
#include <cstdint>
#include <liquid/liquid.h>
#include "ssb_mapper.h"
#include "spdlog/spdlog.h"
#include "sss.h"
#include "dsp.h"
#include "utils.h"
#include "pbch.h"

using namespace std;

/** 
 * Constructor for ssb_mapper.
 */
ssb_mapper::ssb_mapper(shared_ptr<nr::phy> phy) :
  pbch(phy) {
  this->phy = phy;
}

/** 
 * Destructor for ssb_mapper.
 */
ssb_mapper::~ssb_mapper() {

}

/** 
 * Processes input OFDM symbols, looking for SSS and sending symbols to PBCH.
 * @param symbols contains OFDM symbols
 */
void ssb_mapper::process(shared_ptr<vector<symbol>>& symbols, int64_t metadata) {
  SPDLOG_DEBUG("Got {} symbols", symbols->size());

  auto ssss = phy->ssss;

  if(symbols->size() >= 4) {
    if (phy->in_synch){
      // No need to re-find SSS. fine time synch with SSS is performed in syncer.cc.
      auto pbch_symbols = make_shared<vector<symbol>>(symbols->begin(), symbols->begin()+4);
      pbch.initialize_dmrs_seq();
      pbch.work(pbch_symbols);
    } else {
      assert(symbols->at(2).symbol_index == 4); // Assert SSS is the 5th symbol (index 4)

      // Extract SSS
      span<complex<float>> sss_res = symbols->at(2).get_res(56, 182);
      assert(sss_res.size() == sss_length);
      
      // Find SSS through correlation
      float max_corr = 0.0f;
      uint16_t max_nid = 0;
      float avg_corr = 0.0f;
      for(uint16_t nid1 = 0; nid1 <= 335; nid1++) {
        auto sss_ref = ssss.at(nid1*3 + phy->nid2);
        vector<float> out(1);

        correlate_magnitude(out, sss_ref, sss_res);

        float corr = out.at(0);
        avg_corr += corr;
        if(corr > max_corr) {
          max_corr = corr;
          max_nid = nid1;
        }
      }
      avg_corr /= 355;

      SPDLOG_DEBUG("SSS max corr: {} ({} avg)", max_corr, avg_corr);

      if (max_corr > pss_sss_times_avg_threshold * avg_corr) {
        SPDLOG_DEBUG("NID1: {}, corr {} avg {} ratio {}", max_nid, max_corr, avg_corr, max_corr / avg_corr);
        this->on_sss_found(max_nid);

        // PBCH processing TODO can happen in parallel
        auto pbch_symbols = make_shared<vector<symbol>>(symbols->begin(), symbols->begin()+4);
        pbch.initialize_dmrs_seq();
        pbch.work(pbch_symbols);
      } else {
        this->on_sss_not_found();
      }
    }
  } else {
    SPDLOG_ERROR("Need at least 4 symbols to be able to process SSB.");
  }
}
