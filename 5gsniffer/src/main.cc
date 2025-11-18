#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"
#include "file_sink.h"
#include "sniffer.h"
#include "exceptions.h"
#include "config.h"

using namespace std;
extern struct config config;

static void usage() {
  cout << "Usage: 5g_sniffer <path_to_config.toml>" << endl;
}

/** 
 * Main function of the 5G sniffer.
 *
 * @param argc 
 * @param argv 
 */
int main(int argc, char** argv) {
  string config_path;

  #ifdef DEBUG_BUILD
    SPDLOG_INFO("=== This is a debug mode build ===");
  #endif

  // Load spdlog level from environment variable
  // For example: export SPDLOG_LEVEL=debug
  spdlog::cfg::load_env_levels();

  // Set logger pattern
  spdlog::set_pattern("[%^%l%$] [%H:%M:%S.%f thread %t] [%s:%#] %v");

  // Get config path from command-line args
  if (argc == 1) {
    config_path = string("config.toml");
  } else if (argc == 2) {
    config_path = string(argv[1]);
  } else {
    usage();
    exit(1);
  }

  try {
    // Load the config
    config = config::load(config_path);

    // Create sniffer
    if(config.file_path.compare("") == 0) {
      // MHZ - Pass zmq_address. Using SDR, real-time detection
      sniffer sniffer(config.sample_rate, config.frequency, config.rf_args, config.ssb_numerology, config.zmq_address);
      sniffer.start();
    } else {
      // MHZ - Pass zmq_address. Using file, offline detection
      sniffer sniffer(config.sample_rate, config.file_path.data(), config.ssb_numerology, config.zmq_address);
      sniffer.start();
    }
  } catch (sniffer_exception& e) {
    SPDLOG_ERROR(e.what());
    return 1;
  } catch (const toml::parse_error& err) {
    std::cerr << "Error parsing sniffer configuration file '" << *err.source().path << "':\n" << err.description() << "\n  (" << err.source().begin << ")\n";
    return 1;
  }

  return 0;
}
