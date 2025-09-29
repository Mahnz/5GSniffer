#ifndef FILE_SINK_H
#define FILE_SINK_H

#include <fstream>
#include <string>
#include <vector>
#include <complex>
#include <memory>
#include "worker.h"

using namespace std;

/**
 * A sample_worker that consumes complex samples and writes them to a file.
 */
class file_sink : public worker {
  public:
    file_sink(string path);
    virtual ~file_sink();
    void process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) override;
  private:
    ofstream f;
};

#endif