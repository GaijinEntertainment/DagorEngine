// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_localConv.h>
#include <supp/_platform.h>
#include <ctype.h>

static unsigned char uptab[256], lwtab[256];

const unsigned char *dd_local_cmp_lwtab = lwtab;
const unsigned char *dd_local_cmp_uptab = uptab;

#if _TARGET_PC_WIN
#pragma warning(disable : 6385) // warning 6385| Invalid data: accessing '*tab', the readable size is '256' bytes, but '1001' bytes
                                // might be read
#endif

extern "C" void dd_init_local_conv(void)
{
  int i;

  uptab[0] = lwtab[0] = 0;
  for (i = 1; i < 128; ++i)
  {
    uptab[i] = toupper(i);
    lwtab[i] = tolower(i);
  }
  for (i = 128; i < 256; ++i)
    uptab[i] = lwtab[i] = i;
}

static struct DDLocalConvInitializer
{
  DDLocalConvInitializer() { dd_init_local_conv(); }
} dd_initializer;

extern "C" char dd_charupr(unsigned char c) { return uptab[c]; }

extern "C" char dd_charlwr(unsigned char c) { return lwtab[c]; }

extern "C" char *dd_strupr(char *s)
{
  unsigned char *p;

  if (!s)
    return s;
  for (p = (unsigned char *)s; *p; ++p)
    *p = uptab[*p];
  return s;
}

extern "C" char *dd_strlwr(char *s)
{
  unsigned char *p;

  if (!s)
    return s;
  for (p = (unsigned char *)s; *p; ++p)
    *p = lwtab[*p];
  return s;
}

extern "C" int dd_stricmp(const char *a, const char *b)
{
  int d;

  for (;; ++a, ++b)
  {
    d = (int)uptab[(unsigned char)*a] - (int)uptab[(unsigned char)*b];
    if (d)
      return d;
    if (!*a || !*b)
      break;
  }
  return d;
}

extern "C" int dd_strnicmp(const char *a, const char *b, int n)
{
  int d;

  for (d = 0; n > 0; ++a, ++b, --n)
  {
    d = (int)uptab[(unsigned char)*a] - (int)uptab[(unsigned char)*b];
    if (d)
      return d;
    if (!*a || !*b)
      break;
  }
  return d;
}

extern "C" int dd_memicmp(const char *a, const char *b, int n)
{
  int d;

  for (d = 0; n > 0; ++a, ++b, --n)
  {
    d = (int)uptab[(unsigned char)*a] - (int)uptab[(unsigned char)*b];
    if (d)
      return d;
  }
  return d;
}

#define EXPORT_PULL dll_pull_osapiwrappers_localCmp
#include <supp/exportPull.h>
