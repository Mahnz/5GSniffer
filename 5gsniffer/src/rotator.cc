#include "rotator.h"
#include "spdlog/spdlog.h"
#include "dsp.h"

using namespace std;

/** 
 * Constructor for rotator.
 *
 * @param frequency frequency to rotate
 */
rotator::rotator(uint32_t sample_rate, float frequency) : 
  frequency(frequency),
  sample_rate(sample_rate) {
}

/** 
 * Rotate the input samples.
 * @param samples shared_ptr to sample buffer
 */
void rotator::process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) {
  rotate(*samples, *samples, frequency, sample_rate);
  send_to_next_workers(samples, metadata);
}