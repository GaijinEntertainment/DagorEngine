#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2017 Nicholas Frechette & Animation Compression Library contributors
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

#if defined(ACL_USE_SJSON)

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <sjson/parser_error.h>

#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	struct clip_reader_error : sjson::ParserError
	{
		enum : uint32_t
		{
			UnsupportedVersion = sjson::ParserError::Last,
			NoParentBoneWithThatName,
			NoBoneWithThatName,
			UnsignedIntegerExpected,
			InvalidCompressionSetting,
			InvalidAdditiveClipFormat,
			PositiveValueExpected,
			InvalidTrackType,
		};

		clip_reader_error() noexcept = default;

		clip_reader_error(const sjson::ParserError& e)
		{
			error = e.error;
			line = e.line;
			column = e.column;
		}

		virtual const char* get_description() const override
		{
			switch (error)
			{
			case UnsupportedVersion:
				return "This library does not support this version of animation file";
			case NoParentBoneWithThatName:
				return "There is no parent bone with this name";
			case NoBoneWithThatName:
				return "The skeleton does not define a bone with this name";
			case UnsignedIntegerExpected:
				return "An unsigned integer is expected here";
			case InvalidCompressionSetting:
				return "Invalid compression settings provided";
			case InvalidAdditiveClipFormat:
				return "Invalid additive clip format provided";
			case PositiveValueExpected:
				return "A positive value is expected here";
			case InvalidTrackType:
				return "Invalid raw track type";
			default:
				return sjson::ParserError::get_description();
			}
		}
	};

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP

#endif	// #if defined(ACL_USE_SJSON)
