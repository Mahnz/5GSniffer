#ifndef DSP_H
#define DSP_H

#include <complex>
#include <vector>
#include <span>
#include <cstdint>

using namespace std;

void correlate(vector<complex<float>>& output, span<complex<float>> a, span<complex<float>> b);
void correlate(vector<complex<float>>& output, span<complex<float>> a, span<complex<float>> b, int step_size);
void moving_correlate(vector<complex<float>>& output, span<complex<float>> a, span<complex<float>> b, size_t window_size);
void correlate_magnitude(vector<float>& output, span<complex<float>> a, span<complex<float>> b);
void correlate_magnitude(vector<float>& output, span<complex<float>> a, span<complex<float>> b, int step_size);
void correlate_magnitude_normalized(vector<float>& output, span<complex<float>> a, span<complex<float>> b);
void magnitude(vector<float>& output, span<complex<float>> input);
float frobenius_norm(span<complex<float>> input);
void rotate(vector<complex<float>>& output, span<complex<float>> input, float frequency, uint32_t sample_rate);


#endif // DSP_H