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

#include <ffxm_hash.h>

namespace arm {

constexpr uint64_t kHashM = 0xc6a4a7935bd1e995;
constexpr uint64_t kHashR = 47;

uint64_t appendHash(const void* buffer, size_t bufferSize, uint64_t h)
{
	const uint64_t* data = static_cast<const uint64_t*>(buffer);
	const uint64_t* const end = data + (bufferSize / sizeof(uint64_t));

	while(data != end)
	{
		uint64_t k = *data++;

		k *= kHashM;
		k ^= k >> kHashR;
		k *= kHashM;

		h ^= k;
		h *= kHashM;
	}

	const uint8_t* data2 = reinterpret_cast<const uint8_t*>(data);

	switch(bufferSize & (sizeof(uint64_t) - 1))
	{
	case 7:
		h ^= uint64_t(data2[6]) << 48;
	case 6:
		h ^= uint64_t(data2[5]) << 40;
	case 5:
		h ^= uint64_t(data2[4]) << 32;
	case 4:
		h ^= uint64_t(data2[3]) << 24;
	case 3:
		h ^= uint64_t(data2[2]) << 16;
	case 2:
		h ^= uint64_t(data2[1]) << 8;
	case 1:
		h ^= uint64_t(data2[0]);
		h *= kHashM;
	};

	h ^= h >> kHashR;
	h *= kHashM;
	h ^= h >> kHashR;

	return h;
}

uint64_t computeHash(const void* buffer, size_t bufferSize, uint64_t seed)
{
	const uint64_t h = seed ^ (bufferSize * kHashM);
	return appendHash(buffer, bufferSize, h);
}

} // end namespace arm
