#include "sniffer.h"
#include "bandwidth_part.h"
#include "sdr.h"
#include "file_source.h"
#include "file_sink.h"
#include "spdlog/spdlog.h"
#include "phy_params_common.h"
#include "utils.h"
#include <memory>

using namespace std;

// MHZ - Import
#include "config.h"
#include "rnti_tracker.hpp"

/** 
 * Constructor for sniffer when using SDR.
 *
 * @param sample_rate
 * @param frequency
 * @param rf_args
 * @param ssb_numerology
 */
sniffer::sniffer(uint64_t sample_rate, uint64_t frequency, string rf_args, uint16_t ssb_numerology) :
  sample_rate(sample_rate),
  ssb_numerology(ssb_numerology),
  device(make_unique<sdr>(sample_rate, frequency, rf_args)) {
  init();
}

/** 
 * Constructor for sniffer when using file source.
 *
 * @param sample_rate
 * @param path
 * @param ssb_numerology
 */
sniffer::sniffer(uint64_t sample_rate, string path, uint16_t ssb_numerology) :
  sample_rate(sample_rate),
  ssb_numerology(ssb_numerology),
  device(make_unique<file_source>(sample_rate, path)) {
  init();
}

/** 
 * Common initializer helper function shared amongst constructors.
 */
void sniffer::init() {
  // Create blocks
  auto phy = make_shared<nr::phy>();  
  phy->ssb_bwp = make_unique<bandwidth_part>(3'840'000 * (1<<ssb_numerology), ssb_numerology, ssb_rb); // Default bandwidth part that captures at least 256 subcarriers (240 needed for SSB).
  auto syncer = make_shared<class syncer>(sample_rate, phy);

  // Callbacks
  device->on_end = std::bind(&sniffer::stop, this);

  device->connect(syncer);
}

void sniffer::start() {
  running = true;
  float seconds_per_chunk = 0.0080;

  uint32_t num_samples_per_chunk = static_cast<uint32_t>(sample_rate * seconds_per_chunk);
  
  // MHZ - Configure RNTI Tracker (if enabled by TOML)
  if (config.rnti_tracker.enabled) {
    RntiTracker::instance().configure(
        config.rnti_tracker.output_path,
        config.rnti_tracker.format,
        config.rnti_tracker.ttl_seconds);
    SPDLOG_INFO("RNTI Tracker enabled: path='{}', fmt='{}', ttl={}s",
                config.rnti_tracker.output_path,
                config.rnti_tracker.format,
                config.rnti_tracker.ttl_seconds);
  } else {
    SPDLOG_INFO("RNTI Tracker disabled via config.");
  }
  
  while(running) {
    // MHZ - This message is called both with file_source and SDR. Correct?
    SPDLOG_DEBUG("Calling device work for SDR");

    auto sniffer_work_t0 = time_profile_start();
    device->work(num_samples_per_chunk);
    time_profile_end(sniffer_work_t0, "sniffer::work");
  }

  SPDLOG_DEBUG("Terminating sniffer");
}

void sniffer::stop() {
  SPDLOG_DEBUG("Received signal to stop sniffer");
  running = false;
}

/** 
 * Destructor for sniffer.
 */
sniffer::~sniffer() {

}
