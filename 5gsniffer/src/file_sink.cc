#include "file_sink.h"
#include "spdlog/spdlog.h"

using namespace std;

/** 
 * Constructor for file_sink.
 *
 * @param path path to the file to write
 */
file_sink::file_sink(string path) :
  f{path, ofstream::binary} {
    SPDLOG_DEBUG("Opening file_sink to {}", path);
}

/** 
 * Destructor for file_sink.
 */
file_sink::~file_sink() {
  f.close();
}

/** 
 * Writes vector of complex samples held in shared_ptr to a file.
 *
 * @param samples shared_ptr to sample buffer to write
 */
void file_sink::process(shared_ptr<vector<complex<float>>>& samples, int64_t metadata) {
  size_t size_bytes = samples->size() * sizeof(complex<float>);
  f.write(reinterpret_cast<char*>(samples->data()), size_bytes);
  SPDLOG_DEBUG("Wrote {} samples ({} bytes)", samples->size(), size_bytes);
}