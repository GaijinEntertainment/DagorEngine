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
#include "acl/core/impl/compiler_utils.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Enum for versions used by 'compressed_tracks'.
	// These values are used by serialization, do not change them once assigned.
	// To add a new version, create a new entry and follow the naming and numbering scheme
	// described below.
	enum class compressed_tracks_version16 : uint16_t
	{
		//////////////////////////////////////////////////////////////////////////
		// Special version identifier used when decompressing.
		// This indicates that any version is supported by decompression and
		// the code isn't optimized for any one version in particular.
		// It is not a valid value for compressed tracks.
		any			= 0,

		//////////////////////////////////////////////////////////////////////////
		// Special version identifier used when decompressing.
		// This indicates that no version is supported by decompression.
		// It is not a valid value for compressed tracks.
		none		= 0xFFFF,

		//////////////////////////////////////////////////////////////////////////
		// Actual versions in sequential order.
		// Keep the enum values sequential.
		// Enum value name should be of the form: major, minor, patch version.
		// Two digits are reserved for safety and future proofing.

		//v00_01_00 = 0,			// ACL v0.1.0
		//v00_06_00 = 1,			// ACL v0.6.0
		//v00_08_00 = 2,			// ACL v0.8.0
		//v01_01_00 = 3,			// ACL v1.1.0
		//v01_02_00 = 4,			// ACL v1.2.0
		//v01_03_00 = 5,			// ACL v1.3.0
		//v01_99_99	= 6,			// ACL v2.0.0-wip
		v02_00_00	= 7,			// ACL v2.0.0
		v02_01_99	= 8,			// ACL v2.1.0-wip
		v02_01_99_1	= 9,			// ACL v2.1.0-wip (removed constant thresholds in track desc, increased bit rates, remapped raw num bits to 31 in compressed tracks)
		v02_01_99_2 = 10,			// ACL v2.1.0-wip (converted error contribution metadata)
		v02_01_00	= 10,			// ACL v2.1.0

		//////////////////////////////////////////////////////////////////////////
		// First version marker, this is equal to the first version supported: ACL 2.0.0
		// Versions prior to ACL 2.0 are not backwards compatible.
		// It is used for range checks.
		first		= v02_00_00,

		//////////////////////////////////////////////////////////////////////////
		// Always assigned to the latest version supported.
		latest		= v02_01_00,
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
