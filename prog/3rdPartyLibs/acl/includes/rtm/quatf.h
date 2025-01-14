#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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

#include "rtm/math.h"
#include "rtm/scalarf.h"
#include "rtm/vector4f.h"
#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/macros.mask4.impl.h"
#include "rtm/impl/memory_utils.h"
#include "rtm/impl/quat_common.h"

#include <limits>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Setters, getters, and casts
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned quaternion from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_load(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_loadu_ps(input);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_f32(input);
#else
		return quat_set(input[0], input[1], input[2], input[3]);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned quat from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_load(const float4f* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_loadu_ps(&input->x);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_f32(&input->x);
#else
		return quat_set(input->x, input->y, input->z, input->w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a vector4 to a quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL vector_to_quat(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS) || defined(RTM_NEON_INTRINSICS)
		return input;
#else
		return quatf{ input.x, input.y, input.z, input.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a quaternion float64 variant to a float32 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_cast(const quatd& input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_shuffle_ps(_mm_cvtpd_ps(input.xy), _mm_cvtpd_ps(input.zw), _MM_SHUFFLE(1, 0, 1, 0));
#else
		return quat_set(float(input.x), float(input.y), float(input.z), float(input.w));
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_get_x
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtss_f32(input);
#elif defined(RTM_NEON_INTRINSICS)
				return vgetq_lane_f32(input, 0);
#else
				return input.x;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ input };
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [x] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_get_x RTM_SIMD_CALL quat_get_x(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_get_x{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [x] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_get_x_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ input };
#else
		return quat_get_x(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_get_y
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtss_f32(_mm_shuffle_ps(input, input, _MM_SHUFFLE(1, 1, 1, 1)));
#elif defined(RTM_NEON_INTRINSICS)
				return vgetq_lane_f32(input, 1);
#else
				return input.y;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(1, 1, 1, 1)) };
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [y] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_get_y RTM_SIMD_CALL quat_get_y(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_get_y{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [y] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_get_y_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(1, 1, 1, 1)) };
#else
		return quat_get_y(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_get_z
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtss_f32(_mm_shuffle_ps(input, input, _MM_SHUFFLE(2, 2, 2, 2)));
#elif defined(RTM_NEON_INTRINSICS)
				return vgetq_lane_f32(input, 2);
#else
				return input.z;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(2, 2, 2, 2)) };
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [z] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_get_z RTM_SIMD_CALL quat_get_z(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_get_z{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [z] component (real part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_get_z_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(2, 2, 2, 2)) };
#else
		return quat_get_z(input);
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_get_w
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtss_f32(_mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 3, 3, 3)));
#elif defined(RTM_NEON_INTRINSICS)
				return vgetq_lane_f32(input, 3);
#else
				return input.w;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 3, 3, 3)) };
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [w] component (imaginary part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_get_w RTM_SIMD_CALL quat_get_w(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_get_w{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion [w] component (imaginary part).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_get_w_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 3, 3, 3)) };
#else
		return quat_get_w(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [x] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_x(quatf_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_move_ss(input, _mm_set_ss(lane_value));
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(lane_value, input, 0);
#else
		return quatf{ lane_value, input.y, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [x] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_x(quatf_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
		return _mm_move_ss(input, lane_value.value);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [y] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_y(quatf_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, _mm_set_ss(lane_value), 0x10);
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128 yxzw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 2, 0, 1));
		const __m128 vxzw = _mm_move_ss(yxzw, _mm_set_ss(lane_value));
		return _mm_shuffle_ps(vxzw, vxzw, _MM_SHUFFLE(3, 2, 0, 1));
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(lane_value, input, 1);
#else
		return quatf{ input.x, lane_value, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [y] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_y(quatf_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, lane_value.value, 0x10);
#else
		const __m128 yxzw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 2, 0, 1));
		const __m128 vxzw = _mm_move_ss(yxzw, lane_value.value);
		return _mm_shuffle_ps(vxzw, vxzw, _MM_SHUFFLE(3, 2, 0, 1));
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [z] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_z(quatf_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, _mm_set_ss(lane_value), 0x20);
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128 zyxw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 0, 1, 2));
		const __m128 vyxw = _mm_move_ss(zyxw, _mm_set_ss(lane_value));
		return _mm_shuffle_ps(vyxw, vyxw, _MM_SHUFFLE(3, 0, 1, 2));
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(lane_value, input, 2);
#else
		return quatf{ input.x, input.y, lane_value, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [z] component (real part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_z(quatf_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, lane_value.value, 0x20);
#else
		const __m128 zyxw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 0, 1, 2));
		const __m128 vyxw = _mm_move_ss(zyxw, lane_value.value);
		return _mm_shuffle_ps(vyxw, vyxw, _MM_SHUFFLE(3, 0, 1, 2));
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [w] component (imaginary part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_w(quatf_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, _mm_set_ss(lane_value), 0x30);
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128 wyzx = _mm_shuffle_ps(input, input, _MM_SHUFFLE(0, 2, 1, 3));
		const __m128 vyzx = _mm_move_ss(wyzx, _mm_set_ss(lane_value));
		return _mm_shuffle_ps(vyzx, vyzx, _MM_SHUFFLE(0, 2, 1, 3));
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(lane_value, input, 3);
#else
		return quatf{ input.x, input.y, input.z, lane_value };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the quaternion [w] component (imaginary part) and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_set_w(quatf_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, lane_value.value, 0x30);
#else
		const __m128 wyzx = _mm_shuffle_ps(input, input, _MM_SHUFFLE(0, 2, 1, 3));
		const __m128 vyzx = _mm_move_ss(wyzx, lane_value.value);
		return _mm_shuffle_ps(vyzx, vyzx, _MM_SHUFFLE(0, 2, 1, 3));
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Writes a quaternion to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL quat_store(quatf_arg0 input, float* output) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		_mm_storeu_ps(output, input);
#else
		output[0] = quat_get_x(input);
		output[1] = quat_get_y(input);
		output[2] = quat_get_z(input);
		output[3] = quat_get_w(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a quaternion to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL quat_store(quatf_arg0 input, float4f* output) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		_mm_storeu_ps(&output->x, input);
#else
		output->x = quat_get_x(input);
		output->y = quat_get_y(input);
		output->z = quat_get_z(input);
		output->w = quat_get_w(input);
#endif
	}



	//////////////////////////////////////////////////////////////////////////
	// Arithmetic
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion conjugate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_conjugate(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 signs = RTM_VECTOR4F_MAKE(-0.0F, -0.0F, -0.0F, 0.0F);
		return _mm_xor_ps(input, signs);
#else
		// On ARMv7 the scalar version performs best or among the best while on ARM64 it beats the others.
		return quat_set(-quat_get_x(input), -quat_get_y(input), -quat_get_z(input), quat_get_w(input));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies two quaternions.
	// Note that due to floating point rounding, the result might not be perfectly normalized.
	// Multiplication order is as follow: local_to_world = quat_mul(local_to_object, object_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_mul(quatf_arg0 lhs, quatf_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS) && 0
		// TODO: Profile this, the accuracy is the same as with SSE2, should be binary exact
		constexpr __m128 signs_x = RTM_VECTOR4F_MAKE(1.0F,  1.0F,  1.0F, -1.0F);
		constexpr __m128 signs_y = RTM_VECTOR4F_MAKE(1.0F, -1.0F,  1.0F,  1.0F);
		constexpr __m128 signs_z = RTM_VECTOR4F_MAKE(1.0F,  1.0F, -1.0F,  1.0F);
		constexpr __m128 signs_w = RTM_VECTOR4F_MAKE(1.0F, -1.0F, -1.0F, -1.0F);
		// x = dot(rhs.wxyz, lhs.xwzy * signs_x)
		// y = dot(rhs.wxyz, lhs.yzwx * signs_y)
		// z = dot(rhs.wxyz, lhs.zyxw * signs_z)
		// w = dot(rhs.wxyz, lhs.wxyz * signs_w)
		__m128 rhs_wxyz = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(2, 1, 0, 3));
		__m128 lhs_xwzy = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(1, 2, 3, 0));
		__m128 lhs_yzwx = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(0, 3, 2, 1));
		__m128 lhs_zyxw = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3, 0, 1, 2));
		__m128 lhs_wxyz = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(2, 1, 0, 3));
		__m128 x = _mm_dp_ps(rhs_wxyz, _mm_mul_ps(lhs_xwzy, signs_x), 0xFF);
		__m128 y = _mm_dp_ps(rhs_wxyz, _mm_mul_ps(lhs_yzwx, signs_y), 0xFF);
		__m128 z = _mm_dp_ps(rhs_wxyz, _mm_mul_ps(lhs_zyxw, signs_z), 0xFF);
		__m128 w = _mm_dp_ps(rhs_wxyz, _mm_mul_ps(lhs_wxyz, signs_w), 0xFF);
		__m128 xxyy = _mm_shuffle_ps(x, y, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 zzww = _mm_shuffle_ps(z, w, _MM_SHUFFLE(0, 0, 0, 0));
		return _mm_shuffle_ps(xxyy, zzww, _MM_SHUFFLE(2, 0, 2, 0));
#elif defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 control_wzyx = RTM_VECTOR4F_MAKE(0.0F, -0.0F,  0.0F, -0.0F);
		constexpr __m128 control_zwxy = RTM_VECTOR4F_MAKE(0.0F,  0.0F, -0.0F, -0.0F);
		constexpr __m128 control_yxwz = RTM_VECTOR4F_MAKE(-0.0F,  0.0F,  0.0F, -0.0F);

		const __m128 r_xxxx = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 r_yyyy = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 r_zzzz = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(2, 2, 2, 2));
		const __m128 r_wwww = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3, 3, 3, 3));

		const __m128 lxrw_lyrw_lzrw_lwrw = _mm_mul_ps(r_wwww, lhs);
		const __m128 l_wzyx = _mm_shuffle_ps(lhs, lhs,_MM_SHUFFLE(0, 1, 2, 3));

		const __m128 lwrx_lzrx_lyrx_lxrx = _mm_mul_ps(r_xxxx, l_wzyx);
		const __m128 l_zwxy = _mm_shuffle_ps(l_wzyx, l_wzyx,_MM_SHUFFLE(2, 3, 0, 1));

		const __m128 lwrx_nlzrx_lyrx_nlxrx = _mm_xor_ps(lwrx_lzrx_lyrx_lxrx, control_wzyx);

		const __m128 lzry_lwry_lxry_lyry = _mm_mul_ps(r_yyyy, l_zwxy);
		const __m128 l_yxwz = _mm_shuffle_ps(l_zwxy, l_zwxy,_MM_SHUFFLE(0, 1, 2, 3));

		const __m128 lzry_lwry_nlxry_nlyry = _mm_xor_ps(lzry_lwry_lxry_lyry, control_zwxy);

		const __m128 lyrz_lxrz_lwrz_lzrz = _mm_mul_ps(r_zzzz, l_yxwz);
		const __m128 result0 = _mm_add_ps(lxrw_lyrw_lzrw_lwrw, lwrx_nlzrx_lyrx_nlxrx);

		const __m128 nlyrz_lxrz_lwrz_wlzrz = _mm_xor_ps(lyrz_lxrz_lwrz_lzrz, control_yxwz);
		const __m128 result1 = _mm_add_ps(lzry_lwry_nlxry_nlyry, nlyrz_lxrz_lwrz_wlzrz);
		return _mm_add_ps(result0, result1);
