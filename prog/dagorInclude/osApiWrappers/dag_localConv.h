//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdarg.h>

#include <supp/dag_define_KRNLIMP.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // Initialize tables for operate on symbols of current DOS code page
  KRNLIMP void dd_init_local_conv(void);

  // Convert all letters to upper-case
  KRNLIMP char *dd_strupr(char *s);

  // Convert all letters to lower-case
  KRNLIMP char *dd_strlwr(char *s);

  KRNLIMP char dd_charupr(unsigned char c);

  KRNLIMP char dd_charlwr(unsigned char c);

  // Compare strings (case-insensitive)
  // stricmp() analogue
  KRNLIMP int dd_stricmp(const char *a, const char *b);

  // Compare no more than n first symbols of strings (case-insensitive)
  //  strnicmp() analogue
  KRNLIMP int dd_strnicmp(const char *a, const char *b, int n);

  // Compare n symbols (case-insensitive)
  // memicmp() analogue
  KRNLIMP int dd_memicmp(const char *a, const char *b, int n);

  //! returns pointer to internal uptab[256]
  extern KRNLIMP const unsigned char *dd_local_cmp_uptab;
  //! returns pointer to internal lwtab[256]
  extern KRNLIMP const unsigned char *dd_local_cmp_lwtab;

#ifdef __cplusplus
}
#endif
