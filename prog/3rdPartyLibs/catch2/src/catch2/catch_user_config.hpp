
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

/*
 * All compile-time configuration options need to be here, and also documented in `docs/configuration.md`.
 */

#ifndef CATCH_USER_CONFIG_HPP_INCLUDED
#define CATCH_USER_CONFIG_HPP_INCLUDED


// ------
// Overridable compilation flags,
// these can have 3 "states": Force Yes, Force No, Use Default.
// Setting both Force Yes and Force No is an error
// ------

//#define CATCH_CONFIG_ANDROID_LOGWRITE
#define CATCH_CONFIG_NO_ANDROID_LOGWRITE

#if defined( CATCH_CONFIG_ANDROID_LOGWRITE ) && \
    defined( CATCH_CONFIG_NO_ANDROID_LOGWRITE )
#    error Cannot force ANDROID_LOGWRITE to both ON and OFF
#endif

// #define CATCH_CONFIG_COLOUR_WIN32
// #define CATCH_CONFIG_NO_COLOUR_WIN32

#if defined( CATCH_CONFIG_COLOUR_WIN32 ) && \
    defined( CATCH_CONFIG_NO_COLOUR_WIN32 )
#    error Cannot force COLOUR_WIN32 to be ON and OFF
#endif

#define CATCH_CONFIG_COUNTER
// #define CATCH_CONFIG_NO_COUNTER

#if defined( CATCH_CONFIG_COUNTER ) && \
    defined( CATCH_CONFIG_NO_COUNTER )
#    error Cannot force COUNTER to both ON and OFF
#endif



#define CATCH_CONFIG_CPP11_TO_STRING
// #define CATCH_CONFIG_NO_CPP11_TO_STRING

#if defined( CATCH_CONFIG_CPP11_TO_STRING ) && \
    defined( CATCH_CONFIG_NO_CPP11_TO_STRING )
#    error Cannot force CPP11_TO_STRING to both ON and OFF
#endif



#define CATCH_CONFIG_CPP17_BYTE
// #define CATCH_CONFIG_NO_CPP17_BYTE

#if defined( CATCH_CONFIG_CPP17_BYTE ) && \
    defined( CATCH_CONFIG_NO_CPP17_BYTE )
#    error Cannot force CPP17_BYTE to both ON and OFF
#endif



#define CATCH_CONFIG_CPP17_OPTIONAL
// #define CATCH_CONFIG_NO_CPP17_OPTIONAL

#if defined( CATCH_CONFIG_CPP17_OPTIONAL ) && \
    defined( CATCH_CONFIG_NO_CPP17_OPTIONAL )
#    error Cannot force CPP17_OPTIONAL to both ON and OFF
#endif



#define CATCH_CONFIG_CPP17_STRING_VIEW
// #define CATCH_CONFIG_NO_CPP17_STRING_VIEW

#if defined( CATCH_CONFIG_CPP17_STRING_VIEW ) && \
    defined( CATCH_CONFIG_NO_CPP17_STRING_VIEW )
#    error Cannot force CPP17_STRING_VIEW to both ON and OFF
#endif



#define CATCH_CONFIG_CPP17_UNCAUGHT_EXCEPTIONS
// #define CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS

#if defined( CATCH_CONFIG_CPP17_UNCAUGHT_EXCEPTIONS ) && \
    defined( CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS )
#    error Cannot force CPP17_UNCAUGHT_EXCEPTIONS to both ON and OFF
#endif



#define CATCH_CONFIG_CPP17_VARIANT
// #define CATCH_CONFIG_NO_CPP17_VARIANT

#if defined( CATCH_CONFIG_CPP17_VARIANT ) && \
    defined( CATCH_CONFIG_NO_CPP17_VARIANT )
#    error Cannot force CPP17_VARIANT to both ON and OFF
#endif



// #define CATCH_CONFIG_GLOBAL_NEXTAFTER
// #define CATCH_CONFIG_NO_GLOBAL_NEXTAFTER

#if defined( CATCH_CONFIG_GLOBAL_NEXTAFTER ) && \
    defined( CATCH_CONFIG_NO_GLOBAL_NEXTAFTER )
#    error Cannot force GLOBAL_NEXTAFTER to both ON and OFF
#endif


#ifndef _WIN32
#define CATCH_CONFIG_POSIX_SIGNALS
#else
#define CATCH_CONFIG_NO_POSIX_SIGNALS
#endif

#if defined( CATCH_CONFIG_POSIX_SIGNALS ) && \
    defined( CATCH_CONFIG_NO_POSIX_SIGNALS )
#    error Cannot force POSIX_SIGNALS to both ON and OFF
#endif



#define CATCH_CONFIG_GETENV
// #define CATCH_CONFIG_NO_GETENV

#if defined( CATCH_CONFIG_GETENV ) && \
    defined( CATCH_CONFIG_NO_GETENV )
#    error Cannot force GETENV to both ON and OFF
#endif



