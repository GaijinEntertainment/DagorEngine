#pragma once

#ifdef __HAIKU__
#define _GNU_SOURCE 1
#endif

#ifdef _MSC_VER
#pragma warning(disable:4005)    // macro redifinition (in flex file)
#pragma warning(disable:4146)    // unsigned unary minus
#pragma warning(disable:4996)    // swap ranges
#pragma warning(disable:4201)    // nonstandard extension used : nameless struct / union
#pragma warning(disable:4324)    // structure was padded due to alignment specifier
#pragma warning(disable:4067)    // unexpected tokens following preprocessor directive - expected a newline
#pragma warning(disable:4800)    // forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable:4127)    // conditional expression is constant
#pragma warning(disable:4702)    // unreachable code (due to exceptions)
#pragma warning(disable:4316)    // '__m128': object allocated on the heap may not be aligned 16
#pragma warning(disable:4714)    // marked as __forceinline not inlined
#pragma warning(disable:4180)    // qualifier applied to function type has no meaning; ignored
#pragma warning(disable:4305)    // truncation from 'double' to 'float'
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wsequence-point"
#if __GNUC__>=9
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#endif

#include <assert.h>
#include <string.h>

#ifdef _MSC_VER
#include <intsafe.h>
#include <stdarg.h>
#endif

#ifdef _MSC_VER
    #define DAS_NON_POD_PADDING 1
#else
    #define DAS_NON_POD_PADDING 0
#endif

#ifdef _MSC_VER
  #ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES 1
  #endif
#endif


#ifdef _EMSCRIPTEN_VER
    typedef struct _IO_FILE { char __x; } FILE;
#endif

#include <math.h>

#include <stdint.h>
#include <float.h>
#include <daScript/das_config.h>

#if _TARGET_PC_MACOSX && __SSE__
   #define DAS_EVAL_ABI [[clang::vectorcall]]
#elif (defined(_MSC_VER) || defined(__clang__)) && __SSE__ && !defined __HAIKU__
    #define DAS_EVAL_ABI __vectorcall
#else
    #define DAS_EVAL_ABI
#endif

#ifndef _MSC_VER
    #define __forceinline inline __attribute__((always_inline))
    #define ___noinline __attribute__((noinline))
#else
    #define ___noinline __declspec(noinline)
#endif

#if defined(__has_feature)
    #if __has_feature(address_sanitizer)
        #define DAS_SUPPRESS_UB  __attribute__((no_sanitize("undefined")))
    #endif
#endif

#ifndef DAS_SUPPRESS_UB
#define DAS_SUPPRESS_UB
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define DAS_NORETURN_PREFIX
    #define DAS_NORETURN_SUFFIX                 __attribute__((noreturn))
    #define DAS_FORMAT_PRINT_ATTRIBUTE(a,b)     __attribute__((format(printf,a,b)))
    #define DAS_FORMAT_STRING_PREFIX
#elif defined(_MSC_VER)
    #if _MSC_VER>=1900
        #define DAS_NORETURN_PREFIX             __declspec(noreturn)
        #define DAS_NORETURN_SUFFIX
        #define DAS_FORMAT_PRINT_ATTRIBUTE(a,b)
        #define DAS_FORMAT_STRING_PREFIX        __format_string
    #else
        #define DAS_NORETURN_PREFIX
        #define DAS_NORETURN_SUFFIX
        #define DAS_FORMAT_PRINT_ATTRIBUTE(a,b)
        #define DAS_FORMAT_STRING_PREFIX
    #endif
#else
    #define DAS_NORETURN_PREFIX
    #define DAS_NORETURN_SUFFIX
    #define DAS_FORMAT_PRINT_ATTRIBUTE(a,b)
    #define DAS_FORMAT_STRING_PREFIX
#endif

#if defined(_MSC_VER)
    __forceinline uint32_t das_clz(uint32_t x) {
        unsigned long r = 0;
        _BitScanReverse(&r, x);
        return uint32_t(31 - r);
    }
    __forceinline uint32_t das_ctz(uint32_t x) {
        unsigned long r = 0;
        _BitScanForward(&r, x);
        return uint32_t(r);
    }
    __forceinline uint64_t das_clz64(uint64_t x) {
        unsigned long r = 0;
    #if defined(__i386__) || defined(_M_IX86)
        if ( x >> 32 ) {
            _BitScanReverse(&r, (unsigned long)(x >> 32));
            r += 32;
        } else {
            _BitScanReverse(&r, (unsigned long)x);
        }
    #else
        _BitScanReverse64(&r, x);
    #endif
        return uint64_t(63 - r);
    }
    __forceinline uint64_t das_ctz64(uint64_t x) {
        unsigned long r = 0;
    #if defined(__i386__) || defined(_M_IX86)
        if ((uint32_t)x != 0) {
            _BitScanForward(&r, (unsigned long)x);
        } else {
            _BitScanForward(&r, (unsigned long)(x >> 32));
            r += 32;
        }
    #else
        _BitScanForward64(&r, x);
    #endif
        return uint64_t(r);
    }
    __forceinline uint32_t das_popcount(uint32_t x) {
        return uint32_t(__popcnt(x));
    }
    __forceinline uint64_t das_popcount64(uint64_t x) {
    #if defined(__i386__) || defined(_M_IX86)
        return uint64_t(__popcnt(uint32_t(x)) + __popcnt(uint32_t(x >> 32)));
    #else
        return uint64_t(__popcnt64(x));
    #endif
    }
#else
    #define das_clz __builtin_clz
    #define das_ctz __builtin_ctz
    #define das_clz64 __builtin_clzll
    #define das_ctz64 __builtin_ctzll
    #define das_popcount __builtin_popcount
    #define das_popcount64 __builtin_popcountll
