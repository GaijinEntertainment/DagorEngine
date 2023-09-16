#ifndef _BMG_UTILS_H_
#define _BMG_UTILS_H_
/*
    some handy utilities used in several units

    Copyright 2001
    M. Scott Heiman
    All Rights Reserved

    Modified by Nic Tiger (Dagor Technologies, Inc.) for DagorEngine3, 2006
*/

#include "bmgImage.h"

/* the following 2 functions are for dealing with file formats
   that store data in big endian format */
static unsigned long SwapULong(unsigned long __x)
{
  return (unsigned long)
  ((((unsigned long)__x & 0x000000ffUL) << 24) |
   (((unsigned long)__x & 0x0000ff00UL) << 8) |
   (((unsigned long)__x & 0x00ff0000UL) >> 8) |
   (((unsigned long)__x & 0xff000000UL) >> 24));
}

static unsigned short SwapUShort(unsigned short __x)
{
  return (unsigned short)
  ((((unsigned short)__x & 0x00ffUL) << 8) |
   (((unsigned short)__x & 0xff00UL) >> 8));
}

#endif