#elif defined(RTM_NEON64_INTRINSICS)
		// Use shuffles and negation instead of loading constants and doing mul/xor.
		// On ARM64, this is the fastest version.

		// Dispatch rev first, if we can't dual dispatch with neg below, we won't stall it
		// [l.y, l.x, l.w, l.z]
		const float32x4_t y_x_w_z = vrev64q_f32(lhs);

		// [-l.x, -l.y, -l.z, -l.w]
		const float32x4_t neg_lhs = vnegq_f32(lhs);

		// trn([l.y, l.x, l.w, l.z], [-l.x, -l.y, -l.z, -l.w]) = [l.y, -l.x, l.w, -l.z], [l.x, -l.y, l.z, -l.w]
		float32x4x2_t y_nx_w_nz__x_ny_z_nw = vtrnq_f32(y_x_w_z, neg_lhs);

		// [l.w, -l.z, l.y, -l.x]
		float32x4_t l_wzyx = vcombine_f32(vget_high_f32(y_nx_w_nz__x_ny_z_nw.val[0]), vget_low_f32(y_nx_w_nz__x_ny_z_nw.val[0]));

		// [l.z, l.w, -l.x, -l.y]
		float32x4_t l_zwxy = vcombine_f32(vget_high_f32(lhs), vget_low_f32(neg_lhs));

		// neg([l.w, -l.z, l.y, -l.x]) = [-l.w, l.z, -l.y, l.x]
		float32x4_t nw_z_ny_x = vnegq_f32(l_wzyx);

		// [-l.y, l.x, l.w, -l.z]
		float32x4_t l_yxwz = vcombine_f32(vget_high_f32(nw_z_ny_x), vget_low_f32(l_wzyx));

		const float32x2_t r_xy = vget_low_f32(rhs);
		const float32x2_t r_zw = vget_high_f32(rhs);

		const float32x4_t lxrw_lyrw_lzrw_lwrw = vmulq_lane_f32(lhs, r_zw, 1);

	#if defined(RTM_NEON64_INTRINSICS)
		const float32x4_t result0 = vfmaq_lane_f32(lxrw_lyrw_lzrw_lwrw, l_wzyx, r_xy, 0);
		const float32x4_t result1 = vfmaq_lane_f32(result0, l_zwxy, r_xy, 1);
		return vfmaq_lane_f32(result1, l_yxwz, r_zw, 0);
	#else
		const float32x4_t result0 = vmlaq_lane_f32(lxrw_lyrw_lzrw_lwrw, l_wzyx, r_xy, 0);
		const float32x4_t result1 = vmlaq_lane_f32(result0, l_zwxy, r_xy, 1);
		return vmlaq_lane_f32(result1, l_yxwz, r_zw, 0);
	#endif
