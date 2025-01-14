#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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
////////////////////////////////////////////////////////////////////////////////

#include "acl/version.h"
#include "acl/core/impl/bit_cast.impl.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/decompression/impl/track_cache.h"
#include "acl/decompression/impl/decompression_context.transform.h"
#include "acl/math/quatf.h"
#include "acl/math/vector4f.h"

#include <rtm/mask4f.h>
#include <rtm/quatf.h>
#include <rtm/vector4f.h>
#include <rtm/packing/quatf.h>

#include <cstdint>

#define ACL_IMPL_USE_ANIMATED_PREFETCH

// On x86/x64 platforms the prefetching instruction can have a long latency and it requires
// a few other registers to compute the address which is problematic when registers are scarce.
// As such, we attempt to hide the prefetching behind longer latency instructions like square-roots
// and divisions.
// On other platforms (e.g. ARM), the instruction is cheaper and we have more registers which gives
// the compiler more freedom to hide the address calculation cost between other instructions.
// Because the CPU is generally slower as well, we want to prefetch as soon as possible without
// waiting for the next expensive instruction.
// If your target CPU has a high clock rate, you might benefit from disabling early prefetching
#if !defined(ACL_NO_EARLY_PREFETCHING) && !defined(ACL_IMPL_PREFETCH_EARLY)
	#if !defined(RTM_SSE2_INTRINSICS)
		#define ACL_IMPL_PREFETCH_EARLY
	#endif
#endif

// This defined enables the SIMD 8 wide AVX decompression code path
// Note that currently, it is often slower than the regular SIMD 4 wide AVX code path
// On Intel Haswell and AMD Zen2 CPUs, the 8 wide code is measurably slower
// Perhaps it is faster on newer Intel CPUs but I don't have one to test with
// Enable at your own risk
//#define ACL_IMPL_USE_AVX_8_WIDE_DECOMP

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
	#if !defined(RTM_AVX_INTRINSICS)
		// AVX isn't enabled, disable the 8 wide code path
		#undef ACL_IMPL_USE_AVX_8_WIDE_DECOMP
	#endif
#endif

ACL_IMPL_FILE_PRAGMA_PUSH

// We only initialize some variables when we need them which prompts the compiler to complain
// The usage is perfectly safe and because this code is VERY hot and needs to be as fast as possible,
// we disable the warning to avoid zeroing out things we don't need
#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(push)
	// warning C26495: Variable '...' is uninitialized. Always initialize a member variable (type.6).
	// We explicitly control initialization
	#pragma warning(disable : 26495)
	// warning C26451: Arithmetic overflow: Using operator '*' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator '*' to avoid overflow (io.2).
	// We can't overflow because compressed clips cannot contain more than 4 GB worth of data
	#pragma warning(disable : 26451)
#endif

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	namespace acl_impl
	{
#if defined(ACL_IMPL_USE_ANIMATED_PREFETCH)
#define ACL_IMPL_ANIMATED_PREFETCH(ptr) memory_prefetch(ptr)
#else
#define ACL_IMPL_ANIMATED_PREFETCH(ptr) (void)(ptr)
#endif

		struct clip_animated_sampling_context_v0
		{
			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Order depends on animated track order. If we have 6 animated rotation tracks before the first animated
			// translation track, we'll have 8 animated rotation sub-tracks followed by 4 animated translation sub-tracks.
			// Once we reach the end, there is no extra padding. The last group might be less than 4 sub-tracks.
			// This is because we always process 4 animated sub-tracks at a time and cache the results.

			const uint8_t* clip_range_data;				// Range information of the current sub-track in the clip
		};

		struct segment_animated_sampling_context_v0
		{
			// Data is ordered in groups of 4 animated sub-tracks (e.g rot0, rot1, rot2, rot3)
			// Order depends on animated track order. If we have 6 animated rotation tracks before the first animated
			// translation track, we'll have 8 animated rotation sub-tracks followed by 4 animated translation sub-tracks.
			// Once we reach the end, there is no extra padding. The last group might be less than 4 sub-tracks.
			// This is because we always process 4 animated sub-tracks at a time and cache the results.

			const uint8_t* format_per_track_data;		// Metadata of the current sub-track
			const uint8_t* segment_range_data;			// Range information (or constant sample if bit rate is 0) of the current sub-track in this segment

			// For the animated samples, constant bit rate sub-tracks (with a bit rate of 0) do not contain samples.
			// As such, their group will not contain 4 sub-tracks.

			const uint8_t* animated_track_data;			// Base of animated sample data, constant and doesn't change after init
			uint32_t animated_track_data_bit_offset;	// Bit offset of the current animated sub-track
		};

		struct alignas(32) segment_animated_scratch_v0
		{
			// We store out potential range data in SOA form and we have no W, just XYZ
			// To facilitate AVX and wider SIMD usage, we store our data interleaved in a single contiguous array
			// Segment 0 has a base offset of 0 bytes and afterwards every write has a 32 byte offset
			// Segment 1 has a base offset of 16 bytes and afterwards every write has a 32 byte offset

			// segment_range_min_xxxx0, segment_range_min_xxxx1, segment_range_min_yyyy0, segment_range_min_yyyy1, segment_range_min_zzzz0, segment_range_min_zzzz1
			rtm::vector4f segment_range_min[6];

			// segment_range_extent_xxxx0, segment_range_extent_xxxx1, segment_range_extent_yyyy0, segment_range_extent_yyyy1, segment_range_extent_zzzz0, segment_range_extent_zzzz1
			rtm::vector4f segment_range_extent[6];
		};

#if defined(RTM_SSE2_INTRINSICS)
		using range_reduction_masks_t = __m128i;
#elif defined(RTM_NEON_INTRINSICS)
		using range_reduction_masks_t = int16x8_t;
#else
		using range_reduction_masks_t = uint64_t;
#endif

		// About 9 cycles with AVX on Skylake
		inline RTM_DISABLE_SECURITY_COOKIE_CHECK void unpack_segment_range_data(const uint8_t* segment_range_data, uint32_t scratch_offset, segment_animated_scratch_v0& output_scratch)
		{
			// Segment range is packed: min.xxxx, min.yyyy, min.zzzz, extent.xxxx, extent.yyyy, extent.zzzz

#if defined(RTM_SSE2_INTRINSICS)
			const __m128i zero = _mm_setzero_si128();

			const __m128i segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8 = _mm_loadu_si128((const __m128i*)segment_range_data);
			const __m128i segment_range_extent_yyyy_zzzz_u8 = _mm_loadu_si128((const __m128i*)(segment_range_data + 16));

			// Convert from u8 to u32
			const __m128i segment_range_min_xxxx_yyyy_u16 = _mm_unpacklo_epi8(segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8, zero);
			const __m128i segment_range_min_zzzz_extent_xxxx_u16 = _mm_unpackhi_epi8(segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8, zero);
			const __m128i segment_range_extent_yyyy_zzzz_u16 = _mm_unpacklo_epi8(segment_range_extent_yyyy_zzzz_u8, zero);

			__m128i segment_range_min_xxxx_u32 = _mm_unpacklo_epi16(segment_range_min_xxxx_yyyy_u16, zero);
			__m128i segment_range_min_yyyy_u32 = _mm_unpackhi_epi16(segment_range_min_xxxx_yyyy_u16, zero);
			__m128i segment_range_min_zzzz_u32 = _mm_unpacklo_epi16(segment_range_min_zzzz_extent_xxxx_u16, zero);

			const __m128i segment_range_extent_xxxx_u32 = _mm_unpackhi_epi16(segment_range_min_zzzz_extent_xxxx_u16, zero);
			const __m128i segment_range_extent_yyyy_u32 = _mm_unpacklo_epi16(segment_range_extent_yyyy_zzzz_u16, zero);
			const __m128i segment_range_extent_zzzz_u32 = _mm_unpackhi_epi16(segment_range_extent_yyyy_zzzz_u16, zero);

			__m128 segment_range_min_xxxx = _mm_cvtepi32_ps(segment_range_min_xxxx_u32);
			__m128 segment_range_min_yyyy = _mm_cvtepi32_ps(segment_range_min_yyyy_u32);
			__m128 segment_range_min_zzzz = _mm_cvtepi32_ps(segment_range_min_zzzz_u32);

			__m128 segment_range_extent_xxxx = _mm_cvtepi32_ps(segment_range_extent_xxxx_u32);
			__m128 segment_range_extent_yyyy = _mm_cvtepi32_ps(segment_range_extent_yyyy_u32);
			__m128 segment_range_extent_zzzz = _mm_cvtepi32_ps(segment_range_extent_zzzz_u32);

			const __m128 normalization_value = _mm_set_ps1(1.0F / 255.0F);

			segment_range_min_xxxx = _mm_mul_ps(segment_range_min_xxxx, normalization_value);
			segment_range_min_yyyy = _mm_mul_ps(segment_range_min_yyyy, normalization_value);
			segment_range_min_zzzz = _mm_mul_ps(segment_range_min_zzzz, normalization_value);

			segment_range_extent_xxxx = _mm_mul_ps(segment_range_extent_xxxx, normalization_value);
			segment_range_extent_yyyy = _mm_mul_ps(segment_range_extent_yyyy, normalization_value);
			segment_range_extent_zzzz = _mm_mul_ps(segment_range_extent_zzzz, normalization_value);
#elif defined(RTM_NEON_INTRINSICS)
			const uint8x16_t segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8 = vld1q_u8(segment_range_data);
			const uint8x8_t segment_range_extent_yyyy_zzzz_u8 = vld1_u8(segment_range_data + 16);

			// Convert from u8 to u32
			const uint16x8_t segment_range_min_xxxx_yyyy_u16 = vmovl_u8(vget_low_u8(segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8));
			const uint16x8_t segment_range_min_zzzz_extent_xxxx_u16 = vmovl_u8(vget_high_u8(segment_range_min_xxxx_yyyy_zzzz_extent_xxxx_u8));
			const uint16x8_t segment_range_extent_yyyy_zzzz_u16 = vmovl_u8(segment_range_extent_yyyy_zzzz_u8);

			uint32x4_t segment_range_min_xxxx_u32 = vmovl_u16(vget_low_u16(segment_range_min_xxxx_yyyy_u16));
			uint32x4_t segment_range_min_yyyy_u32 = vmovl_u16(vget_high_u16(segment_range_min_xxxx_yyyy_u16));
			uint32x4_t segment_range_min_zzzz_u32 = vmovl_u16(vget_low_u16(segment_range_min_zzzz_extent_xxxx_u16));

			const uint32x4_t segment_range_extent_xxxx_u32 = vmovl_u16(vget_high_u16(segment_range_min_zzzz_extent_xxxx_u16));
			const uint32x4_t segment_range_extent_yyyy_u32 = vmovl_u16(vget_low_u16(segment_range_extent_yyyy_zzzz_u16));
			const uint32x4_t segment_range_extent_zzzz_u32 = vmovl_u16(vget_high_u16(segment_range_extent_yyyy_zzzz_u16));

			float32x4_t segment_range_min_xxxx = vcvtq_f32_u32(segment_range_min_xxxx_u32);
			float32x4_t segment_range_min_yyyy = vcvtq_f32_u32(segment_range_min_yyyy_u32);
			float32x4_t segment_range_min_zzzz = vcvtq_f32_u32(segment_range_min_zzzz_u32);

			float32x4_t segment_range_extent_xxxx = vcvtq_f32_u32(segment_range_extent_xxxx_u32);
			float32x4_t segment_range_extent_yyyy = vcvtq_f32_u32(segment_range_extent_yyyy_u32);
			float32x4_t segment_range_extent_zzzz = vcvtq_f32_u32(segment_range_extent_zzzz_u32);

			const float normalization_value = 1.0F / 255.0F;

			segment_range_min_xxxx = vmulq_n_f32(segment_range_min_xxxx, normalization_value);
			segment_range_min_yyyy = vmulq_n_f32(segment_range_min_yyyy, normalization_value);
			segment_range_min_zzzz = vmulq_n_f32(segment_range_min_zzzz, normalization_value);

			segment_range_extent_xxxx = vmulq_n_f32(segment_range_extent_xxxx, normalization_value);
			segment_range_extent_yyyy = vmulq_n_f32(segment_range_extent_yyyy, normalization_value);
			segment_range_extent_zzzz = vmulq_n_f32(segment_range_extent_zzzz, normalization_value);
#else
			rtm::vector4f segment_range_min_xxxx = rtm::vector_set(float(segment_range_data[0]), float(segment_range_data[1]), float(segment_range_data[2]), float(segment_range_data[3]));
			rtm::vector4f segment_range_min_yyyy = rtm::vector_set(float(segment_range_data[4]), float(segment_range_data[5]), float(segment_range_data[6]), float(segment_range_data[7]));
			rtm::vector4f segment_range_min_zzzz = rtm::vector_set(float(segment_range_data[8]), float(segment_range_data[9]), float(segment_range_data[10]), float(segment_range_data[11]));

			rtm::vector4f segment_range_extent_xxxx = rtm::vector_set(float(segment_range_data[12]), float(segment_range_data[13]), float(segment_range_data[14]), float(segment_range_data[15]));
			rtm::vector4f segment_range_extent_yyyy = rtm::vector_set(float(segment_range_data[16]), float(segment_range_data[17]), float(segment_range_data[18]), float(segment_range_data[19]));
			rtm::vector4f segment_range_extent_zzzz = rtm::vector_set(float(segment_range_data[20]), float(segment_range_data[21]), float(segment_range_data[22]), float(segment_range_data[23]));

			const float normalization_value = 1.0F / 255.0F;

			segment_range_min_xxxx = rtm::vector_mul(segment_range_min_xxxx, normalization_value);
			segment_range_min_yyyy = rtm::vector_mul(segment_range_min_yyyy, normalization_value);
			segment_range_min_zzzz = rtm::vector_mul(segment_range_min_zzzz, normalization_value);

			segment_range_extent_xxxx = rtm::vector_mul(segment_range_extent_xxxx, normalization_value);
			segment_range_extent_yyyy = rtm::vector_mul(segment_range_extent_yyyy, normalization_value);
			segment_range_extent_zzzz = rtm::vector_mul(segment_range_extent_zzzz, normalization_value);
#endif

#if defined(ACL_IMPL_PREFETCH_EARLY)
			// Prefetch the next cache line even if we don't have any data left
			// By the time we unpack again, it will have arrived in the CPU cache
			// If our format is full precision, we have at most 4 samples per cache line
			// If our format is drop W, we have at most 5.33 samples per cache line

			// If our pointer was already aligned to a cache line before we unpacked our 4 values,
			// it now points to the first byte of the next cache line. Any offset between 0-63 will fetch it.
			// If our pointer had some offset into a cache line, we might have spanned 2 cache lines.
			// If this happens, we probably already read some data from the next cache line in which
			// case we don't need to prefetch it and we can go to the next one. Any offset after the end
			// of this cache line will fetch it. For safety, we prefetch 63 bytes ahead.
			// Prefetch 4 samples ahead in all levels of the CPU cache
			ACL_IMPL_ANIMATED_PREFETCH(segment_range_data + 64);
#endif

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
			// With AVX, we must duplicate our data for the first segment in case we don't have a second segment
			if (scratch_offset == 0)
			{
				// First segment data is duplicated
				// Use 256 bit stores to avoid doing too many stores which might stall
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_min[0]), _mm256_set_m128(segment_range_min_xxxx, segment_range_min_xxxx));
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_min[2]), _mm256_set_m128(segment_range_min_yyyy, segment_range_min_yyyy));
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_min[4]), _mm256_set_m128(segment_range_min_zzzz, segment_range_min_zzzz));
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_extent[0]), _mm256_set_m128(segment_range_extent_xxxx, segment_range_extent_xxxx));
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_extent[2]), _mm256_set_m128(segment_range_extent_yyyy, segment_range_extent_yyyy));
				_mm256_store_ps(bit_cast<float*>(&output_scratch.segment_range_extent[4]), _mm256_set_m128(segment_range_extent_zzzz, segment_range_extent_zzzz));
			}
			else
			{
				// Second segment overwrites our data
				output_scratch.segment_range_min[1] = segment_range_min_xxxx;
				output_scratch.segment_range_min[3] = segment_range_min_yyyy;
				output_scratch.segment_range_min[5] = segment_range_min_zzzz;
				output_scratch.segment_range_extent[1] = segment_range_extent_xxxx;
				output_scratch.segment_range_extent[3] = segment_range_extent_yyyy;
				output_scratch.segment_range_extent[5] = segment_range_extent_zzzz;
			}
