#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2018 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/types.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/scalarf.h"
#include "rtm/scalard.h"
#include "rtm/version.h"

#include <cstdint>
#include <cstring>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(float x, float y, float z, float w) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_set_ps(w, z, y, x);
#elif defined(RTM_NEON_INTRINSICS)
		float32x2_t V0 = vset_lane_f32(y, vmov_n_f32(x), 1);
		float32x2_t V1 = vset_lane_f32(w, vmov_n_f32(z), 1);
		return vcombine_f32(V0, V1);
#else
		return vector4f{ x, y, z, w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from the [xyz] components and sets [w] to 0.0.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(float x, float y, float z) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_set_ps(0.0F, z, y, x);
#elif defined(RTM_NEON_INTRINSICS)
		float32x2_t V0 = vset_lane_f32(y, vmov_n_f32(x), 1);
		float32x2_t V1 = vset_lane_f32(z, vdup_n_f32(0.0F), 0);
		return vcombine_f32(V0, V1);
#else
		return vector4f{ x, y, z, 0.0f };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from a single value for all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(float xyzw) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_set_ps1(xyzw);
#elif defined(RTM_NEON_INTRINSICS)
		return vdupq_n_f32(xyzw);
#else
		return vector4f{ xyzw, xyzw, xyzw, xyzw };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(scalarf_arg0 x, scalarf_arg1 y, scalarf_arg2 z, scalarf_arg3 w) RTM_NO_EXCEPT
	{
		const __m128 xy = _mm_unpacklo_ps(x.value, y.value);
		const __m128 zw = _mm_unpacklo_ps(z.value, w.value);
		return _mm_shuffle_ps(xy, zw, _MM_SHUFFLE(1, 0, 1, 0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from the [xyz] components and sets [w] to 0.0.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(scalarf_arg0 x, scalarf_arg1 y, scalarf_arg2 z) RTM_NO_EXCEPT
	{
		const __m128 xy = _mm_unpacklo_ps(x.value, y.value);
		const __m128 zw = _mm_unpacklo_ps(z.value, _mm_setzero_ps());
		return _mm_shuffle_ps(xy, zw, _MM_SHUFFLE(1, 0, 1, 0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from a single value for all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set(scalarf_arg0 xyzw) RTM_NO_EXCEPT
	{
		return _mm_shuffle_ps(xyzw.value, xyzw.value, _MM_SHUFFLE(0, 0, 0, 0));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(double x, double y, double z, double w) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_set_pd(y, x), _mm_set_pd(w, z) };
#else
		return vector4d{ x, y, z, w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from the [xyz] components and sets [w] to 0.0.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(double x, double y, double z) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_set_pd(y, x), _mm_set_pd(0.0, z) };
#else
		return vector4d{ x, y, z, 0.0 };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from a single value for all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(double xyzw) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128d xyzw_pd = _mm_set1_pd(xyzw);
		return vector4d{ xyzw_pd, xyzw_pd };
#else
		return vector4d{ xyzw, xyzw, xyzw, xyzw };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(scalard_arg0 x, scalard_arg1 y, scalard_arg2 z, scalard_arg3 w) RTM_NO_EXCEPT
	{
		const __m128d xy = _mm_unpacklo_pd(x.value, y.value);
		const __m128d zw = _mm_unpacklo_pd(z.value, w.value);
		return vector4d{ xy, zw };
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from the [xyz] components and sets [w] to 0.0.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(scalard_arg0 x, scalard_arg1 y, scalard_arg2 z) RTM_NO_EXCEPT
	{
		const __m128d xy = _mm_unpacklo_pd(x.value, y.value);
		const __m128d zw = _mm_unpacklo_pd(z.value, _mm_setzero_pd());
		return vector4d{ xy, zw };
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a vector4 from a single value for all 4 components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set(scalard_arg0 xyzw) RTM_NO_EXCEPT
	{
		const __m128d xyzw_pd = _mm_shuffle_pd(xyzw.value, xyzw.value, 0);
		return vector4d{ xyzw_pd, xyzw_pd };
	}
#endif

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// Returns true if mix4 component is one of [xyzw]
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool is_mix_xyzw(mix4 arg) RTM_NO_EXCEPT { return uint32_t(arg) <= uint32_t(mix4::w); }

		//////////////////////////////////////////////////////////////////////////
		// Returns true if mix4 component is one of [abcd]
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool is_mix_abcd(mix4 arg) RTM_NO_EXCEPT { return uint32_t(arg) >= uint32_t(mix4::a); }

		//////////////////////////////////////////////////////////////////////////
		// Converts a mix4 value to a component4 value
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr component4 mix_to_component(mix4 arg) RTM_NO_EXCEPT { return static_cast<component4>(static_cast<uint32_t>(arg) % 4); }

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to help manipulate SIMD masks.
		//////////////////////////////////////////////////////////////////////////
		union mask_converter
		{
			uint64_t u64;
			uint32_t u32[2];

			RTM_DISABLE_SECURITY_COOKIE_CHECK explicit RTM_FORCE_INLINE constexpr mask_converter(uint64_t value) RTM_NO_EXCEPT : u64(value) {}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr operator uint32_t() const RTM_NO_EXCEPT { return u32[0]; }
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr operator uint64_t() const RTM_NO_EXCEPT { return u64; }
		};

		//////////////////////////////////////////////////////////////////////////
		// Returns a SIMD mask value from a boolean.
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr mask_converter get_mask_value(bool is_true) RTM_NO_EXCEPT
		{
			return mask_converter(is_true ? uint64_t(0xFFFFFFFFFFFFFFFFULL) : uint64_t(0));
		}

		//////////////////////////////////////////////////////////////////////////
		// Selects if_false if the SIMD mask value is 0, otherwise if_true.
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double RTM_SIMD_CALL select(uint64_t mask, double if_true, double if_false) RTM_NO_EXCEPT
		{
			return mask == 0 ? if_false : if_true;
		}

		//////////////////////////////////////////////////////////////////////////
		// Selects if_false if the SIMD mask value is 0, otherwise if_true.
		//////////////////////////////////////////////////////////////////////////
		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float RTM_SIMD_CALL select(uint32_t mask, float if_true, float if_false) RTM_NO_EXCEPT
		{
			return mask == 0 ? if_false : if_true;
		}

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector_zero_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				const __m128d zero_pd = _mm_setzero_pd();
				return vector4d{ zero_pd, zero_pd };
#else
				return vector_set(0.0);
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_setzero_ps();
#else
				return vector_set(0.0F);
#endif
			}
		};

		enum class vector_constant
		{
			zero,
			one,
		};

		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float get_constant32(vector_constant constant)
		{
			return constant == vector_constant::zero ? 0.0F : 1.0F;
		}

		RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double get_constant64(vector_constant constant)
		{
			return constant == vector_constant::zero ? 0.0 : 1.0;
		}

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		template<vector_constant x, vector_constant y, vector_constant z, vector_constant w>
		struct vector_constant_impl
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				constexpr __m128d constant_xy = RTM_VECTOR2D_MAKE(get_constant64(x), get_constant64(y));
				constexpr __m128d constant_zw = RTM_VECTOR2D_MAKE(get_constant64(z), get_constant64(w));
				return vector4d{ constant_xy, constant_zw };
#else
				return vector_set(get_constant64(x), get_constant64(y), get_constant64(z), get_constant64(w));
#endif
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				constexpr __m128 constant = RTM_VECTOR4F_MAKE(get_constant32(x), get_constant32(y), get_constant32(z), get_constant32(w));
				return constant;
#else
				return vector_set(get_constant32(x), get_constant32(y), get_constant32(z), get_constant32(w));
#endif
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// Various vector widths we can load
		//////////////////////////////////////////////////////////////////////////
		enum class vector_unaligned_loader_width
		{
			vec1,
			vec2,
			vec3,
			vec4,
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		template<vector_unaligned_loader_width width>
		struct vector_unaligned_loader
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
				switch (width)
				{
				case vector_unaligned_loader_width::vec1:
				{
					double data[1];
					std::memcpy(&data[0], ptr, sizeof(double) * 1);
					return vector_set(data[0], 0.0, 0.0, 0.0);
				}
				case vector_unaligned_loader_width::vec2:
				{
					double data[2];
					std::memcpy(&data[0], ptr, sizeof(double) * 2);
					return vector_set(data[0], data[1], 0.0, 0.0);
				}
				case vector_unaligned_loader_width::vec3:
				{
					double data[3];
					std::memcpy(&data[0], ptr, sizeof(double) * 3);
					return vector_set(data[0], data[1], data[2], 0.0);
				}
				case vector_unaligned_loader_width::vec4:
				default:
				{
					vector4d result;
					std::memcpy(&result, ptr, sizeof(vector4d));
					return result;
				}
				}
			}

			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
				switch (width)
				{
				case vector_unaligned_loader_width::vec1:
				{
					float data[1];
					std::memcpy(&data[0], ptr, sizeof(float) * 1);
					return vector_set(data[0], 0.0F, 0.0F, 0.0F);
				}
				case vector_unaligned_loader_width::vec2:
				{
					float data[2];
					std::memcpy(&data[0], ptr, sizeof(float) * 2);
					return vector_set(data[0], data[1], 0.0F, 0.0F);
				}
				case vector_unaligned_loader_width::vec3:
				{
					float data[3];
					std::memcpy(&data[0], ptr, sizeof(float) * 3);
					return vector_set(data[0], data[1], data[2], 0.0F);
				}
				case vector_unaligned_loader_width::vec4:
				default:
				{
#if defined(RTM_SSE2_INTRINSICS)
					return _mm_loadu_ps((const float*)ptr);
#elif defined(RTM_NEON_INTRINSICS)
					return vreinterpretq_f32_u8(vld1q_u8(ptr));
#else
					vector4f result;
					std::memcpy(&result, ptr, sizeof(vector4f));
					return result;
#endif
				}
				}
			}

			const uint8_t* ptr;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_to_scalarf
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtss_f32(value);
#elif defined(RTM_NEON_INTRINSICS)
				return vgetq_lane_f32(value, 0);
#else
				return value.x;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ value };
			}
#endif

			vector4f value;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_to_scalard
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(value.xy);
#else
				return value.x;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ value.xy };
			}
#endif

			vector4d value;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_get_min_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128 zwzw = _mm_movehl_ps(value, value);
				__m128 xz_yw_zz_ww = _mm_min_ps(value, zwzw);
				__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
				return _mm_cvtss_f32(_mm_min_ps(xz_yw_zz_ww, yw_yw_yw_yw));
#elif defined(RTM_NEON_INTRINSICS)
				float32x2_t xy_zw = vpmin_f32(vget_low_f32(value), vget_high_f32(value));
				return vget_lane_f32(vpmin_f32(xy_zw, xy_zw), 0);
#else
				return scalar_min(scalar_min(value.x, value.y), scalar_min(value.z, value.w));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				__m128 zwzw = _mm_movehl_ps(value, value);
				__m128 xz_yw_zz_ww = _mm_min_ps(value, zwzw);
				__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
				return scalarf{ _mm_min_ps(xz_yw_zz_ww, yw_yw_yw_yw) };
			}
#endif

			vector4f value;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_get_max_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128 zwzw = _mm_movehl_ps(value, value);
				__m128 xz_yw_zz_ww = _mm_max_ps(value, zwzw);
				__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
				return _mm_cvtss_f32(_mm_max_ps(xz_yw_zz_ww, yw_yw_yw_yw));
#elif defined(RTM_NEON_INTRINSICS)
				float32x2_t xy_zw = vpmax_f32(vget_low_f32(value), vget_high_f32(value));
				return vget_lane_f32(vpmax_f32(xy_zw, xy_zw), 0);
#else
				return scalar_max(scalar_max(value.x, value.y), scalar_max(value.z, value.w));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				__m128 zwzw = _mm_movehl_ps(value, value);
				__m128 xz_yw_zz_ww = _mm_max_ps(value, zwzw);
				__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
				return scalarf{ _mm_max_ps(xz_yw_zz_ww, yw_yw_yw_yw) };
			}
