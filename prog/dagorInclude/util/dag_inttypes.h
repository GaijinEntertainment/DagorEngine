//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if !defined(_MSC_VER) || _MSC_VER > 1600
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#else

#include <util/dag_stdint.h>


/* fscanf macros for signed integers */
#define SCNd8  "hhd" /* int8_t */
#define SCNd16 "hd"  /* int16_t */
#define SCNd32 "d"   /* int32_t */
#define SCNd64 "lld" /* int64_t */


/* fscanf macros for unsigned integers */
#define SCNx8  "hhx" /* uint8_t */
#define SCNx16 "hx"  /* uint16_t */
#define SCNx32 "x"   /* uint32_t */
#define SCNx64 "llx" /* uint64_t */


/* printf macros for signed integers */
#define PRId8  "hhd" /* int8_t */
#define PRId16 "hd"  /* int16_t */
#define PRId32 "d"   /* int32_t */
#define PRId64 "lld" /* int64_t */


#endif //! defined(_MSC_VER) || _MSC_VER > 1600
