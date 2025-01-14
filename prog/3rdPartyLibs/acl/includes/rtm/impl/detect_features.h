#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2021 Nicholas Frechette & Realtime Math contributors
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
// Helper macro to determine if vrndns_f32 is supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VRNDNS_SUPPORTED)
	// ARM documentation states __ARM_FEATURE_DIRECTED_ROUNDING must be defined
	#if defined(__ARM_FEATURE_DIRECTED_ROUNDING)
		// Only support it with clang for now
		#if defined(RTM_COMPILER_CLANG)
			// Apple redefines __clang_major__ to match the XCode version
			#if defined(__APPLE__)
				#if __clang_major__ >= 10
					// Apple clang supports it starting with XCode 10
					#define RTM_IMPL_VRNDNS_SUPPORTED
				#endif
			#else
				#if __clang_major__ >= 6
					// Ordinary clang supports it starting with clang 6
					#define RTM_IMPL_VRNDNS_SUPPORTED
				#endif
			#endif
		#endif
	#endif

	// MSVC doesn't appear to define __ARM_FEATURE_DIRECTED_ROUNDING but it supports the
	// intrinsic as of VS2019
	// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
	#if !defined(RTM_IMPL_VRNDNS_SUPPORTED) && defined(RTM_COMPILER_MSVC) && defined(vrndns_f32)
		#define RTM_IMPL_VRNDNS_SUPPORTED
	#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Helper macro to determine if the vca* (e.g vcagtq_f32) family of intrinsics are supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VCA_SUPPORTED)
	#if defined(RTM_COMPILER_MSVC)
		#if defined(vcaleq_f32)
			// Support was introduced in VS2019
			// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
			#define RTM_IMPL_VCA_SUPPORTED
		#endif
	#else
		// Always enable with GCC and clang for now
		#define RTM_IMPL_VCA_SUPPORTED
	#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Helper macro to determine if the vc*z* (e.g vceqq_f32) family of intrinsics are supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VCZ_SUPPORTED)
	#if defined(RTM_COMPILER_MSVC)
		#if defined(vceqzq_f32)
			// Support was introduced in VS2019
			// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
			#define RTM_IMPL_VCZ_SUPPORTED
		#endif
	#else
		// Always enable with GCC and clang for now
		#define RTM_IMPL_VCZ_SUPPORTED
	#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Helper macro to determine if the vsqrtq_f32 intrinsic is supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VSQRT_SUPPORTED)
	#if defined(RTM_COMPILER_MSVC)
		#if defined(vsqrtq_f32)
			// Support was introduced in VS2019
			// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
			#define RTM_IMPL_VSQRT_SUPPORTED
		#endif
	#else
		// Always enable with GCC and clang for now
		#define RTM_IMPL_VSQRT_SUPPORTED
	#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Helper macro to determine if vfmss_laneq_f32 is supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VFMSS_SUPPORTED)
	#if defined(RTM_COMPILER_MSVC)
		#if defined(vfmss_laneq_f32)
			// Support was introduced in VS2019
			// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
			#define RTM_IMPL_VFMSS_SUPPORTED
		#endif
	#else
		// Always enable with GCC and clang for now
		#define RTM_IMPL_VFMSS_SUPPORTED
	#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Helper macro to determine if vaddvq_f32 is supported (ARM64 only)
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_ARCH_ARM64) && !defined(RTM_IMPL_VADDVQ_SUPPORTED)
	#if defined(RTM_COMPILER_MSVC)
		#if defined(vaddvq_f32)
			// Support was introduced in VS2019
			// MSVC uses defines for the ARM intrinsics, use them to perform feature detection
			#define RTM_IMPL_VADDVQ_SUPPORTED
		#endif
	#else
		// Always enable with GCC and clang for now
		#define RTM_IMPL_VADDVQ_SUPPORTED
	#endif
#endif