#define CATCH_CONFIG_USE_ASYNC
// #define CATCH_CONFIG_NO_USE_ASYNC

#if defined( CATCH_CONFIG_USE_ASYNC ) && \
    defined( CATCH_CONFIG_NO_USE_ASYNC )
#    error Cannot force USE_ASYNC to both ON and OFF
#endif



#define CATCH_CONFIG_WCHAR
// #define CATCH_CONFIG_NO_WCHAR

#if defined( CATCH_CONFIG_WCHAR ) && \
    defined( CATCH_CONFIG_NO_WCHAR )
#    error Cannot force WCHAR to both ON and OFF
#endif



#ifdef _WIN32
#define CATCH_CONFIG_WINDOWS_SEH
#else
#define CATCH_CONFIG_NO_WINDOWS_SEH
#endif

#if defined( CATCH_CONFIG_WINDOWS_SEH ) && \
    defined( CATCH_CONFIG_NO_WINDOWS_SEH )
#    error Cannot force WINDOWS_SEH to both ON and OFF
#endif


// #define CATCH_CONFIG_EXPERIMENTAL_STATIC_ANALYSIS_SUPPORT
#define CATCH_CONFIG_NO_EXPERIMENTAL_STATIC_ANALYSIS_SUPPORT

#if defined( CATCH_CONFIG_EXPERIMENTAL_STATIC_ANALYSIS_SUPPORT ) && \
    defined( CATCH_CONFIG_NO_EXPERIMENTAL_STATIC_ANALYSIS_SUPPORT )
#    error Cannot force STATIC_ANALYSIS_SUPPORT to both ON and OFF
#endif


#ifndef _MSC_VER
#define CATCH_CONFIG_USE_BUILTIN_CONSTANT_P
#else
#define CATCH_CONFIG_NO_USE_BUILTIN_CONSTANT_P
#endif

#if defined( CATCH_CONFIG_USE_BUILTIN_CONSTANT_P ) && \
    defined( CATCH_CONFIG_NO_USE_BUILTIN_CONSTANT_P )
#    error Cannot force USE_BUILTIN_CONSTANT_P to both ON and OFF
#endif


// ------
// Simple toggle defines
// their value is never used and they cannot be overridden
// ------


// #define CATCH_CONFIG_BAZEL_SUPPORT
// #define CATCH_CONFIG_DISABLE_EXCEPTIONS
// #define CATCH_CONFIG_DISABLE_EXCEPTIONS_CUSTOM_HANDLER
// #define CATCH_CONFIG_DISABLE
// #define CATCH_CONFIG_DISABLE_STRINGIFICATION
#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#define CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER
#define CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER
#define CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER
#define CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER
#define CATCH_CONFIG_EXPERIMENTAL_REDIRECT
#define CATCH_CONFIG_FAST_COMPILE
// #define CATCH_CONFIG_NOSTDOUT
// #define CATCH_CONFIG_PREFIX_ALL
// #define CATCH_CONFIG_PREFIX_MESSAGES

#ifdef _WIN32
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif

// #define CATCH_CONFIG_SHARED_LIBRARY


// ------
// "Variable" defines, these have actual values
// ------

#define CATCH_CONFIG_DEFAULT_REPORTER "console"
#define CATCH_CONFIG_CONSOLE_WIDTH    120

// Unlike the macros above, CATCH_CONFIG_FALLBACK_STRINGIFIER does not
// have a good default value, so we cannot always define it, and cannot
// even expose it as a variable in CMake. The users will have to find
// out about it from docs and set it only if they use it.
#define CATCH_CONFIG_FALLBACK_STRINGIFIER \
    Catch::Detail::dagor_detail::convertUnstreamableStrict


#include <string>
#include <type_traits>

namespace Catch::Detail {
    template <typename E>
    std::string convertUnknownEnumToString( E e );

    namespace dagor_detail {
        // Implement custom fallback formatter
        // It is the same as embedded one,
        // but without a default formatter for unexpected types.

        template <typename T>
        std::string convertUnknownToStringStage2(T const &t);

        template <typename T>
        std::enable_if_t<!std::is_enum_v<T> &&
                             std::is_base_of_v<std::exception, T>,
                         std::string>
        convertUnstreamableStrict( T const& ex ) {
            return ex.what();
        }

        template <typename T>
        std::enable_if_t<std::is_enum_v<T>, std::string>
        convertUnstreamableStrict( T const& value ) {
            return Catch::Detail::convertUnknownEnumToString( value );
        }

        template <typename T>
        std::enable_if_t<!std::is_enum_v<T> &&
                             !std::is_base_of_v<std::exception, T>,
                         std::string>
        convertUnstreamableStrict( T const& value ) {
            return Catch::Detail::dagor_detail::convertUnknownToStringStage2(value);
        }
    } // namespace dagor_detail
} // namespace Catch::Detail


#endif // CATCH_USER_CONFIG_HPP_INCLUDED
