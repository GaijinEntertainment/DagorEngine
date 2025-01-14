#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/impl/detect_compiler.h"

//////////////////////////////////////////////////////////////////////////
// Because this library is made entirely of headers, we have no control over the
// compilation flags used. However, in some cases, certain options must be forced.
// To do this, every header is wrapped in two macros to push and pop the necessary
// pragmas.
//
// Options we use:
//    - Disable fast math, it can hurt precision for little to no performance gain due to the high level of hand tuned optimizations.
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_COMPILER_MSVC)
	#define RTM_IMPL_FILE_PRAGMA_PUSH \
		__pragma(float_control(precise, on, push))

	#define RTM_IMPL_FILE_PRAGMA_POP \
		__pragma(float_control(pop))
#elif defined(RTM_COMPILER_CLANG) && 0
	// For some reason, clang doesn't appear to support disabling fast-math through pragmas
	// See: https://github.com/llvm/llvm-project/issues/55392
	#define RTM_IMPL_FILE_PRAGMA_PUSH \
		_Pragma("float_control(precise, on, push)")

	#define RTM_IMPL_FILE_PRAGMA_POP \
		_Pragma("float_control(pop)")
#elif defined(RTM_COMPILER_GCC)
	#define RTM_IMPL_FILE_PRAGMA_PUSH \
		_Pragma("GCC push_options") \
		_Pragma("GCC optimize (\"no-fast-math\")")

	#define RTM_IMPL_FILE_PRAGMA_POP \
		_Pragma("GCC pop_options")
#else
	#define RTM_IMPL_FILE_PRAGMA_PUSH
	#define RTM_IMPL_FILE_PRAGMA_POP
#endif

//////////////////////////////////////////////////////////////////////////
// In some cases, for performance reasons, we wish to disable stack security
// check cookies. This macro serves this purpose.
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_COMPILER_MSVC)
	#define RTM_DISABLE_SECURITY_COOKIE_CHECK __declspec(safebuffers)
#else
	#define RTM_DISABLE_SECURITY_COOKIE_CHECK
#endif

//////////////////////////////////////////////////////////////////////////
// Force inline macros for when it is necessary.
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_COMPILER_MSVC)
	#define RTM_FORCE_INLINE __forceinline
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
	#define RTM_FORCE_INLINE __attribute__((always_inline)) inline
#else
	#define RTM_FORCE_INLINE inline
#endif

//////////////////////////////////////////////////////////////////////////
// Force no-inline macros for when it is necessary.
//////////////////////////////////////////////////////////////////////////
#if defined(RTM_COMPILER_MSVC)
	#define RTM_FORCE_NOINLINE __declspec(noinline)
#elif defined(RTM_COMPILER_GCC) || defined(RTM_COMPILER_CLANG)
	#define RTM_FORCE_NOINLINE __attribute__((noinline))
#else
	#define RTM_FORCE_NOINLINE
#endif

//////////////////////////////////////////////////////////////////////////
// Joins two pre-processor tokens: RTM_JOIN_TOKENS(foo, bar) yields 'foobar'
//////////////////////////////////////////////////////////////////////////
#define RTM_JOIN_TOKENS(a, b) a ## b
