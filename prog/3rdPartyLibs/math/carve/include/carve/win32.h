// Copyright 2006 Tobias Sargeant (toby@permuted.net)
// All rights reserved.
#pragma once

#pragma warning (disable : 4996)
#pragma warning (disable : 4786)

#include <string.h>
#include <stdlib.h>

inline int strcasecmp(const char *a, const char *b) {
  return _stricmp(a,b);
}

inline void srandom(unsigned long input) {
  srand(input);
}

inline long random() {
  return rand();
}

#if defined(_MSC_VER)
#if _MSC_VER < 1700
#  include <carve/cbrt.h>
#endif

#if _MSC_VER < 1300
// intptr_t is an integer type that is big enough to hold a pointer
// It is not defined in VC6 so include a definition here for the older compiler
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif

#  if _MSC_VER < 1600
// stdint.h is not available before VS2010
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#  else
#    include <stdint.h>
#  endif

#  if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
#    define _SSIZE_T_
#    define _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#  endif
#endif
