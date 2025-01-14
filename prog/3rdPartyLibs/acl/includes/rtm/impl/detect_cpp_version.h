#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2023 Nicholas Frechette & Realtime Math contributors
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
// Macros to identify individual C++ versions
//////////////////////////////////////////////////////////////////////////
#if defined(_MSVC_LANG)
	// Test MSVC first since it used to define __cplusplus to some old version by default

	#define RTM_CPP_VERSION _MSVC_LANG

	#define RTM_CPP_VERSION_14 201402L
	#define RTM_CPP_VERSION_17 201703L
	#define RTM_CPP_VERSION_20 202002L
#elif defined(__cplusplus)
	#define RTM_CPP_VERSION __cplusplus

	#define RTM_CPP_VERSION_14 201402L
	#define RTM_CPP_VERSION_17 201703L
	#define RTM_CPP_VERSION_20 202002L
#else
	// C++ version is unknown or older than C++14
	// Use a made up value to denote C++11
	#define RTM_CPP_VERSION 201100L

	#define RTM_CPP_VERSION_14 201402L
	#define RTM_CPP_VERSION_17 201703L
	#define RTM_CPP_VERSION_20 202002L
#endif