#else
			output_scratch.segment_range_min[scratch_offset + 0] = segment_range_min_xxxx;
			output_scratch.segment_range_min[scratch_offset + 2] = segment_range_min_yyyy;
			output_scratch.segment_range_min[scratch_offset + 4] = segment_range_min_zzzz;
			output_scratch.segment_range_extent[scratch_offset + 0] = segment_range_extent_xxxx;
			output_scratch.segment_range_extent[scratch_offset + 2] = segment_range_extent_yyyy;
			output_scratch.segment_range_extent[scratch_offset + 4] = segment_range_extent_zzzz;
#endif
		}

		// About 19 cycles with AVX on Skylake
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL remap_segment_range_data4(const segment_animated_scratch_v0& segment_scratch, uint32_t scratch_offset, range_reduction_masks_t range_reduction_masks,
			rtm::vector4f& xxxx, rtm::vector4f& yyyy, rtm::vector4f& zzzz)
		{
			// Load and mask out our segment range data
			const rtm::vector4f one_v = rtm::vector_set(1.0F);

			rtm::vector4f segment_range_min_xxxx = segment_scratch.segment_range_min[scratch_offset + 0];
			rtm::vector4f segment_range_min_yyyy = segment_scratch.segment_range_min[scratch_offset + 2];
			rtm::vector4f segment_range_min_zzzz = segment_scratch.segment_range_min[scratch_offset + 4];

			rtm::vector4f segment_range_extent_xxxx = segment_scratch.segment_range_extent[scratch_offset + 0];
			rtm::vector4f segment_range_extent_yyyy = segment_scratch.segment_range_extent[scratch_offset + 2];
			rtm::vector4f segment_range_extent_zzzz = segment_scratch.segment_range_extent[scratch_offset + 4];

#if defined(RTM_SSE2_INTRINSICS)
			// Mask out the segment min we ignore
			const rtm::mask4f segment_range_ignore_mask_v = _mm_castsi128_ps(_mm_unpacklo_epi16(range_reduction_masks, range_reduction_masks));

			segment_range_min_xxxx = _mm_andnot_ps(segment_range_ignore_mask_v, segment_range_min_xxxx);
			segment_range_min_yyyy = _mm_andnot_ps(segment_range_ignore_mask_v, segment_range_min_yyyy);
			segment_range_min_zzzz = _mm_andnot_ps(segment_range_ignore_mask_v, segment_range_min_zzzz);
#elif defined(RTM_NEON_INTRINSICS)
			// Mask out the segment min we ignore
			const uint32x4_t segment_range_ignore_mask_v = vreinterpretq_u32_s32(vmovl_s16(vget_low_s16(range_reduction_masks)));

			segment_range_min_xxxx = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(segment_range_min_xxxx), segment_range_ignore_mask_v));
			segment_range_min_yyyy = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(segment_range_min_yyyy), segment_range_ignore_mask_v));
			segment_range_min_zzzz = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(segment_range_min_zzzz), segment_range_ignore_mask_v));
#else
			const rtm::vector4f zero_v = rtm::vector_zero();

			const uint32_t segment_range_mask_u32 = uint32_t(range_reduction_masks);
			const rtm::mask4f segment_range_ignore_mask_v = rtm::mask_set((segment_range_mask_u32 & 0x000000FF) != 0, (segment_range_mask_u32 & 0x0000FF00) != 0, (segment_range_mask_u32 & 0x00FF0000) != 0, (segment_range_mask_u32 & 0xFF000000) != 0);

			segment_range_min_xxxx = rtm::vector_select(segment_range_ignore_mask_v, zero_v, segment_range_min_xxxx);
			segment_range_min_yyyy = rtm::vector_select(segment_range_ignore_mask_v, zero_v, segment_range_min_yyyy);
			segment_range_min_zzzz = rtm::vector_select(segment_range_ignore_mask_v, zero_v, segment_range_min_zzzz);
#endif

			// Mask out the segment extent we ignore
			segment_range_extent_xxxx = rtm::vector_select(segment_range_ignore_mask_v, one_v, segment_range_extent_xxxx);
			segment_range_extent_yyyy = rtm::vector_select(segment_range_ignore_mask_v, one_v, segment_range_extent_yyyy);
			segment_range_extent_zzzz = rtm::vector_select(segment_range_ignore_mask_v, one_v, segment_range_extent_zzzz);

			// Remap
			xxxx = rtm::vector_mul_add(xxxx, segment_range_extent_xxxx, segment_range_min_xxxx);
			yyyy = rtm::vector_mul_add(yyyy, segment_range_extent_yyyy, segment_range_min_yyyy);
			zzzz = rtm::vector_mul_add(zzzz, segment_range_extent_zzzz, segment_range_min_zzzz);
		}

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL remap_segment_range_data_avx8(const segment_animated_scratch_v0& segment_scratch,
			range_reduction_masks_t range_reduction_masks0, range_reduction_masks_t range_reduction_masks1,
			__m256& xxxx0_xxxx1, __m256& yyyy0_yyyy1, __m256& zzzz0_zzzz1)
		{
			// Load and mask out our segment range data
			const __m256 one_v = _mm256_set1_ps(1.0F);

			__m256 segment_range_min_xxxx0_xxxx1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_min[0]));
			__m256 segment_range_min_yyyy0_yyyy1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_min[2]));
			__m256 segment_range_min_zzzz0_zzzz1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_min[4]));

			__m256 segment_range_extent_xxxx0_xxxx1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_extent[0]));
			__m256 segment_range_extent_yyyy0_yyyy1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_extent[2]));
			__m256 segment_range_extent_zzzz0_zzzz1 = _mm256_load_ps(bit_cast<const float*>(&segment_scratch.segment_range_extent[4]));

			// Mask out the segment min we ignore
			const __m128 segment_range_ignore_mask_v0 = _mm_castsi128_ps(_mm_unpacklo_epi16(range_reduction_masks0, range_reduction_masks0));
			const __m128 segment_range_ignore_mask_v1 = _mm_castsi128_ps(_mm_unpacklo_epi16(range_reduction_masks1, range_reduction_masks1));

			const __m256 segment_range_mask0_mask1 = _mm256_set_m128(segment_range_ignore_mask_v1, segment_range_ignore_mask_v0);

			segment_range_min_xxxx0_xxxx1 = _mm256_andnot_ps(segment_range_mask0_mask1, segment_range_min_xxxx0_xxxx1);
			segment_range_min_yyyy0_yyyy1 = _mm256_andnot_ps(segment_range_mask0_mask1, segment_range_min_yyyy0_yyyy1);
			segment_range_min_zzzz0_zzzz1 = _mm256_andnot_ps(segment_range_mask0_mask1, segment_range_min_zzzz0_zzzz1);

			segment_range_extent_xxxx0_xxxx1 = _mm256_blendv_ps(segment_range_extent_xxxx0_xxxx1, one_v, segment_range_mask0_mask1);
			segment_range_extent_yyyy0_yyyy1 = _mm256_blendv_ps(segment_range_extent_yyyy0_yyyy1, one_v, segment_range_mask0_mask1);
			segment_range_extent_zzzz0_zzzz1 = _mm256_blendv_ps(segment_range_extent_zzzz0_zzzz1, one_v, segment_range_mask0_mask1);

			xxxx0_xxxx1 = _mm256_add_ps(_mm256_mul_ps(xxxx0_xxxx1, segment_range_extent_xxxx0_xxxx1), segment_range_min_xxxx0_xxxx1);
			yyyy0_yyyy1 = _mm256_add_ps(_mm256_mul_ps(yyyy0_yyyy1, segment_range_extent_yyyy0_yyyy1), segment_range_min_yyyy0_yyyy1);
			zzzz0_zzzz1 = _mm256_add_ps(_mm256_mul_ps(zzzz0_zzzz1, segment_range_extent_zzzz0_zzzz1), segment_range_min_zzzz0_zzzz1);
		}
#endif

		// About 24 cycles with AVX on Skylake
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL remap_clip_range_data4(const uint8_t* clip_range_data, uint32_t num_to_unpack,
			range_reduction_masks_t range_reduction_masks0, range_reduction_masks_t range_reduction_masks1,
			rtm::vector4f& xxxx0, rtm::vector4f& yyyy0, rtm::vector4f& zzzz0,
			rtm::vector4f& xxxx1, rtm::vector4f& yyyy1, rtm::vector4f& zzzz1)
		{
			// Always load 4x rotations, we might contain garbage in a few lanes but it's fine
			const uint32_t load_size = num_to_unpack * sizeof(float);

#if defined(RTM_SSE2_INTRINSICS)
			const __m128 clip_range_mask0 = _mm_castsi128_ps(_mm_unpackhi_epi16(range_reduction_masks0, range_reduction_masks0));
			const __m128 clip_range_mask1 = _mm_castsi128_ps(_mm_unpackhi_epi16(range_reduction_masks1, range_reduction_masks1));
#elif defined(RTM_NEON_INTRINSICS)
			const uint32x4_t clip_range_mask0 = vreinterpretq_u32_s32(vmovl_s16(vget_high_s16(range_reduction_masks0)));
			const uint32x4_t clip_range_mask1 = vreinterpretq_u32_s32(vmovl_s16(vget_high_s16(range_reduction_masks1)));
#else
			const uint32_t clip_range_mask_u32_0 = uint32_t(range_reduction_masks0 >> 32);
			const uint32_t clip_range_mask_u32_1 = uint32_t(range_reduction_masks1 >> 32);
			const rtm::mask4f clip_range_mask0 = rtm::mask_set((clip_range_mask_u32_0 & 0x000000FF) != 0, (clip_range_mask_u32_0 & 0x0000FF00) != 0, (clip_range_mask_u32_0 & 0x00FF0000) != 0, (clip_range_mask_u32_0 & 0xFF000000) != 0);
			const rtm::mask4f clip_range_mask1 = rtm::mask_set((clip_range_mask_u32_1 & 0x000000FF) != 0, (clip_range_mask_u32_1 & 0x0000FF00) != 0, (clip_range_mask_u32_1 & 0x00FF0000) != 0, (clip_range_mask_u32_1 & 0xFF000000) != 0);
#endif

			const rtm::vector4f clip_range_min_xxxx = rtm::vector_load(clip_range_data + load_size * 0);
			const rtm::vector4f clip_range_min_yyyy = rtm::vector_load(clip_range_data + load_size * 1);
			const rtm::vector4f clip_range_min_zzzz = rtm::vector_load(clip_range_data + load_size * 2);

			const rtm::vector4f clip_range_extent_xxxx = rtm::vector_load(clip_range_data + load_size * 3);
			const rtm::vector4f clip_range_extent_yyyy = rtm::vector_load(clip_range_data + load_size * 4);
			const rtm::vector4f clip_range_extent_zzzz = rtm::vector_load(clip_range_data + load_size * 5);

			// Mask out the clip ranges we ignore
#if defined(RTM_SSE2_INTRINSICS)
			const rtm::vector4f clip_range_min_xxxx0 = _mm_andnot_ps(clip_range_mask0, clip_range_min_xxxx);
			const rtm::vector4f clip_range_min_yyyy0 = _mm_andnot_ps(clip_range_mask0, clip_range_min_yyyy);
			const rtm::vector4f clip_range_min_zzzz0 = _mm_andnot_ps(clip_range_mask0, clip_range_min_zzzz);

			const rtm::vector4f clip_range_min_xxxx1 = _mm_andnot_ps(clip_range_mask1, clip_range_min_xxxx);
			const rtm::vector4f clip_range_min_yyyy1 = _mm_andnot_ps(clip_range_mask1, clip_range_min_yyyy);
			const rtm::vector4f clip_range_min_zzzz1 = _mm_andnot_ps(clip_range_mask1, clip_range_min_zzzz);
#elif defined(RTM_NEON_INTRINSICS)
			const rtm::vector4f clip_range_min_xxxx0 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_xxxx), clip_range_mask0));
			const rtm::vector4f clip_range_min_yyyy0 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_yyyy), clip_range_mask0));
			const rtm::vector4f clip_range_min_zzzz0 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_zzzz), clip_range_mask0));

			const rtm::vector4f clip_range_min_xxxx1 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_xxxx), clip_range_mask1));
			const rtm::vector4f clip_range_min_yyyy1 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_yyyy), clip_range_mask1));
			const rtm::vector4f clip_range_min_zzzz1 = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(clip_range_min_zzzz), clip_range_mask1));
