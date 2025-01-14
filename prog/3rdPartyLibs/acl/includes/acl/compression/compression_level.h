#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2019 Nicholas Frechette & Animation Compression Library contributors
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

#include "acl/version.h"
#include "acl/core/error.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>
#include <cstring>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// compression_level8 represents how aggressively we attempt to reduce the memory
	// footprint. Higher levels will try more permutations and bit rates. The higher
	// the level, the slower the compression but the smaller the memory footprint.
	enum class compression_level8 : uint8_t
	{
		lowest		= 0,	// Same as medium for now
		low			= 1,	// Same as medium for now
		medium		= 2,
		high		= 3,
		highest		= 4,

		// Automatic attempts to pick the best compression level based on various
		// properties of the input clip: clip length, longest transform chain, etc.
		// This offers a nice balance between size and speed for the majority of clips.
		automatic	= 100,

		//lossless	= 255,	// Not implemented, reserved
	};

	//////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Returns a string representing the compression level.
	// TODO: constexpr
	inline const char* get_compression_level_name(compression_level8 level);

	//////////////////////////////////////////////////////////////////////////
	// Returns the compression level from its string representation.
	inline bool get_compression_level(const char* level_name, compression_level8& out_level);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/compression_level.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
