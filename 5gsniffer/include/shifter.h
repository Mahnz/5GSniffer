#ifndef SHIFTER_H
#define SHIFTER_H

#include <cstdint>
#include <string>
#include <vector>
#include <complex>
#include <memory>
#include "worker.h"

using namespace std;

/**
 * A shifter is a worker that performs a phase shift on the input.
 */
class shifter : public worker {
  public:
    shifter(int64_t num_samples);
    void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
  private:
    int64_t num_samples;
};

#endif // SHIFTER_H