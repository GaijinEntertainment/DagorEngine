/*
// source code for the BMGImage functions
//
// Copyright (C) 2001 Michael S. Heiman
//
//  Modified by Nic Tiger (Dagor Technologies, Inc.) for DagorEngine3, 2006
//
*/
#include "bmgImage.h"
#include "bmgUtils.h"
#include <stdlib.h>
#include <memory.h>
#include <setjmp.h>

/* initializes a BMGImage to default values */
void InitBMGImage(struct BMGImageStruct *img)
{
  img->width = img->height = 0;
  img->bits_per_pixel = 0;
  img->palette_size = 0;
  img->bytes_per_palette_entry = 0;
  img->bits = 0;
  img->palette = 0;
  img->scan_width = 0;
}

/* frees memory allocated to a BMGImage */
void FreeBMGImage(struct BMGImageStruct *img)
{
  if (img->bits != 0)
  {
    memfree_anywhere(img->bits-4);
    img->bits = 0;
  }
  if (img->palette != 0)
  {
    memfree_anywhere(img->palette);
    img->palette = 0;
  }
  img->bits_per_pixel = 0;
  img->palette_size = 0;
  img->bytes_per_palette_entry = 0;
  img->width = img->height = 0;
  img->scan_width = 0;
}

/* allocates memory for the bits & palette.  Assigned values to scan_line
   & bits_per_palette_entry as well. Assumes that all images with bits_per_pixel <= 8
   require a palette.
 */
BMGError AllocateBMGImage(struct BMGImageStruct *img)
{
  unsigned int mempal;

  /* make sure that all REQUIRED parameters are valid */
  if (img->width * img->height <= 0)
  {
    return errInvalidSize;
  }

  switch (img->bits_per_pixel)
  {
    case  1:
    case  4:
    case  8:
    case 16:
    case 24:
    case 32:
      break;
    default:
      return errInvalidPixelFormat;
  }

  /* delete old memory */
  if (img->bits != 0)
  {
    memfree_anywhere(img->bits-4);
    img->bits = 0;
  }
  if (img->palette != 0)
  {
    memfree_anywhere(img->palette);
    img->palette = 0;
  }

  /* allocate memory for the palette */
  if (img->bits_per_pixel <= 8)
  {
    /* we only support 3-byte and 4-byte palettes */
    if (img->bytes_per_palette_entry <= 3U)
      img->bytes_per_palette_entry = 3U;
    else
      img->bytes_per_palette_entry = 4U;
    /*
       use bits_per_pixel to determine palette_size if none was
       specified
    */
    if (img->palette_size == 0)
      img->palette_size = (unsigned short)(1 << img->bits_per_pixel);

    mempal = img->bytes_per_palette_entry * img->palette_size;
    img->palette = (unsigned char*)memalloc(mempal, bmg_cur_mem);
    memset(img->palette, 0, sizeof(mempal));
    if (img->palette == 0)
    {
      return errMemoryAllocation;
    }
  }
  else
  {
    img->bytes_per_palette_entry = 0;
    img->palette_size = 0;
  }

  /*
     set the scan width.  Bitmaps optimized for windows have scan widths that
     are evenly divisible by 4.
  */
  img->scan_width = (img->bits_per_pixel * img->width + 7) / 8;

  /* allocate memory for the bits */
  mempal = img->scan_width * img->height;
  if (mempal > 0)
  {
    img->bits = (unsigned char*)memalloc(mempal+4, bmg_cur_mem);
    if (img->bits == 0)
    {
      if (img->palette != 0)
      {
        memfree_anywhere(img->palette);
        img->palette = 0;
      }
      return errMemoryAllocation;
    }
    memset(img->bits, 0, mempal);
    img->bits += 4;
  }
  else
  {
    return errInvalidSize;
  }

  return BMG_OK;
}
