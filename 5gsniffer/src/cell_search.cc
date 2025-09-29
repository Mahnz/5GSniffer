// #include <iostream>
// #include <cstdlib>
// #include <unistd.h>
// #include "args_manager.h"

// // #ifdef __cplusplus
// // extern "C" {
// // #undef I // Fix complex.h #define I nastiness when using C++
// #include "srsran/srsran.h"
// #include "srsran/phy/rf/rf_utils.h"
// #include "srsran/phy/rf/rf.h"
// // }

// using namespace std;

// char rf_dev[]  = "";
// char rf_args[]  = "";

// int main(int argc, char** argv) {

//   args_t args;
//   args_manager::parse_args(args, argc, argv);

//   int                           n;
//   srsran_rf_t                   rf;
//   srsran_ue_cellsearch_t        cs;
//   srsran_ue_cellsearch_result_t found_cells[3];
//   int                           nof_freqs;
//   // srsran_earfcn_t               channels[MAX_EARFCN];
//   uint32_t                      freq;
//   uint32_t                      n_found_cells = 0;

//   printf("Opening RF device...\n");

//   if (srsran_rf_open_devname(&rf, rf_dev, rf_args, 1)) {
//     // ERROR("Error opening rf");
//     cout << "error opening rf" << endl;
//     exit(-1);
//   }
//   cout << "opened" << endl;

//   // DEBUG(" ----  Receive %d samples  ---- ", nsamples);
//   // return srsran_rf_recv_with_time((srsran_rf_t*)h, data, nsamples, 1, NULL, NULL);
//   srsran_rf_close(&rf);
// }