#else
		// On ARMv7, the scalar version is often the fastest.
		const float lhs_x = quat_get_x(lhs);
		const float lhs_y = quat_get_y(lhs);
		const float lhs_z = quat_get_z(lhs);
		const float lhs_w = quat_get_w(lhs);

		const float rhs_x = quat_get_x(rhs);
		const float rhs_y = quat_get_y(rhs);
		const float rhs_z = quat_get_z(rhs);
		const float rhs_w = quat_get_w(rhs);

		const float x = (rhs_w * lhs_x) + (rhs_x * lhs_w) + (rhs_y * lhs_z) - (rhs_z * lhs_y);
		const float y = (rhs_w * lhs_y) - (rhs_x * lhs_z) + (rhs_y * lhs_w) + (rhs_z * lhs_x);
		const float z = (rhs_w * lhs_z) + (rhs_x * lhs_y) - (rhs_y * lhs_x) + (rhs_z * lhs_w);
		const float w = (rhs_w * lhs_w) - (rhs_x * lhs_x) - (rhs_y * lhs_y) - (rhs_z * lhs_z);

		return quat_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Multiplies a quaternion and a 3D vector, rotating it.
	// Multiplication order is as follow: world_position = quat_mul_vector3(local_vector, local_to_world)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL quat_mul_vector3(vector4f_arg0 vector, quatf_arg1 rotation) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 inv_rotation = quat_conjugate(rotation);

		// Normally when we multiply our inverse rotation quaternion with the input vector as a quaternion with W = 0.0.
		// As a result, we can strip the whole part that uses W saving a few instructions.
		// Because we have the rotation and its inverse, we also can use them to avoid flipping the signs
		// when lining up our SIMD additions. For the first quaternion multiplication, we can avoid 3 XORs by
		// doing 2 shuffles instead. The same trick can also be used with the second quaternion multiplication.
		// We also don't care about the result W lane but it comes for free.

		// temp = quat_mul(inv_rotation, vector_quat)
		__m128 temp;
		{
			const __m128 rotation_tmp0 = _mm_shuffle_ps(rotation, inv_rotation, _MM_SHUFFLE(3, 0, 2, 1));		// r.y, r.z, -r.x, -r.w
			const __m128 rotation_tmp1 = _mm_shuffle_ps(rotation, inv_rotation, _MM_SHUFFLE(3, 1, 2, 0));		// r.x, r.z, -r.y, -r.w

			const __m128 v_xxxx = _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(0, 0, 0, 0));
			const __m128 v_yyyy = _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(1, 1, 1, 1));
			const __m128 v_zzzz = _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(2, 2, 2, 2));

			const __m128 rotation_tmp2 = _mm_shuffle_ps(rotation_tmp0, rotation_tmp1, _MM_SHUFFLE(0, 2, 1, 3));	// -r.w, r.z, -r.y, r.x
			const __m128 lwrx_lzrx_lyrx_lxrx = _mm_mul_ps(v_xxxx, rotation_tmp2);

			const __m128 rotation_tmp3 = _mm_shuffle_ps(inv_rotation, rotation, _MM_SHUFFLE(1, 0, 3, 2));		// -r.z, -r.w, r.x, r.y
			const __m128 lzry_lwry_lxry_lyry = _mm_mul_ps(v_yyyy, rotation_tmp3);

			const __m128 rotation_tmp4 = _mm_shuffle_ps(rotation_tmp0, rotation_tmp1, _MM_SHUFFLE(1, 3, 2, 0));	// r.y, -r.x, -r.w, r.z
			const __m128 lyrz_lxrz_lwrz_lzrz = _mm_mul_ps(v_zzzz, rotation_tmp4);

			temp = _mm_add_ps(_mm_add_ps(lwrx_lzrx_lyrx_lxrx, lzry_lwry_lxry_lyry), lyrz_lxrz_lwrz_lzrz);
		}

		// result = quat_mul(temp, rotation)
		{
			const __m128 rotation_tmp0 = _mm_shuffle_ps(rotation, inv_rotation, _MM_SHUFFLE(2, 0, 2, 0));		// r.x, r.z, -r.x, -r.z

			__m128 r_xxxx = _mm_shuffle_ps(rotation_tmp0, rotation_tmp0, _MM_SHUFFLE(2, 0, 2, 0));				// r.x, -r.x, r.x, -r.x
			__m128 r_yyyy = _mm_shuffle_ps(rotation, inv_rotation, _MM_SHUFFLE(1, 1, 1, 1));					// r.y, r.y, -r.y, -r.y
			__m128 r_zzzz = _mm_shuffle_ps(rotation_tmp0, rotation_tmp0, _MM_SHUFFLE(3, 1, 1, 3));				// -r.z, r.z, r.z, -r.z
			__m128 r_wwww = _mm_shuffle_ps(rotation, rotation, _MM_SHUFFLE(3, 3, 3, 3));						// r.w, r.w, r.w, r.w

			__m128 lxrw_lyrw_lzrw_lwrw = _mm_mul_ps(r_wwww, temp);

			__m128 t_wzyx = _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(0, 1, 2, 3));
			__m128 lwrx_lzrx_lyrx_lxrx = _mm_mul_ps(r_xxxx, t_wzyx);

			__m128 t_zwxy = _mm_shuffle_ps(t_wzyx, t_wzyx, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 lzry_lwry_lxry_lyry = _mm_mul_ps(r_yyyy, t_zwxy);

			__m128 t_yxwz = _mm_shuffle_ps(t_zwxy, t_zwxy, _MM_SHUFFLE(0, 1, 2, 3));
			__m128 lyrz_lxrz_lwrz_lzrz = _mm_mul_ps(r_zzzz, t_yxwz);

			__m128 result0 = _mm_add_ps(lxrw_lyrw_lzrw_lwrw, lwrx_lzrx_lyrx_lxrx);
			__m128 result1 = _mm_add_ps(lzry_lwry_lxry_lyry, lyrz_lxrz_lwrz_lzrz);
			return _mm_add_ps(result0, result1);
		}
