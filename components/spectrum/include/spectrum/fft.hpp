// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>

namespace spectrum {

// In-place radix-2 FFT. `n` must be a power of two. `re` and `im` hold n values each.
void fft(float* re, float* im, size_t n);

}  // namespace spectrum
