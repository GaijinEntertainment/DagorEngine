#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2022 Nicholas Frechette & Realtime Math contributors
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

// Included only once from macros.h

#include "rtm/math.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

//////////////////////////////////////////////////////////////////////////
// This file contains helper macros to help improve code generation where required.
//////////////////////////////////////////////////////////////////////////

#if defined(RTM_NEON64_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULV_ADD(v0, v1, v2) vfmaq_f32((v2), (v0), (v1))

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an float32x2_t.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR2F_MULV_ADD(v0, v1, v2) vfma_f32((v2), (v0), (v1))
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULV_ADD(v0, v1, v2) vmlaq_f32((v2), (v0), (v1))

	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an float32x2_t.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR2F_MULV_ADD(v0, v1, v2) vmla_f32((v2), (v0), (v1))
#elif defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULV_ADD(v0, v1, v2) _mm_add_ps(_mm_mul_ps((v0), (v1)), (v2))
#else
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULV_ADD(v0, v1, v2) RTM_IMPL_NAMESPACE::vector4f { (v2).x + ((v0).x * (v1).x), (v2).y + ((v0).y * (v1).y), (v2).z + ((v0).z * (v1).z), (v2).w + ((v0).w * (v1).w) }
#endif

#if defined(RTM_NEON64_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULS_ADD(v0, s1, v2) vfmaq_n_f32((v2), (v0), (s1))
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULS_ADD(v0, s1, v2) vmlaq_n_f32((v2), (v0), (s1))
#elif defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULS_ADD(v0, s1, v2) _mm_add_ps(_mm_mul_ps((v0), _mm_set_ps1((s1))), (v2))
#else
	//////////////////////////////////////////////////////////////////////////
	// Per component multiplication/addition of the three inputs: v2 + (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_MULS_ADD(v0, s1, v2) RTM_IMPL_NAMESPACE::vector4f { (v2).x + ((v0).x * (s1)), (v2).y + ((v0).y * (s1)), (v2).z + ((v0).z * (s1)), (v2).w + ((v0).w * (s1)) }
#endif

#if defined(RTM_NEON64_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULV_SUB(v0, v1, v2) vfmsq_f32((v2), (v0), (v1))
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULV_SUB(v0, v1, v2) vmlsq_f32((v2), (v0), (v1))
#elif defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULV_SUB(v0, v1, v2) _mm_sub_ps((v2), _mm_mul_ps((v0), (v1)))
#else
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * v1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * v1)
	// All three inputs must be an rtm::vector4f.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULV_SUB(v0, v1, v2) RTM_IMPL_NAMESPACE::vector4f { (v2).x - ((v0).x * (v1).x), (v2).y - ((v0).y * (v1).y), (v2).z - ((v0).z * (v1).z), (v2).w - ((v0).w * (v1).w) }
#endif

#if defined(RTM_NEON64_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULS_SUB(v0, s1, v2) vfmsq_n_f32((v2), (v0), (s1))
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULS_SUB(v0, s1, v2) vmlsq_n_f32((v2), (v0), (s1))
#elif defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULS_SUB(v0, s1, v2) _mm_sub_ps((v2), _mm_mul_ps((v0), _mm_set_ps1((s1))))
#else
	//////////////////////////////////////////////////////////////////////////
	// Per component negative multiplication/subtraction of the three inputs: -((v0 * s1) - v2)
	// This is mathematically equivalent to: v2 - (v0 * s1)
	// The v0 and v2 inputs must be a rtm::vector4f and s1 must be a float.
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_NEG_MULS_SUB(v0, s1, v2) RTM_IMPL_NAMESPACE::vector4f { (v2).x - ((v0).x * (s1)), (v2).y - ((v0).y * (s1)), (v2).z - ((v0).z * (s1)), (v2).w - ((v0).w * (s1)) }
#endif

#if defined(RTM_AVX_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_SELECT(mask, if_true, if_false) _mm_blendv_ps(if_false, if_true, mask)

	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR2D_SELECT(mask, if_true, if_false) _mm_blendv_pd(if_false, if_true, mask)
#elif defined(RTM_SSE2_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_SELECT(mask, if_true, if_false) _mm_or_ps(_mm_andnot_ps(mask, if_false), _mm_and_ps(if_true, mask))

	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR2D_SELECT(mask, if_true, if_false) _mm_or_pd(_mm_andnot_pd(mask, if_false), _mm_and_pd(if_true, mask))
#elif defined(RTM_NEON_INTRINSICS)
	//////////////////////////////////////////////////////////////////////////
	// Per component selection depending on the mask: mask != 0 ? if_true : if_false
	//////////////////////////////////////////////////////////////////////////
	#define RTM_VECTOR4F_SELECT(mask, if_true, if_false) vbslq_f32(mask, if_true, if_false)

	// RTM_VECTOR2D_SELECT not defined for NEON yet, TODO
#else
	// Macros not defined for scalar code path
#endif

#if defined(RTM_SSE2_INTRINSICS)
	#if defined(RTM_COMPILER_MSVC)
		//////////////////////////////////////////////////////////////////////////
		// Creates a vector constant from its 4 components
		//////////////////////////////////////////////////////////////////////////
		#define RTM_VECTOR4F_MAKE(x, y, z, w) { { x, y, z, w } }

		//////////////////////////////////////////////////////////////////////////
		// Creates a vector constant from its 2 components
		//////////////////////////////////////////////////////////////////////////
		#define RTM_VECTOR2D_MAKE(x, y) { { x, y } }
	#else
		//////////////////////////////////////////////////////////////////////////
		// Creates a vector constant from its 4 components
		//////////////////////////////////////////////////////////////////////////
		#define RTM_VECTOR4F_MAKE(x, y, z, w) { x, y, z, w }

		//////////////////////////////////////////////////////////////////////////
		// Creates a vector constant from its 2 components
		//////////////////////////////////////////////////////////////////////////
		#define RTM_VECTOR2D_MAKE(x, y) { x, y }
	#endif
#elif defined(RTM_NEON_INTRINSICS)
	// RTM_VECTOR2D_SELECT not defined for NEON yet, TODO
#else
	// Macros not defined for scalar code path
#endif

RTM_IMPL_FILE_PRAGMA_POP
