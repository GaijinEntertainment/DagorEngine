#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2024 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/types.h"
#include "rtm/version.h"

namespace rtm
{
	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	//////////////////////////////////////////////////////////////////////////
	// Register passing typedefs
	//////////////////////////////////////////////////////////////////////////

#if defined(RTM_ARCH_X64) && defined(RTM_COMPILER_GCC)
	// On x64 with gcc, the first 8x vector4f/quatf arguments can be passed by value in a register,
	// everything else afterwards is passed by const&. They can also be returned by register.

	using vector4f_arg0 = const vector4f;
	using vector4f_arg1 = const vector4f;
	using vector4f_arg2 = const vector4f;
	using vector4f_arg3 = const vector4f;
	using vector4f_arg4 = const vector4f;
	using vector4f_arg5 = const vector4f;
	using vector4f_arg6 = const vector4f;
	using vector4f_arg7 = const vector4f;
	using vector4f_argn = const vector4f&;

	using quatf_arg0 = const quatf;
	using quatf_arg1 = const quatf;
	using quatf_arg2 = const quatf;
	using quatf_arg3 = const quatf;
	using quatf_arg4 = const quatf;
	using quatf_arg5 = const quatf;
	using quatf_arg6 = const quatf;
	using quatf_arg7 = const quatf;
	using quatf_argn = const quatf&;

	using scalarf_arg0 = const scalarf;
	using scalarf_arg1 = const scalarf;
	using scalarf_arg2 = const scalarf;
	using scalarf_arg3 = const scalarf;
	using scalarf_arg4 = const scalarf;
	using scalarf_arg5 = const scalarf;
	using scalarf_arg6 = const scalarf;
	using scalarf_arg7 = const scalarf;
	using scalarf_argn = const scalarf&;

	using scalard_arg0 = const scalard;
	using scalard_arg1 = const scalard;
	using scalard_arg2 = const scalard;
	using scalard_arg3 = const scalard;
	using scalard_arg4 = const scalard;
	using scalard_arg5 = const scalard;
	using scalard_arg6 = const scalard;
	using scalard_arg7 = const scalard;
	using scalard_argn = const scalard&;

	using mask4f_arg0 = const mask4f;
	using mask4f_arg1 = const mask4f;
	using mask4f_arg2 = const mask4f;
	using mask4f_arg3 = const mask4f;
	using mask4f_arg4 = const mask4f;
	using mask4f_arg5 = const mask4f;
	using mask4f_arg6 = const mask4f;
	using mask4f_arg7 = const mask4f;
	using mask4f_argn = const mask4f&;

	using mask4i_arg0 = const mask4i;
	using mask4i_arg1 = const mask4i;
	using mask4i_arg2 = const mask4i;
	using mask4i_arg3 = const mask4i;
	using mask4i_arg4 = const mask4i;
	using mask4i_arg5 = const mask4i;
	using mask4i_arg6 = const mask4i;
	using mask4i_arg7 = const mask4i;
	using mask4i_argn = const mask4i&;

	// gcc does not appear to support passing and returning aggregates by register
	// Non-scalar types using doubles are aggregate types

	using vector4d_arg0 = const vector4d&;
	using vector4d_arg1 = const vector4d&;
	using vector4d_arg2 = const vector4d&;
	using vector4d_arg3 = const vector4d&;
	using vector4d_arg4 = const vector4d&;
	using vector4d_arg5 = const vector4d&;
	using vector4d_arg6 = const vector4d&;
	using vector4d_arg7 = const vector4d&;
	using vector4d_argn = const vector4d&;

	using quatd_arg0 = const quatd&;
	using quatd_arg1 = const quatd&;
	using quatd_arg2 = const quatd&;
	using quatd_arg3 = const quatd&;
	using quatd_arg4 = const quatd&;
	using quatd_arg5 = const quatd&;
	using quatd_arg6 = const quatd&;
	using quatd_arg7 = const quatd&;
	using quatd_argn = const quatd&;

	using mask4d_arg0 = const mask4d&;
	using mask4d_arg1 = const mask4d&;
	using mask4d_arg2 = const mask4d&;
	using mask4d_arg3 = const mask4d&;
	using mask4d_arg4 = const mask4d&;
	using mask4d_arg5 = const mask4d&;
	using mask4d_arg6 = const mask4d&;
	using mask4d_arg7 = const mask4d&;
	using mask4d_argn = const mask4d&;

	using mask4q_arg0 = const mask4q&;
	using mask4q_arg1 = const mask4q&;
	using mask4q_arg2 = const mask4q&;
	using mask4q_arg3 = const mask4q&;
	using mask4q_arg4 = const mask4q&;
	using mask4q_arg5 = const mask4q&;
	using mask4q_arg6 = const mask4q&;
	using mask4q_arg7 = const mask4q&;
	using mask4q_argn = const mask4q&;

	using qvf_arg0 = const qvf&;
	using qvf_arg1 = const qvf&;
	using qvf_argn = const qvf&;

	using qvd_arg0 = const qvd&;
	using qvd_arg1 = const qvd&;
	using qvd_argn = const qvd&;

	using qvsf_arg0 = const qvsf&;
	using qvsf_arg1 = const qvsf&;
	using qvsf_argn = const qvsf&;

	using qvsd_arg0 = const qvsd&;
	using qvsd_arg1 = const qvsd&;
	using qvsd_argn = const qvsd&;

	using qvvf_arg0 = const qvvf&;
	using qvvf_arg1 = const qvvf&;
	using qvvf_argn = const qvvf&;

	using qvvd_arg0 = const qvvd&;
	using qvvd_arg1 = const qvvd&;
	using qvvd_argn = const qvvd&;

	using matrix3x3f_arg0 = const matrix3x3f&;
	using matrix3x3f_arg1 = const matrix3x3f&;
	using matrix3x3f_argn = const matrix3x3f&;

	using matrix3x3d_arg0 = const matrix3x3d&;
	using matrix3x3d_arg1 = const matrix3x3d&;
	using matrix3x3d_argn = const matrix3x3d&;

	using matrix3x4f_arg0 = const matrix3x4f&;
	using matrix3x4f_arg1 = const matrix3x4f&;
	using matrix3x4f_argn = const matrix3x4f&;

	using matrix3x4d_arg0 = const matrix3x4d&;
	using matrix3x4d_arg1 = const matrix3x4d&;
	using matrix3x4d_argn = const matrix3x4d&;

	using matrix4x4f_arg0 = const matrix4x4f&;
	using matrix4x4f_arg1 = const matrix4x4f&;
	using matrix4x4f_argn = const matrix4x4f&;

	using matrix4x4d_arg0 = const matrix4x4d&;
	using matrix4x4d_arg1 = const matrix4x4d&;
	using matrix4x4d_argn = const matrix4x4d&;
#endif

	RTM_IMPL_VERSION_NAMESPACE_END
}
