//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stdint.h>
#elif !defined(__HOST_STDINT_H__)

/* Exact sizes */
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

/* At least sizes */

typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;

typedef short int_least16_t;
typedef unsigned short uint_least16_t;

typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;

/* Fastest minimum width sizes */

typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;

typedef int int_fast16_t;
typedef unsigned uint_fast16_t;

typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;

/* Integer pointer holders */

#if _TARGET_64BIT
typedef __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
typedef __int64 ptrdiff_t;
#else
typedef int intptr_t;
typedef unsigned uintptr_t;
typedef int ptrdiff_t;
#endif

/* Greatest width integer types */

typedef long long intmax_t;
typedef unsigned long long uintmax_t;

/* long long typedefs */

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef long long int_least64_t;
typedef unsigned long long uint_least64_t;

typedef long long int_fast64_t;
typedef unsigned long long uint_fast64_t;

#endif
