#include "shifter.h"
#include "spdlog/spdlog.h"
#include <complex>

using namespace std;

/** 
 * Constructor for shifter.
 *
 * @param num_samples number of samples to shift
 */
shifter::shifter(int64_t num_samples) : 
  num_samples(num_samples) {
}

/** 
 * Shift the input samples.
 * @param samples shared_ptr to sample buffer
 */
void shifter::process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) {
  auto result = make_shared<vector<complex<float>>>(std::move(*samples));

  if(num_samples < 0) {
    if(abs(num_samples) <= result->size())
      result->erase(result->begin(), result->begin() + abs(num_samples));
  } else if (num_samples > 0) {
    vector<complex<float>> tmp(num_samples, 0);
    result->insert(result->begin(), tmp.begin(), tmp.end());
  }
  SPDLOG_DEBUG("Shifer block, shifting {} samples by num samples {}, metadata {}",samples->size(),num_samples, metadata);

  send_to_next_workers(result, metadata);
}