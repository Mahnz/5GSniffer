

#ifndef ARGS_MANAGER_H
#define ARGS_MANAGER_H

#include <stdint.h>
#include <string>

struct args_t {
  int force_N_id_2;
  std::string input_file_name = "";
  std::string rf_args;
  std::string rf_dev;
  double rf_freq;
  double rf_gain;


  };

class args_manager {
public:
  static void default_args(args_t& args);
  static void usage(args_t& args, const std::string& prog);
  static void parse_args(args_t& args, int argc, char **argv);
private:
  args_manager() = delete;
};

#endif