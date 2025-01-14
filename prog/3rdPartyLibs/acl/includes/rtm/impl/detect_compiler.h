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

//////////////////////////////////////////////////////////////////////////
// Macro to identify individual compilers
//////////////////////////////////////////////////////////////////////////
#if defined(__GNUG__) && !defined(__clang__)
	#define RTM_COMPILER_GCC
#elif defined(__clang__)
	#define RTM_COMPILER_CLANG		__clang_major__
#elif defined(_MSC_VER) && !defined(__clang__)
	#define RTM_COMPILER_MSVC		_MSC_VER
	#define RTM_COMPILER_MSVC_2013	1800
	#define RTM_COMPILER_MSVC_2015	1900
	#define RTM_COMPILER_MSVC_2017	1910
	#define RTM_COMPILER_MSVC_2019	1920
	#define RTM_COMPILER_MSVC_2022	1930

	#if RTM_COMPILER_MSVC < RTM_COMPILER_MSVC_2015
		#pragma message("Warning: This version of visual studio isn't officially supported")
	#endif
#endif
