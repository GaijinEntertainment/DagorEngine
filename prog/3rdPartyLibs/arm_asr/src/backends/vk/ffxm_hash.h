// Copyright  © 2023 Advanced Micro Devices, Inc.
// Copyright  © 2024-2025 Arm Limited.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>

namespace arm {

/// @addtogroup util_other
/// @{

/// Computes a hash of a buffer. This function implements the MurmurHash2 algorithm by Austin Appleby.
/// @param[in] buffer The buffer to hash.
/// @param bufferSize The size of the buffer.
/// @param seed A unique seed.
/// @return The hash.
uint64_t computeHash(const void* buffer, size_t bufferSize, uint64_t seed = 123);

/// Computes a hash of a buffer. This function implements the MurmurHash2 algorithm by Austin Appleby.
/// @param[in] buffer The buffer to hash.
/// @param bufferSize The size of the buffer.
/// @param prevHash The hash to append to.
/// @return The new hash.
uint64_t appendHash(const void* buffer, size_t bufferSize, uint64_t prevHash);
/// @}

} // end namespace arm
