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
#include "rtm/version.h"
#include "rtm/impl/error.h"

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic push
#if __GNUC__ > 5
	// GCC complains that the alignment attribute is ignored, we don't care with type traits
	#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif
	// GCC complains that the type is deprecated when we specialize it, we don't care
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

	//////////////////////////////////////////////////////////////////////////
	// Returns the proper types for a floating point type.
	//////////////////////////////////////////////////////////////////////////
	template<typename float_type>
	struct RTM_DEPRECATED("Use 'related_types' instead. To be removed in 2.4") float_traits {};

	template<>
	struct RTM_DEPRECATED("Use 'related_types' instead. To be removed in 2.4") float_traits<float>
	{
		using mask4 = mask4f;

		using scalar = scalarf;
		using vector4 = vector4f;
		using quat = quatf;

		using qv = qvf;
		using qvs = qvsf;
		using qvv = qvvf;

		using matrix3x3 = matrix3x3f;
		using matrix3x4 = matrix3x4f;
		using matrix4x4 = matrix4x4f;

		using float1 = float;
		using float2 = float2f;
		using float3 = float3f;
		using float4 = float4f;

		using int1 = uint32_t;
	};

	template<>
	struct RTM_DEPRECATED("Use 'related_types' instead. To be removed in 2.4") float_traits<double>
	{
		using mask4 = mask4d;

		using scalar = scalard;
		using vector4 = vector4d;
		using quat = quatd;

		using qv = qvd;
		using qvs = qvsd;
		using qvv = qvvd;

		using matrix3x3 = matrix3x3d;
		using matrix3x4 = matrix3x4d;
		using matrix4x4 = matrix4x4d;

		using float1 = double;
		using float2 = float2d;
		using float3 = float3d;
		using float4 = float4d;

		using int1 = uint64_t;
	};

	//////////////////////////////////////////////////////////////////////////
	// Specifies the related types given a source type.
	//////////////////////////////////////////////////////////////////////////
	template<typename source_type>
	struct related_types {};

	template<>
	struct related_types<float>
	{
		using mask4 = mask4f;

		using scalar = scalarf;
		using vector4 = vector4f;
		using quat = quatf;

		using qv = qvf;
		using qvs = qvsf;
		using qvv = qvvf;

		using matrix3x3 = matrix3x3f;
		using matrix3x4 = matrix3x4f;
		using matrix4x4 = matrix4x4f;

		using float1 = float;
		using float2 = float2f;
		using float3 = float3f;
		using float4 = float4f;

		using int1 = uint32_t;
	};

	template<>
	struct related_types<double>
	{
		using mask4 = mask4d;

		using scalar = scalard;
		using vector4 = vector4d;
		using quat = quatd;

		using qv = qvd;
		using qvs = qvsd;
		using qvv = qvvd;

		using matrix3x3 = matrix3x3d;
		using matrix3x4 = matrix3x4d;
		using matrix4x4 = matrix4x4d;

		using float1 = double;
		using float2 = float2d;
		using float3 = float3d;
		using float4 = float4d;

		using int1 = uint64_t;
	};

	// Alias all related float types
	template<> struct related_types<vector4f> : related_types<float> {};
	template<> struct related_types<qvf> : related_types<float> {};
	template<> struct related_types<qvsf> : related_types<float> {};
	template<> struct related_types<qvvf> : related_types<float> {};
	template<> struct related_types<matrix3x3f> : related_types<float> {};
	template<> struct related_types<matrix3x4f> : related_types<float> {};
	template<> struct related_types<matrix4x4f> : related_types<float> {};
	template<> struct related_types<float2f> : related_types<float> {};
	template<> struct related_types<float3f> : related_types<float> {};
	template<> struct related_types<float4f> : related_types<float> {};

#if defined(RTM_SSE2_INTRINSICS)
	// mask4f is an alias of vector4f
	// quatf is an alias of vector4f
	template<> struct related_types<scalarf> : related_types<float> {};
#elif defined(RTM_NEON_INTRINSICS)
	// scalarf is an alias of float
	// quatf is an alias of vector4f
#if !defined(RTM_COMPILER_MSVC)
	// MSVC aliases uint32x4_t and float32x4_t
	template<> struct related_types<mask4f> : related_types<float> {};
#endif
#else
	// scalarf is an alias of float
	template<> struct related_types<mask4f> : related_types<float> {};
	template<> struct related_types<quatf> : related_types<float> {};
#endif

	// Alias all related double types
	template<> struct related_types<mask4d> : related_types<double> {};
	template<> struct related_types<vector4d> : related_types<double> {};
	template<> struct related_types<quatd> : related_types<double> {};
	template<> struct related_types<qvd> : related_types<double> {};
	template<> struct related_types<qvsd> : related_types<double> {};
	template<> struct related_types<qvvd> : related_types<double> {};
	template<> struct related_types<matrix3x3d> : related_types<double> {};
	template<> struct related_types<matrix3x4d> : related_types<double> {};
	template<> struct related_types<matrix4x4d> : related_types<double> {};
	template<> struct related_types<float2d> : related_types<double> {};
	template<> struct related_types<float3d> : related_types<double> {};
	template<> struct related_types<float4d> : related_types<double> {};

#if defined(RTM_SSE2_INTRINSICS)
	template<> struct related_types<scalard> : related_types<double> {};
#elif defined(RTM_NEON_INTRINSICS)
	// scalard is an alias of double
#else
	// scalard is an alias of double
#endif

#if defined(RTM_COMPILER_GCC)
	#pragma GCC diagnostic pop
#endif

	RTM_IMPL_VERSION_NAMESPACE_END
}
