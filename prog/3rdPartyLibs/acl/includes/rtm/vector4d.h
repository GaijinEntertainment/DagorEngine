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
#include "rtm/scalard.h"
#include "rtm/version.h"
#include "rtm/impl/bit_cast.impl.h"
#include "rtm/impl/compiler_utils.h"
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load(const double* input) RTM_NO_EXCEPT
	{
		return vector_set(input[0], input[1], input[2], input[3]);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an input scalar from memory into the [x] component and sets the [yzw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load1(const double* input) RTM_NO_EXCEPT
	{
		return vector_set(input[0], 0.0, 0.0, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector2 from memory and sets the [zw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load2(const double* input) RTM_NO_EXCEPT
	{
		return vector_set(input[0], input[1], 0.0, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector3 from memory and sets the [w] component to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load3(const double* input) RTM_NO_EXCEPT
	{
		return vector_set(input[0], input[1], input[2], 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector4 from memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load(const float4d* input) RTM_NO_EXCEPT
	{
		return vector_set(input->x, input->y, input->z, input->w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector2 from memory and sets the [zw] components to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load2(const float2d* input) RTM_NO_EXCEPT
	{
		return vector_set(input->x, input->y, 0.0, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an unaligned vector3 from memory and sets the [w] component to zero.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_load3(const float3d* input) RTM_NO_EXCEPT
	{
		return vector_set(input->x, input->y, input->z, 0.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Loads an input scalar from memory into the [xyzw] components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_broadcast(const double* input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128d value = _mm_load1_pd(input);
		return vector4d{ value, value };
#else
		return vector_set(*input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a quaternion to a vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL quat_to_vector(quatd_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ input.xy, input.zw };
#else
		return vector4d{ input.x, input.y, input.z, input.w };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Casts a vector4 float32 variant to a float64 variant.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_cast(vector4f_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_cvtps_pd(input), _mm_cvtps_pd(_mm_shuffle_ps(input, input, _MM_SHUFFLE(3, 2, 3, 2))) };
#elif defined(RTM_NEON_INTRINSICS)
		return vector4d{ double(vgetq_lane_f32(input, 0)), double(vgetq_lane_f32(input, 1)), double(vgetq_lane_f32(input, 2)), double(vgetq_lane_f32(input, 3)) };
#else
		return vector4d{ double(input.x), double(input.y), double(input.z), double(input.w) };
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
		struct vector4d_vector_get_x
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(input.xy);
#else
				return input.x;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ input.xy };
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_x RTM_SIMD_CALL vector_get_x(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_x{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [x] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_x_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ input.xy };
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
		struct vector4d_vector_get_y
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(_mm_shuffle_pd(input.xy, input.xy, 1));
#else
				return input.y;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ _mm_shuffle_pd(input.xy, input.xy, 1) };
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_y RTM_SIMD_CALL vector_get_y(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_y{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [y] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_y_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_shuffle_pd(input.xy, input.xy, 1) };
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
		struct vector4d_vector_get_z
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(input.zw);
#else
				return input.z;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ input.zw };
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_z RTM_SIMD_CALL vector_get_z(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_z{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [z] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_z_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ input.zw };
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
		struct vector4d_vector_get_w
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				return _mm_cvtsd_f64(_mm_shuffle_pd(input.zw, input.zw, 1));
#else
				return input.w;
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				return scalard{ _mm_shuffle_pd(input.zw, input.zw, 1) };
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_w RTM_SIMD_CALL vector_get_w(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_w{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 [w] component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_w_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return scalard{ _mm_shuffle_pd(input.zw, input.zw, 1) };
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
		struct vector4d_vector_get_component_static
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
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
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
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

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component, component4 component_ = static_cast<component4>(component)>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component_static<component_> RTM_SIMD_CALL vector_get_component2(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component_static<component_>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component2_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component_static<component_> RTM_SIMD_CALL vector_get_component3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component_static<component_>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component3 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component3_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component_static<component> RTM_SIMD_CALL vector_get_component(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component_static<component>{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	template<component4 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
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
		struct vector4d_vector_get_component
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
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
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
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

			vector4d input;
			component4 component;
			int32_t padding[3];
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component RTM_SIMD_CALL vector_get_component2(vector4d_arg0 input, component2 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component{ input, static_cast<component4>(component), { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector2 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component2_as_scalar(vector4d_arg0 input, component2 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component RTM_SIMD_CALL vector_get_component3(vector4d_arg0 input, component3 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component{ input, static_cast<component4>(component), { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector3 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component3_as_scalar(vector4d_arg0 input, component3 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_get_component RTM_SIMD_CALL vector_get_component(vector4d_arg0 input, component4 component) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_get_component{ input, component, { 0 } };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the vector4 desired component.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_component_as_scalar(vector4d_arg0 input, component4 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_get_min_component RTM_SIMD_CALL vector_get_min_component(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_get_min_component{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the smallest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_min_component_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xz_yw = _mm_min_pd(input.xy, input.zw);
		__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
		return scalard{ _mm_min_pd(xz_yw, yw_yw) };
#else
		return vector_get_min_component(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_get_max_component RTM_SIMD_CALL vector_get_max_component(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_get_max_component{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the largest component in the input vector as a scalar.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_get_max_component_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xz_yw = _mm_max_pd(input.xy, input.zw);
		__m128d yw_yw = _mm_shuffle_pd(xz_yw, xz_yw, 1);
		return scalard{ _mm_max_pd(xz_yw, yw_yw) };
#else
		return vector_get_max_component(input);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [x] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_x(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_move_sd(input.xy, _mm_set_sd(lane_value)), input.zw };
#else
		return vector4d{ lane_value, input.y, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [x] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_x(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return vector4d{ _mm_move_sd(input.xy, lane_value.value), input.zw };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [y] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_y(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_shuffle_pd(input.xy, _mm_set_sd(lane_value), 0), input.zw };
#else
		return vector4d{ input.x, lane_value, input.z, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [y] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_y(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return vector4d{ _mm_shuffle_pd(input.xy, lane_value.value, 0), input.zw };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [z] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_z(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ input.xy, _mm_move_sd(input.zw, _mm_set_sd(lane_value)) };
#else
		return vector4d{ input.x, input.y, lane_value, input.w };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [z] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_z(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return vector4d{ input.xy, _mm_move_sd(input.zw, lane_value.value) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [w] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_w(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ input.xy, _mm_shuffle_pd(input.zw, _mm_set_sd(lane_value), 0) };
#else
		return vector4d{ input.x, input.y, input.z, lane_value };
#endif
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Sets the vector4 [w] component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_w(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
	{
		return vector4d{ input.xy, _mm_shuffle_pd(input.zw, lane_value.value, 0) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Sets the desired vector2 component and returns the new value.
	//////////////////////////////////////////////////////////////////////////
	template<component2 component>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component2(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component2(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component3(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component3(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component(vector4d_arg0 input, double lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component(vector4d_arg0 input, scalard_arg2 lane_value) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component2(vector4d_arg0 input, double lane_value, component2 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component2(vector4d_arg0 input, scalard_arg2 lane_value, component2 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component3(vector4d_arg0 input, double lane_value, component3 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component3(vector4d_arg0 input, scalard_arg2 lane_value, component3 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component(vector4d_arg0 input, double lane_value, component4 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_set_component(vector4d_arg0 input, scalard_arg2 lane_value, component4 component) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE const double* vector_to_pointer(const vector4d& input) RTM_NO_EXCEPT
	{
		return rtm_impl::bit_cast<const double*>(&input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4d_arg0 input, double* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
		output[2] = vector_get_z(input);
		output[3] = vector_get_w(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector1 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store1(vector4d_arg0 input, double* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4d_arg0 input, double* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4d_arg0 input, double* output) RTM_NO_EXCEPT
	{
		output[0] = vector_get_x(input);
		output[1] = vector_get_y(input);
		output[2] = vector_get_z(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4d_arg0 input, uint8_t* output) RTM_NO_EXCEPT
	{
		std::memcpy(output, &input, sizeof(vector4d));
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector1 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store1(vector4d_arg0 input, uint8_t* output)
	{
		std::memcpy(output, &input, sizeof(double) * 1);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4d_arg0 input, uint8_t* output)
	{
		std::memcpy(output, &input, sizeof(double) * 2);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4d_arg0 input, uint8_t* output)
	{
		std::memcpy(output, &input, sizeof(double) * 3);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector4 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store(vector4d_arg0 input, float4d* output) RTM_NO_EXCEPT
	{
		output->x = vector_get_x(input);
		output->y = vector_get_y(input);
		output->z = vector_get_z(input);
		output->w = vector_get_w(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector2 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store2(vector4d_arg0 input, float2d* output) RTM_NO_EXCEPT
	{
		output->x = vector_get_x(input);
		output->y = vector_get_y(input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Writes a vector3 to unaligned memory.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE void RTM_SIMD_CALL vector_store3(vector4d_arg0 input, float3d* output) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_add(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_add_pd(lhs.xy, rhs.xy), _mm_add_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component subtraction of the two inputs: lhs - rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_sub(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_sub_pd(lhs.xy, rhs.xy), _mm_sub_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the two inputs: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_mul_pd(lhs.xy, rhs.xy), _mm_mul_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the vector by a scalar: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul(vector4d_arg0 lhs, double rhs) RTM_NO_EXCEPT
	{
		return vector_mul(lhs, vector_set(rhs));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication of the vector by a scalar: lhs * rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul(vector4d_arg0 lhs, scalard_arg2 rhs) RTM_NO_EXCEPT
	{
		const __m128d rhs_xx = _mm_shuffle_pd(rhs.value, rhs.value, 0);
		return vector4d{ _mm_mul_pd(lhs.xy, rhs_xx), _mm_mul_pd(lhs.zw, rhs_xx) };
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component division of the two inputs: lhs / rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_div(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_div_pd(lhs.xy, rhs.xy), _mm_div_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component maximum of the two inputs: max(lhs, rhs)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_max(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_max_pd(lhs.xy, rhs.xy), _mm_max_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(scalar_max(lhs.x, rhs.x), scalar_max(lhs.y, rhs.y), scalar_max(lhs.z, rhs.z), scalar_max(lhs.w, rhs.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component minimum of the two inputs: min(lhs, rhs)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_min(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_min_pd(lhs.xy, rhs.xy), _mm_min_pd(lhs.zw, rhs.zw) };
#else
		return vector_set(scalar_min(lhs.x, rhs.x), scalar_min(lhs.y, rhs.y), scalar_min(lhs.z, rhs.z), scalar_min(lhs.w, rhs.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component clamping of an input between a minimum and a maximum value: min(max_value, max(min_value, input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_clamp(vector4d_arg0 input, vector4d_arg1 min_value, vector4d_arg2 max_value) RTM_NO_EXCEPT
	{
		return vector_min(max_value, vector_max(min_value, input));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component absolute of the input: abs(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_abs(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		vector4d zero{ _mm_setzero_pd(), _mm_setzero_pd() };
		return vector_max(vector_sub(zero, input), input);
#else
		return vector_set(scalar_abs(input.x), scalar_abs(input.y), scalar_abs(input.z), scalar_abs(input.w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negation of the input: -input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_neg(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_mul(input, -1.0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negation of the input: -input
	// Each template argument controls whether a SIMD lane should be negated (true) or not (false).
	//////////////////////////////////////////////////////////////////////////
	template<bool x, bool y, bool z, bool w>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_neg(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		constexpr __m128d signs_xy = RTM_VECTOR2D_MAKE(x ? -0.0 : 0.0, y ? -0.0 : 0.0);
		constexpr __m128d signs_zw = RTM_VECTOR2D_MAKE(z ? -0.0 : 0.0, w ? -0.0 : 0.0);
		return vector4d{ _mm_xor_pd(input.xy, signs_xy), _mm_xor_pd(input.zw, signs_zw) };
#else
		const vector4d signs = vector_set(x ? -1.0 : 1.0, y ? -1.0 : 1.0, z ? -1.0 : 1.0, w ? -1.0 : 1.0);
		return vector_mul(input, signs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component reciprocal of the input: 1.0 / input
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_reciprocal(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		// Performance note:
		// With modern out-of-order executing processors, it is typically faster to use
		// a full division instead of a reciprocal estimate + Newton-Raphson iterations
		// because the resulting code is more dense and is more likely to inline and
		// as it uses fewer instructions.
		return vector_div(vector_set(1.0), input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component square root of the input: sqrt(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_sqrt(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		return vector4d{ _mm_sqrt_pd(input.xy), _mm_sqrt_pd(input.zw) };
#else
		scalard x = vector_get_x(input);
		scalard y = vector_get_y(input);
		scalard z = vector_get_z(input);
		scalard w = vector_get_w(input);
		return vector_set(scalar_sqrt(x), scalar_sqrt(y), scalar_sqrt(z), scalar_sqrt(w));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component reciprocal square root of the input: 1.0 / sqrt(input)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_sqrt_reciprocal(vector4d_arg0 input) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_ceil(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large_xy = _mm_cmpge_pd(abs_input_xy, fractional_limit);
		__m128d is_input_large_zw = _mm_cmpge_pd(abs_input_zw, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input_xy = _mm_or_pd(is_input_large_xy, is_nan_xy);
		__m128d use_original_input_zw = _mm_or_pd(is_input_large_zw, is_nan_zw);

		// Convert to an integer and back
		__m128d integer_part_xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.xy));
		__m128d integer_part_zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.zw));

		// Test if the returned value is smaller than the original.
		// A positive input will round towards zero and be lower when we need it to be greater.
		__m128d is_positive_xy = _mm_cmplt_pd(integer_part_xy, input.xy);
		__m128d is_positive_zw = _mm_cmplt_pd(integer_part_zw, input.zw);

		// Our mask output is 64 bit wide but to convert to a bias, we need 32 bit integers
		is_positive_xy = _mm_castps_pd(_mm_shuffle_ps(_mm_castpd_ps(is_positive_xy), _mm_castpd_ps(is_positive_xy), _MM_SHUFFLE(2, 0, 2, 0)));
		is_positive_zw = _mm_castps_pd(_mm_shuffle_ps(_mm_castpd_ps(is_positive_zw), _mm_castpd_ps(is_positive_zw), _MM_SHUFFLE(2, 0, 2, 0)));

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Negative values will yield a 0.0 bias
		__m128d bias_xy = _mm_cvtepi32_pd(_mm_castpd_si128(is_positive_xy));
		__m128d bias_zw = _mm_cvtepi32_pd(_mm_castpd_si128(is_positive_zw));

		// Subtract our bias to properly handle positive values
		integer_part_xy = _mm_sub_pd(integer_part_xy, bias_xy);
		integer_part_zw = _mm_sub_pd(integer_part_zw, bias_zw);

		__m128d result_xy = _mm_or_pd(_mm_and_pd(use_original_input_xy, input.xy), _mm_andnot_pd(use_original_input_xy, integer_part_xy));
		__m128d result_zw = _mm_or_pd(_mm_and_pd(use_original_input_zw, input.zw), _mm_andnot_pd(use_original_input_zw, integer_part_zw));
		return vector4d{ result_xy, result_zw };
#else
		return vector_set(scalar_ceil(vector_get_x(input)), scalar_ceil(vector_get_y(input)), scalar_ceil(vector_get_z(input)), scalar_ceil(vector_get_w(input)));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component returns the largest integer value not greater than the input (round towards negative infinity).
	// vector_floor([1.8, 1.0, -1.8, -1.0]) = [1.0, 1.0, -2.0, -1.0]
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_floor(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return vector4d{ _mm_floor_pd(input.xy), _mm_floor_pd(input.zw) };
#elif defined(RTM_SSE2_INTRINSICS)
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large_xy = _mm_cmpge_pd(abs_input_xy, fractional_limit);
		__m128d is_input_large_zw = _mm_cmpge_pd(abs_input_zw, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input_xy = _mm_or_pd(is_input_large_xy, is_nan_xy);
		__m128d use_original_input_zw = _mm_or_pd(is_input_large_zw, is_nan_zw);

		// Convert to an integer and back
		__m128d integer_part_xy = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.xy));
		__m128d integer_part_zw = _mm_cvtepi32_pd(_mm_cvtpd_epi32(input.zw));

		// Test if the returned value is greater than the original.
		// A negative input will round towards zero and be greater when we need it to be smaller.
		__m128d is_negative_xy = _mm_cmpgt_pd(integer_part_xy, input.xy);
		__m128d is_negative_zw = _mm_cmpgt_pd(integer_part_zw, input.zw);

		// Our mask output is 64 bit wide but to convert to a bias, we need 32 bit integers
		is_negative_xy = _mm_castps_pd(_mm_shuffle_ps(_mm_castpd_ps(is_negative_xy), _mm_castpd_ps(is_negative_xy), _MM_SHUFFLE(2, 0, 2, 0)));
		is_negative_zw = _mm_castps_pd(_mm_shuffle_ps(_mm_castpd_ps(is_negative_zw), _mm_castpd_ps(is_negative_zw), _MM_SHUFFLE(2, 0, 2, 0)));

		// Convert our mask to a float, ~0 yields -1.0 since it is a valid signed integer
		// Positive values will yield a 0.0 bias
		__m128d bias_xy = _mm_cvtepi32_pd(_mm_castpd_si128(is_negative_xy));
		__m128d bias_zw = _mm_cvtepi32_pd(_mm_castpd_si128(is_negative_zw));

		// Add our bias to properly handle negative values
		integer_part_xy = _mm_add_pd(integer_part_xy, bias_xy);
		integer_part_zw = _mm_add_pd(integer_part_zw, bias_zw);

		__m128d result_xy = _mm_or_pd(_mm_and_pd(use_original_input_xy, input.xy), _mm_andnot_pd(use_original_input_xy, integer_part_xy));
		__m128d result_zw = _mm_or_pd(_mm_and_pd(use_original_input_zw, input.zw), _mm_andnot_pd(use_original_input_zw, integer_part_zw));
		return vector4d{ result_xy, result_zw };
#else
		return vector_set(scalar_floor(vector_get_x(input)), scalar_floor(vector_get_y(input)), scalar_floor(vector_get_z(input)), scalar_floor(vector_get_w(input)));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D cross product: lhs x rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_cross3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		// cross(a, b) = (a.yzx * b.zxy) - (a.zxy * b.yzx)
		const double lhs_x = vector_get_x(lhs);
		const double lhs_y = vector_get_y(lhs);
		const double lhs_z = vector_get_z(lhs);
		const double rhs_x = vector_get_x(rhs);
		const double rhs_y = vector_get_y(rhs);
		const double rhs_z = vector_get_z(rhs);
		return vector_set((lhs_y * rhs_z) - (lhs_z * rhs_y), (lhs_z * rhs_x) - (lhs_x * rhs_z), (lhs_x * rhs_y) - (lhs_y * rhs_x));
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_vector_dot
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard lhs_x = vector_get_x_as_scalar(lhs);
				const scalard lhs_y = vector_get_y_as_scalar(lhs);
				const scalard lhs_z = vector_get_z_as_scalar(lhs);
				const scalard lhs_w = vector_get_w_as_scalar(lhs);
				const scalard rhs_x = vector_get_x_as_scalar(rhs);
				const scalard rhs_y = vector_get_y_as_scalar(rhs);
				const scalard rhs_z = vector_get_z_as_scalar(rhs);
				const scalard rhs_w = vector_get_w_as_scalar(rhs);
				const scalard xx = scalar_mul(lhs_x, rhs_x);
				const scalard yy = scalar_mul(lhs_y, rhs_y);
				const scalard zz = scalar_mul(lhs_z, rhs_z);
				const scalard ww = scalar_mul(lhs_w, rhs_w);
				return scalar_cast(scalar_add(scalar_add(xx, yy), scalar_add(zz, ww)));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard lhs_x = vector_get_x_as_scalar(lhs);
				const scalard lhs_y = vector_get_y_as_scalar(lhs);
				const scalard lhs_z = vector_get_z_as_scalar(lhs);
				const scalard lhs_w = vector_get_w_as_scalar(lhs);
				const scalard rhs_x = vector_get_x_as_scalar(rhs);
				const scalard rhs_y = vector_get_y_as_scalar(rhs);
				const scalard rhs_z = vector_get_z_as_scalar(rhs);
				const scalard rhs_w = vector_get_w_as_scalar(rhs);
				const scalard xx = scalar_mul(lhs_x, rhs_x);
				const scalard yy = scalar_mul(lhs_y, rhs_y);
				const scalard zz = scalar_mul(lhs_z, rhs_z);
				const scalard ww = scalar_mul(lhs_w, rhs_w);
				return scalar_add(scalar_add(xx, yy), scalar_add(zz, ww));
			}
#endif

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
				const double dot = *this;
				return vector_set(dot);
			}

			vector4d lhs;
			vector4d rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot RTM_SIMD_CALL vector_dot(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_dot_as_scalar(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		const scalard lhs_x = vector_get_x_as_scalar(lhs);
		const scalard lhs_y = vector_get_y_as_scalar(lhs);
		const scalard lhs_z = vector_get_z_as_scalar(lhs);
		const scalard lhs_w = vector_get_w_as_scalar(lhs);
		const scalard rhs_x = vector_get_x_as_scalar(rhs);
		const scalard rhs_y = vector_get_y_as_scalar(rhs);
		const scalard rhs_z = vector_get_z_as_scalar(rhs);
		const scalard rhs_w = vector_get_w_as_scalar(rhs);
		const scalard xx = scalar_mul(lhs_x, rhs_x);
		const scalard yy = scalar_mul(lhs_y, rhs_y);
		const scalard zz = scalar_mul(lhs_z, rhs_z);
		const scalard ww = scalar_mul(lhs_w, rhs_w);
		return scalar_add(scalar_add(xx, yy), scalar_add(zz, ww));
	}

	//////////////////////////////////////////////////////////////////////////
	// 4D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dot_as_vector(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return vector_set(vector_dot_as_scalar(lhs, rhs));
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_vector_dot2
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
				__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
				return _mm_cvtsd_f64(_mm_add_sd(x2_y2, y2));
#else
				return (vector_get_x(lhs) * vector_get_x(rhs)) + (vector_get_y(lhs) * vector_get_y(rhs));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
				__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
				return scalard{ _mm_add_sd(x2_y2, y2) };
			}
#endif

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
				const double dot = *this;
				return vector_set(dot);
			}

			vector4d lhs;
			vector4d rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot2 RTM_SIMD_CALL vector_dot2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot2{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_dot2_as_scalar(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
		__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
		return scalard{ _mm_add_sd(x2_y2, y2) };
#else
		return vector_dot2(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dot2_as_vector(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return vector_set(vector_dot2_as_scalar(lhs, rhs));
	}

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// This is a helper struct to allow a single consistent API between
		// various vector types when the semantics are identical but the return
		// type differs. Implicit coercion is used to return the desired value
		// at the call site.
		//////////////////////////////////////////////////////////////////////////
		struct vector4d_vector_dot3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
#if defined(RTM_SSE2_INTRINSICS)
				__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
				__m128d z2_w2 = _mm_mul_pd(lhs.zw, rhs.zw);
				__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
				__m128d x2y2 = _mm_add_sd(x2_y2, y2);
				return _mm_cvtsd_f64(_mm_add_sd(x2y2, z2_w2));
#else
				return (vector_get_x(lhs) * vector_get_x(rhs)) + (vector_get_y(lhs) * vector_get_y(rhs)) + (vector_get_z(lhs) * vector_get_z(rhs));
#endif
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
				__m128d z2_w2 = _mm_mul_pd(lhs.zw, rhs.zw);
				__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
				__m128d x2y2 = _mm_add_sd(x2_y2, y2);
				return scalard{ _mm_add_sd(x2y2, z2_w2) };
			}
#endif

			RTM_DEPRECATED("Use 'as_vector' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator vector4d() const RTM_NO_EXCEPT
			{
				const double dot = *this;
				return vector_set(dot);
			}

			vector4d lhs;
			vector4d rhs;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot3 RTM_SIMD_CALL vector_dot3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot3{ lhs, rhs };
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_dot3_as_scalar(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d x2_y2 = _mm_mul_pd(lhs.xy, rhs.xy);
		__m128d z2_w2 = _mm_mul_pd(lhs.zw, rhs.zw);
		__m128d y2 = _mm_shuffle_pd(x2_y2, x2_y2, 1);
		__m128d x2y2 = _mm_add_sd(x2_y2, y2);
		return scalard{ _mm_add_sd(x2y2, z2_w2) };
#else
		return vector_dot3(lhs, rhs);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// 3D dot product: lhs . rhs
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dot3_as_vector(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		return vector_set(vector_dot3_as_scalar(lhs, rhs));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot RTM_SIMD_CALL vector_length_squared(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_squared_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_length_squared_as_vector(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot_as_vector(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot2 RTM_SIMD_CALL vector_length_squared2(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot2{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_squared2_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot2_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_length_squared2_as_vector(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot2_as_vector(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_dot3 RTM_SIMD_CALL vector_length_squared3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_dot3{ input, input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_squared3_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_dot3_as_scalar(input, input);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the squared length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_length_squared3_as_vector(vector4d_arg0 input) RTM_NO_EXCEPT
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
		struct vector4d_vector_length
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_length RTM_SIMD_CALL vector_length(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_length{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = vector_length_squared_as_scalar(input);
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
		struct vector4d_vector_length3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared3_as_scalar(input);
				return scalar_cast(scalar_sqrt(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared3_as_scalar(input);
				return scalar_sqrt(len_sq);
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_length3 RTM_SIMD_CALL vector_length3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_length3{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length3_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = vector_length_squared3_as_scalar(input);
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
		struct vector4d_vector_length_reciprocal
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_length_reciprocal RTM_SIMD_CALL vector_length_reciprocal(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_length_reciprocal{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector4.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_reciprocal_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = vector_length_squared_as_scalar(input);
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
		struct vector4d_vector_length_reciprocal2
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared2_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared2_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_length_reciprocal2 RTM_SIMD_CALL vector_length_reciprocal2(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_length_reciprocal2{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector2.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_reciprocal2_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = vector_length_squared2_as_scalar(input);
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
		struct vector4d_vector_length_reciprocal3
		{
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator double() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared3_as_scalar(input);
				return scalar_cast(scalar_sqrt_reciprocal(len_sq));
			}

#if defined(RTM_SSE2_INTRINSICS)
			RTM_DEPRECATED("Use 'as_scalar' suffix instead. To be removed in 2.4.")
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE RTM_SIMD_CALL operator scalard() const RTM_NO_EXCEPT
			{
				const scalard len_sq = vector_length_squared3_as_scalar(input);
				return scalar_sqrt_reciprocal(len_sq);
			}
#endif

			vector4d input;
		};
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr rtm_impl::vector4d_vector_length_reciprocal3 RTM_SIMD_CALL vector_length_reciprocal3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return rtm_impl::vector4d_vector_length_reciprocal3{ input };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the reciprocal length/norm of the vector3.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_length_reciprocal3_as_scalar(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const scalard len_sq = vector_length_squared3_as_scalar(input);
		return scalar_sqrt_reciprocal(len_sq);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the distance between two 3D points.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE rtm_impl::vector4d_vector_length3 RTM_SIMD_CALL vector_distance3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4d difference = vector_sub(lhs, rhs);
		return rtm_impl::vector4d_vector_length3{ difference };
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns the distance between two 3D points.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE scalard RTM_SIMD_CALL vector_distance3_as_scalar(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
		const vector4d difference = vector_sub(lhs, rhs);
		return vector_length3_as_scalar(difference);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector2.
	// If the length of the input is not finite or zero, the result is undefined.
	// For a safe alternative, supply a fallback value and a threshold.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize2(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared2_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector2.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize2(vector4d_arg0 input, vector4d_arg1 fallback, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared2_as_scalar(input);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared3_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector3.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize3(vector4d_arg0 input, vector4d_arg1 fallback, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared3_as_scalar(input);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared_as_scalar(input);
		return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns a normalized vector4.
	// If the length of the input is below the supplied threshold, the
	// fall back value is returned instead.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_normalize(vector4d_arg0 input, vector4d_arg1 fallback, double threshold = 1.0E-8) RTM_NO_EXCEPT
	{
		// Reciprocal is more accurate to normalize with
		const scalard len_sq = vector_length_squared_as_scalar(input);
		if (scalar_cast(len_sq) >= threshold)
			return vector_mul(input, scalar_sqrt_reciprocal(len_sq));
		else
			return fallback;
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the fractional part of the input.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_fraction(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		return vector_set(scalar_fraction(vector_get_x(input)), scalar_fraction(vector_get_y(input)), scalar_fraction(vector_get_z(input)), scalar_fraction(vector_get_w(input)));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul_add(vector4d_arg0 v0, vector4d_arg1 v1, vector4d_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_add(vector_mul(v0, v1), v2);
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul_add(vector4d_arg0 v0, double s1, vector4d_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_add(vector_mul(v0, s1), v2);
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mul_add(vector4d_arg0 v0, scalard_arg2 s1, vector4d_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_add(vector_mul(v0, s1), v2);
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_neg_mul_sub(vector4d_arg0 v0, vector4d_arg1 v1, vector4d_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_sub(v2, vector_mul(v0, v1));
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_neg_mul_sub(vector4d_arg0 v0, double s1, vector4d_arg2 v2) RTM_NO_EXCEPT
	{
		return vector_sub(v2, vector_mul(v0, s1));
	}

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_neg_mul_sub(vector4d_arg0 v0, scalard_arg2 s1, vector4d_arg0 v2) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_lerp(vector4d_arg0 start, vector4d_arg1 end, double alpha) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_lerp(vector4d_arg0 start, vector4d_arg1 end, scalard_arg4 alpha) RTM_NO_EXCEPT
	{
		// ((1.0 - alpha) * start) + (alpha * end) == (start - alpha * start) + (alpha * end)
		const vector4d alpha_v = vector_set(alpha);
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_lerp(vector4d_arg0 start, vector4d_arg1 end, vector4d_arg2 alpha) RTM_NO_EXCEPT
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return mask4d{ xy_lt_pd, zw_lt_pd };
#else
		return mask4d{ rtm_impl::get_mask_value(lhs.x == rhs.x), rtm_impl::get_mask_value(lhs.y == rhs.y), rtm_impl::get_mask_value(lhs.z == rhs.z), rtm_impl::get_mask_value(lhs.w == rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if not equal, otherwise 0: lhs != rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_not_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmpneq_pd(lhs.zw, rhs.zw);
		return mask4d{ xy_lt_pd, zw_lt_pd };
#else
		return mask4d{ rtm_impl::get_mask_value(lhs.x != rhs.x), rtm_impl::get_mask_value(lhs.y != rhs.y), rtm_impl::get_mask_value(lhs.z != rhs.z), rtm_impl::get_mask_value(lhs.w != rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if less than, otherwise 0: lhs < rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_less_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmplt_pd(lhs.zw, rhs.zw);
		return mask4d{xy_lt_pd, zw_lt_pd};
#else
		return mask4d{rtm_impl::get_mask_value(lhs.x < rhs.x), rtm_impl::get_mask_value(lhs.y < rhs.y), rtm_impl::get_mask_value(lhs.z < rhs.z), rtm_impl::get_mask_value(lhs.w < rhs.w)};
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if less equal, otherwise 0: lhs <= rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_less_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmple_pd(lhs.zw, rhs.zw);
		return mask4d{ xy_lt_pd, zw_lt_pd };
#else
		return mask4d{ rtm_impl::get_mask_value(lhs.x <= rhs.x), rtm_impl::get_mask_value(lhs.y <= rhs.y), rtm_impl::get_mask_value(lhs.z <= rhs.z), rtm_impl::get_mask_value(lhs.w <= rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if greater than, otherwise 0: lhs > rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_greater_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpgt_pd(lhs.zw, rhs.zw);
		return mask4d{ xy_ge_pd, zw_ge_pd };
#else
		return mask4d{ rtm_impl::get_mask_value(lhs.x > rhs.x), rtm_impl::get_mask_value(lhs.y > rhs.y), rtm_impl::get_mask_value(lhs.z > rhs.z), rtm_impl::get_mask_value(lhs.w > rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if greater equal, otherwise 0: lhs >= rhs ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_greater_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpge_pd(lhs.zw, rhs.zw);
		return mask4d{ xy_ge_pd, zw_ge_pd };
#else
		return mask4d{ rtm_impl::get_mask_value(lhs.x >= rhs.x), rtm_impl::get_mask_value(lhs.y >= rhs.y), rtm_impl::get_mask_value(lhs.z >= rhs.z), rtm_impl::get_mask_value(lhs.w >= rhs.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are less than, otherwise false: all(lhs.xyzw < rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmplt_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_lt_pd) & _mm_movemask_pd(zw_lt_pd)) == 3;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z && lhs.w < rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are less than, otherwise false: all(lhs.xy < rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_lt_pd) == 3;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are less than, otherwise false: all(lhs.xyz < rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_than3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmplt_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_lt_pd) == 3 && (_mm_movemask_pd(zw_lt_pd) & 1) == 1;
#else
		return lhs.x < rhs.x && lhs.y < rhs.y && lhs.z < rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are less than, otherwise false: any(lhs.xyzw < rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmplt_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_lt_pd) | _mm_movemask_pd(zw_lt_pd)) != 0;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y || lhs.z < rhs.z || lhs.w < rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are less than, otherwise false: any(lhs.xy < rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_lt_pd) != 0;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are less than, otherwise false: any(lhs.xyz < rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_than3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_lt_pd = _mm_cmplt_pd(lhs.xy, rhs.xy);
		__m128d zw_lt_pd = _mm_cmplt_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_lt_pd) != 0 || (_mm_movemask_pd(zw_lt_pd) & 0x1) != 0;
#else
		return lhs.x < rhs.x || lhs.y < rhs.y || lhs.z < rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are less equal, otherwise false: all(lhs.xyzw <= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		__m128d zw_le_pd = _mm_cmple_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_le_pd) & _mm_movemask_pd(zw_le_pd)) == 3;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y && lhs.z <= rhs.z && lhs.w <= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are less equal, otherwise false: all(lhs.xy <= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_le_pd) == 3;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are less equal, otherwise false: all(lhs.xyz <= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_less_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		__m128d zw_le_pd = _mm_cmple_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_le_pd) == 3 && (_mm_movemask_pd(zw_le_pd) & 1) != 0;
#else
		return lhs.x <= rhs.x && lhs.y <= rhs.y && lhs.z <= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are less equal, otherwise false: any(lhs.xyzw <= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		__m128d zw_le_pd = _mm_cmple_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_le_pd) | _mm_movemask_pd(zw_le_pd)) != 0;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y || lhs.z <= rhs.z || lhs.w <= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are less equal, otherwise false: any(lhs.xy <= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_le_pd) != 0;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are less equal, otherwise false: any(lhs.xyz <= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_less_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_le_pd = _mm_cmple_pd(lhs.xy, rhs.xy);
		__m128d zw_le_pd = _mm_cmple_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_le_pd) != 0 || (_mm_movemask_pd(zw_le_pd) & 1) != 0;
#else
		return lhs.x <= rhs.x || lhs.y <= rhs.y || lhs.z <= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are greater than, otherwise false: all(lhs.xyzw > rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpgt_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_ge_pd) & _mm_movemask_pd(zw_ge_pd)) == 3;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y && lhs.z > rhs.z && lhs.w > rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are greater than, otherwise false: all(lhs.xy > rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_ge_pd) == 3;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are greater than, otherwise false: all(lhs.xyz > rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_than3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpgt_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_ge_pd) == 3 && (_mm_movemask_pd(zw_ge_pd) & 1) != 0;
#else
		return lhs.x > rhs.x && lhs.y > rhs.y && lhs.z > rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are greater than, otherwise false: any(lhs.xyzw > rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpgt_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_ge_pd) | _mm_movemask_pd(zw_ge_pd)) != 0;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y || lhs.z > rhs.z || lhs.w > rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are greater than, otherwise false: any(lhs.xy > rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_ge_pd) != 0;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are greater than, otherwise false: any(lhs.xyz > rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_than3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpgt_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpgt_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_ge_pd) != 0 || (_mm_movemask_pd(zw_ge_pd) & 1) != 0;
#else
		return lhs.x > rhs.x || lhs.y > rhs.y || lhs.z > rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are greater equal, otherwise false: all(lhs.xyzw >= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpge_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_ge_pd) & _mm_movemask_pd(zw_ge_pd)) == 3;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y && lhs.z >= rhs.z && lhs.w >= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are greater equal, otherwise false: all(lhs.xy >= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_ge_pd) == 3;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are greater equal, otherwise false: all(lhs.xyz >= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_greater_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpge_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_ge_pd) == 3 && (_mm_movemask_pd(zw_ge_pd) & 1) != 0;
#else
		return lhs.x >= rhs.x && lhs.y >= rhs.y && lhs.z >= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are greater equal, otherwise false: any(lhs.xyzw >= rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpge_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_ge_pd) | _mm_movemask_pd(zw_ge_pd)) != 0;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y || lhs.z >= rhs.z || lhs.w >= rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are greater equal, otherwise false: any(lhs.xy >= rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_ge_pd) != 0;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are greater equal, otherwise false: any(lhs.xyz >= rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_greater_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_ge_pd = _mm_cmpge_pd(lhs.xy, rhs.xy);
		__m128d zw_ge_pd = _mm_cmpge_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_ge_pd) != 0 || (_mm_movemask_pd(zw_ge_pd) & 1) != 0;
#else
		return lhs.x >= rhs.x || lhs.y >= rhs.y || lhs.z >= rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyzw] components are equal, otherwise false: all(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_eq_pd) & _mm_movemask_pd(zw_eq_pd)) == 3;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are equal, otherwise false: all(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_eq_pd) == 3;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are equal, otherwise false: all(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_eq_pd) == 3 && (_mm_movemask_pd(zw_eq_pd) & 1) != 0;
#else
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyzw] components are equal, otherwise false: any(lhs.xyzw == rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_eq_pd) | _mm_movemask_pd(zw_eq_pd)) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z || lhs.w == rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are equal, otherwise false: any(lhs.xy == rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_eq_pd) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are equal, otherwise false: any(lhs.xyz == rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpeq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpeq_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_eq_pd) != 0 || (_mm_movemask_pd(zw_eq_pd) & 1) != 0;
#else
		return lhs.x == rhs.x || lhs.y == rhs.y || lhs.z == rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyzw] components are not equal, otherwise false: all(lhs.xyzw != rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpneq_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_eq_pd) & _mm_movemask_pd(zw_eq_pd)) == 3;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y && lhs.z != rhs.z && lhs.w != rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are not equal, otherwise false: all(lhs.xy != rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_eq_pd) == 3;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are not equal, otherwise false: all(lhs.xyz != rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_not_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpneq_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_eq_pd) == 3 && (_mm_movemask_pd(zw_eq_pd) & 1) != 0;
#else
		return lhs.x != rhs.x && lhs.y != rhs.y && lhs.z != rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyzw] components are not equal, otherwise false: any(lhs.xyzw != rhs.xyzw)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpneq_pd(lhs.zw, rhs.zw);
		return (_mm_movemask_pd(xy_eq_pd) | _mm_movemask_pd(zw_eq_pd)) != 0;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are not equal, otherwise false: any(lhs.xy != rhs.xy)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		return _mm_movemask_pd(xy_eq_pd) != 0;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are not equal, otherwise false: any(lhs.xyz != rhs.xyz)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_not_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy_eq_pd = _mm_cmpneq_pd(lhs.xy, rhs.xy);
		__m128d zw_eq_pd = _mm_cmpneq_pd(lhs.zw, rhs.zw);
		return _mm_movemask_pd(xy_eq_pd) != 0 || (_mm_movemask_pd(zw_eq_pd) & 1) != 0;
#else
		return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all 4 components are near equal, otherwise false: all(abs(lhs - rhs).xyzw <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_all_less_equal(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are near equal, otherwise false: all(abs(lhs - rhs).xy <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_all_less_equal2(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are near equal, otherwise false: all(abs(lhs - rhs).xyz <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_all_near_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_all_less_equal3(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any 4 components are near equal, otherwise false: any(abs(lhs - rhs).xyzw <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_any_less_equal(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xy] components are near equal, otherwise false: any(abs(lhs - rhs).xy <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal2(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_any_less_equal2(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if any [xyz] components are near equal, otherwise false: any(abs(lhs - rhs).xyz <= threshold)
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_any_near_equal3(vector4d_arg0 lhs, vector4d_arg1 rhs, double threshold = 0.00001) RTM_NO_EXCEPT
	{
		return vector_any_less_equal3(vector_abs(vector_sub(lhs, rhs)), vector_set(threshold));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component ~0 if input is finite, otherwise 0: finite(input) ? ~0 : 0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE mask4d RTM_SIMD_CALL vector_finite(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));

		const __m128d infinity = _mm_set1_pd(std::numeric_limits<double>::infinity());
		__m128d is_not_infinity_xy = _mm_cmpneq_pd(abs_input_xy, infinity);
		__m128d is_not_infinity_zw = _mm_cmpneq_pd(abs_input_zw, infinity);

		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		__m128d is_finite_xy = _mm_andnot_pd(is_nan_xy, is_not_infinity_xy);
		__m128d is_finite_zw = _mm_andnot_pd(is_nan_zw, is_not_infinity_zw);
		return mask4d{ is_finite_xy, is_finite_zw };
#else
		return mask4d{
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));

		const __m128d infinity = _mm_set1_pd(std::numeric_limits<double>::infinity());
		__m128d is_infinity_xy = _mm_cmpeq_pd(abs_input_xy, infinity);
		__m128d is_infinity_zw = _mm_cmpeq_pd(abs_input_zw, infinity);

		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		__m128d is_not_finite_xy = _mm_or_pd(is_infinity_xy, is_nan_xy);
		__m128d is_not_finite_zw = _mm_or_pd(is_infinity_zw, is_nan_zw);
		__m128d is_not_finite = _mm_or_pd(is_not_finite_xy, is_not_finite_zw);
		return _mm_movemask_pd(is_not_finite) == 0x0;
#else
		return scalar_is_finite(vector_get_x(input)) && scalar_is_finite(vector_get_y(input)) && scalar_is_finite(vector_get_z(input)) && scalar_is_finite(vector_get_w(input));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xy] components are finite (not NaN/Inf), otherwise false: all(finite(input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite2(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));

		const __m128d infinity = _mm_set1_pd(std::numeric_limits<double>::infinity());
		__m128d is_infinity_xy = _mm_cmpeq_pd(abs_input_xy, infinity);

		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);

		__m128d is_not_finite_xy = _mm_or_pd(is_infinity_xy, is_nan_xy);
		return _mm_movemask_pd(is_not_finite_xy) == 0x0;
#else
		return scalar_is_finite(vector_get_x(input)) && scalar_is_finite(vector_get_y(input));
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns true if all [xyz] components are finite (not NaN/Inf), otherwise false: all(finite(input))
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE bool RTM_SIMD_CALL vector_is_finite3(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));

		const __m128d infinity = _mm_set1_pd(std::numeric_limits<double>::infinity());
		__m128d is_infinity_xy = _mm_cmpeq_pd(abs_input_xy, infinity);
		__m128d is_infinity_zw = _mm_cmpeq_pd(abs_input_zw, infinity);

		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		__m128d is_not_finite_xy = _mm_or_pd(is_infinity_xy, is_nan_xy);
		__m128d is_not_finite_zw = _mm_or_pd(is_infinity_zw, is_nan_zw);
		return _mm_movemask_pd(is_not_finite_xy) == 0 && (_mm_movemask_pd(is_not_finite_zw) & 0x1) == 0;
#else
		return scalar_is_finite(vector_get_x(input)) && scalar_is_finite(vector_get_y(input)) && scalar_is_finite(vector_get_z(input));
#endif
	}



	//////////////////////////////////////////////////////////////////////////
	// Swizzling, permutations, and mixing
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_select(mask4d_arg0 mask, vector4d_arg1 if_true, vector4d_arg2 if_false) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = RTM_VECTOR2D_SELECT(mask.xy, if_true.xy, if_false.xy);
		__m128d zw = RTM_VECTOR2D_SELECT(mask.zw, if_true.zw, if_false.zw);
		return vector4d{ xy, zw };
#else
		return vector4d{ rtm_impl::select(mask.x, if_true.x, if_false.x), rtm_impl::select(mask.y, if_true.y, if_false.y), rtm_impl::select(mask.z, if_true.z, if_false.z), rtm_impl::select(mask.w, if_true.w, if_false.w) };
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Mixes two inputs and returns the desired components.
	// [xyzw] indexes into the first input while [abcd] indexes in the second.
	//////////////////////////////////////////////////////////////////////////
	template<mix4 comp0, mix4 comp1, mix4 comp2, mix4 comp3>
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_mix(vector4d_arg0 input0, vector4d_arg1 input1) RTM_NO_EXCEPT
	{
		// Slow code path, not yet optimized or not using intrinsics
		constexpr component4 component0 = rtm_impl::mix_to_component(comp0);
		constexpr component4 component1 = rtm_impl::mix_to_component(comp1);
		constexpr component4 component2 = rtm_impl::mix_to_component(comp2);
		constexpr component4 component3 = rtm_impl::mix_to_component(comp3);

		const double x = rtm_impl::is_mix_xyzw(comp0) ? vector_get_component(input0, component0) : vector_get_component(input1, component0);
		const double y = rtm_impl::is_mix_xyzw(comp1) ? vector_get_component(input0, component1) : vector_get_component(input1, component1);
		const double z = rtm_impl::is_mix_xyzw(comp2) ? vector_get_component(input0, component2) : vector_get_component(input1, component2);
		const double w = rtm_impl::is_mix_xyzw(comp3) ? vector_get_component(input0, component3) : vector_get_component(input1, component3);

		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [x] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dup_x(vector4d_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::x, mix4::x, mix4::x, mix4::x>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [y] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dup_y(vector4d_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::y, mix4::y, mix4::y, mix4::y>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [z] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dup_z(vector4d_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::z, mix4::z, mix4::z, mix4::z>(input, input); }

	//////////////////////////////////////////////////////////////////////////
	// Replicates the [w] component in all components.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_dup_w(vector4d_arg0 input) RTM_NO_EXCEPT { return vector_mix<mix4::w, mix4::w, mix4::w, mix4::w>(input, input); }


	//////////////////////////////////////////////////////////////////////////
	// Logical
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Per component logical AND between the inputs: input0 & input1
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_and(vector4d_arg0 input0, vector4d_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_and_pd(input0.xy, input1.xy);
		__m128d zw = _mm_and_pd(input0.zw, input1.zw);
		return vector4d{ xy, zw };
#else
		const uint64_t* input0_ = rtm_impl::bit_cast<const uint64_t*>(&input0);
		const uint64_t* input1_ = rtm_impl::bit_cast<const uint64_t*>(&input1);

		vector4d result = input0;
		uint64_t* result_ = rtm_impl::bit_cast<uint64_t*>(&result);

		result_[0] = input0_[0] & input1_[0];
		result_[1] = input0_[1] & input1_[1];
		result_[2] = input0_[2] & input1_[2];
		result_[3] = input0_[3] & input1_[3];

		return result;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical OR between the inputs: input0 | input1
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_or(vector4d_arg0 input0, vector4d_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_or_pd(input0.xy, input1.xy);
		__m128d zw = _mm_or_pd(input0.zw, input1.zw);
		return vector4d{ xy, zw };
#else
		const uint64_t* input0_ = rtm_impl::bit_cast<const uint64_t*>(&input0);
		const uint64_t* input1_ = rtm_impl::bit_cast<const uint64_t*>(&input1);

		vector4d result = input0;
		uint64_t* result_ = rtm_impl::bit_cast<uint64_t*>(&result);

		result_[0] = input0_[0] | input1_[0];
		result_[1] = input0_[1] | input1_[1];
		result_[2] = input0_[2] | input1_[2];
		result_[3] = input0_[3] | input1_[3];

		return result;
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Per component logical XOR between the inputs: input0 ^ input1
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_xor(vector4d_arg0 input0, vector4d_arg1 input1) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		__m128d xy = _mm_xor_pd(input0.xy, input1.xy);
		__m128d zw = _mm_xor_pd(input0.zw, input1.zw);
		return vector4d{ xy, zw };
#else
		const uint64_t* input0_ = rtm_impl::bit_cast<const uint64_t*>(&input0);
		const uint64_t* input1_ = rtm_impl::bit_cast<const uint64_t*>(&input1);

		vector4d result = input0;
		uint64_t* result_ = rtm_impl::bit_cast<uint64_t*>(&result);

		result_[0] = input0_[0] ^ input1_[0];
		result_[1] = input0_[1] ^ input1_[1];
		result_[2] = input0_[2] ^ input1_[2];
		result_[3] = input0_[3] ^ input1_[3];

		return result;
#endif
	}


	//////////////////////////////////////////////////////////////////////////
	// Miscellaneous
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sign of the input vector: input >= 0.0 ? 1.0 : -1.0
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_sign(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		const mask4d mask = vector_greater_equal(input, vector_zero());
		return vector_select(mask, vector_set(1.0), vector_set(-1.0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the input with the sign of the control value.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_copy_sign(vector4d_arg0 input, vector4d_arg1 control_sign) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE2_INTRINSICS)
		const __m128d sign_bit = _mm_set1_pd(-0.0);
		__m128d signs_xy = _mm_and_pd(sign_bit, control_sign.xy);
		__m128d signs_zw = _mm_and_pd(sign_bit, control_sign.zw);
		__m128d abs_input_xy = _mm_andnot_pd(sign_bit, input.xy);
		__m128d abs_input_zw = _mm_andnot_pd(sign_bit, input.zw);
		__m128d xy = _mm_or_pd(abs_input_xy, signs_xy);
		__m128d zw = _mm_or_pd(abs_input_zw, signs_zw);
		return vector4d{ xy, zw };
#else
		double x = vector_get_x(input);
		double y = vector_get_y(input);
		double z = vector_get_z(input);
		double w = vector_get_w(input);

		double x_sign = vector_get_x(control_sign);
		double y_sign = vector_get_y(control_sign);
		double z_sign = vector_get_z(control_sign);
		double w_sign = vector_get_w(control_sign);

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
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_round_symmetric(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		// NaN, +- Infinity, and numbers larger or equal to 2^23 remain unchanged
		// since they have no fractional part.

#if defined(RTM_SSE4_INTRINSICS)
		__m128d zero = _mm_setzero_pd();
		__m128d is_positive_xy = _mm_cmpge_pd(input.xy, zero);
		__m128d is_positive_zw = _mm_cmpge_pd(input.zw, zero);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign_xy = _mm_andnot_pd(is_positive_xy, sign_mask);
		__m128d sign_zw = _mm_andnot_pd(is_positive_zw, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128d half = _mm_set1_pd(0.5);
		__m128d bias_xy = _mm_or_pd(sign_xy, half);
		__m128d bias_zw = _mm_or_pd(sign_zw, half);
		__m128d biased_input_xy = _mm_add_pd(input.xy, bias_xy);
		__m128d biased_input_zw = _mm_add_pd(input.zw, bias_zw);

		__m128d floored_xy = _mm_floor_pd(biased_input_xy);
		__m128d floored_zw = _mm_floor_pd(biased_input_zw);
		__m128d ceiled_xy = _mm_ceil_pd(biased_input_xy);
		__m128d ceiled_zw = _mm_ceil_pd(biased_input_zw);

		__m128d result_xy = RTM_VECTOR2D_SELECT(is_positive_xy, floored_xy, ceiled_xy);
		__m128d result_zw = RTM_VECTOR2D_SELECT(is_positive_zw, floored_zw, ceiled_zw);
		return vector4d{ result_xy, result_zw };
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52

		// Build our mask, larger values that have no fractional part, and infinities will be true
		// Smaller values and NaN will be false
		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large_xy = _mm_cmpge_pd(abs_input_xy, fractional_limit);
		__m128d is_input_large_zw = _mm_cmpge_pd(abs_input_zw, fractional_limit);

		// Test if our input is NaN with (value != value), it is only true for NaN
		__m128d is_nan_xy = _mm_cmpneq_pd(input.xy, input.xy);
		__m128d is_nan_zw = _mm_cmpneq_pd(input.zw, input.zw);

		// Combine our masks to determine if we should return the original value
		__m128d use_original_input_xy = _mm_or_pd(is_input_large_xy, is_nan_xy);
		__m128d use_original_input_zw = _mm_or_pd(is_input_large_zw, is_nan_zw);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign_xy = _mm_and_pd(input.xy, sign_mask);
		__m128d sign_zw = _mm_and_pd(input.zw, sign_mask);

		// For positive values, we add a bias of 0.5.
		// For negative values, we add a bias of -0.5.
		__m128d half = _mm_set1_pd(0.5);
		__m128d bias_xy = _mm_or_pd(sign_xy, half);
		__m128d bias_zw = _mm_or_pd(sign_zw, half);
		__m128d biased_input_xy = _mm_add_pd(input.xy, bias_xy);
		__m128d biased_input_zw = _mm_add_pd(input.zw, bias_zw);

		// Convert to an integer with truncation and back, this rounds towards zero.
		__m128d integer_part_xy = _mm_cvtepi32_pd(_mm_cvttpd_epi32(biased_input_xy));
		__m128d integer_part_zw = _mm_cvtepi32_pd(_mm_cvttpd_epi32(biased_input_zw));

		__m128d result_xy = _mm_or_pd(_mm_and_pd(use_original_input_xy, input.xy), _mm_andnot_pd(use_original_input_xy, integer_part_xy));
		__m128d result_zw = _mm_or_pd(_mm_and_pd(use_original_input_zw, input.zw), _mm_andnot_pd(use_original_input_zw, integer_part_zw));

		return vector4d{ result_xy, result_zw };
#else
		const vector4d half = vector_set(0.5);
		const vector4d floored = vector_floor(vector_add(input, half));
		const vector4d ceiled = vector_ceil(vector_sub(input, half));
		const mask4d is_greater_equal = vector_greater_equal(input, vector_zero());
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
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE vector4d RTM_SIMD_CALL vector_round_bankers(vector4d_arg0 input) RTM_NO_EXCEPT
	{
#if defined(RTM_SSE4_INTRINSICS)
		return vector4d{ _mm_round_pd(input.xy, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm_round_pd(input.zw, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC) };
#elif defined(RTM_SSE2_INTRINSICS)
		const __m128i abs_mask = _mm_set_epi64x(0x7FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL);

		const __m128d sign_mask = _mm_set_pd(-0.0, -0.0);
		__m128d sign_xy = _mm_and_pd(input.xy, sign_mask);
		__m128d sign_zw = _mm_and_pd(input.zw, sign_mask);

		// We add the largest integer that a 64 bit floating point number can represent and subtract it afterwards.
		// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
		// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
		// This has the effect of removing the fractional part while simultaneously rounding.
		// Use the same sign as the input value to make sure we handle positive and negative values.
		const __m128d fractional_limit = _mm_set1_pd(4503599627370496.0); // 2^52
		__m128d truncating_offset_xy = _mm_or_pd(sign_xy, fractional_limit);
		__m128d truncating_offset_zw = _mm_or_pd(sign_zw, fractional_limit);
		__m128d integer_part_xy = _mm_sub_pd(_mm_add_pd(input.xy, truncating_offset_xy), truncating_offset_xy);
		__m128d integer_part_zw = _mm_sub_pd(_mm_add_pd(input.zw, truncating_offset_zw), truncating_offset_zw);

		__m128d abs_input_xy = _mm_and_pd(input.xy, _mm_castsi128_pd(abs_mask));
		__m128d abs_input_zw = _mm_and_pd(input.zw, _mm_castsi128_pd(abs_mask));
		__m128d is_input_large_xy = _mm_cmpge_pd(abs_input_xy, fractional_limit);
		__m128d is_input_large_zw = _mm_cmpge_pd(abs_input_zw, fractional_limit);
		__m128d result_xy = _mm_or_pd(_mm_and_pd(is_input_large_xy, input.xy), _mm_andnot_pd(is_input_large_xy, integer_part_xy));
		__m128d result_zw = _mm_or_pd(_mm_and_pd(is_input_large_zw, input.zw), _mm_andnot_pd(is_input_large_zw, integer_part_zw));
		return vector4d{ result_xy, result_zw };
#else
		scalard x = scalar_round_bankers(scalard(vector_get_x(input)));
		scalard y = scalar_round_bankers(scalard(vector_get_y(input)));
		scalard z = scalar_round_bankers(scalard(vector_get_z(input)));
		scalard w = scalar_round_bankers(scalard(vector_get_w(input)));
		return vector_set(x, y, z, w);
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_sin(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		scalard x = scalar_sin(vector_get_x_as_scalar(input));
		scalard y = scalar_sin(vector_get_y_as_scalar(input));
		scalard z = scalar_sin(vector_get_z_as_scalar(input));
		scalard w = scalar_sin(vector_get_w_as_scalar(input));
		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-sine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_asin(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		scalard x = scalar_asin(vector_get_x_as_scalar(input));
		scalard y = scalar_asin(vector_get_y_as_scalar(input));
		scalard z = scalar_asin(vector_get_z_as_scalar(input));
		scalard w = scalar_asin(vector_get_w_as_scalar(input));
		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_cos(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		scalard x = scalar_cos(vector_get_x_as_scalar(input));
		scalard y = scalar_cos(vector_get_y_as_scalar(input));
		scalard z = scalar_cos(vector_get_z_as_scalar(input));
		scalard w = scalar_cos(vector_get_w_as_scalar(input));
		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-cosine of the input.
	// Input value must be in the range [-1.0, 1.0].
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_acos(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		scalard x = scalar_acos(vector_get_x_as_scalar(input));
		scalard y = scalar_acos(vector_get_y_as_scalar(input));
		scalard z = scalar_acos(vector_get_z_as_scalar(input));
		scalard w = scalar_acos(vector_get_w_as_scalar(input));
		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the sine and cosine of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline void RTM_SIMD_CALL vector_sincos(vector4d_arg0 input, vector4d& out_sine, vector4d& out_cosine) RTM_NO_EXCEPT
	{
		const vector4d x = scalar_sincos(vector_get_x_as_scalar(input));
		const vector4d y = scalar_sincos(vector_get_y_as_scalar(input));
		const vector4d z = scalar_sincos(vector_get_z_as_scalar(input));
		const vector4d w = scalar_sincos(vector_get_w_as_scalar(input));

		const vector4d cos_xy = vector_mix<mix4::y, mix4::b, mix4::y, mix4::b>(x, y);
		const vector4d cos_zw = vector_mix<mix4::y, mix4::b, mix4::y, mix4::b>(z, w);
		out_cosine = vector_mix<mix4::x, mix4::y, mix4::a, mix4::b>(cos_xy, cos_zw);

		const vector4d sin_xy = vector_mix<mix4::x, mix4::a, mix4::x, mix4::a>(x, y);
		const vector4d sin_zw = vector_mix<mix4::x, mix4::a, mix4::x, mix4::a>(z, w);
		out_sine = vector_mix<mix4::x, mix4::y, mix4::a, mix4::b>(sin_xy, sin_zw);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the tangent of the input angle.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_tan(vector4d_arg0 angle) RTM_NO_EXCEPT
	{
		// Use the identity: tan(angle) = sin(angle) / cos(angle)
		vector4d sin_;
		vector4d cos_;
		vector_sincos(angle, sin_, cos_);

		const mask4d is_cos_zero = vector_equal(cos_, vector_zero());
		const vector4d signed_infinity = vector_copy_sign(vector_set(std::numeric_limits<double>::infinity()), angle);
		const vector4d result = vector_div(sin_, cos_);
		return vector_select(is_cos_zero, signed_infinity, result);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-tangent of the input.
	// Note that due to the sign ambiguity, atan cannot determine which quadrant
	// the value resides in.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_atan(vector4d_arg0 input) RTM_NO_EXCEPT
	{
		scalard x = scalar_atan(vector_get_x_as_scalar(input));
		scalard y = scalar_atan(vector_get_y_as_scalar(input));
		scalard z = scalar_atan(vector_get_z_as_scalar(input));
		scalard w = scalar_atan(vector_get_w_as_scalar(input));
		return vector_set(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// Returns per component the arc-tangent of [y/x] using the sign of the arguments to
	// determine the correct quadrant.
	// Y represents the proportion of the y-coordinate.
	// X represents the proportion of the x-coordinate.
	//////////////////////////////////////////////////////////////////////////
	RTM_DISABLE_SECURITY_COOKIE_CHECK inline vector4d RTM_SIMD_CALL vector_atan2(vector4d_arg0 y, vector4d_arg1 x) RTM_NO_EXCEPT
	{
		scalard x_ = scalar_atan2(vector_get_x_as_scalar(y), vector_get_x_as_scalar(x));
		scalard y_ = scalar_atan2(vector_get_y_as_scalar(y), vector_get_y_as_scalar(x));
		scalard z_ = scalar_atan2(vector_get_z_as_scalar(y), vector_get_z_as_scalar(x));
		scalard w_ = scalar_atan2(vector_get_w_as_scalar(y), vector_get_w_as_scalar(x));
		return vector_set(x_, y_, z_, w_);
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
