#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Animation Compression Library contributors
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
#include "acl/core/compressed_tracks_version.h"
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Deriving from this struct and overriding these constexpr functions
	// allow you to control which code is stripped for maximum performance.
	//
	// By default, everything is supported.
	//////////////////////////////////////////////////////////////////////////
	struct database_settings
	{
		//////////////////////////////////////////////////////////////////////////
		// Which version we should optimize for.
		// If 'any' is specified, the database context will support every single version
		// with full backwards compatibility.
		// Using a specific version allows the compiler to statically strip code for all other
		// versions. This allows the creation of context objects specialized for specific
		// versions which yields optimal performance.
		// Must be static constexpr!
		static constexpr compressed_tracks_version16 version_supported() { return compressed_tracks_version16::any; }
	};

	//////////////////////////////////////////////////////////////////////////
	// These settings disable database usage.
	//////////////////////////////////////////////////////////////////////////
	struct null_database_settings final : public database_settings
	{
		static constexpr compressed_tracks_version16 version_supported() { return compressed_tracks_version16::none; }
	};

	//////////////////////////////////////////////////////////////////////////
	// These are debug settings, everything is enabled and nothing is stripped.
	// It will have the worst performance but allows every feature.
	//////////////////////////////////////////////////////////////////////////
	struct debug_database_settings : public database_settings
	{
	};

	//////////////////////////////////////////////////////////////////////////
	// These are the default settings. Only the generally optimal settings
	// are enabled and will offer the overall best performance.
	// Supports every version.
	//////////////////////////////////////////////////////////////////////////
	struct default_database_settings : public database_settings
	{
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
