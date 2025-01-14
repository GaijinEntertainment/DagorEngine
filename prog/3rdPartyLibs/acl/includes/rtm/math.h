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

#include "rtm/impl/detect_arch.h"
#include "rtm/impl/detect_compiler.h"

//////////////////////////////////////////////////////////////////////////
// Detect which intrinsics the current compilation environment supports.
//////////////////////////////////////////////////////////////////////////

#if !defined(RTM_NO_INTRINSICS)
	#if defined(__AVX2__)
		#define RTM_AVX2_INTRINSICS
		#define RTM_FMA_INTRINSICS
		#define RTM_AVX_INTRINSICS
		#define RTM_SSE4_INTRINSICS
		#define RTM_SSE3_INTRINSICS
		#define RTM_SSE2_INTRINSICS
	#elif defined(__AVX__)
		#define RTM_AVX_INTRINSICS
		#define RTM_SSE4_INTRINSICS
		#define RTM_SSE3_INTRINSICS
		#define RTM_SSE2_INTRINSICS
	#elif defined(__SSE4_1__)
		#define RTM_SSE4_INTRINSICS
		#define RTM_SSE3_INTRINSICS
		#define RTM_SSE2_INTRINSICS
	#elif defined(__SSSE3__)
		#define RTM_SSE3_INTRINSICS
		#define RTM_SSE2_INTRINSICS
	#elif defined(__SSE2__) || defined(RTM_ARCH_X86) || defined(RTM_ARCH_X64)
		#define RTM_SSE2_INTRINSICS
	#endif

	#if defined(RTM_ARCH_ARM64)
		#define RTM_NEON_INTRINSICS
		#define RTM_NEON64_INTRINSICS
	#elif defined(RTM_ARCH_ARM)
		#define RTM_NEON_INTRINSICS
	#endif

	// If SSE2 and NEON aren't used, we default to the scalar implementation
	#if !defined(RTM_SSE2_INTRINSICS) && !defined(RTM_NEON_INTRINSICS)
		#define RTM_NO_INTRINSICS
	#endif
#endif

#if defined(RTM_SSE2_INTRINSICS)
	#include <xmmintrin.h>
	#include <emmintrin.h>

	// With MSVC and SSE2, we can use the __vectorcall calling convention to pass vector types and aggregates by value through registers
	// for improved code generation
	#if defined(RTM_COMPILER_MSVC) && !defined(_MANAGED) && !defined(_M_CEE) && (!defined(_M_IX86_FP) || (_M_IX86_FP > 1)) && !defined(RTM_SIMD_CALL)
		#if RTM_COMPILER_MSVC >= RTM_COMPILER_MSVC_2015
			#define RTM_USE_VECTORCALL
		#endif
	#endif
#endif

#if defined(RTM_SSE3_INTRINSICS)
	#include <pmmintrin.h>
	#include <tmmintrin.h>
#endif

#if defined(RTM_SSE4_INTRINSICS)
	#include <smmintrin.h>
#endif

#if defined(RTM_AVX_INTRINSICS)
	#include <immintrin.h>
#endif

#if defined(RTM_NEON64_INTRINSICS) && defined(RTM_COMPILER_MSVC)
	// MSVC specific header
	#include <arm64_neon.h>
#elif defined(RTM_NEON_INTRINSICS)
	#include <arm_neon.h>
#endif

// Specify the SIMD calling convention is we can
#if !defined(RTM_SIMD_CALL)
	#if defined(RTM_USE_VECTORCALL)
		#define RTM_SIMD_CALL __vectorcall
	#else
		#define RTM_SIMD_CALL
	#endif
#endif

// By default, we include the type definitions, feature detection, and error handling
#include "rtm/impl/detect_features.h"
#include "rtm/impl/error.h"
#include "rtm/types.h"

#include <vecmath/dag_vecMath.h>
#include <math/dag_check_nan.h>
