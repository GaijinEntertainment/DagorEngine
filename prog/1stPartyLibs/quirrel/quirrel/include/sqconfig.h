#ifndef _SQ64
#define _SQ64 // always use 64 bit Integers (even on 32 bit platform)
#endif

#define __STDC_FORMAT_MACROS // Linux/Adnroid won't define PRId* macroses without this
#include <stdint.h>
#include <inttypes.h>

#ifdef _SQ64
    typedef int64_t  SQInteger;
    typedef uint64_t SQUnsignedInteger;
    typedef uint64_t SQHash; /*should be the same size of a pointer*/
#else
    typedef intptr_t SQInteger;
    typedef uintptr_t SQUnsignedInteger;
    typedef uintptr_t SQHash; /*should be the same size of a pointer*/
#endif

typedef int SQInt32;
typedef unsigned int SQUnsignedInteger32;


#ifndef __forceinline
#define __forceinline inline
#endif

#ifdef SQUSEDOUBLE
    typedef double SQFloat;
#else
    typedef float SQFloat;
#endif

#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
    typedef int64_t SQRawObjectVal; //must be 64bits
    #define SQ_OBJECT_RAWINIT() { _unVal.raw = 0; }
#else
    typedef SQUnsignedInteger SQRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
    #define SQ_OBJECT_RAWINIT()
#endif

#ifndef SQ_ALIGNMENT // SQ_ALIGNMENT shall be less than or equal to SQ_MALLOC alignments, and its value shall be power of 2.
    #if defined(SQUSEDOUBLE) || defined(_SQ64)
        #define SQ_ALIGNMENT 8
    #else
        #define SQ_ALIGNMENT 4
    #endif
#endif

typedef void* SQUserPointer;
typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

typedef char SQChar;
#define _SC(a) a
#if defined __EMSCRIPTEN__
#define scsprintf   snprintf
#elif (!defined(_TARGET_PC_WIN) && !defined(_TARGET_XBOX)) || (defined(_MSC_VER) && _MSC_VER <= 1800)
#define scsprintf   _snprintf
#else
#define scsprintf   snprintf
#endif
#ifdef _SQ64
#ifdef _MSC_VER
#define scstrtol    _strtoi64
#else
#define scstrtol    strtoll
#endif
#else
#define scstrtol    strtol
#endif
#define SQ_MAX_CHAR 0xFF

#ifdef _SQ64
    #define _PRINT_INT_PREC _SC("ll")
    #define _PRINT_INT_FMT _SC("%" PRId64)
#else
    #define _PRINT_INT_FMT _SC("%d")
#endif

#define SQ_USED_MEM_COUNTER_DECL namespace sqmemtrace { extern unsigned mem_used; }
#define SQ_USED_MEM_COUNTER sqmemtrace::mem_used

#define SQ_CHECK_THREAD_LEVEL_NONE 0
#define SQ_CHECK_THREAD_LEVEL_FAST 1
#define SQ_CHECK_THREAD_LEVEL_DEEP 2

#ifndef SQ_CHECK_THREAD
#define SQ_CHECK_THREAD SQ_CHECK_THREAD_LEVEL_NONE
#endif

#define MIN_SQ_INTEGER SQInteger(1ULL << (sizeof(SQInteger) * 8 - 1))