#elif defined(RTM_NEON_INTRINSICS)
		// On ARMv7 and ARM64, the scalar version is often the fastest.
		const float32x4_t n_rotation = vnegq_f32(rotation);

		// temp = quat_mul(inv_rotation, vector_quat)
		float temp_x;
		float temp_y;
		float temp_z;
		float temp_w;
		{
			const float lhs_x = quat_get_x(n_rotation);
			const float lhs_y = quat_get_y(n_rotation);
			const float lhs_z = quat_get_z(n_rotation);
			const float lhs_w = quat_get_w(rotation);

			const float rhs_x = quat_get_x(vector);
			const float rhs_y = quat_get_y(vector);
			const float rhs_z = quat_get_z(vector);

			temp_x = (rhs_x * lhs_w) + (rhs_y * lhs_z) - (rhs_z * lhs_y);
			temp_y = -(rhs_x * lhs_z) + (rhs_y * lhs_w) + (rhs_z * lhs_x);
			temp_z = (rhs_x * lhs_y) - (rhs_y * lhs_x) + (rhs_z * lhs_w);
			temp_w = -(rhs_x * lhs_x) - (rhs_y * lhs_y) - (rhs_z * lhs_z);
		}

		// result = quat_mul(temp, rotation)
		{
			const float lhs_x = temp_x;
			const float lhs_y = temp_y;
			const float lhs_z = temp_z;
			const float lhs_w = temp_w;

			const float rhs_x = quat_get_x(rotation);
			const float rhs_y = quat_get_y(rotation);
			const float rhs_z = quat_get_z(rotation);
			const float rhs_w = quat_get_w(rotation);

			const float x = (rhs_w * lhs_x) + (rhs_x * lhs_w) + (rhs_y * lhs_z) - (rhs_z * lhs_y);
			const float y = (rhs_w * lhs_y) - (rhs_x * lhs_z) + (rhs_y * lhs_w) + (rhs_z * lhs_x);
			const float z = (rhs_w * lhs_z) + (rhs_x * lhs_y) - (rhs_y * lhs_x) + (rhs_z * lhs_w);

			return vector_set(x, y, z, z);
		}
#else
		quatf vector_quat = quat_set_w(vector_to_quat(vector), 0.0f);
		quatf inv_rotation = quat_conjugate(rotation);
		return quat_to_vector(quat_mul(quat_mul(inv_rotation, vector_quat), rotation));
#endif
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_dot
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF));
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
				__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
				__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
				return _mm_cvtss_f32(x2y2z2w2_0_0_0);
#else
				return (quat_get_x(lhs) * quat_get_x(rhs)) + (quat_get_y(lhs) * quat_get_y(rhs)) + (quat_get_z(lhs) * quat_get_z(rhs)) + (quat_get_w(lhs) * quat_get_w(rhs));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return scalarf{ _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF)) };
#else
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
				__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
				__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
				return scalarf{ x2y2z2w2_0_0_0 };
#endif
			}
#endif

			quatf lhs;
			quatf rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Quaternion dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_dot RTM_SIMD_CALL quat_dot(quatf_arg0 lhs, quatf_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_dot{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// Quaternion dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_dot_as_scalar(quatf_arg0 lhs, quatf_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
	#if defined(RTM_SSE4_INTRINSICS) && 0
		// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
		return scalarf{ _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF)) };
	#else
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
		__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
		__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
		return scalarf{ x2y2z2w2_0_0_0 };
	#endif