#endif

#ifdef _MSC_VER

__forceinline uint32_t rotl_c(uint32_t a, uint32_t b) {
    return _rotl(a, b);
}

__forceinline uint32_t rotr_c(uint32_t a, uint32_t b) {
    return _rotr(a, b);
}

#else

__forceinline uint32_t rotl_c(uint32_t a, uint32_t b) {
    return (a << (b & 31)) | (a >> ((32 - b) & 31));
}

__forceinline uint32_t rotr_c(uint32_t a, uint32_t b) {
    return (a >> (b &31)) | (a << ((32 - b) & 31));
}

#endif

#include "daScript/misc/hal.h"

void os_debug_break();

#ifndef DAS_FATAL_LOG
#define DAS_FATAL_LOG(...)   do { printf(__VA_ARGS__); fflush(stdout); } while(0)
#endif

#ifndef DAS_FATAL_ERROR
#define DAS_FATAL_ERROR(...) { \
    DAS_FATAL_LOG(__VA_ARGS__); \
    assert(0 && "fatal error"); \
    exit(-1); \
}
#endif

#ifndef DAS_ASSERT
    #ifdef NDEBUG
        #define DAS_ASSERT(cond)
    #else
        #define DAS_ASSERT(cond) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("assertion failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                os_debug_break(); \
            } \
        }
    #endif
#endif

#ifndef DAS_ASSERTF
    #ifdef NDEBUG
        #define DAS_ASSERTF(cond,...)
    #else
        #define DAS_ASSERTF(cond,...) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("assertion failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                DAS_FATAL_LOG(__VA_ARGS__); \
                os_debug_break(); \
            } \
        }
    #endif
#endif


#ifndef DAS_VERIFY
    #ifdef NDEBUG
        #define DAS_VERIFY(cond) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("verify failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                exit(-1); \
            } \
        }
    #else
        #define DAS_VERIFY(cond) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("verify failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                os_debug_break(); \
            } \
        }
    #endif
#endif

#ifndef DAS_VERIFYF
    #ifdef NDEBUG
        #define DAS_VERIFYF(cond,...) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("verify failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                DAS_FATAL_LOG(__VA_ARGS__); \
                exit(-1); \
            } \
        }
    #else
        #define DAS_VERIFYF(cond,...) { \
            if ( !(cond) ) { \
                DAS_FATAL_LOG("verify failed: %s, %s:%d\n", #cond, __FILE__, __LINE__); \
                DAS_FATAL_LOG(__VA_ARGS__); \
                os_debug_break(); \
            } \
        }
    #endif
#endif

#ifndef DAS_ALIGNED_ALLOC
#define DAS_ALIGNED_ALLOC 1
inline void *das_aligned_alloc16(size_t size) {
#if defined(_MSC_VER)
    return _aligned_malloc(size, 16);
#else
    void * mem = nullptr;
    if (posix_memalign(&mem, 16, size)) {
        DAS_ASSERTF(0, "posix_memalign returned nullptr");
        return nullptr;
    }
    return mem;
#endif
}
inline void das_aligned_free16(void *ptr) {
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}
#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined (__linux__) || defined (_EMSCRIPTEN_VER) || defined __HAIKU__
#include <malloc.h>
#endif
inline size_t das_aligned_memsize(void * ptr){
#if defined(_MSC_VER)
    return _aligned_msize(ptr, 16, 0);
#elif defined(__APPLE__)
    return malloc_size(ptr);
#else
    return malloc_usable_size(ptr);
#endif
}
#endif

// when enabled, Context heap memory will track where the allocation came from
// via mark_location and mark_comment
#ifndef DAS_TRACK_ALLOCATIONS
#define DAS_TRACK_ALLOCATIONS   0
#endif

// when enabled, Context heap memory will be filled with 0xcd when deleted
#ifndef DAS_SANITIZER
#define DAS_SANITIZER   0
#endif

// when enabled, TypeDecl, Expression, Variable, Structure, Enumeration and Function
// will be filled with 0xcd when deleted
#ifndef DAS_MACRO_SANITIZER
#define DAS_MACRO_SANITIZER 0
#endif

#if !_TARGET_64BIT && !defined(__clang__) && (_MSC_VER <= 1900)
#define _msc_inline_bug __declspec(noinline)
#else
#define _msc_inline_bug __forceinline
#endif

#ifndef DAS_THREAD_LOCAL
#define DAS_THREAD_LOCAL  thread_local
#endif

#ifndef DAS_AOT_INLINE_LAMBDA
    #ifdef _MSC_VER
        #if __cplusplus >= 202002L
            #define DAS_AOT_INLINE_LAMBDA [[msvc::forceinline]]
        #else
            #define DAS_AOT_INLINE_LAMBDA
        #endif
    #else
        #define DAS_AOT_INLINE_LAMBDA __attribute__((always_inline))
    #endif
#endif

#ifdef DAS_SMART_PTR_DEBUG
    #define DAS_SMART_PTR_TRACKER   1
    #define DAS_SMART_PTR_MAGIC     1
#endif

#ifndef DAS_SMART_PTR_TRACKER
    #ifdef NDEBUG
        #define DAS_SMART_PTR_TRACKER   0
    #else
        #define DAS_SMART_PTR_TRACKER   1
    #endif
#endif

#ifndef DAS_SMART_PTR_MAGIC
    #ifdef NDEBUG
        #define DAS_SMART_PTR_MAGIC     0
    #else
        #define DAS_SMART_PTR_MAGIC     1
    #endif
#endif

#include "daScript/misc/smart_ptr.h"

