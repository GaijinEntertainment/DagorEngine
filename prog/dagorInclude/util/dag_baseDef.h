//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/// common real number type, defined as float
#ifndef __GAIJIN__REAL_TYPE_DEFINED__
#define __GAIJIN__REAL_TYPE_DEFINED__
typedef float real;
#endif

#if defined(__GNUC__) || defined(__SNC__)
typedef long long __int64;
#endif


#ifndef del_it
#define del_it(a) \
  do              \
  {               \
    if (a)        \
    {             \
      delete (a); \
      (a) = NULL; \
    }             \
  } while (0)
#endif

#ifndef free_it
#define free_it(a) \
  do               \
  {                \
    if (a)         \
    {              \
      free(a);     \
      (a) = NULL;  \
    }              \
  } while (0)
#endif

#ifndef destroy_it
#define destroy_it(a) \
  do                  \
  {                   \
    if (a)            \
    {                 \
      (a)->destroy(); \
      (a) = NULL;     \
    }                 \
  } while (0)
#endif

#ifdef __cplusplus
template <typename T>
struct DestroyDeleter
{
  void operator()(T *ptr) { ptr ? ptr->destroy() : (void)0; }
};
#endif

#define P2D(a)  double((a).x), double((a).y)
#define P3D(a)  double((a).x), double((a).y), double((a).z)
#define P4D(a)  double((a).x), double((a).y), double((a).z), double((a).w)
#define B2D(a)  double((a)[0].x), double((a)[0].y), double((a)[1].x), double((a)[1].y)
#define B3D(a)  P3D(a[0]), P3D(a[1])
#define V3D(a)  v_extract_x(a), v_extract_y(a), v_extract_z(a)
#define V4D(a)  v_extract_x(a), v_extract_y(a), v_extract_z(a), v_extract_w(a)
#define V3I(a)  v_extract_xi(a), v_extract_yi(a), v_extract_zi(a)
#define V4I(a)  v_extract_xi(a), v_extract_yi(a), v_extract_zi(a), v_extract_wi(a)
#define VB3D(a) V3D(a.bmin), V3D(a.bmax)

#define TMD(a)                                                                                                                      \
  (a).m[0][0], (a).m[0][1], (a).m[0][2], (a).m[1][0], (a).m[1][1], (a).m[1][2], (a).m[2][0], (a).m[2][1], (a).m[2][2], (a).m[3][0], \
    (a).m[3][1], (a).m[3][2]

#define VTMD(a) V3D(a.col0), V3D(a.col1), V3D(a.col2), V3D(a.col3)

#if defined(_TARGET_CPU_BE)
//   Big Endian version (XBOX360 PPC)

#define MAKE4C(a, b, c, d) ((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))

#define _MAKE4C(x) int(x)

// dump four-cc code for pattern %c%c%c%c - for debug output
#define DUMP4C(x) char(((x) >> 24) & 0xFF), char(((x) >> 16) & 0xFF), char(((x) >> 8) & 0xFF), char((x)&0xFF)

// dump four-cc code for pattern %c%c%c%c (0 is replaced with ' ')
#define _DUMP4C(x)                                                                                                  \
  (((x) >> 24) & 0xFF) ? char((((x) >> 24) & 0xFF)) : ' ', (((x) >> 16) & 0xFF) ? char((((x) >> 16) & 0xFF)) : ' ', \
    (((x) >> 8) & 0xFF) ? char((((x) >> 8) & 0xFF)) : ' ', ((x)&0xFF) ? char(((x)&0xFF)) : ' '
#else
//   Little Endian version (IA32)

#define MAKE4C(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define _MAKE4C(x) MAKE4C((int(x) >> 24) & 0xFF, (int(x) >> 16) & 0xFF, (int(x) >> 8) & 0xFF, int(x) & 0xFF)

// dump four-cc code for pattern %c%c%c%c - for debug output
#define DUMP4C(x)  char((x)&0xFF), char(((x) >> 8) & 0xFF), char(((x) >> 16) & 0xFF), char(((x) >> 24) & 0xFF)

// dump four-cc code for pattern %c%c%c%c (0 is replaced with ' ')
#define _DUMP4C(x)                                                                            \
  ((x)&0xFF) ? char(((x)&0xFF)) : ' ', (((x) >> 8) & 0xFF) ? char((((x) >> 8) & 0xFF)) : ' ', \
    (((x) >> 16) & 0xFF) ? char((((x) >> 16) & 0xFF)) : ' ', (((x) >> 24) & 0xFF) ? char((((x) >> 24) & 0xFF)) : ' '
#endif


#ifndef UNALIGNED
#if defined(__SNC__)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#if defined(_MSC_VER)
#define SNPRINTF(buf, buf_size, ...)                                 \
  do                                                                 \
  {                                                                  \
    int sprintf_result_len_ = _snprintf(buf, buf_size, __VA_ARGS__); \
    G_ASSERT((unsigned)sprintf_result_len_ < (unsigned)buf_size);    \
    static_cast<void>(sprintf_result_len_);                          \
    buf[(buf_size)-1] = 0;                                           \
  } while (0)
#else
#define SNPRINTF(buf, buf_size, ...)                                 \
  do                                                                 \
  {                                                                  \
    int sprintf_result_len_ = _snprintf(buf, buf_size, __VA_ARGS__); \
    G_ASSERT((unsigned)sprintf_result_len_ < (unsigned)buf_size);    \
    static_cast<void>(sprintf_result_len_);                          \
  } while (0)
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C1 | _TARGET_C2
static constexpr int DAGOR_MAX_PATH = 260;
#else
static constexpr int DAGOR_MAX_PATH = 516;
#endif

#if !(_TARGET_PC_WIN | _TARGET_XBOX)
static constexpr int MAX_PATH = DAGOR_MAX_PATH;
#endif