#else
		return quat_dot(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_dot RTM_SIMD_CALL quat_length_squared(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_dot{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_length_squared_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return quat_dot_as_scalar(input, input);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_length
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = quat_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = quat_length_squared_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_length RTM_SIMD_CALL quat_length(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_length{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_length_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = quat_length_squared_as_scalar(input);
		return scalar_sqrt(len_sq);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct quatf_quat_length_reciprocal
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = quat_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = quat_length_squared_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			quatf input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::quatf_quat_length_reciprocal RTM_SIMD_CALL quat_length_reciprocal(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::quatf_quat_length_reciprocal{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL quat_length_reciprocal_as_scalar(quatf_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = quat_length_squared_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized quaternion.
	// Note that if the input quaternion is invalid (pure zero or with NaN/Inf),
	// the result is undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_normalize(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// We first calculate the dot product to get the length squared: dot(input, input)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(input, input);
		__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
		__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
		__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);

		// Keep the dot product result as a scalar within the first lane, it is faster to
		// calculate the reciprocal square root of a single lane VS all 4 lanes
		__m128 dot = x2y2z2w2_0_0_0;

		// Calculate the reciprocal square root to get the inverse length of our vector
		// Perform two passes of Newton-Raphson iteration on the hardware estimate
		__m128 half = _mm_set_ss(0.5F);
		__m128 input_half_v = _mm_mul_ss(dot, half);
		__m128 x0 = _mm_rsqrt_ss(dot);

		// First iteration
		__m128 x1 = _mm_mul_ss(x0, x0);
		x1 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x1));
		x1 = _mm_add_ss(_mm_mul_ss(x0, x1), x0);

		// Second iteration
		__m128 x2 = _mm_mul_ss(x1, x1);
		x2 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x2));
		x2 = _mm_add_ss(_mm_mul_ss(x1, x2), x1);

		// Broadcast the vector length reciprocal to all 4 lanes in order to multiply it with the vector
		__m128 inv_len = _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(0, 0, 0, 0));

		// Multiply the rotation by it's inverse length in order to normalize it
		return _mm_mul_ps(input, inv_len);
#elif defined (RTM_NEON_INTRINSICS)
		// Use sqrt/div/mul to normalize because the sqrt/div are faster than rsqrt
		float inv_len = 1.0F / scalar_sqrt(vector_length_squared(input));
		return vector_mul(input, inv_len);
#else
		// Reciprocal is more accurate to normalize with
		float inv_len = quat_length_reciprocal(input);
		return vector_to_quat(vector_mul(quat_to_vector(input), inv_len));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized quaternion using a deterministic algorithm.
	// This ensures that for a given input, the output will be identical on all
	// platforms that implement IEEE-754. This can be slower than `quat_normalize`.
	// This is only guaranteed if the rounding modes are consistent.
	// Note that if the input quaternion is invalid (pure zero or with NaN/Inf),
	// the result is undefined.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_normalize_deterministic(quatf_arg0 input) RTM_NO_EXCEPT
	{
		vector4f inputv = quat_to_vector(input);

		// Multiply once and retrieve floats, can't use scalarf because we need to use volatile
		// volatile will force a roundtrip to memory and should prevent re-ordering as well as
		// other optimizations such as FMA.
		vector4f input_sq = vector_mul(inputv, inputv);
		volatile float x_sq = vector_get_x(input_sq);
		volatile float y_sq = vector_get_y(input_sq);
		volatile float z_sq = vector_get_z(input_sq);
		volatile float w_sq = vector_get_w(input_sq);
		volatile float sum_xy_sq = x_sq + y_sq;
		volatile float sum_zw_sq = z_sq + w_sq;
		float len_sq = sum_xy_sq + sum_zw_sq;

		// sqrt(float) might not be exact when fast math or similar optimizations are enabled
		// We might not be able to disable these
		// As such, we promote to a double to ensure a deterministic result
		// We add volatile to ensure rsqrt or similar isn't used
		volatile double len = scalar_sqrt(double(len_sq));
		return vector_to_quat(vector_div(inputv, vector_set(float(len))));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation between start and end for a given alpha value.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when 'alpha' is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// Note that if 'start' and 'end' are on the opposite ends of the hypersphere, 'end' is negated
	// before we interpolate. As such, when 'alpha' is 1.0, either 'end' or its negated equivalent
	// is returned. Furthermore, if 'start' and 'end' aren't exactly normalized, the result might
	// not match exactly when 'alpha' is 0.0 or 1.0 because we normalize the resulting quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_lerp(quatf_arg0 start, quatf_arg1 end, float alpha) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Calculate the vector4 dot product: dot(start, end)
		__m128 dot;
#if defined(RTM_SSE4_INTRINSICS)
		// If both rotations are on opposite ends of the hypersphere, the result will be
		// very negative. If we are on the edge, the rotations are nearly opposite but not quite which
		// means that the linear interpolation here will have terrible accuracy to begin with. It is designed
		// for interpolating rotations that are reasonably close together. The bias check is mainly necessary
		// because the W component is often kept positive which flips the sign.
		// Using the dpps instruction reduces the number of registers that we need and helps the function get
		// inlined.
		dot = _mm_dp_ps(start, end, 0xFF);
#else
		{
			__m128 x2_y2_z2_w2 = _mm_mul_ps(start, end);
			__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
			__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
			__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
			// Shuffle the dot product to all SIMD lanes, there is no _mm_and_ss and loading
			// the constant from memory with the 'and' instruction is faster, it uses fewer registers
			// and fewer instructions
			dot = _mm_shuffle_ps(x2y2z2w2_0_0_0, x2y2z2w2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
		}
#endif

		// Calculate the bias, if the dot product is positive or zero, there is no bias
		// but if it is negative, we want to flip the 'end' rotation XYZW components
		__m128 bias = _mm_and_ps(dot, _mm_set_ps1(-0.0F));

		// Lerp the rotation after applying the bias
		// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
		__m128 alpha_ = _mm_set_ps1(alpha);
		__m128 interpolated_rotation = _mm_add_ps(_mm_sub_ps(start, _mm_mul_ps(alpha_, start)), _mm_mul_ps(alpha_, _mm_xor_ps(end, bias)));

		// Now we need to normalize the resulting rotation. We first calculate the
		// dot product to get the length squared: dot(interpolated_rotation, interpolated_rotation)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(interpolated_rotation, interpolated_rotation);
		__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
		__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
		__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);

		// Keep the dot product result as a scalar within the first lane, it is faster to
		// calculate the reciprocal square root of a single lane VS all 4 lanes
		dot = x2y2z2w2_0_0_0;

		// Calculate the reciprocal square root to get the inverse length of our vector
		// Perform two passes of Newton-Raphson iteration on the hardware estimate
		__m128 half = _mm_set_ss(0.5F);
		__m128 input_half_v = _mm_mul_ss(dot, half);
		__m128 x0 = _mm_rsqrt_ss(dot);

		// First iteration
		__m128 x1 = _mm_mul_ss(x0, x0);
		x1 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x1));
		x1 = _mm_add_ss(_mm_mul_ss(x0, x1), x0);

		// Second iteration
		__m128 x2 = _mm_mul_ss(x1, x1);
		x2 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x2));
		x2 = _mm_add_ss(_mm_mul_ss(x1, x2), x1);

		// Broadcast the vector length reciprocal to all 4 lanes in order to multiply it with the vector
		__m128 inv_len = _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(0, 0, 0, 0));

		// Multiply the rotation by it's inverse length in order to normalize it
		return _mm_mul_ps(interpolated_rotation, inv_len);
#elif defined (RTM_NEON64_INTRINSICS)
		// On ARM64 with NEON, we load 1.0 once and use it twice which is faster than
		// using a AND/XOR with the bias (same number of instructions)
		float dot = vector_dot(start, end);
		float bias = dot >= 0.0F ? 1.0F : -1.0F;
		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4f interpolated_rotation = vector_mul_add(vector_mul(end, bias), alpha, vector_neg_mul_sub(start, alpha, start));
		// Use sqrt/div/mul to normalize because the sqrt/div are faster than rsqrt
		float inv_len = 1.0F / scalar_sqrt(vector_length_squared(interpolated_rotation));
		return vector_mul(interpolated_rotation, inv_len);
