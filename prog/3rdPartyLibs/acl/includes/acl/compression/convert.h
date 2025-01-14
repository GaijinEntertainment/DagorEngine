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
#include "acl/compression/track_array.h"
#include "acl/core/compressed_tracks.h"
#include "acl/core/error_result.h"
#include "acl/core/impl/compiler_utils.h"
#include "acl/core/iallocator.h"

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Convert a track array instance into a raw compressed tracks instance.
	// This is a lossless process.
	//////////////////////////////////////////////////////////////////////////
	error_result convert_track_list(iallocator& allocator, const track_array& track_list, compressed_tracks*& out_compressed_tracks);

	//////////////////////////////////////////////////////////////////////////
	// Convert a compressed tracks instance into a track array instance.
	// This is a lossless process if all the metadata is present.
	//////////////////////////////////////////////////////////////////////////
	error_result convert_track_list(iallocator& allocator, const compressed_tracks& tracks, track_array& out_track_list);

	ACL_IMPL_VERSION_NAMESPACE_END
}

#include "acl/compression/impl/convert.impl.h"

ACL_IMPL_FILE_PRAGMA_POP
