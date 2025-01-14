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
#include "rtm/version.h"

#include <cstdint>

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

#if defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	using quatf = __m128;

	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	struct quatd
	{
		__m128d xy;
		__m128d zw;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	using vector4f = __m128;

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	struct vector4d
	{
		__m128d xy;
		__m128d zw;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask for 32 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	using mask4f = __m128;

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask for 64 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct mask4d
	{
		__m128d xy;
		__m128d zw;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	using mask4i = __m128i;

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct mask4q
	{
		__m128i xy;
		__m128i zw;
	};
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	using quatf = float32x4_t;

	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) quatd
	{
		double x;
		double y;
		double z;
		double w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	using vector4f = float32x4_t;

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) vector4d
	{
		double x;
		double y;
		double z;
		double w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask for 32 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	using mask4f = uint32x4_t;

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask for 64 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4d
	{
		uint64_t x;
		uint64_t y;
		uint64_t z;
		uint64_t w;
	};

	// MSVC uses a simple typedef to an identical underlying type for uint32x4_t and float32x4_t
	// We also want two different types for mask4f and mask4i and mask4f is more commonly used
	// To avoid issues of duplicate symbols, we introduce a concrete type for mask4i

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask for 32 bit integers: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4i
	{
		uint32x4_t value;
	};

	// Helper macros to simplify usage
	#define RTM_IMPL_MASK4i_GET(mask) mask.value
	#define RTM_IMPL_MASK4i_SET(mask) mask4i{ mask }

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask for 64 bit integers: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4q
	{
		uint64_t x;
		uint64_t y;
		uint64_t z;
		uint64_t w;
	};
#else
	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) quatf
	{
		float x;
		float y;
		float z;
		float w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A quaternion (4D complex number) where the imaginary part is the [w] component.
	// It accurately represents a 3D rotation with no gimbal lock as long as it is kept normalized.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) quatd
	{
		double x;
		double y;
		double z;
		double w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) vector4f
	{
		float x;
		float y;
		float z;
		float w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4D vector.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) vector4d
	{
		double x;
		double y;
		double z;
		double w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask for 32 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4f
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask for 64 bit floats: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4d
	{
		uint64_t x;
		uint64_t y;
		uint64_t z;
		uint64_t w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x32 bit vector comparison mask: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4i
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t w;
	};

	//////////////////////////////////////////////////////////////////////////
	// A 4x64 bit vector comparison mask: ~0 if true, 0 otherwise.
	//////////////////////////////////////////////////////////////////////////
	struct alignas(16) mask4q
	{
		uint64_t x;
		uint64_t y;
		uint64_t z;
		uint64_t w;
	};
#endif

#if defined(RTM_SSE2_INTRINSICS)
	// With SSE2, we use a concrete type for scalarf/scalard unlike other platforms and other types
	// like vector4f and quatf. We don't use a concrete type when we can avoid it to help the compiler
	// optimize as much as possible. But we must be able to tell a scalar apart from a vector for
	// return type overloading and argument overloading.
	// For example, we want to support vector_mul(vec4, vec4) and vector_mul(vec4, scalar).
	// When scalarf is a 'float', the type is distinct and everything works as expected
	// but if we use __m128, the type is the same as vector4f and we won't be able to tell
	// them apart.
	// Another example is vector_dot where we want to support returning a float, a scalarf, and
	// a vector4f depending on what the user expects. We could always return a float/scalarf but
	// if we need a vector4f it is less efficient if _mm_dp_ps is used: we would have an extra
	// shuffle.
	// Using a concrete type here allows us to tell the types apart and properly overload them
	// when required. The compiler should still be able to optimize properly.

	//////////////////////////////////////////////////////////////////////////
	// A SIMD friendly scalar type. Different architectures have an easier or harder time
	// working with scalar floating point numbers. For example, older PowerPC processors
	// had to write to memory and reload from it to transfer from one register file into
	// another (e.g convert from a float to a SIMD vector). Modern processors handle
	// this much better but inefficiencies remain, especially with SSE. While it is
	// free to convert a SIMD scalar into a float with _mm_cvtss_f32(..) the reverse generally
	// requires the compiler to fill the unused SIMD lanes with known values (either zero or the same).
	// This introduces an extra instruction that isn't always required when only the first lane is used
	// such as with scalar_sqrt_reciprocal(..). By introducing a type for SIMD scalar values,
	// each platform is free to make an optimal choice.
	//////////////////////////////////////////////////////////////////////////
	struct scalarf
	{
		__m128 value;
	};

	//////////////////////////////////////////////////////////////////////////
	// A SIMD friendly scalar type. Different architectures have an easier or harder time
	// working with scalar floating point numbers. For example, older PowerPC processors
	// had to write to memory and reload from it to transfer from one register file into
	// another (e.g convert from a float to a SIMD vector). Modern processors handle
	// this much better but inefficiencies remain, especially with SSE. While it is
	// free to convert a SIMD scalar into a float with _mm_cvtss_f32(..) the reverse generally
	// requires the compiler to fill the unused SIMD lanes with known values (either zero or the same).
	// This introduces an extra instruction that isn't always required when only the first lane is used
	// such as with scalar_sqrt_reciprocal(..). By introducing a type for SIMD scalar values,
	// each platform is free to make an optimal choice.
	//////////////////////////////////////////////////////////////////////////
	struct scalard
	{
		__m128d value;
	};
#else
	//////////////////////////////////////////////////////////////////////////
	// A SIMD friendly scalar type. Different architectures have an easier or harder time
	// working with scalar floating point numbers. For example, older PowerPC processors
	// had to write to memory and reload from it to transfer from one register file into
	// another (e.g convert from a float to a SIMD vector). Modern processors handle
	// this much better but inefficiencies remain, especially with SSE. While it is
	// free to convert a SIMD scalar into a float with _mm_cvtss_f32(..) the reverse generally
	// requires the compiler to fill the unused SIMD lanes with known values (either zero or the same).
	// This introduces an extra instruction that isn't always required when only the first lane is used
	// such as with scalar_sqrt_reciprocal(..). By introducing a type for SIMD scalar values,
	// each platform is free to make an optimal choice.
	//////////////////////////////////////////////////////////////////////////
	using scalarf = float;

	//////////////////////////////////////////////////////////////////////////
	// A SIMD friendly scalar type. Different architectures have an easier or harder time
	// working with scalar floating point numbers. For example, older PowerPC processors
	// had to write to memory and reload from it to transfer from one register file into
	// another (e.g convert from a float to a SIMD vector). Modern processors handle
	// this much better but inefficiencies remain, especially with SSE. While it is
	// free to convert a SIMD scalar into a float with _mm_cvtss_f32(..) the reverse generally
	// requires the compiler to fill the unused SIMD lanes with known values (either zero or the same).
	// This introduces an extra instruction that isn't always required when only the first lane is used
	// such as with scalar_sqrt_reciprocal(..). By introducing a type for SIMD scalar values,
	// each platform is free to make an optimal choice.
	//////////////////////////////////////////////////////////////////////////
	using scalard = double;
#endif

	//////////////////////////////////////////////////////////////////////////
	// A QV transform represents a 3D rotation (quaternion) and a 3D translation (vector).
	//////////////////////////////////////////////////////////////////////////
	struct qvf
	{
		quatf		rotation;
		vector4f	translation;	// [w] is undefined
	};

	//////////////////////////////////////////////////////////////////////////
	// A QV transform represents a 3D rotation (quaternion) and a 3D translation (vector).
	//////////////////////////////////////////////////////////////////////////
	struct qvd
	{
		quatd		rotation;
		vector4d	translation;	// [w] is undefined
	};

	//////////////////////////////////////////////////////////////////////////
	// A QVS transform represents a 3D rotation (quaternion), a 3D translation (vector),
	// and a single scalar uniform scale value.
	//////////////////////////////////////////////////////////////////////////
	struct qvsf
	{
		quatf		rotation;
		vector4f	translation_scale;	// [xyz] for translation, [w] for scale
	};

	//////////////////////////////////////////////////////////////////////////
	// A QVS transform represents a 3D rotation (quaternion), a 3D translation (vector),
	// and a single scalar uniform scale value.
	//////////////////////////////////////////////////////////////////////////
	struct qvsd
	{
		quatd		rotation;
		vector4d	translation_scale;	// [xyz] for translation, [w] for scale
	};

	//////////////////////////////////////////////////////////////////////////
	// A QVV transform represents a 3D rotation (quaternion), 3D translation (vector), and 3D non-uniform scale (vector).
	// It properly handles positive scaling but negative scaling is a bit more problematic.
	// A best effort is made by converting the quaternion to a matrix during those operations.
	// If scale fidelity is important, consider using an affine matrix 3x4 instead.
	//////////////////////////////////////////////////////////////////////////
	struct qvvf
	{
		quatf		rotation;
		vector4f	translation;	// [w] is undefined
		vector4f	scale;			// [w] is undefined
	};

	//////////////////////////////////////////////////////////////////////////
	// A QVV transform represents a 3D rotation (quaternion), 3D translation (vector), and 3D non-uniform scale (vector).
	// It properly handles positive scaling but negative scaling is a bit more problematic.
	// A best effort is made by converting the quaternion to a matrix during those operations.
	// If scale fidelity is important, consider using an affine matrix 3x4 instead.
	//////////////////////////////////////////////////////////////////////////
	struct qvvd
	{
		quatd		rotation;
		vector4d	translation;	// [w] is undefined
		vector4d	scale;			// [w] is undefined
	};

	//////////////////////////////////////////////////////////////////////////
	// A generic 3x3 matrix.
	// Note: The [w] component of every column vector is undefined.
	//////////////////////////////////////////////////////////////////////////
	struct matrix3x3f
	{
		vector4f	x_axis;
		vector4f	y_axis;
		vector4f	z_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// A generic 3x3 matrix.
	// Note: The [w] component of every column vector is undefined.
	//////////////////////////////////////////////////////////////////////////
	struct matrix3x3d
	{
		vector4d	x_axis;
		vector4d	y_axis;
		vector4d	z_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// An 3x4 affine matrix represents a 3D rotation, 3D translation, and 3D scale.
	// It properly deals with skew/shear when present but once scale with mirroring is combined,
	// it cannot be safely extracted back.
	//
	// Affine matrices are 4x4 but have their last row always equal to [0, 0, 0, 1] which is why it is 3x4.
	// Note: We do not track the implicit last row and it is thus undefined.
	//
	// Left handed coordinate system:
	// X axis == forward
	// Y axis == right
	// Z axis == up
	//////////////////////////////////////////////////////////////////////////
	struct matrix3x4f
	{
		vector4f	x_axis;
		vector4f	y_axis;
		vector4f	z_axis;
		vector4f	w_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// An 3x4 affine matrix represents a 3D rotation, 3D translation, and 3D scale.
	// It properly deals with skew/shear when present but once scale with mirroring is combined,
	// it cannot be safely extracted back.
	//
	// Affine matrices are 4x4 but have their last row always equal to [0, 0, 0, 1] which is why it is 3x4.
	// Note: We do not track the implicit last row and it is thus undefined.
	//
	// Left handed coordinate system:
	// X axis == forward
	// Y axis == right
	// Z axis == up
	//////////////////////////////////////////////////////////////////////////
	struct matrix3x4d
	{
		vector4d	x_axis;
		vector4d	y_axis;
		vector4d	z_axis;
		vector4d	w_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// A generic 4x4 matrix.
	//////////////////////////////////////////////////////////////////////////
	struct matrix4x4f
	{
		vector4f	x_axis;
		vector4f	y_axis;
		vector4f	z_axis;
		vector4f	w_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// A generic 4x4 matrix.
	//////////////////////////////////////////////////////////////////////////
	struct matrix4x4d
	{
		vector4d	x_axis;
		vector4d	y_axis;
		vector4d	z_axis;
		vector4d	w_axis;
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents a component when mixing/shuffling/permuting vectors.
	// [xyzw] are used to refer to the first input while [abcd] refer to the second input.
	//////////////////////////////////////////////////////////////////////////
	enum class mix4
	{
		x = 0,
		y = 1,
		z = 2,
		w = 3,

		a = 4,
		b = 5,
		c = 6,
		d = 7,
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents an axis in 3D.
	//////////////////////////////////////////////////////////////////////////
	enum class axis3
	{
		x = 0,
		y = 1,
		z = 2,
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents an axis in 4D.
	//////////////////////////////////////////////////////////////////////////
	enum class axis4
	{
		x = 0,
		y = 1,
		z = 2,
		w = 3,
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents 2D vector components.
	//////////////////////////////////////////////////////////////////////////
	enum class component2
	{
		x = 0,
		y = 1,
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents 3D vector components.
	//////////////////////////////////////////////////////////////////////////
	enum class component3
	{
		x = 0,
		y = 1,
		z = 2,
	};

	//////////////////////////////////////////////////////////////////////////
	// Represents 4D vector components.
	//////////////////////////////////////////////////////////////////////////
	enum class component4
	{
		x = 0,
		y = 1,
		z = 2,
		w = 3,
	};


	//////////////////////////////////////////////////////////////////////////
	// Various unaligned types suitable for interop. with GPUs, etc.
	//////////////////////////////////////////////////////////////////////////


	struct float2f
	{
		float x;
		float y;
	};

	struct float3f
	{
		float x;
		float y;
		float z;
	};

	struct float4f
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct float2d
	{
		double x;
		double y;
	};

	struct float3d
	{
		double x;
		double y;
		double z;
	};

	struct float4d
	{
		double x;
		double y;
		double z;
		double w;
	};

	RTM_IMPL_VERSION_NAMESPACE_END
}

// Always include the register passing typedefs
#include "rtm/impl/type_args.h"
