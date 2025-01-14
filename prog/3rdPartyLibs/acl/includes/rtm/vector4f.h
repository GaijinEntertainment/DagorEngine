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

#include "rtm/macros.h"
#include "rtm/math.h"
#include "rtm/scalarf.h"
#include "rtm/version.h"
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"
#include "rtm/impl/macros.mask4.impl.h"
#include "rtm/impl/memory_utils.h"
#include "rtm/impl/vector_common.h"

#include <cstring>
#include <limits>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Setters, getters, and casts
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector4 from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_loadu_ps(input);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_f32(input);
#else
		return vector_set(input[0], input[1], input[2], input[3]);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an input scalar from memory into the [x] component and sets the [yzw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load1(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_load_ss(input);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_lane_f32(input, vdupq_n_f32(0.0F), 0);
#else
		return vector_set(input[0], 0.0F, 0.0F, 0.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector2 from memory and sets the [zw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load2(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_castpd_ps(_mm_load_sd(rtm_impl::bit_cast<const double*>(input)));
#elif defined(RTM_NEON_INTRINSICS)
		const float32x2_t xy = vld1_f32(input);
		return vcombine_f32(xy, vdup_n_f32(0.0F));
#else
		return vector_set(input[0], input[1], 0.0F, 0.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector3 from memory and sets the [w] component to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load3(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 xy = _mm_castpd_ps(_mm_load_sd(rtm_impl::bit_cast<const double*>(input)));
		const __m128 z = _mm_load_ss(input + 2);
		return _mm_movelh_ps(xy, z);
#elif defined(RTM_NEON_INTRINSICS)
		const float32x2_t xy = vld1_f32(input);
		const float32x2_t z = vld1_lane_f32(input + 2, vdup_n_f32(0.0F), 0);
		return vcombine_f32(xy, z);
#else
		return vector_set(input[0], input[1], input[2], 0.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector4 from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load(const float4f* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_loadu_ps(&input->x);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_f32(&input->x);
#else
		return vector_set(input->x, input->y, input->z, input->w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector2 from memory and sets the [zw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load2(const float2f* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_castpd_ps(_mm_load_sd(rtm_impl::bit_cast<const double*>(&input->x)));
#elif defined(RTM_NEON_INTRINSICS)
		const float32x2_t xy = vld1_f32(&input->x);
		return vcombine_f32(xy, vdup_n_f32(0.0F));
#else
		return vector_set(input->x, input->y, 0.0F, 0.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector3 from memory and sets the [w] component to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_load3(const float3f* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 xy = _mm_castpd_ps(_mm_load_sd(rtm_impl::bit_cast<const double*>(&input->x)));
		const __m128 z = _mm_load_ss(&input->z);
		return _mm_movelh_ps(xy, z);
#elif defined(RTM_NEON_INTRINSICS)
		const float32x2_t xy = vld1_f32(&input->x);
		const float32x2_t z = vld1_lane_f32(&input->z, vdup_n_f32(0.0F), 0);
		return vcombine_f32(xy, z);
#else
		return vector_set(input->x, input->y, input->z, 0.0F);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an input scalar from memory into the [xyzw] components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_broadcast(const float* input) RTM_NO_EXCEPT
	{
#if defined(RTM_AVX_INTRINSICS)
		return _mm_broadcast_ss(input);
#elif defined(RTM_SSE2_INTRINSICS)
		return _mm_load_ps1(input);
#elif defined(RTM_NEON_INTRINSICS)
		return vld1q_dup_f32(input);
#else
		return vector_set(*input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a quaternion to a vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL quat_to_vector(quatf_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS) || defined(RTM_NEON_INTRINSICS)
		return input;
#else
		return vector4f{ input.x, input.y, input.z, input.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a vector4 float64 variant to a float32 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_cast(const vector4d& input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_shuffle_ps(_mm_cvtpd_ps(input.xy), _mm_cvtpd_ps(input.zw), _MM_SHUFFLE(1, 0, 1, 0));
#else
		return vector_set(float(input.x), float(input.y), float(input.z), float(input.w));
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
		struct vector4f_vector_get_x
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

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_x RTM_SIMD_CALL vector_get_x(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_x{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_x_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ input };
#else
		return vector_get_x(input);
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
		struct vector4f_vector_get_y
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

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_y RTM_SIMD_CALL vector_get_y(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_y{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_y_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(1, 1, 1, 1)) };
#else
		return vector_get_y(input);
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
		struct vector4f_vector_get_z
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

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_z RTM_SIMD_CALL vector_get_z(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_z{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_z_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(2, 2, 2, 2)) };
#else
		return vector_get_z(input);
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
		struct vector4f_vector_get_w
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

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_w RTM_SIMD_CALL vector_get_w(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_w{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_w_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalarf{ _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 3, 3, 3)) };
#else
		return vector_get_w(input);
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
		template<component4 component>
		struct vector4f_vector_get_component_static
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				switch (component)
				{
					default:
					case component4::x:	return vector_get_x(input);
					case component4::y:	return vector_get_y(input);
					case component4::z:	return vector_get_z(input);
					case component4::w:	return vector_get_w(input);
				}
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				switch (component)
				{
					default:
					case component4::x:	return vector_get_x_as_scalar(input);
					case component4::y:	return vector_get_y_as_scalar(input);
					case component4::z:	return vector_get_z_as_scalar(input);
					case component4::w:	return vector_get_w_as_scalar(input);
				}
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component, component4 component_ = static_cast<component4>(component)>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component_static<component_> RTM_SIMD_CALL vector_get_component2(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component_static<component_>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component2_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_get_x_as_scalar(input);
			case component2::y:	return vector_get_y_as_scalar(input);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component3 component, component4 component_ = static_cast<component4>(component)>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component_static<component_> RTM_SIMD_CALL vector_get_component3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component_static<component_>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component3 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component3_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_get_x_as_scalar(input);
			case component3::y:	return vector_get_y_as_scalar(input);
			case component3::z:	return vector_get_z_as_scalar(input);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component4 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component_static<component> RTM_SIMD_CALL vector_get_component(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component_static<component>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component4 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_get_x_as_scalar(input);
			case component4::y:	return vector_get_y_as_scalar(input);
			case component4::z:	return vector_get_z_as_scalar(input);
			case component4::w:	return vector_get_w_as_scalar(input);
		}
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_vector_get_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				switch (component)
				{
					default:
					case component4::x:	return vector_get_x(input);
					case component4::y:	return vector_get_y(input);
					case component4::z:	return vector_get_z(input);
					case component4::w:	return vector_get_w(input);
				}
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				switch (component)
				{
					default:
					case component4::x:	return vector_get_x_as_scalar(input);
					case component4::y:	return vector_get_y_as_scalar(input);
					case component4::z:	return vector_get_z_as_scalar(input);
					case component4::w:	return vector_get_w_as_scalar(input);
				}
			}
#endif

			vector4f input;
			component4 component;
			int32_t padding[3];
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component RTM_SIMD_CALL vector_get_component2(vector4f_arg0 input, component2 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component{ input, static_cast<component4>(component), { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component2_as_scalar(vector4f_arg0 input, component2 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_get_x_as_scalar(input);
			case component2::y:	return vector_get_y_as_scalar(input);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component RTM_SIMD_CALL vector_get_component3(vector4f_arg0 input, component3 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component{ input, static_cast<component4>(component), { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component3_as_scalar(vector4f_arg0 input, component3 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_get_x_as_scalar(input);
			case component3::y:	return vector_get_y_as_scalar(input);
			case component3::z:	return vector_get_z_as_scalar(input);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_get_component RTM_SIMD_CALL vector_get_component(vector4f_arg0 input, component4 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_get_component{ input, component, { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_component_as_scalar(vector4f_arg0 input, component4 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_get_x_as_scalar(input);
			case component4::y:	return vector_get_y_as_scalar(input);
			case component4::z:	return vector_get_z_as_scalar(input);
			case component4::w:	return vector_get_w_as_scalar(input);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_get_min_component RTM_SIMD_CALL vector_get_min_component(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_get_min_component{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_min_component_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128 zwzw = _mm_movehl_ps(input, input);
		__m128 xz_yw_zz_ww = _mm_min_ps(input, zwzw);
		__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
		return scalarf{ _mm_min_ps(xz_yw_zz_ww, yw_yw_yw_yw) };
#else
		return vector_get_min_component(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_get_max_component RTM_SIMD_CALL vector_get_max_component(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_get_max_component{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_get_max_component_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128 zwzw = _mm_movehl_ps(input, input);
		__m128 xz_yw_zz_ww = _mm_max_ps(input, zwzw);
		__m128 yw_yw_yw_yw = _mm_shuffle_ps(xz_yw_zz_ww, xz_yw_zz_ww, _MM_SHUFFLE(1, 1, 1, 1));
		return scalarf{ _mm_max_ps(xz_yw_zz_ww, yw_yw_yw_yw) };
#else
		return vector_get_max_component(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [x] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_x(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_move_ss(input, _mm_set_ss(lane_value));
#elif defined(RTM_NEON_INTRINSICS)
		return vsetq_lane_f32(lane_value, input, 0);
#else
		return vector4f{ lane_value, input.y, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [x] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_x(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
		return _mm_move_ss(input, lane_value.value);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [y] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_y(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
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
		return vector4f{ input.x, lane_value, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [y] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_y(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
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
	// Sets the vector4 [z] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_z(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
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
		return vector4f{ input.x, input.y, lane_value, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [z] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_z(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, lane_value.value, 0x20);
#else
		const __m128 yxzw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 0, 1, 2));
		const __m128 vxzw = _mm_move_ss(yxzw, lane_value.value);
		return _mm_shuffle_ps(vxzw, vxzw, _MM_SHUFFLE(3, 0, 1, 2));
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [w] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_w(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
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
		return vector4f{ input.x, input.y, input.z, lane_value };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [w] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_w(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_insert_ps(input, lane_value.value, 0x30);
#else
		const __m128 yxzw = _mm_shuffle_ps(input, input, _MM_SHUFFLE(0, 2, 1, 3));
		const __m128 vxzw = _mm_move_ss(yxzw, lane_value.value);
		return _mm_shuffle_ps(vxzw, vxzw, _MM_SHUFFLE(0, 2, 1, 3));
#endif
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector2 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component2(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_set_x(input, lane_value);
			case component2::y:	return vector_set_y(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector2 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component2(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_set_x(input, lane_value);
			case component2::y:	return vector_set_y(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector3 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component3 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component3(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_set_x(input, lane_value);
			case component3::y:	return vector_set_y(input, lane_value);
			case component3::z:	return vector_set_z(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector3 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component3 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component3(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_set_x(input, lane_value);
			case component3::y:	return vector_set_y(input, lane_value);
			case component3::z:	return vector_set_z(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector4 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component4 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component(vector4f_arg0 input, float lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_set_x(input, lane_value);
			case component4::y:	return vector_set_y(input, lane_value);
			case component4::z:	return vector_set_z(input, lane_value);
			case component4::w:	return vector_set_w(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector4 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component4 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component(vector4f_arg0 input, scalarf_arg1 lane_value) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_set_x(input, lane_value);
			case component4::y:	return vector_set_y(input, lane_value);
			case component4::z:	return vector_set_z(input, lane_value);
			case component4::w:	return vector_set_w(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector2 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component2(vector4f_arg0 input, float lane_value, component2 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_set_x(input, lane_value);
			case component2::y:	return vector_set_y(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector2 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component2(vector4f_arg0 input, scalarf_arg1 lane_value, component2 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component2::x:	return vector_set_x(input, lane_value);
			case component2::y:	return vector_set_y(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector3 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component3(vector4f_arg0 input, float lane_value, component3 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_set_x(input, lane_value);
			case component3::y:	return vector_set_y(input, lane_value);
			case component3::z:	return vector_set_z(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector3 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component3(vector4f_arg0 input, scalarf_arg1 lane_value, component3 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component3::x:	return vector_set_x(input, lane_value);
			case component3::y:	return vector_set_y(input, lane_value);
			case component3::z:	return vector_set_z(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector4 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component(vector4f_arg0 input, float lane_value, component4 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_set_x(input, lane_value);
			case component4::y:	return vector_set_y(input, lane_value);
			case component4::z:	return vector_set_z(input, lane_value);
			case component4::w:	return vector_set_w(input, lane_value);
		}
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector4 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_set_component(vector4f_arg0 input, scalarf_arg1 lane_value, component4 component) RTM_NO_EXCEPT
	{
		switch (component)
		{
			default:
			case component4::x:	return vector_set_x(input, lane_value);
			case component4::y:	return vector_set_y(input, lane_value);
			case component4::z:	return vector_set_z(input, lane_value);
			case component4::w:	return vector_set_w(input, lane_value);
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns a floating point pointer to the vector4 data.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE const float* RTM_SIMD_CALL vector_to_pointer(const vector4f& input) RTM_NO_EXCEPT
	{
		return rtm_impl::bit_cast<const float*>(&input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4f_arg0 input, float* output) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		_mm_storeu_ps(output, input);
#else
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
		output[2] = vector_get_z(input);
		output[3] = vector_get_w(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector1 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store1(vector4f_arg0 input, float* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4f_arg0 input, float* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4f_arg0 input, float* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
		output[2] = vector_get_z(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4f_arg0 input, uint8_t* output) RTM_NO_EXCEPT
	{
		std::memcpy(output, &input, sizeof(vector4f));
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector1 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store1(vector4f_arg0 input, uint8_t* output) RTM_NO_EXCEPT
	{
		std::memcpy(output, &input, sizeof(float) * 1);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4f_arg0 input, uint8_t* output) RTM_NO_EXCEPT
	{
		std::memcpy(output, &input, sizeof(float) * 2);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4f_arg0 input, uint8_t* output) RTM_NO_EXCEPT
	{
		std::memcpy(output, &input, sizeof(float) * 3);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4f_arg0 input, float4f* output) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		_mm_storeu_ps(&output->x, input);
#else
		output->x = vector_get_x(input);
		output->y = vector_get_y(input);
		output->z = vector_get_z(input);
		output->w = vector_get_w(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4f_arg0 input, float2f* output) RTM_NO_EXCEPT
	{
		output->x = vector_get_x(input);
		output->y = vector_get_y(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4f_arg0 input, float3f* output) RTM_NO_EXCEPT
	{
		output->x = vector_get_x(input);
		output->y = vector_get_y(input);
		output->z = vector_get_z(input);
	}



	//////////////////////////////////////////////////////////////////////////
	// Arithmetic
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Per component addition of the two inputs: lhs + rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_add(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_add_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vaddq_f32(lhs, rhs);
#else
		return vector_set(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component subtraction of the two inputs: lhs - rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_sub(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_sub_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vsubq_f32(lhs, rhs);
#else
		return vector_set(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the two inputs: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_mul_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vmulq_f32(lhs, rhs);
#else
		return vector_set(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the vector by a scalar: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul(vector4f_arg0 lhs, float rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_NEON_INTRINSICS)
		return vmulq_n_f32(lhs, rhs);
#else
		return vector_mul(lhs, vector_set(rhs));
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the vector by a scalar: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul(vector4f_arg0 lhs, scalarf_arg1 rhs) RTM_NO_EXCEPT
	{
		return _mm_mul_ps(lhs, _mm_shuffle_ps(rhs.value, rhs.value, _MM_SHUFFLE(0, 0, 0, 0)));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component division of the two inputs: lhs / rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_div(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_div_ps(lhs, rhs);
#elif defined (RTM_NEON64_INTRINSICS)
		return vdivq_f32(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		// Use scalar division on ARMv7, slow but accurate
		float x = vgetq_lane_f32(lhs, 0) / vgetq_lane_f32(rhs, 0);
		float y = vgetq_lane_f32(lhs, 1) / vgetq_lane_f32(rhs, 1);
		float z = vgetq_lane_f32(lhs, 2) / vgetq_lane_f32(rhs, 2);
		float w = vgetq_lane_f32(lhs, 3) / vgetq_lane_f32(rhs, 3);

		float32x4_t result = lhs;
		result = vsetq_lane_f32(x, result, 0);
		result = vsetq_lane_f32(y, result, 1);
		result = vsetq_lane_f32(z, result, 2);
		result = vsetq_lane_f32(w, result, 3);
		return result;
#else
		return vector_set(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component maximum of the two inputs: max(lhs, rhs)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_max(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_max_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vmaxq_f32(lhs, rhs);
#else
		return vector_set(scalar_max(lhs.x, rhs.x), scalar_max(lhs.y, rhs.y), scalar_max(lhs.z, rhs.z), scalar_max(lhs.w, rhs.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component minimum of the two inputs: min(lhs, rhs)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_min(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_min_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vminq_f32(lhs, rhs);
#else
		return vector_set(scalar_min(lhs.x, rhs.x), scalar_min(lhs.y, rhs.y), scalar_min(lhs.z, rhs.z), scalar_min(lhs.w, rhs.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component clamping of an input between a minimum and a maximum value: min(max_value, max(min_value, input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_clamp(vector4f_arg0 input, vector4f_arg1 min_value, vector4f_arg2 max_value) RTM_NO_EXCEPT
	{
		return vector_min(max_value, vector_max(min_value, input));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component absolute of the input: abs(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_abs(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		return _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
#elif defined(RTM_NEON_INTRINSICS)
		return vabsq_f32(input);
#else
		return vector_set(scalar_abs(input.x), scalar_abs(input.y), scalar_abs(input.z), scalar_abs(input.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negation of the input: -input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_neg(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 signs = RTM_VECTOR4F_MAKE(-0.0F, -0.0F, -0.0F, -0.0F);
		return _mm_xor_ps(input, signs);
#elif defined(RTM_NEON_INTRINSICS)
		return vnegq_f32(input);
#else
		return vector_mul(input, -1.0f);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negation of the input: -input
	// Each template argument controls whether a SIMD lane should be negated (true) or not (false).
	//////////////////////////////////////////////////////////////////////////
	template<bool x, bool y, bool z, bool w>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_neg(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 signs = RTM_VECTOR4F_MAKE(x ? -0.0F : 0.0F, y ? -0.0F : 0.0F, z ? -0.0F : 0.0F, w ? -0.0F : 0.0F);
		return _mm_xor_ps(input, signs);
#elif defined(RTM_NEON_INTRINSICS)
		alignas(16) constexpr uint32_t sign_bit_i[4] =
		{
			x ? 0x80000000U : 0,
			y ? 0x80000000U : 0,
			z ? 0x80000000U : 0,
			w ? 0x80000000U : 0,
		};
		const uint32x4_t sign_bit = *rtm_impl::bit_cast<const uint32x4_t*>(&sign_bit_i[0]);
		return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(input), sign_bit));
#else
		const vector4f signs = vector_set(x ? -1.0F : 1.0F, y ? -1.0F : 1.0F, z ? -1.0F : 1.0F, w ? -1.0F : 1.0F);
		return vector_mul(input, signs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component reciprocal of the input: 1.0 / input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_reciprocal(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// Performance note:
		// With modern out-of-order executing processors, it is typically faster to use
		// a full division instead of a reciprocal estimate + Newton-Raphson iterations
		// because the resulting code is more dense and is more likely to inline and
		// as it uses fewer instructions.
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_div_ps(_mm_set_ps1(1.0F), input);
#elif defined(RTM_NEON64_INTRINSICS)
		return vdivq_f32(vdupq_n_f32(1.0F), input);
#elif defined(RTM_NEON_INTRINSICS)
		// Perform two passes of Newton-Raphson iteration on the hardware estimate
		float32x4_t x0 = vrecpeq_f32(input);

		// First iteration
		float32x4_t x1 = vmulq_f32(x0, vrecpsq_f32(x0, input));

		// Second iteration
		float32x4_t x2 = vmulq_f32(x1, vrecpsq_f32(x1, input));
		return x2;
#else
		return vector_div(vector_set(1.0F), input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component square root of the input: sqrt(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_sqrt(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_sqrt_ps(input);
#elif defined(RTM_NEON64_INTRINSICS) && defined(RTM_IMPL_VSQRT_SUPPORTED)
		return vsqrtq_f32(input);
#else
		scalarf x = vector_get_x(input);
		scalarf y = vector_get_y(input);
		scalarf z = vector_get_z(input);
		scalarf w = vector_get_w(input);
		return vector_set(scalar_sqrt(x), scalar_sqrt(y), scalar_sqrt(z), scalar_sqrt(w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component reciprocal square root of the input: 1.0 / sqrt(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_sqrt_reciprocal(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// Performance note:
		// With modern out-of-order executing processors, it is typically faster to use
		// a full division/square root instead of a reciprocal estimate + Newton-Raphson iterations
		// because the resulting code is more dense and is more likely to inline and
		// as it uses fewer instructions.
		return vector_reciprocal(vector_sqrt(input));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component returns the smallest integer value not less than the input (round towards positive infinity).
	// vector_ceil([1.8, 1.0, -1.8, -1.0]) = [2.0, 1.0, -1.0, -1.0]
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_ceil(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_ceil_ps(input);
#elif defined(RTM_SSE2_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ps(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ps(input, input);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvtps_epi32(input));

		// Test if the returned value is smaller than the original.
		// A positive input will round towards zero and be lower when we need it to be greater.
		__m128 is_positive = _mm_cmplt_ps(integer_part, input);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Negative values will yield a 0.0 bias
		__m128 bias = _mm_cvtepi32_ps(_mm_castps_si128(is_positive));

		// Subtract our bias to properly handle positive values
		integer_part = _mm_sub_ps(integer_part, bias);

		return _mm_or_ps(_mm_and_ps(use_original_input, input), _mm_andnot_ps(use_original_input, integer_part));
#elif defined(RTM_NEON64_INTRINSICS)
		return vrndpq_f32(input);
#elif defined(RTM_NEON_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		float32x4_t fractional_limit = vdupq_n_f32(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		uint32x4_t is_input_large = vcageq_f32(input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		uint32x4_t is_nan = vmvnq_u32(vceqq_f32(input, input));

		// Combine our masks to determine if we should return the original value
		uint32x4_t use_original_input = vorrq_u32(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		float32x4_t integer_part = vcvtq_f32_s32(vcvtq_s32_f32(input));

		// Test if the returned value is smaller than the original.
		// A positive input will round towards zero and be lower when we need it to be greater.
		uint32x4_t is_positive = vcltq_f32(integer_part, input);

		float32x4_t bias = vcvtq_f32_s32(is_positive);

		// Subtract our bias to properly handle positive values
		integer_part = vsubq_f32(integer_part, bias);

		return vbslq_f32(use_original_input, input, integer_part);
#else
		return vector_set(scalar_ceil(vector_get_x(input)), scalar_ceil(vector_get_y(input)), scalar_ceil(vector_get_z(input)), scalar_ceil(vector_get_w(input)));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component returns the largest integer value not greater than the input (round towards negative infinity).
	// vector_floor([1.8, 1.0, -1.8, -1.0]) = [1.0, 1.0, -2.0, -1.0]
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_floor(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_floor_ps(input);
#elif defined(RTM_SSE2_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ps(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ps(input, input);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvtps_epi32(input));

		// Test if the returned value is greater than the original.
		// A negative input will round towards zero and be greater when we need it to be smaller.
		__m128 is_negative = _mm_cmpgt_ps(integer_part, input);

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Positive values will yield a 0.0 bias
		__m128 bias = _mm_cvtepi32_ps(_mm_castps_si128(is_negative));

		// Add our bias to properly handle negative values
		integer_part = _mm_add_ps(integer_part, bias);

		return _mm_or_ps(_mm_and_ps(use_original_input, input), _mm_andnot_ps(use_original_input, integer_part));
#elif defined(RTM_NEON64_INTRINSICS)
		return vrndmq_f32(input);
#elif defined(RTM_NEON_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		float32x4_t fractional_limit = vdupq_n_f32(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		uint32x4_t is_input_large = vcageq_f32(input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		uint32x4_t is_nan = vmvnq_u32(vceqq_f32(input, input));

		// Combine our masks to determine if we should return the original value
		uint32x4_t use_original_input = vorrq_u32(is_input_large, is_nan);

		// Convert to an integer and back. This does banker's rounding by default
		float32x4_t integer_part = vcvtq_f32_s32(vcvtq_s32_f32(input));

		// Test if the returned value is greater than the original.
		// A negative input will round towards zero and be greater when we need it to be smaller.
		uint32x4_t is_negative = vcgtq_f32(integer_part, input);

		float32x4_t bias = vcvtq_f32_s32(is_negative);

		// Add our bias to properly handle negative values
		integer_part = vaddq_f32(integer_part, bias);

		return vbslq_f32(use_original_input, input, integer_part);
#else
		return vector_set(scalar_floor(vector_get_x(input)), scalar_floor(vector_get_y(input)), scalar_floor(vector_get_z(input)), scalar_floor(vector_get_w(input)));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D cross product: lhs x rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_cross3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// cross(a, b).zxy = (a * b.yzx) - (a.yzx * b)
		__m128 lhs_yzx = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 rhs_yzx = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 tmp_zxy = _mm_sub_ps(_mm_mul_ps(lhs, rhs_yzx), _mm_mul_ps(lhs_yzx, rhs));

		// cross(a, b) = ((a * b.yzx) - (a.yzx * b)).yzx
		return _mm_shuffle_ps(tmp_zxy, tmp_zxy, _MM_SHUFFLE(3, 0, 2, 1));
#elif defined(RTM_NEON_INTRINSICS)
		// cross(a, b) = (a.yzx * b.zxy) - (a.zxy * b.yzx)
		float32x4_t lhs_yzwx = vextq_f32(lhs, lhs, 1);
		float32x4_t rhs_wxyz = vextq_f32(rhs, rhs, 3);

		float32x4_t lhs_yzx = vsetq_lane_f32(vgetq_lane_f32(lhs, 0), lhs_yzwx, 2);
		float32x4_t rhs_zxy = vsetq_lane_f32(vgetq_lane_f32(rhs, 2), rhs_wxyz, 0);

		// part_a = (a.yzx * b.zxy)
		float32x4_t part_a = vmulq_f32(lhs_yzx, rhs_zxy);

		float32x4_t lhs_wxyz = vextq_f32(lhs, lhs, 3);
		float32x4_t rhs_yzwx = vextq_f32(rhs, rhs, 1);
		float32x4_t lhs_zxy = vsetq_lane_f32(vgetq_lane_f32(lhs, 2), lhs_wxyz, 0);
		float32x4_t rhs_yzx = vsetq_lane_f32(vgetq_lane_f32(rhs, 0), rhs_yzwx, 2);

		return vmlsq_f32(part_a, lhs_zxy, rhs_yzx);
#else
		// cross(a, b) = (a.yzx * b.zxy) - (a.zxy * b.yzx)
		const float lhs_x = vector_get_x(lhs);
		const float lhs_y = vector_get_y(lhs);
		const float lhs_z = vector_get_z(lhs);
		const float rhs_x = vector_get_x(rhs);
		const float rhs_y = vector_get_y(rhs);
		const float rhs_z = vector_get_z(rhs);
		return vector_set((lhs_y * rhs_z) - (lhs_z * rhs_y), (lhs_z * rhs_x) - (lhs_x * rhs_z), (lhs_x * rhs_y) - (lhs_y * rhs_x));
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
		struct vector4f_vector_dot
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
#elif defined(RTM_NEON64_INTRINSICS) && defined(RTM_IMPL_VADDVQ_SUPPORTED)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				return vaddvq_f32(x2_y2_z2_w2);
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
				float32x2_t x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
				float32x2_t x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);
				return vget_lane_f32(x2y2z2w2, 0);
#else
				return (vector_get_x(lhs) * vector_get_x(rhs)) + (vector_get_y(lhs) * vector_get_y(rhs)) + (vector_get_z(lhs) * vector_get_z(rhs)) + (vector_get_w(lhs) * vector_get_w(rhs));
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

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_dp_ps(lhs, rhs, 0xFF);
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
				__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
				__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
				return _mm_shuffle_ps(x2y2z2w2_0_0_0, x2y2z2w2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON64_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x4_t x2y2_z2w2_x2y2_z2w2 = vpaddq_f32(x2_y2_z2_w2, x2_y2_z2_w2);
				float32x4_t x2y2z2w2_x2y2z2w2_x2y2z2w2_x2y2z2w2 = vpaddq_f32(x2y2_z2w2_x2y2_z2w2, x2y2_z2w2_x2y2_z2w2);
				return x2y2z2w2_x2y2z2w2_x2y2z2w2_x2y2z2w2;
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
				float32x2_t x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
				float32x2_t x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);
				return vcombine_f32(x2y2z2w2, x2y2z2w2);
#else
				scalarf result = *this;
				return vector_set(result);
#endif
			}

			vector4f lhs;
			vector4f rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot RTM_SIMD_CALL vector_dot(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_dot_as_scalar(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
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
	return vector_dot(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dot_as_vector(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS) && 0
		// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
		return _mm_dp_ps(lhs, rhs, 0xFF);
#elif defined(RTM_SSE2_INTRINSICS)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 z2_w2_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 3, 2));
		__m128 x2z2_y2w2_0_0 = _mm_add_ps(x2_y2_z2_w2, z2_w2_0_0);
		__m128 y2w2_0_0_0 = _mm_shuffle_ps(x2z2_y2w2_0_0, x2z2_y2w2_0_0, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2z2w2_0_0_0 = _mm_add_ps(x2z2_y2w2_0_0, y2w2_0_0_0);
		return _mm_shuffle_ps(x2y2z2w2_0_0_0, x2y2z2w2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON64_INTRINSICS)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
		float32x4_t x2y2_z2w2_x2y2_z2w2 = vpaddq_f32(x2_y2_z2_w2, x2_y2_z2_w2);
		float32x4_t x2y2z2w2_x2y2z2w2_x2y2z2w2_x2y2z2w2 = vpaddq_f32(x2y2_z2w2_x2y2_z2w2, x2y2_z2w2_x2y2_z2w2);
		return x2y2z2w2_x2y2z2w2_x2y2z2w2_x2y2z2w2;
#elif defined(RTM_NEON_INTRINSICS)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
		float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
		float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
		float32x2_t x2z2_y2w2 = vadd_f32(x2_y2, z2_w2);
		float32x2_t x2y2z2w2 = vpadd_f32(x2z2_y2w2, x2z2_y2w2);
		return vcombine_f32(x2y2z2w2, x2y2z2w2);
#else
		return vector_set(vector_dot_as_scalar(lhs, rhs));
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
		struct vector4f_vector_dot2
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0x7F));
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
				return _mm_cvtss_f32(x2y2_0_0_0);
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
				return vget_lane_f32(x2y2_x2y2, 0);
#else
				return (vector_get_x(lhs) * vector_get_x(rhs)) + (vector_get_y(lhs) * vector_get_y(rhs));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				return scalarf{ _mm_add_ss(x2_y2_z2_w2, y2_0_0_0) };
			}
#endif

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF));
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
				return _mm_shuffle_ps(x2y2_0_0_0, x2y2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
				return vcombine_f32(x2y2_x2y2, x2y2_x2y2);
#else
				scalarf result = *this;
				return vector_set(result);
#endif
			}

			vector4f lhs;
			vector4f rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot2 RTM_SIMD_CALL vector_dot2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot2{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_dot2_as_scalar(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
		return scalarf{ _mm_add_ss(x2_y2_z2_w2, y2_0_0_0) };
#else
		return vector_dot2(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dot2_as_vector(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS) && 0
		// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
		return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF));
#elif defined(RTM_SSE2_INTRINSICS)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
		return _mm_shuffle_ps(x2y2_0_0_0, x2y2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON_INTRINSICS)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
		float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
		float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
		return vcombine_f32(x2y2_x2y2, x2y2_x2y2);
#else
		return vector_set(vector_dot2_as_scalar(lhs, rhs));
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
		struct vector4f_vector_dot3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0x7F));
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
				__m128 z2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 2));
				__m128 x2y2z2_0_0_0 = _mm_add_ss(x2y2_0_0_0, z2_0_0_0);
				return _mm_cvtss_f32(x2y2z2_0_0_0);
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
				float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
				float32x2_t z2_z2 = vdup_lane_f32(z2_w2, 0);
				float32x2_t x2y2z2_x2y2z2 = vadd_f32(x2y2_x2y2, z2_z2);
				return vget_lane_f32(x2y2z2_x2y2z2, 0);
#else
				return (vector_get_x(lhs) * vector_get_x(rhs)) + (vector_get_y(lhs) * vector_get_y(rhs)) + (vector_get_z(lhs) * vector_get_z(rhs));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
				__m128 z2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 2));
				return scalarf{ _mm_add_ss(x2y2_0_0_0, z2_0_0_0) };
			}
#endif

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4f() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE4_INTRINSICS) && 0
				// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
				return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF));
#elif defined(RTM_SSE2_INTRINSICS)
				__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
				__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
				__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
				__m128 z2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 2));
				__m128 x2y2z2_0_0_0 = _mm_add_ss(x2y2_0_0_0, z2_0_0_0);
				return _mm_shuffle_ps(x2y2z2_0_0_0, x2y2z2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON64_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x4_t x2_y2_z2 = vsetq_lane_f32(0.0F, x2_y2_z2_w2, 3);
				float32x4_t x2y2_z2_x2y2_z2 = vpaddq_f32(x2_y2_z2, x2_y2_z2);
				float32x4_t x2y2z2_x2y2z2_x2y2z2_x2y2z2 = vpaddq_f32(x2y2_z2_x2y2_z2, x2y2_z2_x2y2_z2);
				return x2y2z2_x2y2z2_x2y2z2_x2y2z2;
#elif defined(RTM_NEON_INTRINSICS)
				float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
				float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
				float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
				float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
				float32x2_t z2_z2 = vdup_lane_f32(z2_w2, 0);
				float32x2_t x2y2z2_x2y2z2 = vadd_f32(x2y2_x2y2, z2_z2);
				return vdupq_lane_f32(x2y2z2_x2y2z2, 0);
#else
				scalarf result = *this;
				return vector_set(result);
#endif
			}

			vector4f lhs;
			vector4f rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot3 RTM_SIMD_CALL vector_dot3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot3{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_dot3_as_scalar(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
		__m128 z2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 2));
		return scalarf{ _mm_add_ss(x2y2_0_0_0, z2_0_0_0) };
#else
		return vector_dot3(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dot3_as_vector(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS) && 0
		// SSE4 dot product instruction appears slower on Zen2, is it the case elsewhere as well?
		return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xFF));
#elif defined(RTM_SSE2_INTRINSICS)
		__m128 x2_y2_z2_w2 = _mm_mul_ps(lhs, rhs);
		__m128 y2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 1));
		__m128 x2y2_0_0_0 = _mm_add_ss(x2_y2_z2_w2, y2_0_0_0);
		__m128 z2_0_0_0 = _mm_shuffle_ps(x2_y2_z2_w2, x2_y2_z2_w2, _MM_SHUFFLE(0, 0, 0, 2));
		__m128 x2y2z2_0_0_0 = _mm_add_ss(x2y2_0_0_0, z2_0_0_0);
		return _mm_shuffle_ps(x2y2z2_0_0_0, x2y2z2_0_0_0, _MM_SHUFFLE(0, 0, 0, 0));
#elif defined(RTM_NEON64_INTRINSICS)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
		float32x4_t x2_y2_z2 = vsetq_lane_f32(0.0F, x2_y2_z2_w2, 3);
		float32x4_t x2y2_z2_x2y2_z2 = vpaddq_f32(x2_y2_z2, x2_y2_z2);
		float32x4_t x2y2z2_x2y2z2_x2y2z2_x2y2z2 = vpaddq_f32(x2y2_z2_x2y2_z2, x2y2_z2_x2y2_z2);
		return x2y2z2_x2y2z2_x2y2z2_x2y2z2;
#elif defined(RTM_NEON_INTRINSICS)
		float32x4_t x2_y2_z2_w2 = vmulq_f32(lhs, rhs);
		float32x2_t x2_y2 = vget_low_f32(x2_y2_z2_w2);
		float32x2_t z2_w2 = vget_high_f32(x2_y2_z2_w2);
		float32x2_t x2y2_x2y2 = vpadd_f32(x2_y2, x2_y2);
		float32x2_t z2_z2 = vdup_lane_f32(z2_w2, 0);
		float32x2_t x2y2z2_x2y2z2 = vadd_f32(x2y2_x2y2, z2_z2);
		return vdupq_lane_f32(x2y2z2_x2y2z2, 0);
#else
		return vector_set(vector_dot3_as_scalar(lhs, rhs));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot RTM_SIMD_CALL vector_length_squared(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_squared_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_length_squared_as_vector(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot_as_vector(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot2 RTM_SIMD_CALL vector_length_squared2(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot2{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_squared2_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot2_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_length_squared2_as_vector(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot2_as_vector(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_dot3 RTM_SIMD_CALL vector_length_squared3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_dot3{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_squared3_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot3_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_length_squared3_as_vector(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot3_as_vector(input, input);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_vector_length
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_length RTM_SIMD_CALL vector_length(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_length{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = vector_length_squared_as_scalar(input);
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
		struct vector4f_vector_length3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared3_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared3_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_length3 RTM_SIMD_CALL vector_length3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_length3{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length3_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = vector_length_squared3_as_scalar(input);
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
		struct vector4f_vector_length_reciprocal
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_length_reciprocal RTM_SIMD_CALL vector_length_reciprocal(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_length_reciprocal{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_reciprocal_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = vector_length_squared_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_vector_length_reciprocal2
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared2_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared2_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_length_reciprocal2 RTM_SIMD_CALL vector_length_reciprocal2(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_length_reciprocal2{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_reciprocal2_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = vector_length_squared2_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4f_vector_length_reciprocal3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator float() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared3_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalarf() const RTM_NO_EXCEPT
			{
				const scalarf len_sq = vector_length_squared3_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4f input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4f_vector_length_reciprocal3 RTM_SIMD_CALL vector_length_reciprocal3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4f_vector_length_reciprocal3{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_length_reciprocal3_as_scalar(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		const scalarf len_sq = vector_length_squared3_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the distance between two 3D points.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm_impl::vector4f_vector_length3 RTM_SIMD_CALL vector_distance3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4f difference = vector_sub(lhs, rhs);
		return rtm_impl::vector4f_vector_length3{ difference };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the distance between two 3D points.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalarf RTM_SIMD_CALL vector_distance3_as_scalar(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4f difference = vector_sub(lhs, rhs);
		return vector_length3_as_scalar(difference);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector2.
	// If the length of the input is not finite or zero, the result is undefined.
	// For a safe alternative, supply a fallback value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize2(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared2_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector2.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize2(vector4f_arg0 input, vector4f_arg1 fallback, float threshold = 1.0E-8F) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared2_as_scalar(input);
		if (scalar_cast(len_sq) >= threshold)
			return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
		else
			return fallback;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector3.
	// If the length of the input is not finite or zero, the result is undefined.
	// For a safe alternative, supply a fallback value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared3_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector3.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize3(vector4f_arg0 input, vector4f_arg1 fallback, float threshold = 1.0E-8F) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared3_as_scalar(input);
		if (scalar_cast(len_sq) >= threshold)
			return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
		else
			return fallback;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector4.
	// If the length of the input is not finite or zero, the result is undefined.
	// For a safe alternative, supply a fallback value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector4.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_normalize(vector4f_arg0 input, vector4f_arg1 fallback, float threshold = 1.0E-8F) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalarf len_sq = vector_length_squared_as_scalar(input);
		if (scalar_cast(len_sq) >= threshold)
			return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
		else
			return fallback;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the fractional part of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_fraction(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return vector_set(scalar_fraction(vector_get_x(input)), scalar_fraction(vector_get_y(input)), scalar_fraction(vector_get_z(input)), scalar_fraction(vector_get_w(input)));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul_add(vector4f_arg0 v0, vector4f_arg1 v1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return RTM_VECTOR4F_MULV_ADD(v0, v1, v2);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul_add(vector4f_arg0 v0, float s1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return RTM_VECTOR4F_MULS_ADD(v0, s1, v2);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mul_add(vector4f_arg0 v0, scalarf_arg1 s1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_add(vector_mul(v0, s1), v2);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_neg_mul_sub(vector4f_arg0 v0, vector4f_arg1 v1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return RTM_VECTOR4F_NEG_MULV_SUB(v0, v1, v2);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_neg_mul_sub(vector4f_arg0 v0, float s1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return RTM_VECTOR4F_NEG_MULS_SUB(v0, s1, v2);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_neg_mul_sub(vector4f_arg0 v0, scalarf_arg1 s1, vector4f_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_sub(v2, vector_mul(v0, s1));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_lerp(vector4f_arg0 start, vector4f_arg1 end, float alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		return vector_mul_add(end, alpha, vector_neg_mul_sub(start, alpha, start));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_lerp(vector4f_arg0 start, vector4f_arg1 end, scalarf_arg2 alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		const vector4f alpha_v = vector_set(alpha);
		return vector_mul_add(end, alpha_v, vector_neg_mul_sub(start, alpha_v, start));
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component linear interpolation of the two inputs at the specified alpha.
	// The formula used is: ((1.0 - alpha) * start) + (alpha * end).
	// Interpolation is stable and will return 'start' when alpha is 0.0 and 'end' when it is 1.0.
	// This is the same instruction count when FMA is present but it might be slightly slower
	// due to the extra multiplication compared to: start + (alpha * (end - start)).
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_lerp(vector4f_arg0 start, vector4f_arg1 end, vector4f_arg2 alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		return vector_mul_add(end, alpha, vector_neg_mul_sub(start, alpha, start));
	}



	//////////////////////////////////////////////////////////////////////////
	// Comparisons and masking
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if equal, otherwise 0: lhs == rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmpeq_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vceqq_f32(lhs, rhs);
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x == rhs.x), rtm_impl::get_mask_value(lhs.y == rhs.y), rtm_impl::get_mask_value(lhs.z == rhs.z), rtm_impl::get_mask_value(lhs.w == rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if not equal, otherwise 0: lhs != rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_not_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmpneq_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vmvnq_u32(vceqq_f32(lhs, rhs));
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x != rhs.x), rtm_impl::get_mask_value(lhs.y != rhs.y), rtm_impl::get_mask_value(lhs.z != rhs.z), rtm_impl::get_mask_value(lhs.w != rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if less than, otherwise 0: lhs < rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_less_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmplt_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vcltq_f32(lhs, rhs);
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x < rhs.x), rtm_impl::get_mask_value(lhs.y < rhs.y), rtm_impl::get_mask_value(lhs.z < rhs.z), rtm_impl::get_mask_value(lhs.w < rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if less equal, otherwise 0: lhs <= rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_less_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmple_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vcleq_f32(lhs, rhs);
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x <= rhs.x), rtm_impl::get_mask_value(lhs.y <= rhs.y), rtm_impl::get_mask_value(lhs.z <= rhs.z), rtm_impl::get_mask_value(lhs.w <= rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if greater than, otherwise 0: lhs > rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_greater_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmpgt_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vcgtq_f32(lhs, rhs);
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x > rhs.x), rtm_impl::get_mask_value(lhs.y > rhs.y), rtm_impl::get_mask_value(lhs.z > rhs.z), rtm_impl::get_mask_value(lhs.w > rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if greater equal, otherwise 0: lhs >= rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_greater_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_cmpge_ps(lhs, rhs);
#elif defined(RTM_NEON_INTRINSICS)
		return vcgeq_f32(lhs, rhs);
#else
		return mask4f{ rtm_impl::get_mask_value(lhs.x >= rhs.x), rtm_impl::get_mask_value(lhs.y >= rhs.y), rtm_impl::get_mask_value(lhs.z >= rhs.z), rtm_impl::get_mask_value(lhs.w >= rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are less than, otherwise false: all(lhs.xyzw < rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcltq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z && lhs.w < rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are less than, otherwise false: all(lhs.xy < rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vclt_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are less than, otherwise false: all(lhs.xyz < rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcltq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are less than, otherwise false: any(lhs.xyzw < rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcltq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y || lhs.z < rhs.z || lhs.w < rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are less than, otherwise false: any(lhs.xy < rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vclt_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are less than, otherwise false: any(lhs.xyz < rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmplt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcltq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y || lhs.z < rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are less equal, otherwise false: all(lhs.xyzw <= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcleq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y && lhs.z <= rhs.z && lhs.w <= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are less equal, otherwise false: all(lhs.xy <= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcle_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are less equal, otherwise false: all(lhs.xyz <= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcleq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y && lhs.z <= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are less equal, otherwise false: any(lhs.xyzw <= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcleq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y || lhs.z <= rhs.z || lhs.w <= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are less equal, otherwise false: any(lhs.xy <= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcle_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are less equal, otherwise false: any(lhs.xyz <= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmple_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcleq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y || lhs.z <= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are greater than, otherwise false: all(lhs.xyzw > rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgtq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y && lhs.z > rhs.z && lhs.w > rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are greater than, otherwise false: all(lhs.xy > rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcgt_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are greater than, otherwise false: all(lhs.xyz > rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgtq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y && lhs.z > rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are greater than, otherwise false: any(lhs.xyzw > rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgtq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y || lhs.z > rhs.z || lhs.w > rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are greater than, otherwise false: any(lhs.xy > rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcgt_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are greater than, otherwise false: any(lhs.xyz > rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpgt_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgtq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y || lhs.z > rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are greater equal, otherwise false: all(lhs.xyzw >= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgeq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y && lhs.z >= rhs.z && lhs.w >= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are greater equal, otherwise false: all(lhs.xy >= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcge_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are greater equal, otherwise false: all(lhs.xyz >= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgeq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y && lhs.z >= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are greater equal, otherwise false: any(lhs.xyzw >= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgeq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y || lhs.z >= rhs.z || lhs.w >= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are greater equal, otherwise false: any(lhs.xy >= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vcge_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are greater equal, otherwise false: any(lhs.xyz >= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpge_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vcgeq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y || lhs.z >= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyzw] components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
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
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vceq_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vceqq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyzw] components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vceqq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z || lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vceq_f32(vget_low_f32(lhs), vget_low_f32(rhs));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpeq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vceqq_f32(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyzw] components are not equal, otherwise false: all(lhs.xyzw != rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vmvnq_u32(vceqq_f32(lhs, rhs));

		bool result;
		RTM_MASK4F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y && lhs.z != rhs.z && lhs.w != rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are not equal, otherwise false: all(lhs.xy != rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vmvn_u32(vceq_f32(vget_low_f32(lhs), vget_low_f32(rhs)));

		bool result;
		RTM_MASK2F_ALL_TRUE(mask, result);
		return result;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are not equal, otherwise false: all(lhs.xyz != rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vmvnq_u32(vceqq_f32(lhs, rhs));

		bool result;
		RTM_MASK4F_ALL_TRUE3(mask, result);
		return result;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y && lhs.z != rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyzw] components are not equal, otherwise false: any(lhs.xyzw != rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vmvnq_u32(vceqq_f32(lhs, rhs));

		bool result;
		RTM_MASK4F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are not equal, otherwise false: any(lhs.xy != rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE2(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x2_t mask = vmvn_u32(vceq_f32(vget_low_f32(lhs), vget_low_f32(rhs)));

		bool result;
		RTM_MASK2F_ANY_TRUE(mask, result);
		return result;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are not equal, otherwise false: any(lhs.xyz != rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 mask = _mm_cmpneq_ps(lhs, rhs);

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		const uint32x4_t mask = vmvnq_u32(vceqq_f32(lhs, rhs));

		bool result;
		RTM_MASK4F_ANY_TRUE3(mask, result);
		return result;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are near equal, otherwise false: all(abs(lhs - rhs).xyzw <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_all_less_equal(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are near equal, otherwise false: all(abs(lhs - rhs).xy <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_all_less_equal2(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are near equal, otherwise false: all(abs(lhs - rhs).xyz <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_all_less_equal3(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are near equal, otherwise false: any(abs(lhs - rhs).xyzw <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_any_less_equal(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are near equal, otherwise false: any(abs(lhs - rhs).xy <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal2(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_any_less_equal2(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are near equal, otherwise false: any(abs(lhs - rhs).xyz <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal3(vector4f_arg0 lhs, vector4f_arg1 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
	{
		return vector_any_less_equal3(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if input is finite, otherwise 0: finite(input) ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4f RTM_SIMD_CALL vector_finite(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));

		const __m128 infinity = _mm_set_ps1(std::numeric_limits<float>::infinity());
		__m128 is_not_infinity = _mm_cmpneq_ps(abs_input, infinity);

		__m128 is_nan = _mm_cmpneq_ps(input, input);

		__m128 is_finite = _mm_andnot_ps(is_nan, is_not_infinity);
		return is_finite;
#elif defined(RTM_NEON_INTRINSICS)
		const float32x4_t abs_input = vabsq_f32(input);
		const float32x4_t infinity = vdupq_n_f32(std::numeric_limits<float>::infinity());
		const uint32x4_t is_not_infinity = vmvnq_u32(vceqq_f32(abs_input, infinity));
		const uint32x4_t is_not_nan = vceqq_f32(input, input);
		const uint32x4_t is_finite = vandq_u32(is_not_infinity, is_not_nan);
		return is_finite;
#else
		return mask4f{
			rtm_impl::get_mask_value(scalar_is_finite(vector_get_x(input))),
			rtm_impl::get_mask_value(scalar_is_finite(vector_get_y(input))),
			rtm_impl::get_mask_value(scalar_is_finite(vector_get_z(input))),
			rtm_impl::get_mask_value(scalar_is_finite(vector_get_w(input)))
			};
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are finite (not NaN/Inf), otherwise false: all(finite(input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return v_test_xyzw_finite(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are finite (not NaN/Inf), otherwise false: all(finite(input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite2(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return v_test_xy_finite(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are finite (not NaN/Inf), otherwise false: all(finite(input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite3(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		return v_test_xyz_finite(input);
	}



	//////////////////////////////////////////////////////////////////////////
	// Swizzling, permutations, and mixing
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_select(mask4f_arg0 mask, vector4f_arg1 if_true, vector4f_arg2 if_false) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS) || defined(RTM_NEON_INTRINSICS)
		return RTM_VECTOR4F_SELECT(mask, if_true, if_false);
#else
		return vector4f{ rtm_impl::select(mask.x, if_true.x, if_false.x), rtm_impl::select(mask.y, if_true.y, if_false.y), rtm_impl::select(mask.z, if_true.z, if_false.z), rtm_impl::select(mask.w, if_true.w, if_false.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Mixes two inputs and returns the desired components.
	// [xyzw] indexes into the first input while [abcd] indexes in the second.
	//////////////////////////////////////////////////////////////////////////
	template<mix4 comp0, mix4 comp1, mix4 comp2, mix4 comp3>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_mix(vector4f_arg0 input0, vector4f_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
        // Each component comes from the respective position of input 0 or input 1
        if (rtm_impl::static_condition<(comp0 == mix4::a || comp0 == mix4::x) && (comp1 == mix4::b || comp1 == mix4::y) &&
                                       (comp2 == mix4::c || comp2 == mix4::z) && (comp3 == mix4::d || comp3 == mix4::w)>::test())
        {
            constexpr int mask = (comp0 == mix4::a ? 1 : 0) | (comp1 == mix4::b ? 2 : 0) |
                                 (comp2 == mix4::c ? 4 : 0) | (comp3 == mix4::d ? 8 : 0);
            return _mm_blend_ps(input0, input1, mask);
        }

        // First component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0) && comp1 == mix4::y && comp2 == mix4::z && comp3 == mix4::w>::test())
            return _mm_insert_ps(input0, input1, (int(comp0) % 4) << 6);

        // Second component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && rtm_impl::is_mix_abcd(comp1) && comp2 == mix4::z && comp3 == mix4::w>::test())
            return _mm_insert_ps(input0, input1, ((int(comp1) % 4) << 6) | (1 << 4));

        // Third component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::y && rtm_impl::is_mix_abcd(comp2) && comp3 == mix4::w>::test())
            return _mm_insert_ps(input0, input1, ((int(comp2) % 4) << 6) | (2 << 4));

        // Fourth component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::y && comp2 == mix4::z && rtm_impl::is_mix_abcd(comp3)>::test())
            return _mm_insert_ps(input0, input1, ((int(comp3) % 4) << 6) | (3 << 4));

        // First component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0) && comp1 == mix4::b && comp2 == mix4::c && comp3 == mix4::d>::test())
            return _mm_insert_ps(input1, input0, (int(comp0) % 4) << 6);

        // Second component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && rtm_impl::is_mix_xyzw(comp1) && comp2 == mix4::c && comp3 == mix4::d>::test())
            return _mm_insert_ps(input1, input0, ((int(comp1) % 4) << 6) | (1 << 4));

        // Third component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::b && rtm_impl::is_mix_xyzw(comp2) && comp3 == mix4::d>::test())
            return _mm_insert_ps(input1, input0, ((int(comp2) % 4) << 6) | (2 << 4));

        // Fourth component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::b && comp2 == mix4::c && rtm_impl::is_mix_xyzw(comp3)>::test())
            return _mm_insert_ps(input1, input0, ((int(comp3) % 4) << 6) | (3 << 4));
#endif // defined(RTM_SSE4_INTRINSICS)

#if defined(RTM_SSE2_INTRINSICS)
		// All four components come from input 0
		if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0) && rtm_impl::is_mix_xyzw(comp1) && rtm_impl::is_mix_xyzw(comp2) && rtm_impl::is_mix_xyzw(comp3)>::test())
			return _mm_shuffle_ps(input0, input0, _MM_SHUFFLE(int(comp3) % 4, int(comp2) % 4, int(comp1) % 4, int(comp0) % 4));

		// All four components come from input 1
		if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0) && rtm_impl::is_mix_abcd(comp1) && rtm_impl::is_mix_abcd(comp2) && rtm_impl::is_mix_abcd(comp3)>::test())
			return _mm_shuffle_ps(input1, input1, _MM_SHUFFLE(int(comp3) % 4, int(comp2) % 4, int(comp1) % 4, int(comp0) % 4));

		// First two components come from input 0, second two come from input 1
		if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0) && rtm_impl::is_mix_xyzw(comp1) && rtm_impl::is_mix_abcd(comp2) && rtm_impl::is_mix_abcd(comp3)>::test())
			return _mm_shuffle_ps(input0, input1, _MM_SHUFFLE(int(comp3) % 4, int(comp2) % 4, int(comp1) % 4, int(comp0) % 4));

		// First two components come from input 1, second two come from input 0
		if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0) && rtm_impl::is_mix_abcd(comp1) && rtm_impl::is_mix_xyzw(comp2) && rtm_impl::is_mix_xyzw(comp3)>::test())
			return _mm_shuffle_ps(input1, input0, _MM_SHUFFLE(int(comp3) % 4, int(comp2) % 4, int(comp1) % 4, int(comp0) % 4));

		// Low words from both inputs are interleaved
		if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::a && comp2 == mix4::y && comp3 == mix4::b>::test())
			return _mm_unpacklo_ps(input0, input1);

		// Low words from both inputs are interleaved
		if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::x && comp2 == mix4::b && comp3 == mix4::y>::test())
			return _mm_unpacklo_ps(input1, input0);

		// High words from both inputs are interleaved
		if (rtm_impl::static_condition<comp0 == mix4::z && comp1 == mix4::c && comp2 == mix4::w && comp3 == mix4::d>::test())
			return _mm_unpackhi_ps(input0, input1);

		// High words from both inputs are interleaved
		if (rtm_impl::static_condition<comp0 == mix4::c && comp1 == mix4::z && comp2 == mix4::d && comp3 == mix4::w>::test())
			return _mm_unpackhi_ps(input1, input0);
#endif	// defined(RTM_SSE2_INTRINSICS)

#if defined(RTM_NEON64_INTRINSICS)
        // Low words from both inputs are interleaved
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::a && comp2 == mix4::y && comp3 == mix4::b>::test())
            return vzip1q_f32(input0, input1);

        // Low words from both inputs are interleaved
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::x && comp2 == mix4::b && comp3 == mix4::y>::test())
            return vzip1q_f32(input1, input0);

        // High words from both inputs are interleaved
        if (rtm_impl::static_condition<comp0 == mix4::z && comp1 == mix4::c && comp2 == mix4::w && comp3 == mix4::d>::test())
            return vzip2q_f32(input0, input1);

        // High words from both inputs are interleaved
        if (rtm_impl::static_condition<comp0 == mix4::c && comp1 == mix4::z && comp2 == mix4::d && comp3 == mix4::w>::test())
            return vzip2q_f32(input1, input0);

        // Even-numbered vector elements, consecutively
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::z && comp2 == mix4::a && comp3 == mix4::c>::test())
            return vuzp1q_f32(input0, input1);

        // Even-numbered vector elements, consecutively
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::c && comp2 == mix4::x && comp3 == mix4::z>::test())
            return vuzp1q_f32(input1, input0);

        // Odd-numbered vector elements, consecutively
        if (rtm_impl::static_condition<comp0 == mix4::y && comp1 == mix4::w && comp2 == mix4::b && comp3 == mix4::d>::test())
            return vuzp2q_f32(input0, input1);

        // Odd-numbered vector elements, consecutively
        if (rtm_impl::static_condition<comp0 == mix4::b && comp1 == mix4::d && comp2 == mix4::y && comp3 == mix4::w>::test())
            return vuzp2q_f32(input1, input0);

        // Even-numbered vector elements, interleaved
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::a && comp2 == mix4::z && comp3 == mix4::c>::test())
            return vtrn1q_f32(input0, input1);

        // Even-numbered vector elements, interleaved
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::x && comp2 == mix4::c && comp3 == mix4::z>::test())
            return vtrn1q_f32(input1, input0);

        // Odd-numbered vector elements, interleaved
        if (rtm_impl::static_condition<comp0 == mix4::y && comp1 == mix4::b && comp2 == mix4::w && comp3 == mix4::d>::test())
            return vtrn2q_f32(input0, input1);

        // Odd-numbered vector elements, interleaved
        if (rtm_impl::static_condition<comp0 == mix4::b && comp1 == mix4::y && comp2 == mix4::d && comp3 == mix4::w>::test())
            return vtrn2q_f32(input1, input0);
#endif // defined(RTM_NEON64_INTRINSICS)

#if defined(RTM_NEON_INTRINSICS)
        // The highest vector elements from input 0 and the lowest vector elements from input 1, consecutively
        if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0) &&
                                       int(comp0) + 1 == int(comp1) &&
                                       int(comp1) + 1 == int(comp2) &&
                                       int(comp2) + 1 == int(comp3)>::test())
            return vextq_f32(input0, input1, int(comp0) % 4);

        // The highest vector elements from input 1 and the lowest vector elements from input 0, consecutively
        if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0) &&
                                       (int(comp0) + 1) % 8 == int(comp1) &&
                                       (int(comp1) + 1) % 8 == int(comp2) &&
                                       (int(comp2) + 1) % 8 == int(comp3)>::test())
            return vextq_f32(input1, input0, int(comp0) % 4);

        // All four components come from input 0, reversed order in each doubleword
        if (rtm_impl::static_condition<comp0 == mix4::y && comp1 == mix4::x && comp2 == mix4::w && comp3 == mix4::z>::test())
            return vrev64q_f32(input0);

        // All four components come from input 1, reversed order in each doubleword
        if (rtm_impl::static_condition<comp0 == mix4::b && comp1 == mix4::a && comp2 == mix4::d && comp3 == mix4::c>::test())
            return vrev64q_f32(input1);

        // First component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0) && comp1 == mix4::y && comp2 == mix4::z && comp3 == mix4::w>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input1, int(comp0) % 4), input0, 0);

        // Second component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && rtm_impl::is_mix_abcd(comp1) && comp2 == mix4::z && comp3 == mix4::w>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input1, int(comp1) % 4), input0, 1);

        // Third component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::y && rtm_impl::is_mix_abcd(comp2) && comp3 == mix4::w>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input1, int(comp2) % 4), input0, 2);

        // Fourth component comes from input 1, others come from the respective positions of input 0
        if (rtm_impl::static_condition<comp0 == mix4::x && comp1 == mix4::y && comp2 == mix4::z && rtm_impl::is_mix_abcd(comp3)>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input1, int(comp3) % 4), input0, 3);

        // First component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0) && comp1 == mix4::b && comp2 == mix4::c && comp3 == mix4::d>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input0, int(comp0) % 4), input1, 0);

        // Second component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && rtm_impl::is_mix_xyzw(comp1) && comp2 == mix4::c && comp3 == mix4::d>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input0, int(comp1) % 4), input1, 1);

        // Third component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::b && rtm_impl::is_mix_xyzw(comp2) && comp3 == mix4::d>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input0, int(comp2) % 4), input1, 2);

        // Fourth component comes from input 0, others come from the respective positions of input 1
        if (rtm_impl::static_condition<comp0 == mix4::a && comp1 == mix4::b && comp2 == mix4::c && rtm_impl::is_mix_xyzw(comp3)>::test())
            return vsetq_lane_f32(vgetq_lane_f32(input0, int(comp3) % 4), input1, 3);


        // All comes from the same position
        if (rtm_impl::static_condition<comp0 == comp1 && comp0 == comp2 && comp0 == comp3>::test()) {
            // All comes from the same position of input0
            if (rtm_impl::static_condition<rtm_impl::is_mix_xyzw(comp0)>::test())
                return vmovq_n_f32(vgetq_lane_f32(input0, int(comp0) % 4));
            // All comes from the same position of input1
            if (rtm_impl::static_condition<rtm_impl::is_mix_abcd(comp0)>::test())
                return vmovq_n_f32(vgetq_lane_f32(input1, int(comp0) % 4));
        }
#endif // defined(RTM_NEON_INTRINSICS)

        // Slow code path, not yet optimized or not using intrinsics
		constexpr component4 component0 = rtm_impl::mix_to_component(comp0);
		constexpr component4 component1 = rtm_impl::mix_to_component(comp1);
		constexpr component4 component2 = rtm_impl::mix_to_component(comp2);
		constexpr component4 component3 = rtm_impl::mix_to_component(comp3);

		const float x0 = vector_get_component(input0, component0);
		const float x1 = vector_get_component(input1, component0);
		const float x = rtm_impl::is_mix_xyzw(comp0) ? x0 : x1;

		const float y0 = vector_get_component(input0, component1);
		const float y1 = vector_get_component(input1, component1);
		const float y = rtm_impl::is_mix_xyzw(comp1) ? y0 : y1;

		const float z0 = vector_get_component(input0, component2);
		const float z1 = vector_get_component(input1, component2);
		const float z = rtm_impl::is_mix_xyzw(comp2) ? z0 : z1;

		const float w0 = vector_get_component(input0, component3);
		const float w1 = vector_get_component(input1, component3);
		const float w = rtm_impl::is_mix_xyzw(comp3) ? w0 : w1;

		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [x] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dup_x(vector4f_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::x, mix4::x, mix4::x, mix4::x>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [y] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dup_y(vector4f_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::y, mix4::y, mix4::y, mix4::y>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [z] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dup_z(vector4f_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::z, mix4::z, mix4::z, mix4::z>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [w] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_dup_w(vector4f_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::w, mix4::w, mix4::w, mix4::w>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Logical
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: input0 & input1
	//////////////////////////////////////////////////////////////////////////
	inline vector4f RTM_SIMD_CALL vector_and(vector4f_arg0 input0, vector4f_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_and_ps(input0, input1);
#elif defined(RTM_NEON_INTRINSICS)
		return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(input0), vreinterpretq_u32_f32(input1)));
#else

#if defined(RTM_COMPILER_GCC)
	// GCC complains 'result' is used uninitialized but that is not true, ignore it
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

		const uint32_t* input0_ = rtm_impl::bit_cast<const uint32_t*>(&input0);
		const uint32_t* input1_ = rtm_impl::bit_cast<const uint32_t*>(&input1);

		vector4f result;
		uint32_t* result_ = rtm_impl::bit_cast<uint32_t*>(&result);

		result_[0] = input0_[0] & input1_[0];
		result_[1] = input0_[1] & input1_[1];
		result_[2] = input0_[2] & input1_[2];
		result_[3] = input0_[3] & input1_[3];

		return result;

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical OR between the inputs: input0 | input1
	//////////////////////////////////////////////////////////////////////////
	inline vector4f RTM_SIMD_CALL vector_or(vector4f_arg0 input0, vector4f_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_or_ps(input0, input1);
#elif defined(RTM_NEON_INTRINSICS)
		return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(input0), vreinterpretq_u32_f32(input1)));
#else

#if defined(RTM_COMPILER_GCC)
	// GCC complains 'result' is used uninitialized but that is not true, ignore it
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

		const uint32_t* input0_ = rtm_impl::bit_cast<const uint32_t*>(&input0);
		const uint32_t* input1_ = rtm_impl::bit_cast<const uint32_t*>(&input1);

		vector4f result;
		uint32_t* result_ = rtm_impl::bit_cast<uint32_t*>(&result);

		result_[0] = input0_[0] | input1_[0];
		result_[1] = input0_[1] | input1_[1];
		result_[2] = input0_[2] | input1_[2];
		result_[3] = input0_[3] | input1_[3];

		return result;

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical XOR between the inputs: input0 ^ input1
	//////////////////////////////////////////////////////////////////////////
	inline vector4f RTM_SIMD_CALL vector_xor(vector4f_arg0 input0, vector4f_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return _mm_xor_ps(input0, input1);
#elif defined(RTM_NEON_INTRINSICS)
		return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(input0), vreinterpretq_u32_f32(input1)));
#else

#if defined(RTM_COMPILER_GCC)
	// GCC complains 'result' is used uninitialized but that is not true, ignore it
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

		const uint32_t* input0_ = rtm_impl::bit_cast<const uint32_t*>(&input0);
		const uint32_t* input1_ = rtm_impl::bit_cast<const uint32_t*>(&input1);

		vector4f result;
		uint32_t* result_ = rtm_impl::bit_cast<uint32_t*>(&result);

		result_[0] = input0_[0] ^ input1_[0];
		result_[1] = input0_[1] ^ input1_[1];
		result_[2] = input0_[2] ^ input1_[2];
		result_[3] = input0_[3] ^ input1_[3];

		return result;

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif
#endif
	}


	//////////////////////////////////////////////////////////////////////////
	// Miscellaneous
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sign of the input vector: input >= 0.0 ? 1.0 : -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_sign(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128 signs = RTM_VECTOR4F_MAKE(-0.0F, -0.0F, -0.0F, -0.0F);
		constexpr __m128 one = RTM_VECTOR4F_MAKE(1.0F, 1.0F, 1.0F, 1.0F);
		const __m128 sign_bits = _mm_and_ps(input, signs);	// Mask out the sign bit
		return _mm_or_ps(sign_bits, one);					// Copy the sign bit onto +-1.0f
#else
		const mask4f mask = vector_greater_equal(input, vector_zero());
		return vector_select(mask, vector_set(1.0F), vector_set(-1.0F));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the input with the sign of the control value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_copy_sign(vector4f_arg0 input, vector4f_arg1 control_sign) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128 sign_bit = _mm_set_ps1(-0.0F);
		__m128 signs = _mm_and_ps(sign_bit, control_sign);
		__m128 abs_input = _mm_andnot_ps(sign_bit, input);
		return _mm_or_ps(abs_input, signs);
#else
		float x = vector_get_x(input);
		float y = vector_get_y(input);
		float z = vector_get_z(input);
		float w = vector_get_w(input);

		float x_sign = vector_get_x(control_sign);
		float y_sign = vector_get_y(control_sign);
		float z_sign = vector_get_z(control_sign);
		float w_sign = vector_get_w(control_sign);

		return vector_set(rtm_impl::copysign(x, x_sign), rtm_impl::copysign(y, y_sign), rtm_impl::copysign(z, z_sign), rtm_impl::copysign(w, w_sign));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the rounded input using a symmetric algorithm.
	// vector_round_symmetric(1.5) = 2.0
	// vector_round_symmetric(1.2) = 1.0
	// vector_round_symmetric(-1.5) = -2.0
	// vector_round_symmetric(-1.2) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_round_symmetric(vector4f_arg0 input) RTM_NO_EXCEPT
	{
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

#if defined(RTM_SSE4_INTRINSICS)
		__m128 is_positive = _mm_cmpge_ps(input, _mm_setzero_ps());

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_andnot_ps(is_positive, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128 bias = _mm_or_ps(sign, _mm_set_ps1(0.5F));
		__m128 biased_input = _mm_add_ps(input, bias);

		__m128 floored = _mm_floor_ps(biased_input);
		__m128 ceiled = _mm_ceil_ps(biased_input);

		return vector_select(is_positive, floored, ceiled);
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ps(abs_input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128 is_nan = _mm_cmpneq_ps(input, input);

		// Combine our masks to determine if we should return the original value
		__m128 use_original_input = _mm_or_ps(is_input_large, is_nan);

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(input, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128 bias = _mm_or_ps(sign, _mm_set_ps1(0.5F));
		__m128 biased_input = _mm_add_ps(input, bias);

		// Convert to an integer with truncation and back, this rounds towards zero.
		__m128 integer_part = _mm_cvtepi32_ps(_mm_cvttps_epi32(biased_input));

		return _mm_or_ps(_mm_and_ps(use_original_input, input), _mm_andnot_ps(use_original_input, integer_part));
#elif defined(RTM_NEON_INTRINSICS) && !defined(RTM_NEON64_INTRINSICS) // arm64 is faster with scalar code
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		float32x4_t fractional_limit = vdupq_n_f32(8388608.0F); // 2^23

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		uint32x4_t is_input_large = vcageq_f32(input, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		uint32x4_t is_nan = vmvnq_u32(vceqq_f32(input, input));

		// Combine our masks to determine if we should return the original value
		uint32x4_t use_original_input = vorrq_u32(is_input_large, is_nan);

        uint32x4_t sign_mask = vreinterpretq_u32_f32(vdupq_n_f32(-0.0F));
		uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(input), sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		float32x4_t bias = vreinterpretq_f32_u32(vorrq_u32(sign, vreinterpretq_u32_f32(vdupq_n_f32(0.5F))));
		float32x4_t biased_input = vaddq_f32(input, bias);

		// Convert to an integer and back. This does banker's rounding by default
		float32x4_t integer_part = vcvtq_f32_s32(vcvtq_s32_f32(biased_input));

		return vbslq_f32(use_original_input, input, integer_part);
#else
		const vector4f half = vector_set(0.5F);
		const vector4f floored = vector_floor(vector_add(input, half));
		const vector4f ceiled = vector_ceil(vector_sub(input, half));
		const mask4f is_greater_equal = vector_greater_equal(input, vector_zero());
		return vector_select(is_greater_equal, floored, ceiled);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the rounded input using banker's rounding (half to even).
	// vector_round_bankers(2.5) = 2.0
	// vector_round_bankers(1.5) = 2.0
	// vector_round_bankers(1.2) = 1.0
	// vector_round_bankers(-2.5) = -2.0
	// vector_round_bankers(-1.5) = -2.0
	// vector_round_bankers(-1.2) = -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4f RTM_SIMD_CALL vector_round_bankers(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return _mm_round_ps(input, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(input, sign_mask);

		// We add the largest integer that a 32 bit floating point number can represent and subtract it afterwards.
		// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
		// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
		// This has the effect of removing the fractional part while simultaneously rounding.
		// Use the same sign as the input value to make sure we handle positive and negative values.
		const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23
		__m128 truncating_offset = _mm_or_ps(sign, fractional_limit);
		__m128 integer_part = _mm_sub_ps(_mm_add_ps(input, truncating_offset), truncating_offset);

		// If our input was so large that it had no fractional part, return it unchanged
		// Otherwise return our integer part
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
		__m128 is_input_large = _mm_cmpge_ps(abs_input, fractional_limit);
		return _mm_or_ps(_mm_and_ps(is_input_large, input), _mm_andnot_ps(is_input_large, integer_part));
#elif defined(RTM_NEON64_INTRINSICS)
		return vrndnq_f32(input);
#elif defined(RTM_NEON_INTRINSICS)
        uint32x4_t sign_mask = vreinterpretq_u32_f32(vdupq_n_f32(-0.0F));
		uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(input), sign_mask);

		// We add the largest integer that a 32 bit floating point number can represent and subtract it afterwards.
		// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
		// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
		// This has the effect of removing the fractional part while simultaneously rounding.
		// Use the same sign as the input value to make sure we handle positive and negative values.
		float32x4_t fractional_limit = vdupq_n_f32(8388608.0F); // 2^23
		float32x4_t truncating_offset = vreinterpretq_f32_u32(vorrq_u32(sign, vreinterpretq_u32_f32(fractional_limit)));
		float32x4_t integer_part = vsubq_f32(vaddq_f32(input, truncating_offset), truncating_offset);

		// If our input was so large that it had no fractional part, return it unchanged
		// Otherwise return our integer part
		uint32x4_t is_input_large = vcageq_f32(input, fractional_limit);
		return vbslq_f32(is_input_large, input, integer_part);
#else
		scalarf x = scalar_round_bankers(scalarf(vector_get_x(input)));
		scalarf y = scalar_round_bankers(scalarf(vector_get_y(input)));
		scalarf z = scalar_round_bankers(scalarf(vector_get_z(input)));
		scalarf w = scalar_round_bankers(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_sin(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use a degree 11 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		__m128 quotient = _mm_mul_ps(input, _mm_set_ps1(constants::one_div_two_pi()));
		quotient = vector_round_bankers(quotient);
		quotient = _mm_mul_ps(quotient, _mm_set_ps1(constants::two_pi()));
		__m128 x = _mm_sub_ps(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 sign = _mm_and_ps(x, sign_mask);
		__m128 reference = _mm_or_ps(sign, _mm_set_ps1(constants::pi()));

		const __m128 reflection = _mm_sub_ps(reference, x);
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		const __m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));

		__m128 is_less_equal_than_half_pi = _mm_cmple_ps(x_abs, _mm_set_ps1(constants::half_pi()));

		x = RTM_VECTOR4F_SELECT(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		const __m128 x2 = _mm_mul_ps(x, x);
		__m128 result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.3828544692960918e-8F)), _mm_set_ps1(2.7521557770526783e-6F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-1.9840782426250314e-4F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(8.3333303183525942e-3F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-1.6666666601721269e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.0F));
		result = _mm_mul_ps(result, x);
		return result;
#elif defined(RTM_NEON_INTRINSICS)
		// Use a degree 11 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		float32x4_t quotient = vmulq_n_f32(input, constants::one_div_two_pi());
		quotient = vector_round_bankers(quotient);
		quotient = vmulq_n_f32(quotient, constants::two_pi());
		float32x4_t x = vsubq_f32(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		uint32x4_t sign_mask = vreinterpretq_u32_f32(vdupq_n_f32(-0.0F));
		uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(x), sign_mask);
		float32x4_t reference = vreinterpretq_f32_u32(vorrq_u32(sign, vreinterpretq_u32_f32(vdupq_n_f32(constants::pi()))));

		float32x4_t reflection = vsubq_f32(reference, x);
#if !defined(RTM_IMPL_VCA_SUPPORTED)
		uint32x4_t is_less_equal_than_half_pi = vcleq_f32(vabsq_f32(x), vdupq_n_f32(constants::half_pi()));
#else
		uint32x4_t is_less_equal_than_half_pi = vcaleq_f32(x, vdupq_n_f32(constants::half_pi()));
#endif
		x = vbslq_f32(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		float32x4_t x2 = vmulq_f32(x, x);

		float32x4_t result = RTM_VECTOR4F_MULS_ADD(x2, -2.3828544692960918e-8F, vdupq_n_f32(2.7521557770526783e-6F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-1.9840782426250314e-4F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(8.3333303183525942e-3F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-1.6666666601721269e-1F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(1.0F));

		result = vmulq_f32(result, x);
		return result;
#else
		scalarf x = scalar_sin(scalarf(vector_get_x(input)));
		scalarf y = scalar_sin(scalarf(vector_get_y(input)));
		scalarf z = scalar_sin(scalarf(vector_get_z(input)));
		scalarf w = scalar_sin(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_asin(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use a degree 7 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		// Use the sign bit to generate our absolute value since we'll re-use that constant
		const __m128 sign_bit = _mm_set_ps1(-0.0F);
		__m128 abs_value = _mm_andnot_ps(sign_bit, input);

		// Calculate our value
		__m128 result = _mm_add_ps(_mm_mul_ps(abs_value, _mm_set_ps1(-1.2690614339589956e-3F)), _mm_set_ps1(6.7072304676685235e-3F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-1.7162031184398074e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(3.0961594977611639e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-5.0207843052845647e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(8.8986946573346160e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-2.1459960076929829e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(1.5707963267948966F));

		// Scale our result
		__m128 scale = _mm_sqrt_ps(_mm_sub_ps(_mm_set_ps1(1.0F), abs_value));
		result = _mm_mul_ps(result, scale);

		// Normally the math is as follow:
		// If input is positive: PI/2 - result
		// If input is negative: PI/2 - (PI - result) = PI/2 - PI + result = -PI/2 + result

		// As such, the offset is PI/2 and it takes the sign of the input
		// This allows us to load a single constant from memory directly
		__m128 input_sign = _mm_and_ps(input, sign_bit);
		__m128 offset = _mm_or_ps(input_sign, _mm_set_ps1(constants::half_pi()));

		// And our result has the opposite sign of the input
		result = _mm_xor_ps(result, _mm_xor_ps(input_sign, sign_bit));
		return _mm_add_ps(result, offset);
#else
		scalarf x = scalar_asin(scalarf(vector_get_x(input)));
		scalarf y = scalar_asin(scalarf(vector_get_y(input)));
		scalarf z = scalar_asin(scalarf(vector_get_z(input)));
		scalarf w = scalar_asin(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_cos(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		__m128 quotient = _mm_mul_ps(input, _mm_set_ps1(constants::one_div_two_pi()));
		quotient = vector_round_bankers(quotient);
		quotient = _mm_mul_ps(quotient, _mm_set_ps1(constants::two_pi()));
		__m128 x = _mm_sub_ps(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 x_sign = _mm_and_ps(x, sign_mask);
		__m128 reference = _mm_or_ps(x_sign, _mm_set_ps1(constants::pi()));
		const __m128 reflection = _mm_sub_ps(reference, x);

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));
		__m128 is_less_equal_than_half_pi = _mm_cmple_ps(x_abs, _mm_set_ps1(constants::half_pi()));

		x = RTM_VECTOR4F_SELECT(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		const __m128 x2 = _mm_mul_ps(x, x);
		__m128 result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.6051615464872668e-7F)), _mm_set_ps1(2.4760495088926859e-5F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-1.3888377661039897e-3F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(4.1666638865338612e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-4.9999999508695869e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.0F));

		// Remap into [-pi, pi]
		return _mm_or_ps(result, _mm_andnot_ps(is_less_equal_than_half_pi, sign_mask));
#elif defined(RTM_NEON_INTRINSICS)
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		float32x4_t quotient = vmulq_n_f32(input, constants::one_div_two_pi());
		quotient = vector_round_bankers(quotient);
		quotient = vmulq_n_f32(quotient, constants::two_pi());
		float32x4_t x = vsubq_f32(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		uint32x4_t sign_mask = vreinterpretq_u32_f32(vdupq_n_f32(-0.0F));
		uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(x), sign_mask);
		float32x4_t reference = vreinterpretq_f32_u32(vorrq_u32(sign, vreinterpretq_u32_f32(vdupq_n_f32(constants::pi()))));

		float32x4_t reflection = vsubq_f32(reference, x);
#if !defined(RTM_IMPL_VCA_SUPPORTED)
		uint32x4_t is_less_equal_than_half_pi = vcleq_f32(vabsq_f32(x), vdupq_n_f32(constants::half_pi()));
#else
		uint32x4_t is_less_equal_than_half_pi = vcaleq_f32(x, vdupq_n_f32(constants::half_pi()));
#endif
		x = vbslq_f32(is_less_equal_than_half_pi, x, reflection);

		// Calculate our value
		float32x4_t x2 = vmulq_f32(x, x);

		float32x4_t result = RTM_VECTOR4F_MULS_ADD(x2, -2.6051615464872668e-7F, vdupq_n_f32(2.4760495088926859e-5F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-1.3888377661039897e-3F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(4.1666638865338612e-2F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-4.9999999508695869e-1F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(1.0F));

		// Remap into [-pi, pi]
		return vbslq_f32(is_less_equal_than_half_pi, result, vnegq_f32(result));
#else
		scalarf x = scalar_cos(scalarf(vector_get_x(input)));
		scalarf y = scalar_cos(scalarf(vector_get_y(input)));
		scalarf z = scalar_cos(scalarf(vector_get_z(input)));
		scalarf w = scalar_cos(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_acos(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use the identity: acos(value) + asin(value) = PI/2
		// This ends up being: acos(value) = PI/2 - asin(value)
		// Since asin(value) = PI/2 - sqrt(1.0 - polynomial(value))
		// Our end result is acos(value) = sqrt(1.0 - polynomial(value))
		// This means we can re-use the same polynomial as asin()
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// We first calculate our scale: sqrt(1.0 - abs(value))
		// Use the sign bit to generate our absolute value since we'll re-use that constant
		const __m128 sign_bit = _mm_set_ps1(-0.0F);
		__m128 abs_value = _mm_andnot_ps(sign_bit, input);

		// Calculate our value
		__m128 result = _mm_add_ps(_mm_mul_ps(abs_value, _mm_set_ps1(-1.2690614339589956e-3F)), _mm_set_ps1(6.7072304676685235e-3F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-1.7162031184398074e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(3.0961594977611639e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-5.0207843052845647e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(8.8986946573346160e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-2.1459960076929829e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(1.5707963267948966F));

		// Scale our result
		__m128 scale = _mm_sqrt_ps(_mm_sub_ps(_mm_set_ps1(1.0F), abs_value));
		result = _mm_mul_ps(result, scale);

		// Normally the math is as follow:
		// If input is positive: result
		// If input is negative: PI - result = -result + PI

		// As such, the offset is 0.0 when the input is positive and PI when negative
		__m128 is_input_negative = _mm_cmplt_ps(input, _mm_setzero_ps());
		__m128 offset = _mm_and_ps(is_input_negative, _mm_set_ps1(constants::pi()));

		// And our result has the same sign of the input
		__m128 input_sign = _mm_and_ps(input, sign_bit);
		result = _mm_or_ps(result, input_sign);
		return _mm_add_ps(result, offset);
#else
		scalarf x = scalar_acos(scalarf(vector_get_x(input)));
		scalarf y = scalar_acos(scalarf(vector_get_y(input)));
		scalarf z = scalar_acos(scalarf(vector_get_z(input)));
		scalarf w = scalar_acos(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sine and cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void RTM_SIMD_CALL vector_sincos(vector4f_arg0 input, vector4f& out_sine, vector4f& out_cosine) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		__m128 quotient = _mm_mul_ps(input, _mm_set_ps1(constants::one_div_two_pi()));
		quotient = vector_round_bankers(quotient);
		quotient = _mm_mul_ps(quotient, _mm_set_ps1(constants::two_pi()));
		__m128 x = _mm_sub_ps(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 x_sign = _mm_and_ps(x, sign_mask);
		__m128 reference = _mm_or_ps(x_sign, _mm_set_ps1(constants::pi()));
		const __m128 reflection = _mm_sub_ps(reference, x);

		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));
		__m128 is_less_equal_than_half_pi = _mm_cmple_ps(x_abs, _mm_set_ps1(constants::half_pi()));

		x = RTM_VECTOR4F_SELECT(is_less_equal_than_half_pi, x, reflection);
		const __m128 x2 = _mm_mul_ps(x, x);

		// Calculate our cosine value
		__m128 cosine_result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.6051615464872668e-7F)), _mm_set_ps1(2.4760495088926859e-5F));
		cosine_result = _mm_add_ps(_mm_mul_ps(cosine_result, x2), _mm_set_ps1(-1.3888377661039897e-3F));
		cosine_result = _mm_add_ps(_mm_mul_ps(cosine_result, x2), _mm_set_ps1(4.1666638865338612e-2F));
		cosine_result = _mm_add_ps(_mm_mul_ps(cosine_result, x2), _mm_set_ps1(-4.9999999508695869e-1F));
		cosine_result = _mm_add_ps(_mm_mul_ps(cosine_result, x2), _mm_set_ps1(1.0F));

		// Remap into [-pi, pi]
		out_cosine = _mm_or_ps(cosine_result, _mm_andnot_ps(is_less_equal_than_half_pi, sign_mask));

		// Calculate our sine value
		__m128 sine_result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.3828544692960918e-8F)), _mm_set_ps1(2.7521557770526783e-6F));
		sine_result = _mm_add_ps(_mm_mul_ps(sine_result, x2), _mm_set_ps1(-1.9840782426250314e-4F));
		sine_result = _mm_add_ps(_mm_mul_ps(sine_result, x2), _mm_set_ps1(8.3333303183525942e-3F));
		sine_result = _mm_add_ps(_mm_mul_ps(sine_result, x2), _mm_set_ps1(-1.6666666601721269e-1F));
		sine_result = _mm_add_ps(_mm_mul_ps(sine_result, x2), _mm_set_ps1(1.0F));
		out_sine = _mm_mul_ps(sine_result, x);
#elif defined(RTM_NEON_INTRINSICS)
		// Use a degree 10 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Remap our input in the [-pi, pi] range
		float32x4_t quotient = vmulq_n_f32(input, constants::one_div_two_pi());
		quotient = vector_round_bankers(quotient);
		quotient = vmulq_n_f32(quotient, constants::two_pi());
		float32x4_t x = vsubq_f32(input, quotient);

		// Remap our input in the [-pi/2, pi/2] range
		const uint32x4_t sign_mask = vreinterpretq_u32_f32(vdupq_n_f32(-0.0F));
		const uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(x), sign_mask);
		const float32x4_t reference = vreinterpretq_f32_u32(vorrq_u32(sign, vreinterpretq_u32_f32(vdupq_n_f32(constants::pi()))));

		const float32x4_t reflection = vsubq_f32(reference, x);
#if !defined(RTM_IMPL_VCA_SUPPORTED)
		const uint32x4_t is_less_equal_than_half_pi = vcleq_f32(vabsq_f32(x), vdupq_n_f32(constants::half_pi()));
#else
		const uint32x4_t is_less_equal_than_half_pi = vcaleq_f32(x, vdupq_n_f32(constants::half_pi()));
#endif
		x = vbslq_f32(is_less_equal_than_half_pi, x, reflection);
		const float32x4_t x2 = vmulq_f32(x, x);

		// Calculate our cosine value
		float32x4_t cosine_result = RTM_VECTOR4F_MULS_ADD(x2, -2.6051615464872668e-7F, vdupq_n_f32(2.4760495088926859e-5F));
		cosine_result = RTM_VECTOR4F_MULV_ADD(cosine_result, x2, vdupq_n_f32(-1.3888377661039897e-3F));
		cosine_result = RTM_VECTOR4F_MULV_ADD(cosine_result, x2, vdupq_n_f32(4.1666638865338612e-2F));
		cosine_result = RTM_VECTOR4F_MULV_ADD(cosine_result, x2, vdupq_n_f32(-4.9999999508695869e-1F));
		cosine_result = RTM_VECTOR4F_MULV_ADD(cosine_result, x2, vdupq_n_f32(1.0F));

		// Remap into [-pi, pi]
		out_cosine = vbslq_f32(is_less_equal_than_half_pi, cosine_result, vnegq_f32(cosine_result));

		// Calculate our sine value
		float32x4_t sine_result = RTM_VECTOR4F_MULS_ADD(x2, -2.3828544692960918e-8F, vdupq_n_f32(2.7521557770526783e-6F));
		sine_result = RTM_VECTOR4F_MULV_ADD(sine_result, x2, vdupq_n_f32(-1.9840782426250314e-4F));
		sine_result = RTM_VECTOR4F_MULV_ADD(sine_result, x2, vdupq_n_f32(8.3333303183525942e-3F));
		sine_result = RTM_VECTOR4F_MULV_ADD(sine_result, x2, vdupq_n_f32(-1.6666666601721269e-1F));
		sine_result = RTM_VECTOR4F_MULV_ADD(sine_result, x2, vdupq_n_f32(1.0F));

		out_sine = vmulq_f32(sine_result, x);
#else
		const vector4f x = scalar_sincos(scalarf(vector_get_x(input)));
		const vector4f y = scalar_sincos(scalarf(vector_get_y(input)));
		const vector4f z = scalar_sincos(scalarf(vector_get_z(input)));
		const vector4f w = scalar_sincos(scalarf(vector_get_w(input)));

		out_cosine = vector4f{ x.y, y.y, z.y, w.y };
		out_sine = vector4f{ x.x, y.x, z.x, w.x };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_tan(vector4f_arg0 angle) RTM_NO_EXCEPT
	{
		// Use the identity: tan(angle) = sin(angle) / cos(angle)
		vector4f sin_;
		vector4f cos_;
		vector_sincos(angle, sin_, cos_);

		const mask4f is_cos_zero = vector_equal(cos_, vector_zero());
		const vector4f signed_infinity = vector_copy_sign(vector_set(std::numeric_limits<float>::infinity()), angle);
		const vector4f result = vector_div(sin_, cos_);
		return vector_select(is_cos_zero, signed_infinity, result);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_atan(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// Use a degree 13 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Discard our sign, we'll restore it later
		const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
		__m128 abs_value = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));

		// Compute our value
		__m128 is_larger_than_one = _mm_cmpgt_ps(abs_value, _mm_set_ps1(1.0F));
		__m128 reciprocal = vector_reciprocal(abs_value);

		__m128 x = vector_select(is_larger_than_one, reciprocal, abs_value);

		__m128 x2 = _mm_mul_ps(x, x);

		__m128 result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(7.2128853633444123e-3F)), _mm_set_ps1(-3.5059680836411644e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(8.1675882859940430e-2F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-1.3374657325451267e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.9856563505717162e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-3.3324998579202170e-1F));
		result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.0F));
		result = _mm_mul_ps(result, x);

		__m128 remapped = _mm_sub_ps(_mm_set_ps1(constants::half_pi()), result);

		// pi/2 - result
		result = vector_select(is_larger_than_one, remapped, result);

		// Keep the original sign
		return _mm_or_ps(result, _mm_and_ps(input, _mm_set_ps1(-0.0F)));
#elif defined(RTM_NEON_INTRINSICS)
		// Use a degree 13 minimax approximation polynomial
		// See: GPGPU Programming for Games and Science (David H. Eberly)

		// Discard our sign, we'll restore it later
		float32x4_t abs_value = vabsq_f32(input);

		// Compute our value
#if !defined(RTM_IMPL_VCA_SUPPORTED)
		uint32x4_t is_larger_than_one = vcgtq_f32(vabsq_f32(input), vdupq_n_f32(1.0F));
#else
		uint32x4_t is_larger_than_one = vcagtq_f32(input, vdupq_n_f32(1.0F));
#endif
		float32x4_t reciprocal = vector_reciprocal(abs_value);

		float32x4_t x = vbslq_f32(is_larger_than_one, reciprocal, abs_value);

		float32x4_t x2 = vmulq_f32(x, x);

		float32x4_t result = RTM_VECTOR4F_MULS_ADD(x2, 7.2128853633444123e-3F, vdupq_n_f32(-3.5059680836411644e-2F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(8.1675882859940430e-2F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-1.3374657325451267e-1F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(1.9856563505717162e-1F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(-3.3324998579202170e-1F));
		result = RTM_VECTOR4F_MULV_ADD(result, x2, vdupq_n_f32(1.0F));

		result = vmulq_f32(result, x);

		float32x4_t remapped = vsubq_f32(vdupq_n_f32(constants::half_pi()), result);

		// pi/2 - result
		result = vbslq_f32(is_larger_than_one, remapped, result);

		// Keep the original sign
		return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(result), vandq_u32(vreinterpretq_u32_f32(input), vreinterpretq_u32_f32(vdupq_n_f32(-0.0F)))));
#else
		scalarf x = scalar_atan(scalarf(vector_get_x(input)));
		scalarf y = scalar_atan(scalarf(vector_get_y(input)));
		scalarf z = scalar_atan(scalarf(vector_get_z(input)));
		scalarf w = scalar_atan(scalarf(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4f RTM_SIMD_CALL vector_atan2(vector4f_arg0 y, vector4f_arg1 x) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

		const __m128 zero = _mm_setzero_ps();
		__m128 is_x_zero = _mm_cmpeq_ps(x, zero);
		__m128 is_y_zero = _mm_cmpeq_ps(y, zero);
		__m128 inputs_are_zero = _mm_and_ps(is_x_zero, is_y_zero);

		__m128 is_x_positive = _mm_cmpgt_ps(x, zero);

		const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
		__m128 y_sign = _mm_and_ps(y, sign_mask);

		// If X == 0.0, our offset is PI/2 otherwise it is PI both with the sign of Y
		__m128 half_pi = _mm_set_ps1(constants::half_pi());
		__m128 pi = _mm_set_ps1(constants::pi());
		__m128 offset = _mm_or_ps(_mm_and_ps(is_x_zero, half_pi), _mm_andnot_ps(is_x_zero, pi));
		offset = _mm_or_ps(offset, y_sign);

		// If X > 0.0, our offset is 0.0
		offset = _mm_andnot_ps(is_x_positive, offset);

		// If X == 0.0 and Y == 0.0, our offset is 0.0
		offset = _mm_andnot_ps(inputs_are_zero, offset);

		__m128 angle = _mm_div_ps(y, x);
		__m128 value = vector_atan(angle);

		// If X == 0.0, our value is 0.0 otherwise it is atan(y/x)
		value = _mm_andnot_ps(is_x_zero, value);

		// If X == 0.0 and Y == 0.0, our value is 0.0
		value = _mm_andnot_ps(inputs_are_zero, value);

		return _mm_add_ps(value, offset);
#elif defined(RTM_NEON64_INTRINSICS)
		// If X == 0.0 and Y != 0.0, we return PI/2 with the sign of Y
		// If X == 0.0 and Y == 0.0, we return 0.0
		// If X > 0.0, we return atan(y/x)
		// If X < 0.0, we return atan(y/x) + sign(Y) * PI
		// See: https://en.wikipedia.org/wiki/Atan2#Definition_and_computation

#if !defined(RTM_IMPL_VCZ_SUPPORTED)
		float32x4_t zero = vdupq_n_f32(0.0F);
		uint32x4_t is_x_zero = vceqq_f32(x, zero);
		uint32x4_t is_y_zero = vceqq_f32(y, zero);
		uint32x4_t inputs_are_zero = vandq_u32(is_x_zero, is_y_zero);

		uint32x4_t is_x_positive = vcgtq_f32(x, zero);
#else
		uint32x4_t is_x_zero = vceqzq_f32(x);
		uint32x4_t is_y_zero = vceqzq_f32(y);
		uint32x4_t inputs_are_zero = vandq_u32(is_x_zero, is_y_zero);

		uint32x4_t is_x_positive = vcgtzq_f32(x);
#endif

		uint32x4_t y_sign = vandq_u32(vreinterpretq_u32_f32(y), vreinterpretq_u32_f32(vdupq_n_f32(-0.0F)));

		// If X == 0.0, our offset is PI/2 otherwise it is PI both with the sign of Y
		float32x4_t half_pi = vdupq_n_f32(constants::half_pi());
		float32x4_t pi = vdupq_n_f32(constants::pi());
		float32x4_t offset = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(is_x_zero, vreinterpretq_u32_f32(half_pi)), vandq_u32(vmvnq_u32(is_x_zero), vreinterpretq_u32_f32(pi))));
		offset = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(offset), y_sign));

		// If X > 0.0, our offset is 0.0
		offset = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(is_x_positive), vreinterpretq_u32_f32(offset)));

		// If X == 0.0 and Y == 0.0, our offset is 0.0
		offset = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(inputs_are_zero), vreinterpretq_u32_f32(offset)));

		float32x4_t angle = vector_div(y, x);
		float32x4_t value = vector_atan(angle);

		// If X == 0.0, our value is 0.0 otherwise it is atan(y/x)
		value = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(is_x_zero), vreinterpretq_u32_f32(value)));

		// If X == 0.0 and Y == 0.0, our value is 0.0
		value = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(inputs_are_zero), vreinterpretq_u32_f32(value)));

		return vaddq_f32(value, offset);
#else
		scalarf x_ = scalar_atan2(scalarf(vector_get_x(y)), scalarf(vector_get_x(x)));
		scalarf y_ = scalar_atan2(scalarf(vector_get_y(y)), scalarf(vector_get_y(x)));
		scalarf z_ = scalar_atan2(scalarf(vector_get_z(y)), scalarf(vector_get_z(x)));
		scalarf w_ = scalar_atan2(scalarf(vector_get_w(y)), scalarf(vector_get_w(x)));
		return vector_set(x_, y_, z_, w_);
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