#elif defined(RTM_NEON_INTRINSICS)
		// Calculate the vector4 dot product: dot(start, end)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(start, end);
		float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
		float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
		float32x2_t x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
		float32x2_t x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);

		// Calculate the bias, if the dot product is positive or zero, there is no bias
		// but if it is negative, we want to flip the 'end' rotation XYZW components
		// On ARM-v7-A, the AND/XOR trick is faster than the cmp/fsel
		uint32x2_t bias = vand_u32(vreinterpret_u32_f32(x2y2z2w2), vdup_n_u32(0x80000000));

		// Lerp the rotation after applying the bias
		// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
		float32x4_t end_biased = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(end), vcombine_u32(bias, bias)));
		float32x4_t interpolated_rotation = vmlaq_n_f32(vmlsq_n_f32(start, start, alpha), end_biased, alpha);

		// Now we need to normalize the resulting rotation. We first calculate the
		// dot product to get the length squared: dot(interpolated_rotation, interpolated_rotation)
		x2_y2_z2_w2 = vmulq_f32(interpolated_rotation, interpolated_rotation);
		x2_y2 = vget_low_f32(x2_y2_z2_w2);
		z2_w2 = vget_high_f32(x2_y2_z2_w2);
		x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
		x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);

		float dot = vget_lane_f32(x2y2z2w2, 0);

		// Use sqrt/div/mul to normalize because the sqrt/div are faster than rsqrt
		float inv_len = 1.0F / scalar_sqrt(dot);
		return vector_mul(interpolated_rotation, inv_len);
#else
		// To ensure we take the shortest path, we apply a bias if the dot product is negative
		vector4f start_vector = quat_to_vector(start);
		vector4f end_vector = quat_to_vector(end);
		float dot = vector_dot(start_vector, end_vector);
		float bias = dot >= 0.0F ? 1.0F : -1.0F;
		// ((1.0 - alpha) * start) + (alpha * (end * bias)) == (start - alpha * start) + (alpha * (end * bias))
		vector4f interpolated_rotation = vector_mul_add(vector_mul(end_vector, bias), alpha, vector_neg_mul_sub(start_vector, alpha, start_vector));
		return quat_normalize(vector_to_quat(interpolated_rotation));
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the linear interpolation between start and end for a given alpha value.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when 'alpha' is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	// Note that if 'start' and 'end' are on the opposite ends of the hypersphere, 'end' is negated
	// before we interpolate. As such, when 'alpha' is 1.0, either 'end' or its negated equivalent
	// is returned. Furthermore, if 'start' and 'end' aren't exactly normalized, the result might
	// not match exactly when 'alpha' is 0.0 or 1.0 because we normalize the resulting quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_lerp(quatf_arg0 start, quatf_arg1 end, scalarf_arg2 alpha) RTM_NO_EXCEPT
	{
		// Calculate the vector4 dot product: dot(start, end)
		__m128 dot;
#if defined(RTM_SSE4_INTRINSICS)
		// If both rotations are on opposite ends of the hypersphere, the result will be
		// very negative. If we are on the edge, the rotations are nearly opposite but not quite which
		// means that the linear interpolation here will have terrible accuracy to begin with. It is designed
		// for interpolating rotations that are reasonably close together. The bias check is mainly necessary
		// because the W component is often kept positive which flips the sign.
		// Using the dpps instruction reduces the number of registers that we need and helps the function get
		// inlined.
		dot = _mm_dp_ps(start, end, 0xFF);
#else
		{
			__m128 x2_y2_z2_w2 = _mm_mul_ps(start, end);
			__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
			__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
			__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
			__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
			// Shuffle the dot product to all SIMD lanes, there is no _mm_and_ss and loading
			// the constant from memory with the 'and' instruction is faster, it uses fewer registers
			// and fewer instructions
			dot = _mm_shuffle_ps(x2y2z2w2_0_0_0, x2y2z2w2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
		}
#endif

		// Calculate the bias, if the dot product is positive or zero, there is no bias
		// but if it is negative, we want to flip the 'end' rotation XYZW components
		__m128 bias = _mm_and_ps(dot, _mm_set_ps1(-0.0F));

		// Lerp the rotation after applying the bias
		// ((1.0 - alpha) * start) + (alpha * (end ^ bias)) == (start - alpha * start) + (alpha * (end ^ bias))
		__m128 alpha_ = _mm_shuffle_ps(alpha.value, alpha.value, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 interpolated_rotation = _mm_add_ps(_mm_sub_ps(start, _mm_mul_ps(alpha_, start)), _mm_mul_ps(alpha_, _mm_xor_ps(end, bias)));

		// Now we need to normalize the resulting rotation. We first calculate the
		// dot product to get the length squared: dot(interpolated_rotation, interpolated_rotation)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(interpolated_rotation, interpolated_rotation);
		__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
		__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
		__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);

		// Keep the dot product result as a scalar within the first lane, it is faster to
		// calculate the reciprocal square root of a single lane VS all 4 lanes
		dot = x2y2z2w2_0_0_0;

		// Calculate the reciprocal square root to get the inverse length of our vector
		// Perform two passes of Newton-Raphson iteration on the hardware estimate
		__m128 half = _mm_set_ss(0.5F);
		__m128 input_half_v = _mm_mul_ss(dot, half);
		__m128 x0 = _mm_rsqrt_ss(dot);

		// First iteration
		__m128 x1 = _mm_mul_ss(x0, x0);
		x1 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x1));
		x1 = _mm_add_ss(_mm_mul_ss(x0, x1), x0);

		// Second iteration
		__m128 x2 = _mm_mul_ss(x1, x1);
		x2 = _mm_sub_ss(half, _mm_mul_ss(input_half_v, x2));
		x2 = _mm_add_ss(_mm_mul_ss(x1, x2), x1);

		// Broadcast the vector length reciprocal to all 4 lanes in order to multiply it with the vector
		__m128 inv_len = _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(0, 0, 0, 0));

		// Multiply the rotation by it's inverse length in order to normalize it
		return _mm_mul_ps(interpolated_rotation, inv_len);
	}
