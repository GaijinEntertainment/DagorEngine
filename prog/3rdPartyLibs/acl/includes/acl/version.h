#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2022 Nicholas Frechette & Animation Compression Library contributors
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

#include <rtm/impl/detect_compiler.h>

////////////////////////////////////////////////////////////////////////////////
// Macros to detect the ACL version
////////////////////////////////////////////////////////////////////////////////

#define ACL_VERSION_MAJOR 2
#define ACL_VERSION_MINOR 1
#define ACL_VERSION_PATCH 99

////////////////////////////////////////////////////////////////////////////////
// In order to allow multiple versions of this library to coexist side by side
// within the same executable/library, the symbols have to be unique per version.
// We achieve this by using a versioned namespace that we optionally inline.
// To disable namespace inlining, define ACL_NO_INLINE_NAMESPACE before including
// any ACL header. To disable the versioned namespace altogether,
// define ACL_NO_VERSION_NAMESPACE before including any ACL header.
////////////////////////////////////////////////////////////////////////////////

#if !defined(ACL_NO_VERSION_NAMESPACE)
	#if defined(RTM_COMPILER_MSVC) && RTM_COMPILER_MSVC == RTM_COMPILER_MSVC_2015
		// VS2015 struggles with type resolution when inline namespaces are used
		// For that reason, we disable it explicitly
		#define ACL_NO_VERSION_NAMESPACE
	#endif
#endif

// Force macro expansion to concatenate namespace identifier
#define ACL_IMPL_VERSION_CONCAT_IMPL(prefix, major, minor, patch) prefix ## major ## minor ## patch
#define ACL_IMPL_VERSION_CONCAT(prefix, major, minor, patch) ACL_IMPL_VERSION_CONCAT_IMPL(prefix, major, minor, patch)

// Name of the namespace, e.g. v205
#define ACL_IMPL_VERSION_NAMESPACE_NAME ACL_IMPL_VERSION_CONCAT(v, ACL_VERSION_MAJOR, ACL_VERSION_MINOR, ACL_VERSION_PATCH)

// Because this is being introduced in a patch release, as caution, it is disabled
// by default. It does break ABI if host runtimes forward declare types but that
// is something they shouldn't do with a 3rd party library. Now, we offer forward
// declaration headers to help prepare the migration in the next minor release.
#if defined(ACL_NO_VERSION_NAMESPACE) || !defined(ACL_ENABLE_VERSION_NAMESPACE)
	// Namespace is inlined, its usage does not need to be qualified with the
	// full version everywhere
	#define ACL_IMPL_NAMESPACE acl

	#define ACL_IMPL_VERSION_NAMESPACE_BEGIN
	#define ACL_IMPL_VERSION_NAMESPACE_END
#elif defined(ACL_NO_INLINE_NAMESPACE)
	// Namespace won't be inlined, its usage will have to be qualified with the
	// full version everywhere
	#define ACL_IMPL_NAMESPACE acl::ACL_IMPL_VERSION_NAMESPACE_NAME

	#define ACL_IMPL_VERSION_NAMESPACE_BEGIN \
		namespace ACL_IMPL_VERSION_NAMESPACE_NAME \
		{

	#define ACL_IMPL_VERSION_NAMESPACE_END \
		}
#else
	// Namespace is inlined, its usage does not need to be qualified with the
	// full version everywhere
	#define ACL_IMPL_NAMESPACE acl

	#define ACL_IMPL_VERSION_NAMESPACE_BEGIN \
		inline namespace ACL_IMPL_VERSION_NAMESPACE_NAME \
		{

	#define ACL_IMPL_VERSION_NAMESPACE_END \
		}
#endif
