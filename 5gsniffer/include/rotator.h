#ifndef ROTATOR_H
#define ROTATOR_H

#include <string>
#include <vector>
#include <complex>
#include <memory>
#include "worker.h"

using namespace std;

/**
 * A rotator is a worker that rotates the input samples according to specified
 * frequency.
 */
class rotator : public worker {
  public:
    rotator(uint32_t sample_rate, float frequency);
    void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
  private:
    float frequency;
    uint32_t sample_rate;
};

#endif // ROTATOR_H