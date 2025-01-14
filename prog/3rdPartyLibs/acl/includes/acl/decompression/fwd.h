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

#include "acl/version.h"

////////////////////////////////////////////////////////////////////////////////
// This header provides forward declarations for all public ACL types.
// Forward declaring symbols from a 3rd party library is a bad idea, use this
// header instead.
// See also: https://blog.libtorrent.org/2017/12/forward-declarations-and-abi/
////////////////////////////////////////////////////////////////////////////////

namespace acl
{
    ACL_IMPL_VERSION_NAMESPACE_BEGIN

    struct database_settings;
    struct null_database_settings;
    struct debug_database_settings;
    struct default_database_settings;

    struct decompression_settings;
    struct debug_scalar_decompression_settings;
    struct default_scalar_decompression_settings;
    struct debug_transform_decompression_settings;
    struct default_transform_decompression_settings;

    enum class streaming_action;
    struct streaming_request;
    struct streaming_request_id;
    class database_streamer;
    class null_database_streamer;

    enum class database_stream_request_result;
    template<class database_settings_type> class database_context;

    template<class decompression_settings_type> class decompression_context;

    ACL_IMPL_VERSION_NAMESPACE_END
}