#else
			const rtm::vector4f zero_v = rtm::vector_zero();

			const rtm::vector4f clip_range_min_xxxx0 = rtm::vector_select(clip_range_mask0, zero_v, clip_range_min_xxxx);
			const rtm::vector4f clip_range_min_yyyy0 = rtm::vector_select(clip_range_mask0, zero_v, clip_range_min_yyyy);
			const rtm::vector4f clip_range_min_zzzz0 = rtm::vector_select(clip_range_mask0, zero_v, clip_range_min_zzzz);

			const rtm::vector4f clip_range_min_xxxx1 = rtm::vector_select(clip_range_mask1, zero_v, clip_range_min_xxxx);
			const rtm::vector4f clip_range_min_yyyy1 = rtm::vector_select(clip_range_mask1, zero_v, clip_range_min_yyyy);
			const rtm::vector4f clip_range_min_zzzz1 = rtm::vector_select(clip_range_mask1, zero_v, clip_range_min_zzzz);
#endif

			const rtm::vector4f one_v = rtm::vector_set(1.0F);

			const rtm::vector4f clip_range_extent_xxxx0 = rtm::vector_select(clip_range_mask0, one_v, clip_range_extent_xxxx);
			const rtm::vector4f clip_range_extent_yyyy0 = rtm::vector_select(clip_range_mask0, one_v, clip_range_extent_yyyy);
			const rtm::vector4f clip_range_extent_zzzz0 = rtm::vector_select(clip_range_mask0, one_v, clip_range_extent_zzzz);

			const rtm::vector4f clip_range_extent_xxxx1 = rtm::vector_select(clip_range_mask1, one_v, clip_range_extent_xxxx);
			const rtm::vector4f clip_range_extent_yyyy1 = rtm::vector_select(clip_range_mask1, one_v, clip_range_extent_yyyy);
			const rtm::vector4f clip_range_extent_zzzz1 = rtm::vector_select(clip_range_mask1, one_v, clip_range_extent_zzzz);

			xxxx0 = rtm::vector_mul_add(xxxx0, clip_range_extent_xxxx0, clip_range_min_xxxx0);
			yyyy0 = rtm::vector_mul_add(yyyy0, clip_range_extent_yyyy0, clip_range_min_yyyy0);
			zzzz0 = rtm::vector_mul_add(zzzz0, clip_range_extent_zzzz0, clip_range_min_zzzz0);

			xxxx1 = rtm::vector_mul_add(xxxx1, clip_range_extent_xxxx1, clip_range_min_xxxx1);
			yyyy1 = rtm::vector_mul_add(yyyy1, clip_range_extent_yyyy1, clip_range_min_yyyy1);
			zzzz1 = rtm::vector_mul_add(zzzz1, clip_range_extent_zzzz1, clip_range_min_zzzz1);
		}

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
		// Force inline this function, we only use it to keep the code readable
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void RTM_SIMD_CALL remap_clip_range_data_avx8(const uint8_t* clip_range_data, uint32_t num_to_unpack,
			range_reduction_masks_t range_reduction_masks0, range_reduction_masks_t range_reduction_masks1,
			__m256& xxxx0_xxxx1, __m256& yyyy0_yyyy1, __m256& zzzz0_zzzz1)
		{
			const __m256 one_v = _mm256_set1_ps(1.0F);

			// Always load 4x rotations, we might contain garbage in a few lanes but it's fine
			const uint32_t load_size = num_to_unpack * sizeof(float);

			const __m128 clip_range_mask0 = _mm_castsi128_ps(_mm_unpackhi_epi16(range_reduction_masks0, range_reduction_masks0));
			const __m128 clip_range_mask1 = _mm_castsi128_ps(_mm_unpackhi_epi16(range_reduction_masks1, range_reduction_masks1));

			const __m256 clip_range_mask0_mask1 = _mm256_set_m128(clip_range_mask1, clip_range_mask0);

			const rtm::vector4f clip_range_min_xxxx = rtm::vector_load(clip_range_data + load_size * 0);
			const rtm::vector4f clip_range_min_yyyy = rtm::vector_load(clip_range_data + load_size * 1);
			const rtm::vector4f clip_range_min_zzzz = rtm::vector_load(clip_range_data + load_size * 2);

			const rtm::vector4f clip_range_extent_xxxx = rtm::vector_load(clip_range_data + load_size * 3);
			const rtm::vector4f clip_range_extent_yyyy = rtm::vector_load(clip_range_data + load_size * 4);
			const rtm::vector4f clip_range_extent_zzzz = rtm::vector_load(clip_range_data + load_size * 5);

			__m256 clip_range_min_xxxx_xxxx = _mm256_set_m128(clip_range_min_xxxx, clip_range_min_xxxx);
			__m256 clip_range_min_yyyy_yyyy = _mm256_set_m128(clip_range_min_yyyy, clip_range_min_yyyy);
			__m256 clip_range_min_zzzz_zzzz = _mm256_set_m128(clip_range_min_zzzz, clip_range_min_zzzz);

			__m256 clip_range_extent_xxxx_xxxx = _mm256_set_m128(clip_range_extent_xxxx, clip_range_extent_xxxx);
			__m256 clip_range_extent_yyyy_yyyy = _mm256_set_m128(clip_range_extent_yyyy, clip_range_extent_yyyy);
			__m256 clip_range_extent_zzzz_zzzz = _mm256_set_m128(clip_range_extent_zzzz, clip_range_extent_zzzz);

			// Mask out the clip ranges we ignore
			clip_range_min_xxxx_xxxx = _mm256_andnot_ps(clip_range_mask0_mask1, clip_range_min_xxxx_xxxx);
			clip_range_min_yyyy_yyyy = _mm256_andnot_ps(clip_range_mask0_mask1, clip_range_min_yyyy_yyyy);
			clip_range_min_zzzz_zzzz = _mm256_andnot_ps(clip_range_mask0_mask1, clip_range_min_zzzz_zzzz);

			clip_range_extent_xxxx_xxxx = _mm256_blendv_ps(clip_range_extent_xxxx_xxxx, one_v, clip_range_mask0_mask1);
			clip_range_extent_yyyy_yyyy = _mm256_blendv_ps(clip_range_extent_yyyy_yyyy, one_v, clip_range_mask0_mask1);
			clip_range_extent_zzzz_zzzz = _mm256_blendv_ps(clip_range_extent_zzzz_zzzz, one_v, clip_range_mask0_mask1);

			xxxx0_xxxx1 = _mm256_add_ps(_mm256_mul_ps(xxxx0_xxxx1, clip_range_extent_xxxx_xxxx), clip_range_min_xxxx_xxxx);
			yyyy0_yyyy1 = _mm256_add_ps(_mm256_mul_ps(yyyy0_yyyy1, clip_range_extent_yyyy_yyyy), clip_range_min_yyyy_yyyy);
			zzzz0_zzzz1 = _mm256_add_ps(_mm256_mul_ps(zzzz0_zzzz1, clip_range_extent_zzzz_zzzz), clip_range_min_zzzz_zzzz);
		}
#endif

		template<class decompression_settings_type>
		inline RTM_DISABLE_SECURITY_COOKIE_CHECK range_reduction_masks_t RTM_SIMD_CALL unpack_animated_quat(const persistent_transform_decompression_context_v0& decomp_context, rtm::vector4f output_scratch[4],
			uint32_t num_to_unpack, segment_animated_sampling_context_v0& segment_sampling_context)
		{
			const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
			const compressed_tracks_version16 version = get_version<decompression_settings_type>(decomp_context.get_version());

			// See write_format_per_track_data(..) for details
			const uint32_t num_raw_bit_rate_bits = version >= compressed_tracks_version16::v02_01_99_1 ? 31U : 32U;

			uint32_t segment_range_ignore_mask = 0;
			uint32_t clip_range_ignore_mask = 0;

			const uint8_t* format_per_track_data = segment_sampling_context.format_per_track_data;
			const uint8_t* segment_range_data = segment_sampling_context.segment_range_data;
			const uint8_t* animated_track_data = segment_sampling_context.animated_track_data;
			uint32_t animated_track_data_bit_offset = segment_sampling_context.animated_track_data_bit_offset;

			// For SIMD, can we load constant samples and write them to scratch? Afterwards its the same as packed on 16 bits
			// We get 4 short branches (test, cmp, 6x loads, 3x ORs, 3x writes, load immediate) followed by a common code path for all 4 samples

			// Maybe we can write in SOA order directly, and we could even write the scaling value per lane, load ones multiply 3 times for xyz in SOA

			// Try inlining the unpacking functions

			for (uint32_t unpack_index = 0; unpack_index < num_to_unpack; ++unpack_index)
			{
				// Our decompressed rotation as a vector4
				rtm::vector4f rotation_as_vec;

				if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
				{
					const uint32_t num_bits_at_bit_rate = format_per_track_data[unpack_index];

					uint32_t sample_segment_range_ignore_mask;
					uint32_t sample_clip_range_ignore_mask;

					if (num_bits_at_bit_rate == 0)	// Constant bit rate
					{
						// Segment range is packed: min.xxxx, min.yyyy, min.zzzz, extent.xxxx, extent.yyyy, extent.zzzz
						// Our constant sample value is packed 8 bits in each group in the sample's lane
						// To load our sample, we need to load: (min.x[unpack_index] << 8) | min.y[unpack_index], (min.z[unpack_index] << 8) | extent.x[unpack_index], (extent.y[unpack_index] << 8) | extent.z[unpack_index]
						// This is more complicated than if we were in AOS form but constant bit rates are somewhat rare while nearly every sample
						// has segment range information which is a lot simpler to load in SOA form
						const uint8_t* shifted_segment_range_data = segment_range_data + unpack_index;
						const uint32_t x = (uint32_t(shifted_segment_range_data[0]) << 8) | shifted_segment_range_data[4];
						const uint32_t y = (uint32_t(shifted_segment_range_data[8]) << 8) | shifted_segment_range_data[12];
						const uint32_t z = (uint32_t(shifted_segment_range_data[16]) << 8) | shifted_segment_range_data[20];

#if defined(RTM_SSE2_INTRINSICS)
						// TODO: Use SIMD for this

						// Load min.xxxx, min.yyyy, 8 bytes, offset by our sample index such that the first byte is our sample
						// Unpack low and interleave xxxx, yyyy, we end up with sample.x in our first lane as uint16_t
						// Unpack low to convert to uint32_t, sample.x lives in lane 0, repeat for sample.yz
						// Total of 2x loads (re-use first load and interleave high for sample.y), 5x unpack
						// Merge sample.xy together (1x shuffle)
						// Merge sample.xyz together (1x shuffle)
						// Convert to floats and normalize
						__m128i xyz = _mm_setr_epi32(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z), 0);
						__m128 xyzf = _mm_cvtepi32_ps(xyz);
						rotation_as_vec = _mm_mul_ps(xyzf, _mm_set_ps1(1.0F / 65535.0F));
#elif defined(RTM_NEON_INTRINSICS)
						uint32x4_t xyz = vcombine_u32(vcreate_u32((uint64_t(y) << 32) | x), vcreate_u32(z));
						float32x4_t xyzf = vcvtq_f32_u32(xyz);
						rotation_as_vec = vmulq_n_f32(xyzf, 1.0F / 65535.0F);
#else
						const rtm::vector4f xyz = rtm::vector_set(float(x), float(y), float(z), 0.0F);
						rotation_as_vec = rtm::vector_mul(xyz, 1.0F / 65535.0F);
#endif

						sample_segment_range_ignore_mask = 0xFF;	// Ignore segment range
						sample_clip_range_ignore_mask = 0x00;
					}
					else if (num_bits_at_bit_rate == num_raw_bit_rate_bits)	// Raw bit rate
					{
						rotation_as_vec = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += 96;
						sample_segment_range_ignore_mask = 0xFF;	// Ignore segment range
						sample_clip_range_ignore_mask = 0xFF;		// Ignore clip range
					}
					else
					{
						rotation_as_vec = unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += num_bits_at_bit_rate * 3;
						sample_segment_range_ignore_mask = 0x00;
						sample_clip_range_ignore_mask = 0x00;
					}

					// Masks are used in little endian format so the first sample is in the LSB end
					segment_range_ignore_mask |= sample_segment_range_ignore_mask << (unpack_index * 8);
					clip_range_ignore_mask |= sample_clip_range_ignore_mask << (unpack_index * 8);
				}
				else
				{
					if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
					{
						rotation_as_vec = unpack_vector4_128_unsafe(animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += 128;
					}
					else // rotation_format8::quatf_drop_w_full
					{
						rotation_as_vec = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += 96;
					}
				}

				output_scratch[unpack_index] = rotation_as_vec;
			}

			// Prefetch the next cache line even if we don't have any data left
			// By the time we unpack again, it will have arrived in the CPU cache
			// If our format is full precision, we have at most 4 samples per cache line
			// If our format is drop W, we have at most 5.33 samples per cache line

			// If our pointer was already aligned to a cache line before we unpacked our 4 values,
			// it now points to the first byte of the next cache line. Any offset between 0-63 will fetch it.
			// If our pointer had some offset into a cache line, we might have spanned 2 cache lines.
			// If this happens, we probably already read some data from the next cache line in which
			// case we don't need to prefetch it and we can go to the next one. Any offset after the end
			// of this cache line will fetch it. For safety, we prefetch 63 bytes ahead.
			// Prefetch 4 samples ahead in all levels of the CPU cache
#if defined(ACL_IMPL_PREFETCH_EARLY)
			ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_track_data_bit_offset / 8) + 63);
