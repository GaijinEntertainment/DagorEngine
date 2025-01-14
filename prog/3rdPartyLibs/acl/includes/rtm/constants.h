#pragma once

////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2020 Nicholas Frechette & Realtime Math contributors
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

#include "rtm/version.h"
#include "rtm/impl/compiler_utils.h"

RTM_IMPL_FILE_PRAGMA_PUSH

//////////////////////////////////////////////////////////////////////////
// This macro allows us to easily define constants that coerce to the proper desired type.
//////////////////////////////////////////////////////////////////////////
#define RTM_DEFINE_FLOAT_CONSTANT(name, value) \
	namespace rtm_impl \
	{ \
		/* Structure that holds our value at compile time */ \
		template<bool sign_bias> \
		struct RTM_JOIN_TOKENS(float_constant_, name) \
		{ \
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr operator double() const noexcept { return sign_bias ? value : -value; } \
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr operator float() const noexcept { return sign_bias ? static_cast<float>(value) : static_cast<float>(-value); } \
			/* Unary helpers */ \
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_JOIN_TOKENS(float_constant_, name) <!sign_bias> operator-() const noexcept { return RTM_JOIN_TOKENS(float_constant_, name) <!sign_bias>{}; } \
			RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> operator+() const noexcept { return RTM_JOIN_TOKENS(float_constant_, name) <sign_bias>{}; } \
		}; \
		/* Multiplication helpers */ \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator*(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs * double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator*(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) * rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator*(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs * float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator*(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) * rhs; } \
		/* Division helpers */ \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator/(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs / double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator/(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) / rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator/(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs / float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator/(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) / rhs; } \
		/* Addition helpers */ \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator+(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs + double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator+(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) + rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator+(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs + float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator+(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) + rhs; } \
		/* Subtraction helpers */ \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator-(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs - double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr double operator-(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) - rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator-(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs - float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr float operator-(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) - rhs; } \
		/* Relational helpers */ \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs < double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) < rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs < float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) < rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<=(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs <= double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) <= rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<=(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs <= float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator<=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) <= rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs > double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) > rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs > float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) > rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>=(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs >= double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) >= rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>=(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs >= float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator>=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) >= rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator==(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs == double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator==(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) == rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator==(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs == float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator==(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) == rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator!=(double lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs != double(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator!=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, double rhs) noexcept { return double(lhs) != rhs; } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator!=(float lhs, RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> rhs) noexcept { return lhs != float(rhs); } \
		template<bool sign_bias> RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr bool operator!=(RTM_JOIN_TOKENS(float_constant_, name) <sign_bias> lhs, float rhs) noexcept { return float(lhs) != rhs; } \
	} \
	/* Function user code calls to return the constant value */ \
	RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE constexpr RTM_JOIN_TOKENS(rtm_impl::float_constant_, name) <true> name() noexcept { return RTM_JOIN_TOKENS(rtm_impl::float_constant_, name) <true>{}; }

namespace rtm
{

	RTM_IMPL_VERSION_NAMESPACE_BEGIN

	namespace constants
	{
		//////////////////////////////////////////////////////////////////////////
		// Constants are defined here. Usage is meant to be as natural as possible.
		// e.g. float foo = rtm::constants::pi();
		//      double bar = rtm::constants::pi() * 0.5;
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// PI
		RTM_DEFINE_FLOAT_CONSTANT(pi, 3.141592653589793238462643383279502884)

		//////////////////////////////////////////////////////////////////////////
		// PI / 2
		RTM_DEFINE_FLOAT_CONSTANT(half_pi, 1.570796326794896619231321691639751442)

		//////////////////////////////////////////////////////////////////////////
		// PI * 2
		RTM_DEFINE_FLOAT_CONSTANT(two_pi, 6.283185307179586476925286766559005768)

		//////////////////////////////////////////////////////////////////////////
		// 1.0 / (PI * 2)
		RTM_DEFINE_FLOAT_CONSTANT(one_div_two_pi, 1.591549430918953357688837633725143620e-01)

		//////////////////////////////////////////////////////////////////////////
		// PI / 180
		RTM_DEFINE_FLOAT_CONSTANT(pi_div_one_eighty, 0.01745329251994329576923690768489)

		//////////////////////////////////////////////////////////////////////////
		// 180 / PI
		RTM_DEFINE_FLOAT_CONSTANT(one_eighty_div_pi, 57.295779513082320876798154814105)
	}

	RTM_IMPL_VERSION_NAMESPACE_END
}

RTM_IMPL_FILE_PRAGMA_POP