#endif

			vector4f value;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_get_min_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128d xz_yw = _mm_min_pd(value.xy, value.zw);
				__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
				return _mm_cvtsd_f64(_mm_min_pd(xz_yw, yw_yw));
#else
				return scalar_min(scalar_min(value.x, value.y), scalar_min(value.z, value.w));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				__m128d xz_yw = _mm_min_pd(value.xy, value.zw);
				__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
				return scalard{ _mm_min_pd(xz_yw, yw_yw) };
			}
#endif

			vector4d value;
		};

		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_get_max_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128d xz_yw = _mm_max_pd(value.xy, value.zw);
				__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
				return _mm_cvtsd_f64(_mm_max_pd(xz_yw, yw_yw));
#else
				return scalar_max(scalar_max(value.x, value.y), scalar_max(value.z, value.w));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				__m128d xz_yw = _mm_max_pd(value.xy, value.zw);
				__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
				return scalard{ _mm_max_pd(xz_yw, yw_yw) };
			}
#endif

			vector4d value;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a vector consisting of all zeros.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_zero_impl RTM_SIMD_CALL vector_zero() RTM_NO_EXCEPT
	{
		return rtm_impl::vector_zero_impl();
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a unit vector pointing in the forward direction of the default coordinate system (Z+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_constant_impl<rtm_impl::vector_constant::zero, rtm_impl::vector_constant::zero, rtm_impl::vector_constant::one, rtm_impl::vector_constant::zero> RTM_SIMD_CALL vector_coord_forward() RTM_NO_EXCEPT
	{
		return rtm_impl::vector_constant_impl<
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::one,
			rtm_impl::vector_constant::zero>();
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a unit vector pointing in the up direction of the default coordinate system (Y+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_constant_impl<rtm_impl::vector_constant::zero, rtm_impl::vector_constant::one, rtm_impl::vector_constant::zero, rtm_impl::vector_constant::zero> RTM_SIMD_CALL vector_coord_up() RTM_NO_EXCEPT
	{
		return rtm_impl::vector_constant_impl<
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::one,
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::zero>();
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a unit vector pointing in the cross direction of the default coordinate system (X+).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_constant_impl<rtm_impl::vector_constant::one, rtm_impl::vector_constant::zero, rtm_impl::vector_constant::zero, rtm_impl::vector_constant::zero> RTM_SIMD_CALL vector_coord_cross() RTM_NO_EXCEPT
	{
		return rtm_impl::vector_constant_impl<
			rtm_impl::vector_constant::one,
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::zero,
			rtm_impl::vector_constant::zero>();
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector4 from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec4> RTM_SIMD_CALL vector_load(const uint8_t* input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec4>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector1 from memory and sets the [yzw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec1> RTM_SIMD_CALL vector_load1(const uint8_t* input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec1>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector2 from memory and sets the [zw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec2> RTM_SIMD_CALL vector_load2(const uint8_t* input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec2>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector3 from memory and sets the [w] component to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec3> RTM_SIMD_CALL vector_load3(const uint8_t* input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector_unaligned_loader<rtm_impl::vector_unaligned_loader_width::vec3>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Coerces an vector4 input into a scalar by grabbing the first SIMD lane.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_to_scalarf RTM_SIMD_CALL vector_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_to_scalarf{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Coerces an vector4 input into a scalar by grabbing the first SIMD lane.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_to_scalard RTM_SIMD_CALL vector_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_to_scalard{ input };
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
