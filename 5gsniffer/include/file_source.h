#ifndef FILE_SOURCE_H
#define FILE_SOURCE_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <complex>
#include <memory>
#include "worker.h"

using namespace std;

/**
 * A sample_worker that produces complex samples by reading them from a file.
 */
class file_source : public worker {
  public:
    int size_bytes;
    uint64_t sample_rate;

    file_source(uint64_t sample_rate, string path, bool repeat = false);
    virtual ~file_source();
    shared_ptr<vector<complex<float>>> produce_samples(size_t num_samples) override;
  private:
    ifstream f;
    bool repeat;
};

#endif