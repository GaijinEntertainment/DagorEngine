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

#include "acl/version.h"
#include "acl/core/impl/compiler_utils.h"

#include <chrono>
#include <cstdint>

ACL_IMPL_FILE_PRAGMA_PUSH

namespace acl
{
	ACL_IMPL_VERSION_NAMESPACE_BEGIN

	////////////////////////////////////////////////////////////////////////////////
	// A scope activated profiler.
	////////////////////////////////////////////////////////////////////////////////
	class scope_profiler
	{
	public:
		////////////////////////////////////////////////////////////////////////////////
		// Creates and starts a scope profiler and automatically starts it.
		scope_profiler();

		////////////////////////////////////////////////////////////////////////////////
		// Destroys a scope profiler and automatically stops it.
		~scope_profiler() = default;

		////////////////////////////////////////////////////////////////////////////////
		// Manually stops the profiler.
		scope_profiler& stop();

		////////////////////////////////////////////////////////////////////////////////
		// Returns whether the profiler has been stopped or not.
		bool is_stopped() const { return m_start_time != m_end_time; }

		////////////////////////////////////////////////////////////////////////////////
		// Returns the elapsed time in nanoseconds since the profiler was started.
		std::chrono::nanoseconds get_elapsed_time() const;

		////////////////////////////////////////////////////////////////////////////////
		// Returns the elapsed time in microseconds since the profiler was started.
		double get_elapsed_microseconds() const { return std::chrono::duration<double, std::chrono::microseconds::period>(get_elapsed_time()).count(); }

		////////////////////////////////////////////////////////////////////////////////
		// Returns the elapsed time in milliseconds since the profiler was started.
		double get_elapsed_milliseconds() const { return std::chrono::duration<double, std::chrono::milliseconds::period>(get_elapsed_time()).count(); }

		////////////////////////////////////////////////////////////////////////////////
		// Returns the elapsed time in seconds since the profiler was started.
		double get_elapsed_seconds() const { return std::chrono::duration<double, std::chrono::seconds::period>(get_elapsed_time()).count(); }

	private:
		scope_profiler(const scope_profiler&) = delete;
		scope_profiler& operator=(const scope_profiler&) = delete;

		using time_point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

		// The time at which the profiler started.
		time_point_t	m_start_time;

		// The time at which the profiler stopped.
		time_point_t	m_end_time;
	};

	//////////////////////////////////////////////////////////////////////////

	inline scope_profiler::scope_profiler()
	{
		m_start_time = m_end_time = std::chrono::high_resolution_clock::now();
	}

	inline scope_profiler& scope_profiler::stop()
	{
		if (!is_stopped())
			m_end_time = std::chrono::high_resolution_clock::now();

		return *this;
	}

	inline std::chrono::nanoseconds scope_profiler::get_elapsed_time() const
	{
		const time_point_t end = is_stopped() ? m_end_time : std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start_time);
	}

	ACL_IMPL_VERSION_NAMESPACE_END
}

ACL_IMPL_FILE_PRAGMA_POP
