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

#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"

//////////////////////////////////////////////////////////////////////////
// Include cmath header and polyfill what is missing for proper C++11 support
//////////////////////////////////////////////////////////////////////////
#include <cmath>

RTM_IMPL_FILE_PRAGMA_PUSH

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace rtm_impl
	{
		//////////////////////////////////////////////////////////////////////////
		// The version of the STL shipped with versions of GCC older than 5.1 are missing a number of type traits and functions,
		// such as std::is_trivially_default_constructible for proper C++11 support.
		// We thus manually polyfill what is missing.
		// This must also be done when the compiler is clang when it makes use of the GCC implementation of the STL,
		// which is the default behavior on linux. Properly detecting the version of the GCC STL used by clang cannot
		// be done with the __GNUC__  macro, which is overridden by clang. Instead, we check for the definition
		// of the macro _GLIBCXX_USE_CXX11_ABI which is only defined with GCC versions greater than 5.
		// _GLIBCXX_USE_C99_MATH_TR1 is sometimes undefined when some C++11 functions are unimplemented which strips most of them out.
		// This is the case for various 32 bit platforms (e.g. ARMv7)
		//////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(_LIBCPP_VERSION) && !defined(_GLIBCXX_USE_CXX11_ABI) && !defined(_GLIBCXX_USE_C99_MATH_TR1)
		inline float copysign(float x, float y) { return ::copysignf(x, y); }
		inline double copysign(double x, double y) { return ::copysign(x, y); }
#else
		inline float copysign(float x, float y) { return std::copysign(x, y); }
		inline double copysign(double x, double y) { return std::copysign(x, y); }
#endif
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