#endif

			// Update our pointers
			if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
			{
#if defined(ACL_IMPL_PREFETCH_EARLY)
				// Prefetch the next cache line in all levels of the CPU cache
				ACL_IMPL_ANIMATED_PREFETCH(format_per_track_data + 64);
#endif

				// Skip our used metadata data, all groups are padded to 4 elements
				segment_sampling_context.format_per_track_data = format_per_track_data + 4;
			}

			segment_sampling_context.animated_track_data_bit_offset = animated_track_data_bit_offset;

			range_reduction_masks_t range_reduction_masks;	// function's return value

			if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
			{
#if defined(RTM_SSE2_INTRINSICS)
				const __m128i ignore_masks_v8 = _mm_set_epi32(0, 0, static_cast<int32_t>(clip_range_ignore_mask), static_cast<int32_t>(segment_range_ignore_mask));
				range_reduction_masks = _mm_unpacklo_epi8(ignore_masks_v8, ignore_masks_v8);
#elif defined(RTM_NEON_INTRINSICS)
				const int8x8_t ignore_masks_v8 = vcreate_s8((uint64_t(clip_range_ignore_mask) << 32) | segment_range_ignore_mask);
				range_reduction_masks = vmovl_s8(ignore_masks_v8);
#else
				range_reduction_masks = (uint64_t(clip_range_ignore_mask) << 32) | segment_range_ignore_mask;
#endif

				// Skip our used segment range data, all groups are padded to 4 elements
				segment_range_data += 6 * 4;

				// Update our ptr
				segment_sampling_context.segment_range_data = segment_range_data;
			}
			else
			{
#if defined(RTM_SSE2_INTRINSICS)
				range_reduction_masks = _mm_setzero_si128();
#elif defined(RTM_NEON_INTRINSICS)
				range_reduction_masks = vcombine_s16(vcreate_s16(0ULL), vcreate_s16(0ULL));
#else
				range_reduction_masks = 0ULL;
#endif
			}

			return range_reduction_masks;
		}

		template<class decompression_settings_type>
		inline RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_single_animated_quat(const persistent_transform_decompression_context_v0& decomp_context,
			uint32_t unpack_index, uint32_t group_size,
			const clip_animated_sampling_context_v0& clip_sampling_context, const segment_animated_sampling_context_v0& segment_sampling_context)
		{
			const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
			const compressed_tracks_version16 version = get_version<decompression_settings_type>(decomp_context.get_version());

			// See write_format_per_track_data(..) for details
			const uint32_t num_raw_bit_rate_bits = version >= compressed_tracks_version16::v02_01_99_1 ? 31U : 32U;

			uint32_t segment_range_ignore_mask = 0;
			uint32_t clip_range_ignore_mask = 0;

			const uint8_t* format_per_track_data = segment_sampling_context.format_per_track_data;
			const uint8_t* segment_range_data = segment_sampling_context.segment_range_data;
			const uint8_t* animated_track_data = segment_sampling_context.animated_track_data;
			uint32_t animated_track_data_bit_offset = segment_sampling_context.animated_track_data_bit_offset;

			// Unpack sample
			rtm::vector4f rotation_as_vec;
			if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
			{
				// Fall-through intentional
				uint32_t skip_size = 0;
				switch (unpack_index)
				{
				default:
				case 3:
				{
					// TODO: Can we do an alternate more efficient implementation? We want to increment by one if num bits == 31
					const uint32_t num_bits_at_bit_rate = format_per_track_data[2];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 2:
				{
					const uint32_t num_bits_at_bit_rate = format_per_track_data[1];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 1:
				{
					const uint32_t num_bits_at_bit_rate = format_per_track_data[0];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 0:
					// Nothing to skip
					(void)skip_size;
				}

				// Skip prior samples
				animated_track_data_bit_offset += skip_size * 3;

				const uint32_t num_bits_at_bit_rate = format_per_track_data[unpack_index];

				if (num_bits_at_bit_rate == 0)	// Constant bit rate
				{
					// Segment range is packed: min.xxxx, min.yyyy, min.zzzz, extent.xxxx, extent.yyyy, extent.zzzz
					// Our constant sample value is packed 8 bits in each group in the sample's lane
					// To load our sample, we need to load: (min.x[unpack_index] << 8) | min.y[unpack_index], (min.z[unpack_index] << 8) | extent.x[unpack_index], (extent.y[unpack_index] << 8) | extent.z[unpack_index]
					// This is more complicated than if we were in AOS form but constant bit rates are somewhat rare while nearly every sample
					// has segment range information which is a lot simpler to load in SOA form
					const uint8_t* shifted_segment_range_data = segment_range_data + unpack_index;
					const uint32_t x = (uint32_t(shifted_segment_range_data[0]) << 8) | shifted_segment_range_data[4];
					const uint32_t y = (uint32_t(shifted_segment_range_data[8]) << 8) | shifted_segment_range_data[12];
					const uint32_t z = (uint32_t(shifted_segment_range_data[16]) << 8) | shifted_segment_range_data[20];

#if defined(RTM_SSE2_INTRINSICS)
					// TODO: Use SIMD for this

					// Load min.xxxx, min.yyyy, 8 bytes, offset by our sample index such that the first byte is our sample
					// Unpack low and interleave xxxx, yyyy, we end up with sample.x in our first lane as uint16_t
					// Unpack low to convert to uint32_t, sample.x lives in lane 0, repeat for sample.yz
					// Total of 2x loads (re-use first load and interleave high for sample.y), 5x unpack
					// Merge sample.xy together (1x shuffle)
					// Merge sample.xyz together (1x shuffle)
					// Convert to floats and normalize
					__m128i xyz = _mm_setr_epi32(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z), 0);
					__m128 xyzf = _mm_cvtepi32_ps(xyz);
					rotation_as_vec = _mm_mul_ps(xyzf, _mm_set_ps1(1.0F / 65535.0F));
#elif defined(RTM_NEON_INTRINSICS)
					uint32x4_t xyz = vcombine_u32(vcreate_u32((uint64_t(y) << 32) | x), vcreate_u32(z));
					float32x4_t xyzf = vcvtq_f32_u32(xyz);
					rotation_as_vec = vmulq_n_f32(xyzf, 1.0F / 65535.0F);
#else
					const rtm::vector4f xyz = rtm::vector_set(float(x), float(y), float(z), 0.0F);
					rotation_as_vec = rtm::vector_mul(xyz, 1.0F / 65535.0F);
#endif

					segment_range_ignore_mask = 0xFF;	// Ignore segment range
					clip_range_ignore_mask = 0x00;
				}
				else if (num_bits_at_bit_rate == num_raw_bit_rate_bits)	// Raw bit rate
				{
					rotation_as_vec = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
					segment_range_ignore_mask = 0xFF;	// Ignore segment range
					clip_range_ignore_mask = 0xFF;		// Ignore clip range
				}
				else
				{
					rotation_as_vec = unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, animated_track_data, animated_track_data_bit_offset);
					segment_range_ignore_mask = 0x00;
					clip_range_ignore_mask = 0x00;
				}
			}
			else
			{
				if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
				{
					animated_track_data_bit_offset += unpack_index * 128;
					rotation_as_vec = unpack_vector4_128_unsafe(animated_track_data, animated_track_data_bit_offset);
				}
				else // rotation_format8::quatf_drop_w_full
				{
					animated_track_data_bit_offset += unpack_index * 96;
					rotation_as_vec = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
				}
			}

			// Remap within our ranges
			if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
			{
				if (decomp_context.has_segments && segment_range_ignore_mask == 0)
				{
					// Segment range is packed: min.xxxx, min.yyyy, min.zzzz, extent.xxxx, extent.yyyy, extent.zzzz
					segment_range_data += unpack_index;	// Offset to our sample

					const uint32_t min_x = segment_range_data[0];
					const uint32_t min_y = segment_range_data[4];
					const uint32_t min_z = segment_range_data[8];

					const uint32_t extent_x = segment_range_data[12];
					const uint32_t extent_y = segment_range_data[16];
					const uint32_t extent_z = segment_range_data[20];

#if defined(RTM_SSE2_INTRINSICS)
					__m128i min_u32 = _mm_setr_epi32(static_cast<int32_t>(min_x), static_cast<int32_t>(min_y), static_cast<int32_t>(min_z), 0);
					__m128i extent_u32 = _mm_setr_epi32(static_cast<int32_t>(extent_x), static_cast<int32_t>(extent_y), static_cast<int32_t>(extent_z), 0);

					rtm::vector4f segment_range_min = _mm_cvtepi32_ps(min_u32);
					rtm::vector4f segment_range_extent = _mm_cvtepi32_ps(extent_u32);
#elif defined(RTM_NEON_INTRINSICS)
					uint32x4_t min_u32 = vcombine_u32(vcreate_u32((uint64_t(min_y) << 32) | min_x), vcreate_u32(min_z));
					uint32x4_t extent_u32 = vcombine_u32(vcreate_u32((uint64_t(extent_y) << 32) | extent_x), vcreate_u32(extent_z));

					rtm::vector4f segment_range_min = vcvtq_f32_u32(min_u32);
					rtm::vector4f segment_range_extent = vcvtq_f32_u32(extent_u32);
#else
					rtm::vector4f segment_range_min = rtm::vector_set(float(min_x), float(min_y), float(min_z), 0.0F);
					rtm::vector4f segment_range_extent = rtm::vector_set(float(extent_x), float(extent_y), float(extent_z), 0.0F);
#endif

					const float normalization_scale = 1.0F / 255.0F;
					segment_range_min = rtm::vector_mul(segment_range_min, normalization_scale);
					segment_range_extent = rtm::vector_mul(segment_range_extent, normalization_scale);

					rotation_as_vec = rtm::vector_mul_add(rotation_as_vec, segment_range_extent, segment_range_min);
				}

				if (clip_range_ignore_mask == 0)
				{
					const float* clip_range_data = bit_cast<const float*>(clip_sampling_context.clip_range_data) + unpack_index;	// Offset to our sample

					const float min_x = clip_range_data[group_size * 0];
					const float min_y = clip_range_data[group_size * 1];
					const float min_z = clip_range_data[group_size * 2];
					const rtm::vector4f clip_range_min = rtm::vector_set(min_x, min_y, min_z, 0.0F);

					const float extent_x = clip_range_data[group_size * 3];
					const float extent_y = clip_range_data[group_size * 4];
					const float extent_z = clip_range_data[group_size * 5];
					const rtm::vector4f clip_range_extent = rtm::vector_set(extent_x, extent_y, extent_z, 0.0F);

					rotation_as_vec = rtm::vector_mul_add(rotation_as_vec, clip_range_extent, clip_range_min);
				}
			}

			return rotation_as_vec;
		}

		template<class decompression_settings_adapter_type>
		inline RTM_DISABLE_SECURITY_COOKIE_CHECK void unpack_animated_vector3(const persistent_transform_decompression_context_v0& decomp_context, rtm::vector4f output_scratch[4],
			uint32_t num_to_unpack,
			const clip_animated_sampling_context_v0& clip_sampling_context, segment_animated_sampling_context_v0& segment_sampling_context)
		{
			const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
			const compressed_tracks_version16 version = get_version<decompression_settings_adapter_type>(decomp_context.get_version());

			// See write_format_per_track_data(..) for details
			const uint32_t num_raw_bit_rate_bits = version >= compressed_tracks_version16::v02_01_99_1 ? 31U : 32U;

			const uint8_t* format_per_track_data = segment_sampling_context.format_per_track_data;
			const uint8_t* segment_range_data = segment_sampling_context.segment_range_data;
			const uint8_t* animated_track_data = segment_sampling_context.animated_track_data;
			uint32_t animated_track_data_bit_offset = segment_sampling_context.animated_track_data_bit_offset;

			const uint8_t* clip_range_data = clip_sampling_context.clip_range_data;

			for (uint32_t unpack_index = 0; unpack_index < num_to_unpack; ++unpack_index)
			{
				// Range ignore flags are used to skip range normalization at the clip and/or segment levels
				// Each sample has two bits like so:
				//    - 0x01 = ignore segment level
				//    - 0x02 = ignore clip level
				uint32_t range_ignore_flags;

				rtm::vector4f sample;
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					const uint32_t num_bits_at_bit_rate = *format_per_track_data;
					format_per_track_data++;

					if (num_bits_at_bit_rate == 0)	// Constant bit rate
					{
						sample = unpack_vector3_u48_unsafe(segment_range_data);
						segment_range_data += sizeof(uint16_t) * 3;
						range_ignore_flags = 0x01;	// Skip segment only
					}
					else if (num_bits_at_bit_rate == num_raw_bit_rate_bits)	// Raw bit rate
					{
						sample = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += 96;
						segment_range_data += sizeof(uint16_t) * 3;	// Raw bit rates have unused range data, skip it
						range_ignore_flags = 0x03;	// Skip clip and segment
					}
					else
					{
						sample = unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, animated_track_data, animated_track_data_bit_offset);
						animated_track_data_bit_offset += num_bits_at_bit_rate * 3;
						range_ignore_flags = 0x00;	// Don't skip range reduction
					}
				}
				else // vector_format8::vector3f_full
				{
					sample = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
					animated_track_data_bit_offset += 96;
					range_ignore_flags = 0x03;	// Skip clip and segment
				}

				// Remap within our ranges
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					if (decomp_context.has_segments && (range_ignore_flags & 0x01) == 0)
					{
						// Apply segment range remapping
						const uint32_t range_entry_size = 3 * sizeof(uint8_t);
						const uint8_t* segment_range_min_ptr = segment_range_data;
						const uint8_t* segment_range_extent_ptr = segment_range_min_ptr + range_entry_size;
						segment_range_data = segment_range_extent_ptr + range_entry_size;

						const rtm::vector4f segment_range_min = unpack_vector3_u24_unsafe(segment_range_min_ptr);
						const rtm::vector4f segment_range_extent = unpack_vector3_u24_unsafe(segment_range_extent_ptr);

						sample = rtm::vector_mul_add(sample, segment_range_extent, segment_range_min);
					}

					if ((range_ignore_flags & 0x02) == 0)
					{
						// Apply clip range remapping
						const uint32_t range_entry_size = 3 * sizeof(float);
						const uint32_t sub_track_offset = range_entry_size * 2 * unpack_index;
						const uint8_t* clip_range_min_ptr = clip_range_data + sub_track_offset;
						const uint8_t* clip_range_extent_ptr = clip_range_min_ptr + range_entry_size;

						const rtm::vector4f clip_range_min = rtm::vector_load(clip_range_min_ptr);
						const rtm::vector4f clip_range_extent = rtm::vector_load(clip_range_extent_ptr);

						sample = rtm::vector_mul_add(sample, clip_range_extent, clip_range_min);
					}
				}

				ACL_ASSERT(rtm::vector_is_finite3(sample), "Vector3 is not valid!");

				// TODO: Fill in W component with something sensible?

				// Cache
				output_scratch[unpack_index] = sample;
			}

			// Update our pointers
			segment_sampling_context.format_per_track_data = format_per_track_data;
			segment_sampling_context.segment_range_data = segment_range_data;
			segment_sampling_context.animated_track_data_bit_offset = animated_track_data_bit_offset;

			// Prefetch the next cache line even if we don't have any data left
			// By the time we unpack again, it will have arrived in the CPU cache
			// If our format is full precision, we have at most 4 samples per cache line
			// If our format is drop W, we have at most 5.33 samples per cache line

			// If our pointer was already aligned to a cache line before we unpacked our 4 values,
			// it now points to the first byte of the next cache line. Any offset between 0-63 will fetch it.
			// If our pointer had some offset into a cache line, we might have spanned 2 cache lines.
			// If this happens, we probably already read some data from the next cache line in which
			// case we don't need to prefetch it and we can go to the next one. Any offset after the end
			// of this cache line will fetch it. For safety, we prefetch 63 bytes ahead.
			// Prefetch 4 samples ahead in all levels of the CPU cache
			ACL_IMPL_ANIMATED_PREFETCH(format_per_track_data + 60);
			ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_track_data_bit_offset / 8) + 63);
			ACL_IMPL_ANIMATED_PREFETCH(segment_range_data + 48);
		}

		template<class decompression_settings_adapter_type>
		inline RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_single_animated_vector3(const persistent_transform_decompression_context_v0& decomp_context,
			uint32_t unpack_index,
			const clip_animated_sampling_context_v0& clip_sampling_context, const segment_animated_sampling_context_v0& segment_sampling_context)
		{
			const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
			const compressed_tracks_version16 version = get_version<decompression_settings_adapter_type>(decomp_context.get_version());

			// See write_format_per_track_data(..) for details
			const uint32_t num_raw_bit_rate_bits = version >= compressed_tracks_version16::v02_01_99_1 ? 31U : 32U;

			const uint8_t* format_per_track_data = segment_sampling_context.format_per_track_data;
			const uint8_t* segment_range_data = segment_sampling_context.segment_range_data;
			const uint8_t* animated_track_data = segment_sampling_context.animated_track_data;
			uint32_t animated_track_data_bit_offset = segment_sampling_context.animated_track_data_bit_offset;

			const uint8_t* clip_range_data = clip_sampling_context.clip_range_data;

			// Range ignore flags are used to skip range normalization at the clip and/or segment levels
			// Each sample has two bits like so:
			//    - 0x01 = ignore segment level
			//    - 0x02 = ignore clip level
			uint32_t range_ignore_flags;

			rtm::vector4f sample;
			if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
			{
				// Fall-through intentional
				uint32_t skip_size = 0;
				switch (unpack_index)
				{
				default:
				case 3:
				{
					// TODO: Can we do an alternate more efficient implementation? We want to increment by one if num bits == 31
					const uint32_t num_bits_at_bit_rate = format_per_track_data[2];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 2:
				{
					const uint32_t num_bits_at_bit_rate = format_per_track_data[1];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 1:
				{
					const uint32_t num_bits_at_bit_rate = format_per_track_data[0];
					skip_size += (num_bits_at_bit_rate == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate;
				}
					ACL_SWITCH_CASE_FALLTHROUGH_INTENTIONAL;
				case 0:
					// Nothing to skip
					(void)skip_size;
				}

				// Skip prior samples
				animated_track_data_bit_offset += skip_size * 3;
				segment_range_data += sizeof(uint8_t) * 6 * unpack_index;
				clip_range_data += sizeof(rtm::float3f) * 2 * unpack_index;

				const uint32_t num_bits_at_bit_rate = format_per_track_data[unpack_index];

				if (num_bits_at_bit_rate == 0)	// Constant bit rate
				{
					sample = unpack_vector3_u48_unsafe(segment_range_data);
					range_ignore_flags = 0x01;	// Skip segment only
				}
				else if (num_bits_at_bit_rate == num_raw_bit_rate_bits)	// Raw bit rate
				{
					sample = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
					range_ignore_flags = 0x03;	// Skip clip and segment
				}
				else
				{
					sample = unpack_vector3_uXX_unsafe(num_bits_at_bit_rate, animated_track_data, animated_track_data_bit_offset);
					range_ignore_flags = 0x00;	// Don't skip range reduction
				}
			}
			else // vector_format8::vector3f_full
			{
				animated_track_data_bit_offset += unpack_index * 96;
				sample = unpack_vector3_96_unsafe(animated_track_data, animated_track_data_bit_offset);
				range_ignore_flags = 0x03;	// Skip clip and segment
			}

			// Remap within our ranges
			if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
			{
				if (decomp_context.has_segments && (range_ignore_flags & 0x01) == 0)
				{
					// Apply segment range remapping
					const rtm::vector4f segment_range_min = unpack_vector3_u24_unsafe(segment_range_data);
					const rtm::vector4f segment_range_extent = unpack_vector3_u24_unsafe(segment_range_data + 3 * sizeof(uint8_t));

					sample = rtm::vector_mul_add(sample, segment_range_extent, segment_range_min);
				}

				if ((range_ignore_flags & 0x02) == 0)
				{
					// Apply clip range remapping
					const rtm::vector4f clip_range_min = rtm::vector_load(clip_range_data);
					const rtm::vector4f clip_range_extent = rtm::vector_load(clip_range_data + sizeof(rtm::float3f));

					sample = rtm::vector_mul_add(sample, clip_range_extent, clip_range_min);
				}
			}

			ACL_ASSERT(rtm::vector_is_finite3(sample), "Vector3 is not valid!");
			return sample;
		}

		// Force inline this function, we only use it to keep the code readable
		template<class decompression_settings_adapter_type>
		RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK void count_animated_group_bit_size(
			const persistent_transform_decompression_context_v0& decomp_context,
			const uint8_t* format_per_track_data0, const uint8_t* format_per_track_data1, uint32_t num_groups_to_skip,
			uint32_t& out_group_bit_size_per_component0, uint32_t& out_group_bit_size_per_component1)
		{
			const compressed_tracks_version16 version = get_version<decompression_settings_adapter_type>(decomp_context.get_version());

			// See write_format_per_track_data(..) for details
			const uint32_t num_raw_bit_rate_bits = version >= compressed_tracks_version16::v02_01_99_1 ? 31U : 32U;

			// TODO: Do the same with NEON
#if defined(RTM_SSE3_INTRINSICS)
			const __m128i zero = _mm_setzero_si128();
			const __m128i num_raw_bit_rate_bits_v = _mm_set1_epi32(static_cast<int32_t>(num_raw_bit_rate_bits));
			__m128i group_bit_size_per_component0_v = zero;
			__m128i group_bit_size_per_component1_v = zero;

			// We add 4 at a time in SIMD
			for (uint32_t group_index = 0; group_index < num_groups_to_skip; ++group_index)
			{
				const uint32_t group_offset = group_index * 4;
				const __m128i group_bit_size_per_component0_u8 = _mm_loadu_si128(bit_cast<const __m128i*>(format_per_track_data0 + group_offset));
				const __m128i group_bit_size_per_component1_u8 = _mm_loadu_si128(bit_cast<const __m128i*>(format_per_track_data1 + group_offset));

				// Unpack from uint8_t to uint32_t
				__m128i group_bit_size_per_component0_u32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(group_bit_size_per_component0_u8, zero), zero);
				__m128i group_bit_size_per_component1_u32 = _mm_unpacklo_epi16(_mm_unpacklo_epi8(group_bit_size_per_component1_u8, zero), zero);

				if (version >= compressed_tracks_version16::v02_01_99_1)
				{
					// If the number of bits is 31, we are the raw bit rate and we need to add 1 (we'll add 31 below, and 1 more for a total of 32 bits)
					// If the number of bits is 31, our mask's value will be 0xFFFFFFFF which is -1, otherwise it is 0x00000000
					const __m128i is_raw_num_bits0 = _mm_cmpeq_epi32(group_bit_size_per_component0_u32, num_raw_bit_rate_bits_v);
					const __m128i is_raw_num_bits1 = _mm_cmpeq_epi32(group_bit_size_per_component1_u32, num_raw_bit_rate_bits_v);

					// We subtract the mask value directly, it is either -1 or 0
					group_bit_size_per_component0_u32 = _mm_sub_epi32(group_bit_size_per_component0_u32, is_raw_num_bits0);
					group_bit_size_per_component1_u32 = _mm_sub_epi32(group_bit_size_per_component1_u32, is_raw_num_bits1);
				}
				else
					(void)num_raw_bit_rate_bits_v;

				// Add how many bits per component we have
				group_bit_size_per_component0_v = _mm_add_epi32(group_bit_size_per_component0_v, group_bit_size_per_component0_u32);
				group_bit_size_per_component1_v = _mm_add_epi32(group_bit_size_per_component1_v, group_bit_size_per_component1_u32);
			}

			// Now we sum horizontally
			group_bit_size_per_component0_v = _mm_hadd_epi32(_mm_hadd_epi32(group_bit_size_per_component0_v, group_bit_size_per_component0_v), group_bit_size_per_component0_v);
			group_bit_size_per_component1_v = _mm_hadd_epi32(_mm_hadd_epi32(group_bit_size_per_component1_v, group_bit_size_per_component1_v), group_bit_size_per_component1_v);

			out_group_bit_size_per_component0 = static_cast<uint32_t>(_mm_cvtsi128_si32(group_bit_size_per_component0_v));
			out_group_bit_size_per_component1 = static_cast<uint32_t>(_mm_cvtsi128_si32(group_bit_size_per_component1_v));
#else
			uint32_t group_bit_size_per_component0 = 0;
			uint32_t group_bit_size_per_component1 = 0;

			for (uint32_t group_index = 0; group_index < num_groups_to_skip; ++group_index)
			{
				// TODO: Can we do an alternate more efficient implementation? We want to increment by one if num bits == 31

				const uint32_t num_bits_at_bit_rate_0_0 = format_per_track_data0[(group_index * 4) + 0];
				const uint32_t num_bits_at_bit_rate_1_0 = format_per_track_data1[(group_index * 4) + 0];
				const uint32_t num_bits_at_bit_rate_0_1 = format_per_track_data0[(group_index * 4) + 1];
				const uint32_t num_bits_at_bit_rate_1_1 = format_per_track_data1[(group_index * 4) + 1];
				const uint32_t num_bits_at_bit_rate_0_2 = format_per_track_data0[(group_index * 4) + 2];
				const uint32_t num_bits_at_bit_rate_1_2 = format_per_track_data1[(group_index * 4) + 2];
				const uint32_t num_bits_at_bit_rate_0_3 = format_per_track_data0[(group_index * 4) + 3];
				const uint32_t num_bits_at_bit_rate_1_3 = format_per_track_data1[(group_index * 4) + 3];

				group_bit_size_per_component0 += (num_bits_at_bit_rate_0_0 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_0_0;
				group_bit_size_per_component1 += (num_bits_at_bit_rate_1_0 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_1_0;

				group_bit_size_per_component0 += (num_bits_at_bit_rate_0_1 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_0_1;
				group_bit_size_per_component1 += (num_bits_at_bit_rate_1_1 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_1_1;

				group_bit_size_per_component0 += (num_bits_at_bit_rate_0_2 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_0_2;
				group_bit_size_per_component1 += (num_bits_at_bit_rate_1_2 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_1_2;

				group_bit_size_per_component0 += (num_bits_at_bit_rate_0_3 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_0_3;
				group_bit_size_per_component1 += (num_bits_at_bit_rate_1_3 == num_raw_bit_rate_bits) ? 32 : num_bits_at_bit_rate_1_3;
			}

			out_group_bit_size_per_component0 = group_bit_size_per_component0;
			out_group_bit_size_per_component1 = group_bit_size_per_component1;
#endif
		}

		// Performance notes:
		//    - Using SOA after unpacking vec3 appears to be slightly slower. Full groups aren't super common
		//      because animated translation/scale isn't common. But even with clips with lots of full groups,
		//      SOA remains slightly slower. It seems the longer dependency chains offsets the gain from
		//      using all SIMD lanes.
		//    - Removing the unpack prefetches seems to harm performance, especially on mobile
		//      I also tried reworking them to be more optimal for single segment usage but while I could get
		//      a small win on desktop, mobile remained under performing.

		struct animated_track_cache_v0
		{
			static constexpr uint32_t k_num_rounding_modes = 4;	// none, floor, ceil, nearest

			track_cache_quatf_v0<8, k_num_rounding_modes>		rotations;
			track_cache_vector4f_v0<8, k_num_rounding_modes>	translations;
			track_cache_vector4f_v0<8, k_num_rounding_modes>	scales;

			// Scratch space when we decompress our samples before we interpolate
			rtm::vector4f scratch0[4];
			rtm::vector4f scratch1[4];

			clip_animated_sampling_context_v0 clip_sampling_context_rotations;
			clip_animated_sampling_context_v0 clip_sampling_context_translations;
			clip_animated_sampling_context_v0 clip_sampling_context_scales;

			segment_animated_sampling_context_v0 segment_sampling_context_rotations[2];
			segment_animated_sampling_context_v0 segment_sampling_context_translations[2];
			segment_animated_sampling_context_v0 segment_sampling_context_scales[2];

			template<class decompression_settings_type, class decompression_settings_translation_adapter_type>
			void RTM_DISABLE_SECURITY_COOKIE_CHECK initialize(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const compressed_tracks* tracks = decomp_context.tracks;
				const transform_tracks_header& transform_header = get_transform_tracks_header(*tracks);

				const segment_header* segment0 = decomp_context.segment_offsets[0].add_to(tracks);
				const segment_header* segment1 = decomp_context.segment_offsets[1].add_to(tracks);

				const uint8_t* animated_track_data0 = decomp_context.animated_track_data[0];
				const uint8_t* animated_track_data1 = decomp_context.animated_track_data[1];

				const uint8_t* clip_range_data_rotations = transform_header.get_clip_range_data();
				clip_sampling_context_rotations.clip_range_data = clip_range_data_rotations;

				const uint8_t* format_per_track_data_rotations0 = decomp_context.format_per_track_data[0];
				const uint8_t* segment_range_data_rotations0 = decomp_context.segment_range_data[0];
				const uint32_t animated_track_data_bit_offset_rotations0 = decomp_context.key_frame_bit_offsets[0];
				segment_sampling_context_rotations[0].format_per_track_data = format_per_track_data_rotations0;
				segment_sampling_context_rotations[0].segment_range_data = segment_range_data_rotations0;
				segment_sampling_context_rotations[0].animated_track_data = animated_track_data0;
				segment_sampling_context_rotations[0].animated_track_data_bit_offset = animated_track_data_bit_offset_rotations0;

				const uint8_t* format_per_track_data_rotations1 = decomp_context.format_per_track_data[1];
				const uint8_t* segment_range_data_rotations1 = decomp_context.segment_range_data[1];
				const uint32_t animated_track_data_bit_offset_rotations1 = decomp_context.key_frame_bit_offsets[1];
				segment_sampling_context_rotations[1].format_per_track_data = format_per_track_data_rotations1;
				segment_sampling_context_rotations[1].segment_range_data = segment_range_data_rotations1;
				segment_sampling_context_rotations[1].animated_track_data = animated_track_data1;
				segment_sampling_context_rotations[1].animated_track_data_bit_offset = animated_track_data_bit_offset_rotations1;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				const bool are_rotations_variable = rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable);

				const uint32_t num_animated_rotation_sub_tracks_padded = align_to(transform_header.num_animated_rotation_sub_tracks, 4);

				// Rotation range data follows translations, no padding
				const uint32_t rotation_clip_range_data_size = are_rotations_variable ? (sizeof(rtm::float3f) * 2U) : 0U;
				const uint8_t* clip_range_data_translations = clip_range_data_rotations + (transform_header.num_animated_rotation_sub_tracks * rotation_clip_range_data_size);
				clip_sampling_context_translations.clip_range_data = clip_range_data_translations;

				// Rotation metadata is padded to 4 sub-tracks (1 byte each)
				const uint32_t rotation_per_track_metadata_size = are_rotations_variable ? 1U : 0U;
				const uint8_t* format_per_track_data_translations0 = format_per_track_data_rotations0 + (num_animated_rotation_sub_tracks_padded * rotation_per_track_metadata_size);
				const uint8_t* format_per_track_data_translations1 = format_per_track_data_rotations1 + (num_animated_rotation_sub_tracks_padded * rotation_per_track_metadata_size);
				segment_sampling_context_translations[0].format_per_track_data = format_per_track_data_translations0;
				segment_sampling_context_translations[1].format_per_track_data = format_per_track_data_translations1;

				// Rotation range data is padded to 4 sub-tracks (6 bytes each)
				const uint32_t rotation_segment_range_data_size = are_rotations_variable ? 6U : 0U;
				const uint8_t* segment_range_data_translations0 = segment_range_data_rotations0 + (num_animated_rotation_sub_tracks_padded * rotation_segment_range_data_size);
				const uint8_t* segment_range_data_translations1 = segment_range_data_rotations1 + (num_animated_rotation_sub_tracks_padded * rotation_segment_range_data_size);
				segment_sampling_context_translations[0].segment_range_data = segment_range_data_translations0;
				segment_sampling_context_translations[1].segment_range_data = segment_range_data_translations1;

				// Every sub-track uses the same base animated track data pointer
				segment_sampling_context_translations[0].animated_track_data = animated_track_data0;
				segment_sampling_context_translations[1].animated_track_data = animated_track_data1;

				const uint32_t animated_track_data_bit_offset_translations0 = animated_track_data_bit_offset_rotations0 + segment0->animated_rotation_bit_size;
				const uint32_t animated_track_data_bit_offset_translations1 = animated_track_data_bit_offset_rotations1 + segment1->animated_rotation_bit_size;
				segment_sampling_context_translations[0].animated_track_data_bit_offset = animated_track_data_bit_offset_translations0;
				segment_sampling_context_translations[1].animated_track_data_bit_offset = animated_track_data_bit_offset_translations1;

				if (decomp_context.has_scale)
				{
					const vector_format8 translation_format = get_vector_format<decompression_settings_translation_adapter_type>(decompression_settings_translation_adapter_type::get_vector_format(decomp_context));
					const bool are_translations_variable = translation_format == vector_format8::vector3f_variable && decompression_settings_translation_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable);

					// Scale data just follows the translation data without any extra padding
					const uint32_t translation_clip_range_data_size = are_translations_variable ? (sizeof(rtm::float3f) * 2U) : 0U;
					clip_sampling_context_scales.clip_range_data = clip_range_data_translations + (transform_header.num_animated_translation_sub_tracks * translation_clip_range_data_size);

					const uint32_t translation_per_track_metadata_size = are_translations_variable ? 1U : 0U;
					segment_sampling_context_scales[0].format_per_track_data = format_per_track_data_translations0 + (transform_header.num_animated_translation_sub_tracks * translation_per_track_metadata_size);
					segment_sampling_context_scales[1].format_per_track_data = format_per_track_data_translations1 + (transform_header.num_animated_translation_sub_tracks * translation_per_track_metadata_size);

					const uint32_t translation_segment_range_data_size = are_translations_variable ? 6U : 0U;
					segment_sampling_context_scales[0].segment_range_data = segment_range_data_translations0 + (transform_header.num_animated_translation_sub_tracks * translation_segment_range_data_size);
					segment_sampling_context_scales[1].segment_range_data = segment_range_data_translations1 + (transform_header.num_animated_translation_sub_tracks * translation_segment_range_data_size);

					segment_sampling_context_scales[0].animated_track_data = animated_track_data0;
					segment_sampling_context_scales[1].animated_track_data = animated_track_data1;

					segment_sampling_context_scales[0].animated_track_data_bit_offset = animated_track_data_bit_offset_translations0 + segment0->animated_translation_bit_size;
					segment_sampling_context_scales[1].animated_track_data_bit_offset = animated_track_data_bit_offset_translations1 + segment1->animated_translation_bit_size;
				}

				rotations.num_left_to_unpack = transform_header.num_animated_rotation_sub_tracks;
				translations.num_left_to_unpack = transform_header.num_animated_translation_sub_tracks;
				scales.num_left_to_unpack = transform_header.num_animated_scale_sub_tracks;
			}

			template<class decompression_settings_type>
			void RTM_DISABLE_SECURITY_COOKIE_CHECK unpack_rotation_group(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const uint32_t num_left_to_unpack = rotations.num_left_to_unpack;
				if (num_left_to_unpack == 0)
					return;	// Nothing left to do, we are done

				// If we have less than 4 cached samples, unpack 4 more and prefetch the next cache line
				const uint32_t num_cached = rotations.get_num_cached();
				if (num_cached >= 4)
					return;	// Enough cached, nothing to do

				const uint32_t num_to_unpack = std::min<uint32_t>(num_left_to_unpack, 4);
				rotations.num_left_to_unpack = num_left_to_unpack - num_to_unpack;

				// Write index will be either 0 or 4 here since we always unpack 4 at a time
				const uint32_t cache_write_index = rotations.cache_write_index % 8;
				rotations.cache_write_index += num_to_unpack;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				const float interpolation_alpha = decomp_context.interpolation_alpha;
				const bool should_interpolate = should_interpolate_samples<decompression_settings_type>(rotation_format, interpolation_alpha);

				segment_animated_scratch_v0 segment_scratch;

				// We start by unpacking our segment range data into our scratch memory
				// We often only use a single segment to interpolate, we can avoid redundant work
				if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
				{
					if (decomp_context.has_segments)
					{
						unpack_segment_range_data(segment_sampling_context_rotations[0].segment_range_data, 0, segment_scratch);

						// We are interpolating between two segments (rare)
						if (!decomp_context.uses_single_segment)
							unpack_segment_range_data(segment_sampling_context_rotations[1].segment_range_data, 1, segment_scratch);

#if !defined(ACL_IMPL_PREFETCH_EARLY)
						// Our segment range data takes 24 bytes per group (4 samples, 6 bytes each), each cache line fits 2.67 groups
						// Prefetch every time while alternating between both segments
						ACL_IMPL_ANIMATED_PREFETCH(segment_sampling_context_rotations[cache_write_index % 2].segment_range_data + 64);
#endif
					}
				}

				const range_reduction_masks_t range_reduction_masks0 = unpack_animated_quat<decompression_settings_type>(decomp_context, scratch0, num_to_unpack, segment_sampling_context_rotations[0]);
				const range_reduction_masks_t range_reduction_masks1 = unpack_animated_quat<decompression_settings_type>(decomp_context, scratch1, num_to_unpack, segment_sampling_context_rotations[1]);

				// Swizzle our samples into SOA form
				rtm::vector4f scratch0_xxxx;
				rtm::vector4f scratch0_yyyy;
				rtm::vector4f scratch0_zzzz;
				rtm::vector4f scratch0_wwww;
				RTM_MATRIXF_TRANSPOSE_4X4(scratch0[0], scratch0[1], scratch0[2], scratch0[3], scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch0_wwww);

				rtm::vector4f scratch1_xxxx;
				rtm::vector4f scratch1_yyyy;
				rtm::vector4f scratch1_zzzz;
				rtm::vector4f scratch1_wwww;
				RTM_MATRIXF_TRANSPOSE_4X4(scratch1[0], scratch1[1], scratch1[2], scratch1[3], scratch1_xxxx, scratch1_yyyy, scratch1_zzzz, scratch1_wwww);

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
				__m256 scratch_xxxx0_xxxx1 = _mm256_set_m128(scratch1_xxxx, scratch0_xxxx);
				__m256 scratch_yyyy0_yyyy1 = _mm256_set_m128(scratch1_yyyy, scratch0_yyyy);
				__m256 scratch_zzzz0_zzzz1 = _mm256_set_m128(scratch1_zzzz, scratch0_zzzz);
#endif

				// If we have a variable bit rate, we perform range reduction, skip the data we used
				if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
				{
					if (decomp_context.has_segments)
					{
#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
						remap_segment_range_data_avx8(segment_scratch, range_reduction_masks0, range_reduction_masks1, scratch_xxxx0_xxxx1, scratch_yyyy0_yyyy1, scratch_zzzz0_zzzz1);
#else
						remap_segment_range_data4(segment_scratch, 0, range_reduction_masks0, scratch0_xxxx, scratch0_yyyy, scratch0_zzzz);
						remap_segment_range_data4(segment_scratch, uint32_t(!decomp_context.uses_single_segment), range_reduction_masks1, scratch1_xxxx, scratch1_yyyy, scratch1_zzzz);
#endif
					}

					const uint8_t* clip_range_data = clip_sampling_context_rotations.clip_range_data;

#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
					remap_clip_range_data_avx8(clip_range_data, num_to_unpack, range_reduction_masks0, range_reduction_masks1, scratch_xxxx0_xxxx1, scratch_yyyy0_yyyy1, scratch_zzzz0_zzzz1);
#else
					remap_clip_range_data4(clip_range_data, num_to_unpack, range_reduction_masks0, range_reduction_masks1, scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch1_xxxx, scratch1_yyyy, scratch1_zzzz);
#endif

					// Skip our data
					clip_range_data += num_to_unpack * sizeof(rtm::float3f) * 2;
					clip_sampling_context_rotations.clip_range_data = clip_range_data;

#if defined(ACL_IMPL_PREFETCH_EARLY)
					// Clip range data is 24 bytes per sub-track and as such we need to prefetch two cache lines ahead to process 4 sub-tracks
					ACL_IMPL_ANIMATED_PREFETCH(clip_range_data + 64);
					ACL_IMPL_ANIMATED_PREFETCH(clip_range_data + 128);
#endif
				}

				// Reconstruct our quaternion W component in SOA
				if (rotation_format != rotation_format8::quatf_full || !decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
				{
#if defined(ACL_IMPL_USE_AVX_8_WIDE_DECOMP)
					const __m256 scratch_wwww0_wwww1 = quat_from_positive_w_avx8(scratch_xxxx0_xxxx1, scratch_yyyy0_yyyy1, scratch_zzzz0_zzzz1);

					// This is the last AVX step, unpack everything
					scratch0_xxxx = _mm256_extractf128_ps(scratch_xxxx0_xxxx1, 0);
					scratch1_xxxx = _mm256_extractf128_ps(scratch_xxxx0_xxxx1, 1);
					scratch0_yyyy = _mm256_extractf128_ps(scratch_yyyy0_yyyy1, 0);
					scratch1_yyyy = _mm256_extractf128_ps(scratch_yyyy0_yyyy1, 1);
					scratch0_zzzz = _mm256_extractf128_ps(scratch_zzzz0_zzzz1, 0);
					scratch1_zzzz = _mm256_extractf128_ps(scratch_zzzz0_zzzz1, 1);
					scratch0_wwww = _mm256_extractf128_ps(scratch_wwww0_wwww1, 0);
					scratch1_wwww = _mm256_extractf128_ps(scratch_wwww0_wwww1, 1);
#else
					scratch0_wwww = quat_from_positive_w4(scratch0_xxxx, scratch0_yyyy, scratch0_zzzz);

#if !defined(ACL_IMPL_PREFETCH_EARLY)
					if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
					{
						// Our segment per track metadata takes 4 bytes per group (4 samples, 1 byte each), each cache line fits 16 groups
						// Prefetch every other 8th group
						// We prefetch here because we have a square-root in quat_from_positive_w4(..) that we'll wait after
						// This allows us to insert the prefetch basically for free in its shadow
						// Branching is faster than prefetching every time and alternating between the two
						if (cache_write_index == 0)
							ACL_IMPL_ANIMATED_PREFETCH(segment_sampling_context_rotations[0].format_per_track_data + 64);
						else if (cache_write_index == 4)
							ACL_IMPL_ANIMATED_PREFETCH(segment_sampling_context_rotations[1].format_per_track_data + 64);
					}
#endif

					scratch1_wwww = quat_from_positive_w4(scratch1_xxxx, scratch1_yyyy, scratch1_zzzz);

#if !defined(ACL_IMPL_PREFETCH_EARLY)
					if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
					{
						// Our clip range data is 24 bytes per sub-track and as such we need to prefetch two cache lines ahead to process 4 sub-tracks
						// Each group is 96 bytes (4 samples, 24 bytes each), each cache line fits 0.67 groups
						// We prefetch here because we have a square-root in quat_from_positive_w4(..) that we'll wait after
						// This allows us to insert the prefetch basically for free in its shadow
						ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_rotations.clip_range_data + 64);
						ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_rotations.clip_range_data + 128);
					}
#endif
#endif

					if (decompression_settings_type::get_rotation_normalization_policy() == rotation_normalization_policy_t::always)
					{
						// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
						// isn't very accurate on small inputs, we need to normalize
						// If we support per track rounding, we need to normalize as we might not interpolate
						// Otherwise, if we don't interpolate we also need to normalize
						if (decompression_settings_type::is_per_track_rounding_supported() || !should_interpolate)
						{
							quat_normalize4(scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch0_wwww);
							quat_normalize4(scratch1_xxxx, scratch1_yyyy, scratch1_zzzz, scratch1_wwww);
						}
					}
				}

				// Interpolate linearly and store our rotations in SOA
				{
					const rtm::vector4f interpolation_alpha_v = rtm::vector_set(interpolation_alpha);

					if (decompression_settings_type::is_per_track_rounding_supported())
					{
						// If we support per track rounding, we have to retain everything
						// Write both floor/ceil samples and interpolate as well
						// When we consume the sample, we'll pick the right one according to the rounding policy

						// We swizzle and store the floor/ceil first because we have sqrt instructions in flight
						// from above. Most of the shuffles below can execute without any dependency.
						// We should have enough registers to avoid spilling and there is enough work to perform
						// to avoid any dependencies and fully hide the sqrt costs. That being said, there is
						// quite a bit of work to do and we might still be CPU bound below.

						{
							// Swizzle out our 4 floor samples
							rtm::vector4f sample0;
							rtm::vector4f sample1;
							rtm::vector4f sample2;
							rtm::vector4f sample3;
							RTM_MATRIXF_TRANSPOSE_4X4(scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch0_wwww, sample0, sample1, sample2, sample3);

							rtm::quatf* cache_ptr = &rotations.cached_samples[static_cast<int>(sample_rounding_policy::floor)][cache_write_index];

							cache_ptr[0] = rtm::vector_to_quat(sample0);
							cache_ptr[1] = rtm::vector_to_quat(sample1);
							cache_ptr[2] = rtm::vector_to_quat(sample2);
							cache_ptr[3] = rtm::vector_to_quat(sample3);
						}

						{
							// Swizzle out our 4 floor ceil
							rtm::vector4f sample0;
							rtm::vector4f sample1;
							rtm::vector4f sample2;
							rtm::vector4f sample3;
							RTM_MATRIXF_TRANSPOSE_4X4(scratch1_xxxx, scratch1_yyyy, scratch1_zzzz, scratch1_wwww, sample0, sample1, sample2, sample3);

							rtm::quatf* cache_ptr = &rotations.cached_samples[static_cast<int>(sample_rounding_policy::ceil)][cache_write_index];

							cache_ptr[0] = rtm::vector_to_quat(sample0);
							cache_ptr[1] = rtm::vector_to_quat(sample1);
							cache_ptr[2] = rtm::vector_to_quat(sample2);
							cache_ptr[3] = rtm::vector_to_quat(sample3);
						}

						{
							// Find nearest and swizzle it out
							const rtm::mask4f use_sample0 = rtm::vector_less_than(interpolation_alpha_v, rtm::vector_set(0.5F));

							const rtm::vector4f nearest_xxxx = rtm::vector_select(use_sample0, scratch0_xxxx, scratch1_xxxx);
							const rtm::vector4f nearest_yyyy = rtm::vector_select(use_sample0, scratch0_yyyy, scratch1_yyyy);
							const rtm::vector4f nearest_zzzz = rtm::vector_select(use_sample0, scratch0_zzzz, scratch1_zzzz);
							const rtm::vector4f nearest_wwww = rtm::vector_select(use_sample0, scratch0_wwww, scratch1_wwww);

							rtm::vector4f sample0;
							rtm::vector4f sample1;
							rtm::vector4f sample2;
							rtm::vector4f sample3;
							RTM_MATRIXF_TRANSPOSE_4X4(nearest_xxxx, nearest_yyyy, nearest_zzzz, nearest_wwww, sample0, sample1, sample2, sample3);

							rtm::quatf* cache_ptr = &rotations.cached_samples[static_cast<int>(sample_rounding_policy::nearest)][cache_write_index];

							cache_ptr[0] = rtm::vector_to_quat(sample0);
							cache_ptr[1] = rtm::vector_to_quat(sample1);
							cache_ptr[2] = rtm::vector_to_quat(sample2);
							cache_ptr[3] = rtm::vector_to_quat(sample3);
						}

						{
							rtm::vector4f interp_xxxx;
							rtm::vector4f interp_yyyy;
							rtm::vector4f interp_zzzz;
							rtm::vector4f interp_wwww;

							// Interpolate our quaternions without normalizing just yet
							quat_lerp_no_normalization4(scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch0_wwww,
								scratch1_xxxx, scratch1_yyyy, scratch1_zzzz, scratch1_wwww,
								interpolation_alpha_v,
								interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww);

							// Due to the interpolation, the result might not be anywhere near normalized!
							// Make sure to normalize afterwards if we need to
							if (decompression_settings_type::get_rotation_normalization_policy() >= rotation_normalization_policy_t::lerp_only)
								quat_normalize4(interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww);

#if !defined(ACL_IMPL_PREFETCH_EARLY)
							{
								// Our animated variable bit packed data uses at most 32 bits per component
								// When we use raw data, that means each group uses 64 bytes (4 bytes per component, 4 components, 4 samples in group), we have 1 group per cache line
								// When we use variable data, the highest bit rate uses 32 bits per component and thus our upper bound is 48 bytes per group (4 bytes per component, 3 components, 4 samples in group), we have 1.33 group per cache line
								// In practice, the highest bit rate is rare and the second higher uses 19 bits per component which brings us to 28.5 bytes per group, leading to 2.24 group per cache line
								// We prefetch both key frames every time to help hide TLB miss latency in large clips
								// We prefetch here because we have a square-root and division in quat_normalize4(..) that we'll wait after
								// This allows us to insert the prefetch basically for free in their shadow
								const uint8_t* animated_track_data = segment_sampling_context_rotations[0].animated_track_data + 64;	// One cache line ahead
								const uint32_t animated_bit_offset0 = segment_sampling_context_rotations[0].animated_track_data_bit_offset;
								const uint32_t animated_bit_offset1 = segment_sampling_context_rotations[1].animated_track_data_bit_offset;
								ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_bit_offset0 / 8));
								ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_bit_offset1 / 8));
							}
#endif

							// Swizzle out our 4 samples
							rtm::vector4f sample0;
							rtm::vector4f sample1;
							rtm::vector4f sample2;
							rtm::vector4f sample3;
							RTM_MATRIXF_TRANSPOSE_4X4(interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww, sample0, sample1, sample2, sample3);

							rtm::quatf* cache_ptr = &rotations.cached_samples[static_cast<int>(sample_rounding_policy::none)][cache_write_index];

							cache_ptr[0] = rtm::vector_to_quat(sample0);
							cache_ptr[1] = rtm::vector_to_quat(sample1);
							cache_ptr[2] = rtm::vector_to_quat(sample2);
							cache_ptr[3] = rtm::vector_to_quat(sample3);
						}
					}
					else
					{
						rtm::vector4f interp_xxxx;
						rtm::vector4f interp_yyyy;
						rtm::vector4f interp_zzzz;
						rtm::vector4f interp_wwww;

						if (should_interpolate)
						{
							// Interpolate our quaternions without normalizing just yet
							quat_lerp_no_normalization4(scratch0_xxxx, scratch0_yyyy, scratch0_zzzz, scratch0_wwww,
								scratch1_xxxx, scratch1_yyyy, scratch1_zzzz, scratch1_wwww,
								interpolation_alpha_v,
								interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww);

							// Due to the interpolation, the result might not be anywhere near normalized!
							// Make sure to normalize afterwards if we need to
							if (decompression_settings_type::get_rotation_normalization_policy() >= rotation_normalization_policy_t::lerp_only)
								quat_normalize4(interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww);
						}
						else
						{
							// If we don't interpolate, just pick the sample we need, it is already normalized after reconstructing
							// the W component or it was raw to begin with
							const rtm::mask4f use_sample0 = rtm::vector_less_equal(interpolation_alpha_v, rtm::vector_zero());

							interp_xxxx = rtm::vector_select(use_sample0, scratch0_xxxx, scratch1_xxxx);
							interp_yyyy = rtm::vector_select(use_sample0, scratch0_yyyy, scratch1_yyyy);
							interp_zzzz = rtm::vector_select(use_sample0, scratch0_zzzz, scratch1_zzzz);
							interp_wwww = rtm::vector_select(use_sample0, scratch0_wwww, scratch1_wwww);
						}

#if !defined(ACL_IMPL_PREFETCH_EARLY)
						{
							// Our animated variable bit packed data uses at most 32 bits per component
							// When we use raw data, that means each group uses 64 bytes (4 bytes per component, 4 components, 4 samples in group), we have 1 group per cache line
							// When we use variable data, the highest bit rate uses 32 bits per component and thus our upper bound is 48 bytes per group (4 bytes per component, 3 components, 4 samples in group), we have 1.33 group per cache line
							// In practice, the highest bit rate is rare and the second higher uses 19 bits per component which brings us to 28.5 bytes per group, leading to 2.24 group per cache line
							// We prefetch both key frames every time to help hide TLB miss latency in large clips
							// We prefetch here because we have a square-root and division in quat_normalize4(..) that we'll wait after
							// This allows us to insert the prefetch basically for free in their shadow
							const uint8_t* animated_track_data = segment_sampling_context_rotations[0].animated_track_data + 64;	// One cache line ahead
							const uint32_t animated_bit_offset0 = segment_sampling_context_rotations[0].animated_track_data_bit_offset;
							const uint32_t animated_bit_offset1 = segment_sampling_context_rotations[1].animated_track_data_bit_offset;
							ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_bit_offset0 / 8));
							ACL_IMPL_ANIMATED_PREFETCH(animated_track_data + (animated_bit_offset1 / 8));
						}
#endif

						// Swizzle out our 4 samples
						rtm::vector4f sample0;
						rtm::vector4f sample1;
						rtm::vector4f sample2;
						rtm::vector4f sample3;
						RTM_MATRIXF_TRANSPOSE_4X4(interp_xxxx, interp_yyyy, interp_zzzz, interp_wwww, sample0, sample1, sample2, sample3);

						// Always first rounding mode (none)
						rtm::quatf* cache_ptr = &rotations.cached_samples[static_cast<int>(sample_rounding_policy::none)][cache_write_index];

						cache_ptr[0] = rtm::vector_to_quat(sample0);
						cache_ptr[1] = rtm::vector_to_quat(sample1);
						cache_ptr[2] = rtm::vector_to_quat(sample2);
						cache_ptr[3] = rtm::vector_to_quat(sample3);
					}
				}
			}

			template<class decompression_settings_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_rotation_groups(const persistent_transform_decompression_context_v0& decomp_context, uint32_t num_groups_to_skip)
			{
				const uint32_t num_left_to_unpack = rotations.num_left_to_unpack;
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				ACL_ASSERT(num_to_skip < num_left_to_unpack, "Cannot skip rotations that aren't present");

				rotations.num_left_to_unpack = num_left_to_unpack - num_to_skip;

				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				if (rotation_format == rotation_format8::quatf_drop_w_variable && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_drop_w_variable))
				{
					const uint8_t* format_per_track_data0 = segment_sampling_context_rotations[0].format_per_track_data;
					const uint8_t* format_per_track_data1 = segment_sampling_context_rotations[1].format_per_track_data;

					uint32_t group_bit_size_per_component0;
					uint32_t group_bit_size_per_component1;
					count_animated_group_bit_size<decompression_settings_type>(decomp_context, format_per_track_data0, format_per_track_data1, num_groups_to_skip, group_bit_size_per_component0, group_bit_size_per_component1);

					const uint32_t format_per_track_data_skip_size = num_groups_to_skip * 4;
					const uint32_t segment_range_data_skip_size = num_groups_to_skip * 6 * 4;

					segment_sampling_context_rotations[0].format_per_track_data = format_per_track_data0 + format_per_track_data_skip_size;
					segment_sampling_context_rotations[0].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_rotations[0].animated_track_data_bit_offset += group_bit_size_per_component0 * 3;

					segment_sampling_context_rotations[1].format_per_track_data = format_per_track_data1 + format_per_track_data_skip_size;
					segment_sampling_context_rotations[1].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_rotations[1].animated_track_data_bit_offset += group_bit_size_per_component1 * 3;

					clip_sampling_context_rotations.clip_range_data += sizeof(rtm::float3f) * 2 * 4 * num_groups_to_skip;
				}
				else
				{
					uint32_t group_bit_size;
					if (rotation_format == rotation_format8::quatf_full && decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
						group_bit_size = 32 * 4 * 4 * num_groups_to_skip;
					else // drop w full
						group_bit_size = 32 * 3 * 4 * num_groups_to_skip;

					segment_sampling_context_rotations[0].animated_track_data_bit_offset += group_bit_size;
					segment_sampling_context_rotations[1].animated_track_data_bit_offset += group_bit_size;
				}
			}

			template<class decompression_settings_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::quatf RTM_SIMD_CALL unpack_rotation_within_group(const persistent_transform_decompression_context_v0& decomp_context, uint32_t unpack_index, float interpolation_alpha)
			{
				ACL_ASSERT(unpack_index < rotations.num_left_to_unpack && unpack_index < 4, "Cannot unpack sample that isn't present");

				const uint32_t group_size = std::min<uint32_t>(rotations.num_left_to_unpack, 4);

				const rtm::vector4f sample_as_vec0 = unpack_single_animated_quat<decompression_settings_type>(decomp_context, unpack_index, group_size, clip_sampling_context_rotations, segment_sampling_context_rotations[0]);
				const rtm::vector4f sample_as_vec1 = unpack_single_animated_quat<decompression_settings_type>(decomp_context, unpack_index, group_size, clip_sampling_context_rotations, segment_sampling_context_rotations[1]);

				rtm::quatf sample0;
				rtm::quatf sample1;

				// Reconstruct our quaternion W component
				const rotation_format8 rotation_format = get_rotation_format<decompression_settings_type>(decomp_context.rotation_format);
				if (rotation_format != rotation_format8::quatf_full || !decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
				{
					sample0 = rtm::quat_from_positive_w(sample_as_vec0);
					sample1 = rtm::quat_from_positive_w(sample_as_vec1);
				}
				else
				{
					sample0 = rtm::vector_to_quat(sample_as_vec0);
					sample1 = rtm::vector_to_quat(sample_as_vec1);
				}

				rtm::quatf result;

				const bool should_interpolate = should_interpolate_samples<decompression_settings_type>(rotation_format, interpolation_alpha);
				if (should_interpolate)
				{
					// Due to the interpolation, the result might not be anywhere near normalized!
					// Make sure to normalize afterwards before using
					if (decompression_settings_type::get_rotation_normalization_policy() >= rotation_normalization_policy_t::lerp_only)
						result = rtm::quat_lerp(sample0, sample1, interpolation_alpha);
					else
						result = quat_lerp_no_normalization(sample0, sample1, interpolation_alpha);
				}
				else
				{
					// If we don't interpolate, just pick the sample we need, it is already normalized after reconstructing
					// the W component or it was raw to begin with
					result = interpolation_alpha <= 0.0F ? sample0 : sample1;

					if (decompression_settings_type::get_rotation_normalization_policy() == rotation_normalization_policy_t::always)
					{
						if (rotation_format != rotation_format8::quatf_full || !decompression_settings_type::is_rotation_format_supported(rotation_format8::quatf_full))
						{
							// quat_from_positive_w might not yield an accurate quaternion because the square-root instruction
							// isn't very accurate on small inputs, we need to normalize
							result = rtm::quat_normalize(result);
						}
					}
				}

				return result;
			}

			RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK const rtm::quatf& consume_rotation(sample_rounding_policy policy)
			{
				ACL_ASSERT(rotations.cache_read_index < rotations.cache_write_index, "Attempting to consume an animated sample that isn't cached");
				const uint32_t cache_read_index = rotations.cache_read_index++;
				return rotations.cached_samples[static_cast<int>(policy)][cache_read_index % 8];
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void unpack_translation_group(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const uint32_t num_left_to_unpack = translations.num_left_to_unpack;
				if (num_left_to_unpack == 0)
					return;	// Nothing left to do, we are done

				// If we have less than 4 cached samples, unpack 4 more and prefetch the next cache line
				const uint32_t num_cached = translations.get_num_cached();
				if (num_cached >= 4)
					return;	// Enough cached, nothing to do

				const uint32_t num_to_unpack = std::min<uint32_t>(num_left_to_unpack, 4);
				translations.num_left_to_unpack = num_left_to_unpack - num_to_unpack;

				// Write index will be either 0 or 4 here since we always unpack 4 at a time
				const uint32_t cache_write_index = translations.cache_write_index % 8;
				translations.cache_write_index += num_to_unpack;

				unpack_animated_vector3<decompression_settings_adapter_type>(decomp_context, scratch0, num_to_unpack, clip_sampling_context_translations, segment_sampling_context_translations[0]);
				unpack_animated_vector3<decompression_settings_adapter_type>(decomp_context, scratch1, num_to_unpack, clip_sampling_context_translations, segment_sampling_context_translations[1]);

				const rtm::vector4f interpolation_alpha = rtm::vector_set(decomp_context.interpolation_alpha);
				const rtm::mask4f use_sample0 = rtm::vector_less_than(interpolation_alpha, rtm::vector_set(0.5F));

				// If we support per track rounding, we have to retain everything
				// Write both floor/ceil/nearest samples and interpolate as well
				// When we consume the sample, we'll pick the right one according to the rounding policy

				rtm::vector4f* cache_ptr_none = &translations.cached_samples[static_cast<int>(sample_rounding_policy::none)][cache_write_index];
				rtm::vector4f* cache_ptr_floor = &translations.cached_samples[static_cast<int>(sample_rounding_policy::floor)][cache_write_index];
				rtm::vector4f* cache_ptr_ceil = &translations.cached_samples[static_cast<int>(sample_rounding_policy::ceil)][cache_write_index];
				rtm::vector4f* cache_ptr_nearest = &translations.cached_samples[static_cast<int>(sample_rounding_policy::nearest)][cache_write_index];

				for (uint32_t unpack_index = 0; unpack_index < num_to_unpack; ++unpack_index)
				{
					const rtm::vector4f sample0 = scratch0[unpack_index];
					const rtm::vector4f sample1 = scratch1[unpack_index];

					if (decompression_settings_adapter_type::is_per_track_rounding_supported())
					{
						// These stores have no dependency and can be dispatched right away
						cache_ptr_floor[unpack_index] = sample0;
						cache_ptr_ceil[unpack_index] = sample1;
						cache_ptr_nearest[unpack_index] = rtm::vector_select(use_sample0, sample0, sample1);
					}

					const rtm::vector4f sample = rtm::vector_lerp(sample0, sample1, interpolation_alpha);

					cache_ptr_none[unpack_index] = sample;
				}

				// If we have clip range data, skip it
				const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					clip_sampling_context_translations.clip_range_data += num_to_unpack * sizeof(rtm::float3f) * 2;

					// Clip range data is 24 bytes per sub-track and as such we need to prefetch two cache lines ahead to process 4 sub-tracks
					ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_translations.clip_range_data + 64);
					ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_translations.clip_range_data + 128);
				}
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_translation_groups(const persistent_transform_decompression_context_v0& decomp_context, uint32_t num_groups_to_skip)
			{
				const uint32_t num_left_to_unpack = translations.num_left_to_unpack;
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				ACL_ASSERT(num_to_skip < num_left_to_unpack, "Cannot skip translations that aren't present");

				translations.num_left_to_unpack = num_left_to_unpack - num_to_skip;

				const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					const uint8_t* format_per_track_data0 = segment_sampling_context_translations[0].format_per_track_data;
					const uint8_t* format_per_track_data1 = segment_sampling_context_translations[1].format_per_track_data;

					uint32_t group_bit_size_per_component0;
					uint32_t group_bit_size_per_component1;
					count_animated_group_bit_size<decompression_settings_adapter_type>(decomp_context, format_per_track_data0, format_per_track_data1, num_groups_to_skip, group_bit_size_per_component0, group_bit_size_per_component1);

					const uint32_t format_per_track_data_skip_size = num_groups_to_skip * 4;
					const uint32_t segment_range_data_skip_size = num_groups_to_skip * 6 * 4;

					segment_sampling_context_translations[0].format_per_track_data = format_per_track_data0 + format_per_track_data_skip_size;
					segment_sampling_context_translations[0].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_translations[0].animated_track_data_bit_offset += group_bit_size_per_component0 * 3;

					segment_sampling_context_translations[1].format_per_track_data = format_per_track_data1 + format_per_track_data_skip_size;
					segment_sampling_context_translations[1].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_translations[1].animated_track_data_bit_offset += group_bit_size_per_component1 * 3;

					clip_sampling_context_translations.clip_range_data += sizeof(rtm::float3f) * 2 * 4 * num_groups_to_skip;
				}
				else
				{
					const uint32_t group_bit_size = 32 * 3 * 4 * num_groups_to_skip;
					segment_sampling_context_translations[0].animated_track_data_bit_offset += group_bit_size;
					segment_sampling_context_translations[1].animated_track_data_bit_offset += group_bit_size;
				}
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_translation_within_group(const persistent_transform_decompression_context_v0& decomp_context, uint32_t unpack_index, float interpolation_alpha)
			{
				ACL_ASSERT(unpack_index < translations.num_left_to_unpack && unpack_index < 4, "Cannot unpack sample that isn't present");

				const rtm::vector4f sample0 = unpack_single_animated_vector3<decompression_settings_adapter_type>(decomp_context, unpack_index, clip_sampling_context_translations, segment_sampling_context_translations[0]);
				const rtm::vector4f sample1 = unpack_single_animated_vector3<decompression_settings_adapter_type>(decomp_context, unpack_index, clip_sampling_context_translations, segment_sampling_context_translations[1]);

				return rtm::vector_lerp(sample0, sample1, interpolation_alpha);
			}

			RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK const rtm::vector4f& consume_translation(sample_rounding_policy policy)
			{
				ACL_ASSERT(translations.cache_read_index < translations.cache_write_index, "Attempting to consume an animated sample that isn't cached");
				const uint32_t cache_read_index = translations.cache_read_index++;
				return translations.cached_samples[static_cast<int>(policy)][cache_read_index % 8];
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void unpack_scale_group(const persistent_transform_decompression_context_v0& decomp_context)
			{
				const uint32_t num_left_to_unpack = scales.num_left_to_unpack;
				if (num_left_to_unpack == 0)
					return;	// Nothing left to do, we are done

				// If we have less than 4 cached samples, unpack 4 more and prefetch the next cache line
				const uint32_t num_cached = scales.get_num_cached();
				if (num_cached >= 4)
					return;	// Enough cached, nothing to do

				const uint32_t num_to_unpack = std::min<uint32_t>(num_left_to_unpack, 4);
				scales.num_left_to_unpack = num_left_to_unpack - num_to_unpack;

				// Write index will be either 0 or 4 here since we always unpack 4 at a time
				const uint32_t cache_write_index = scales.cache_write_index % 8;
				scales.cache_write_index += num_to_unpack;

				unpack_animated_vector3<decompression_settings_adapter_type>(decomp_context, scratch0, num_to_unpack, clip_sampling_context_scales, segment_sampling_context_scales[0]);
				unpack_animated_vector3<decompression_settings_adapter_type>(decomp_context, scratch1, num_to_unpack, clip_sampling_context_scales, segment_sampling_context_scales[1]);

				const rtm::vector4f interpolation_alpha = rtm::vector_set(decomp_context.interpolation_alpha);
				const rtm::mask4f use_sample0 = rtm::vector_less_than(interpolation_alpha, rtm::vector_set(0.5F));

				// If we support per track rounding, we have to retain everything
				// Write both floor/ceil/nearest samples and interpolate as well
				// When we consume the sample, we'll pick the right one according to the rounding policy

				rtm::vector4f* cache_ptr_none = &scales.cached_samples[static_cast<int>(sample_rounding_policy::none)][cache_write_index];
				rtm::vector4f* cache_ptr_floor = &scales.cached_samples[static_cast<int>(sample_rounding_policy::floor)][cache_write_index];
				rtm::vector4f* cache_ptr_ceil = &scales.cached_samples[static_cast<int>(sample_rounding_policy::ceil)][cache_write_index];
				rtm::vector4f* cache_ptr_nearest = &scales.cached_samples[static_cast<int>(sample_rounding_policy::nearest)][cache_write_index];

				for (uint32_t unpack_index = 0; unpack_index < num_to_unpack; ++unpack_index)
				{
					const rtm::vector4f sample0 = scratch0[unpack_index];
					const rtm::vector4f sample1 = scratch1[unpack_index];

					if (decompression_settings_adapter_type::is_per_track_rounding_supported())
					{
						// These stores have no dependency and can be dispatched right away
						cache_ptr_floor[unpack_index] = sample0;
						cache_ptr_ceil[unpack_index] = sample1;
						cache_ptr_nearest[unpack_index] = rtm::vector_select(use_sample0, sample0, sample1);
					}

					const rtm::vector4f sample = rtm::vector_lerp(sample0, sample1, interpolation_alpha);

					cache_ptr_none[unpack_index] = sample;
				}

				// If we have clip range data, skip it
				const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					clip_sampling_context_scales.clip_range_data += num_to_unpack * sizeof(rtm::float3f) * 2;

					// Clip range data is 24 bytes per sub-track and as such we need to prefetch two cache lines ahead to process 4 sub-tracks
					ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_scales.clip_range_data + 64);
					ACL_IMPL_ANIMATED_PREFETCH(clip_sampling_context_scales.clip_range_data + 128);
				}
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK void skip_scale_groups(const persistent_transform_decompression_context_v0& decomp_context, uint32_t num_groups_to_skip)
			{
				const uint32_t num_left_to_unpack = scales.num_left_to_unpack;
				const uint32_t num_to_skip = num_groups_to_skip * 4;
				ACL_ASSERT(num_to_skip < num_left_to_unpack, "Cannot skip scales that aren't present");

				scales.num_left_to_unpack = num_left_to_unpack - num_to_skip;

				const vector_format8 format = get_vector_format<decompression_settings_adapter_type>(decompression_settings_adapter_type::get_vector_format(decomp_context));
				if (format == vector_format8::vector3f_variable && decompression_settings_adapter_type::is_vector_format_supported(vector_format8::vector3f_variable))
				{
					const uint8_t* format_per_track_data0 = segment_sampling_context_scales[0].format_per_track_data;
					const uint8_t* format_per_track_data1 = segment_sampling_context_scales[1].format_per_track_data;

					uint32_t group_bit_size_per_component0;
					uint32_t group_bit_size_per_component1;
					count_animated_group_bit_size<decompression_settings_adapter_type>(decomp_context, format_per_track_data0, format_per_track_data1, num_groups_to_skip, group_bit_size_per_component0, group_bit_size_per_component1);

					const uint32_t format_per_track_data_skip_size = num_groups_to_skip * 4;
					const uint32_t segment_range_data_skip_size = num_groups_to_skip * 6 * 4;

					segment_sampling_context_scales[0].format_per_track_data = format_per_track_data0 + format_per_track_data_skip_size;
					segment_sampling_context_scales[0].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_scales[0].animated_track_data_bit_offset += group_bit_size_per_component0 * 3;

					segment_sampling_context_scales[1].format_per_track_data = format_per_track_data1 + format_per_track_data_skip_size;
					segment_sampling_context_scales[1].segment_range_data += segment_range_data_skip_size;
					segment_sampling_context_scales[1].animated_track_data_bit_offset += group_bit_size_per_component1 * 3;

					clip_sampling_context_scales.clip_range_data += sizeof(rtm::float3f) * 2 * 4 * num_groups_to_skip;
				}
				else
				{
					const uint32_t group_bit_size = 32 * 3 * 4 * num_groups_to_skip;
					segment_sampling_context_scales[0].animated_track_data_bit_offset += group_bit_size;
					segment_sampling_context_scales[1].animated_track_data_bit_offset += group_bit_size;
				}
			}

			template<class decompression_settings_adapter_type>
			RTM_DISABLE_SECURITY_COOKIE_CHECK rtm::vector4f RTM_SIMD_CALL unpack_scale_within_group(const persistent_transform_decompression_context_v0& decomp_context, uint32_t unpack_index, float interpolation_alpha)
			{
				ACL_ASSERT(unpack_index < scales.num_left_to_unpack && unpack_index < 4, "Cannot unpack sample that isn't present");

				const rtm::vector4f sample0 = unpack_single_animated_vector3<decompression_settings_adapter_type>(decomp_context, unpack_index, clip_sampling_context_scales, segment_sampling_context_scales[0]);
				const rtm::vector4f sample1 = unpack_single_animated_vector3<decompression_settings_adapter_type>(decomp_context, unpack_index, clip_sampling_context_scales, segment_sampling_context_scales[1]);

				return rtm::vector_lerp(sample0, sample1, interpolation_alpha);
			}

			RTM_FORCE_INLINE RTM_DISABLE_SECURITY_COOKIE_CHECK const rtm::vector4f& consume_scale(sample_rounding_policy policy)
			{
				ACL_ASSERT(scales.cache_read_index < scales.cache_write_index, "Attempting to consume an animated sample that isn't cached");
				const uint32_t cache_read_index = scales.cache_read_index++;
				return scales.cached_samples[static_cast<int>(policy)][cache_read_index % 8];
			}
		};
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

#if defined(RTM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif

ACL_IMPL_FILE_PRAGMA_POP
