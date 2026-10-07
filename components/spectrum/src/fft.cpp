// SPDX-License-Identifier: GPL-3.0-or-later
#include "spectrum/fft.hpp"

#include <cmath>
#include <utility>

namespace spectrum {

void fft(float* re, float* im, size_t n) {
    // Bit-reversal permutation.
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }
    // Butterflies.
    for (size_t length = 2; length <= n; length <<= 1) {
        const double angle = -2.0 * M_PI / static_cast<double>(length);
        const float w_re = static_cast<float>(std::cos(angle));
        const float w_im = static_cast<float>(std::sin(angle));
        for (size_t start = 0; start < n; start += length) {
            float t_re = 1.0f;
            float t_im = 0.0f;
            for (size_t k = 0; k < length / 2; ++k) {
                const size_t a = start + k;
                const size_t b = a + length / 2;
                const float b_re = re[b] * t_re - im[b] * t_im;
                const float b_im = re[b] * t_im + im[b] * t_re;
                re[b] = re[a] - b_re;
                im[b] = im[a] - b_im;
                re[a] += b_re;
                im[a] += b_im;
                const float next_re = t_re * w_re - t_im * w_im;
                t_im = t_re * w_im + t_im * w_re;
                t_re = next_re;
            }
        }
    }
}

}  // namespace spectrum