#endif

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Returns the spherical interpolation between start and end for a given alpha value.
	// See: https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	// Perhaps try this someday: http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_slerp(quatf_arg0 start, quatf_arg1 end, scalarf_arg2 alpha) RTM_NO_EXCEPT
	{
		vector4f start_v = quat_to_vector(start);
		vector4f end_v = quat_to_vector(end);

		vector4f cos_half_angle_v = vector_dot_as_vector(start_v, end_v);
		mask4f is_angle_negative = vector_less_than(cos_half_angle_v, vector_zero());

		// If the two input quaternions aren't on the same half of the hypersphere, flip one and the angle sign
		end_v = vector_select(is_angle_negative, vector_neg(end_v), end_v);
		cos_half_angle_v = vector_select(is_angle_negative, vector_neg(cos_half_angle_v), cos_half_angle_v);

		// Clamp our half angle cosine
		cos_half_angle_v = vector_clamp(cos_half_angle_v, vector_set(-1.0F), vector_set(1.0F));

		scalarf cos_half_angle = vector_get_x_as_scalar(cos_half_angle_v);
		scalarf half_angle = scalar_acos(cos_half_angle);
		scalarf sin_half_angle = scalar_sqrt(scalar_sub(scalar_set(1.0F), scalar_mul(cos_half_angle, cos_half_angle)));
		scalarf inv_sin_half_angle = scalar_reciprocal(sin_half_angle);

		scalarf start_contribution_angle = scalar_mul(scalar_sub(scalar_set(1.0F), alpha), half_angle);
		scalarf end_contribution_angle = scalar_mul(alpha, half_angle);
		vector4f contribution_angles = vector_set(start_contribution_angle, end_contribution_angle, start_contribution_angle, end_contribution_angle);
		vector4f contributions = vector_mul(vector_sin(contribution_angles), inv_sin_half_angle);
		vector4f start_contribution = vector_dup_x(contributions);
		vector4f end_contribution = vector_dup_y(contributions);

		vector4f result = vector_add(vector_mul(start_v, start_contribution), vector_mul(end_v, end_contribution));
		return vector_to_quat(result);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the spherical interpolation between start and end for a given alpha value.
	// See: https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	// Perhaps try this someday: http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_slerp(quatf_arg0 start, quatf_arg1 end, float alpha) RTM_NO_EXCEPT
	{
		vector4f start_v = quat_to_vector(start);
		vector4f end_v = quat_to_vector(end);
		scalarf alpha_s = scalar_set(alpha);

		vector4f cos_half_angle_v = vector_dot_as_vector(start_v, end_v);
		mask4f is_angle_negative = vector_less_than(cos_half_angle_v, vector_zero());

		// If the two input quaternions aren't on the same half of the hypersphere, flip one and the angle sign
		end_v = vector_select(is_angle_negative, vector_neg(end_v), end_v);
		cos_half_angle_v = vector_select(is_angle_negative, vector_neg(cos_half_angle_v), cos_half_angle_v);

		// Clamp our half angle cosine
		cos_half_angle_v = vector_clamp(cos_half_angle_v, vector_set(-1.0F), vector_set(1.0F));

		scalarf cos_half_angle = vector_get_x_as_scalar(cos_half_angle_v);
		scalarf half_angle = scalar_acos(cos_half_angle);
		scalarf sin_half_angle = scalar_sqrt(scalar_sub(scalar_set(1.0F), scalar_mul(cos_half_angle, cos_half_angle)));
		scalarf inv_sin_half_angle = scalar_reciprocal(sin_half_angle);

		scalarf start_contribution_angle = scalar_mul(scalar_sub(scalar_set(1.0F), alpha_s), half_angle);
		scalarf end_contribution_angle = scalar_mul(alpha_s, half_angle);
		vector4f contribution_angles = vector_set(start_contribution_angle, end_contribution_angle, start_contribution_angle, end_contribution_angle);
		vector4f contributions = vector_mul(vector_sin(contribution_angles), inv_sin_half_angle);
		vector4f start_contribution = vector_dup_x(contributions);
		vector4f end_contribution = vector_dup_y(contributions);

		vector4f result = vector_add(vector_mul(start_v, start_contribution), vector_mul(end_v, end_contribution));
		return vector_to_quat(result);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a component wise negated quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_neg(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 signs = RTM_VECTOR4F_MAKE(-0.0F, -0.0F, -0.0F, -0.0F);
		return _mm_xor_ps(input, signs);
#elif defined(RTM_NEON_INTRINSICS)
		return vnegq_f32(input);
#else
		return vector_to_quat(vector_mul(quat_to_vector(input), -1.0F));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion logarithm for a 3D rotation.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_rotation_log(quatf_arg0 input) RTM_NO_EXCEPT
	{
		// The logarithm equation is as follow:
		// log(q).xyz = (q.xyz / ||q.xyz||) * acos(q.w / ||q||)
		// log(q).w = ln(||q||)
		//
		// If our quaternion is normalized (a rotation), our output W is always 0.0 since ln(1) = 0.0
		// Our XYZ simplifies as well down to: normalize(q.xyz) * acos(q.w)
		// The length of our input XYZ can be calculated either as sqrt(dot(q.xyz, q.xyz)) or
		// by taking the sine of the quaternion half angle with sin(acos(q.w))
		//
		// If our input rotation is near the identity, we cannot calculate the output XYZ since we would
		// divide by zero. If this happens we return a quaternion near zero which should reconstruct as the
		// identity with quat_rotation_exp(..).
		//
		// If our quaternion isn't normalized, more math is required

		const vector4f input_v = quat_to_vector(input);
		const scalarf input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0F), scalar_set(1.0F));
		const scalarf half_angle = scalar_acos(input_w);
		const scalarf xyz_inv_len = vector_length_reciprocal3_as_scalar(input_v);
		vector4f result_xyz = vector_mul(input_v, scalar_mul(xyz_inv_len, half_angle));

		// If we are near the identity, xyz will be set to our input xyz which should be near zero
		// For a true identity input, we'll output zero
		const mask4f is_input_near_identity = vector_greater_than(vector_set(input_w), vector_set(1.0F - 1.0E-6F));
		result_xyz = vector_select(is_input_near_identity, input_v, result_xyz);

		return vector_to_quat(vector_set_w(result_xyz, 0.0F));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the quaternion exponential for a 3D rotation.
	// The result might not be fully normalized if the input is near zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_rotation_exp(quatf_arg0 input) RTM_NO_EXCEPT
	{
		// The exponential equation is as follow:
		// exp(q).xyz = exp(q.w) * (q.xyz / ||q.xyz||) * sin(||q.xyz||)
		// exp(q).w = exp(q.w) * cos(||q.xyz||)
		//
		// If our output (normalized) quaternion is to represent a rotation, its logarithm passed as input has a W component equal to 0.0
		// Furthermore our input XYZ length is equal to our rotation half angle
		// Since exp(0.0) = 1.0, our equation simplifies as follow:
		// exp(q).xyz = (q.xyz / ||q.xyz||) * sin(||q.xyz||) = normalize(q.xyz) * sin(||q.xyz||)
		// exp(q).w = cos(||q.xyz||)
		//
		// If our half angle is 0.0, the whole input is 0,0,0,0 and the result is undefined since we can use any rotation axis.
		// When this happens, we return the approximated identity.
		// This is easily achieved because cos(||q.xyz||) = cos(0.0) = 1.0
		//
		// If our output quaternion does not represent a rotation, more math is required

		const vector4f input_v = quat_to_vector(input);
		const scalarf input_len = vector_length3_as_scalar(input_v);
		const vector4f input_len_v = vector_set(input_len);
		const vector4f input_normalized = vector_div(input_v, input_len_v);
		const vector4f sincos = scalar_sincos(input_len);

		vector4f result_xyz = vector_mul(input_normalized, vector_dup_x(sincos));

		// If we are near zero, xyz will be set to our input xyz which should be near zero
		// For a true zero input, we'll output the identity
		const mask4f is_input_near_zero = vector_less_than(input_len_v, vector_set(1.0E-6F));
		result_xyz = vector_select(is_input_near_zero, input_v, result_xyz);

		const vector4f result_w = vector_dup_y(sincos);
		const vector4f result = vector_mix<mix4::x, mix4::y, mix4::z, mix4::d>(result_xyz, result_w);
		return vector_to_quat(result);
	}



	//////////////////////////////////////////////////////////////////////////
	// Conversion to/from axis/angle/euler
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation axis and rotation angle that make up the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void RTM_SIMD_CALL quat_to_axis_angle(quatf_arg0 input, vector4f& out_axis, float& out_angle) RTM_NO_EXCEPT
	{
		constexpr float epsilon = 1.0E-8F;
		constexpr float epsilon_squared = epsilon * epsilon;

		const scalarf input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0F), scalar_set(1.0F));
		out_angle = scalar_cast(scalar_acos(input_w)) * 2.0F;

		const float scale_sq = scalar_max(1.0F - quat_get_w(input) * quat_get_w(input), 0.0F);
		out_axis = scale_sq >= epsilon_squared ? vector_mul(quat_to_vector(input), vector_set(scalar_sqrt_reciprocal(scale_sq))) : vector_set(1.0F, 0.0F, 0.0F);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation axis part of the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL quat_get_axis(quatf_arg0 input) RTM_NO_EXCEPT
	{
		constexpr float epsilon = 1.0E-8F;
		constexpr float epsilon_squared = epsilon * epsilon;

		const float scale_sq = scalar_max(1.0F - quat_get_w(input) * quat_get_w(input), 0.0F);
		return scale_sq >= epsilon_squared ? vector_mul(quat_to_vector(input), vector_set(scalar_sqrt_reciprocal(scale_sq))) : vector_set(1.0F, 0.0F, 0.0F);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the rotation angle part of the input quaternion.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline float RTM_SIMD_CALL quat_get_angle(quatf_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf input_w = scalar_clamp(quat_get_w_as_scalar(input), scalar_set(-1.0F), scalar_set(1.0F));
		return scalar_cast(scalar_acos(input_w)) * 2.0F;
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from a rotation axis and a rotation angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_from_axis_angle(vector4f_arg0 axis, float angle) RTM_NO_EXCEPT
	{
		vector4f sincos_ = scalar_sincos(0.5F * angle);
		vector4f sin_ = vector_dup_x(sincos_);
		scalarf cos_ = vector_get_y_as_scalar(sincos_);

		return vector_to_quat(vector_set_w(vector_mul(sin_, axis), cos_));
	}

	//////////////////////////////////////////////////////////////////////////
	// Creates a quaternion from Euler Pitch/Yaw/Roll angles.
	// Pitch is around the Y axis (right)
	// Yaw is around the Z axis (up)
	// Roll is around the X axis (forward)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline quatf RTM_SIMD_CALL quat_from_euler(float pitch, float yaw, float roll) RTM_NO_EXCEPT
	{
		float sp;
		float sy;
		float sr;
		float cp;
		float cy;
		float cr;

		scalar_sincos(pitch * 0.5F, sp, cp);
		scalar_sincos(yaw * 0.5F, sy, cy);
		scalar_sincos(roll * 0.5F, sr, cr);

		return quat_set(cr * sp * sy - sr * cp * cy,
			-cr * sp * cy - sr * cp * sy,
			cr * cp * sy - sr * sp * cy,
			cr * cp * cy + sr * sp * sy);
	}



	//////////////////////////////////////////////////////////////////////////
	// Comparisons and masking
	//////////////////////////////////////////////////////////////////////////



	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion does not contain any NaN or Inf, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_is_finite(quatf_arg0 input) RTM_NO_EXCEPT
	{
		return v_test_xyzw_finite(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion is normalized, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_is_normalized(quatf_arg0 input, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		float length_squared = quat_length_squared(input);
		return scalar_abs(length_squared - 1.0F) <= threshold;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the two quaternions are equal component wise, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_are_equal(quatf_arg0 lhs, quatf_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vceqq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the two quaternions are nearly equal component wise, otherwise false.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL quat_near_equal(quatf_arg0 lhs, quatf_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_all_near_equal(quat_to_vector(lhs), quat_to_vector(rhs), threshold);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if the input quaternion is nearly equal to the identity quaternion
	// by comparing its rotation angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline bool RTM_SIMD_CALL quat_near_identity(quatf_arg0 input, float threshold_angle = 0.00284714461F) RTM_NO_EXCEPT
	{
		// Because of floating point precision, we cannot represent very small rotations.
		// The closest float to 1.0 that is not 1.0 itself yields:
		// acos(0.99999994f) * 2.0f  = 0.000690533954 rad
		//
		// An error threshold of 1.e-6f is used by default.
		// acos(1.f - 1.e-6f) * 2.0f = 0.00284714461 rad
		// acos(1.f - 1.e-7f) * 2.0f = 0.00097656250 rad
		//
		// We don't really care about the angle value itself, only if it's close to 0.
		// This will happen whenever quat.w is close to 1.0.
		// If the quat.w is close to -1.0, the angle will be near 2*PI which is close to
		// a negative 0 rotation. By forcing quat.w to be positive, we'll end up with
		// the shortest path.
		const scalarf input_w = quat_get_w_as_scalar(input);
		const scalarf input_abs_w = scalar_min(scalar_abs(input_w), scalar_set(1.0F));
		const float positive_w_angle = scalar_acos(scalar_cast(input_abs_w)) * 2.0F;
		return positive_w_angle <= threshold_angle;
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	// Note that if the mask lanes are not all identical, the resulting quaternion
	// may not be normalized.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE quatf RTM_SIMD_CALL quat_select(mask4f_arg0 mask, quatf_arg1 if_true, quatf_arg2 if_false) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS) || defined(RTM_NEON_INTRINSICS)
		return RTM_VECTOR4F_SELECT(mask, if_true, if_false);
#else
		return quatf{ rtm_impl::select(mask.x, if_true.x, if_false.x), rtm_impl::select(mask.y, if_true.y, if_false.y), rtm_impl::select(mask.z, if_true.z, if_false.z), rtm_impl::select(mask.w, if_true.w, if_false.w) };
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
