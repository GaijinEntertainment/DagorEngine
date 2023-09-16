#ifndef _BMG_IMAGE_H_
#define _BMG_IMAGE_H_
/*
//  header file for the BMGImage functions
//
//  Copyright 2000, 2001 M. Scott Heiman
//
//  Modified by Nic Tiger (Dagor Technologies, Inc.) for DagorEngine3, 2006
//
*/

class IGenLoad;

enum BMG_Error {
  BMG_OK                    = 0,
  errLib                    = 1,
  errInvalidPixelFormat     = 2,
  errMemoryAllocation       = 3,
  errInvalidSize            = 4,
  errInvalidBitmapHandle    = 5,
  errWindowsAPI             = 6,
  errFileOpen               = 7,
  errUnsupportedFileFormat  = 8,
  errInvalidBMGImage        = 9,
  errInvalidFileExtension   = 10,
  errFileRead               = 11,
  errFileWrite              = 12,
  errInvalidGeoTIFFPointer  = 13,
  errUndefinedBGImage       = 14,
  errBGImageTooSmall        = 15,
  errCorruptFile            = 16
};

typedef enum BMG_Error BMGError;

struct BMGImageStruct
{
  unsigned int width;
  unsigned int height;
  unsigned char *bits; // =malloc(size+4)+4
  unsigned char *palette;
  unsigned char bits_per_pixel;
  unsigned char bytes_per_palette_entry;
  unsigned short palette_size;
  unsigned int scan_width;
};

/* default allocator used to allocate memory for images */
extern IMemAlloc *bmg_cur_mem;

/* initializes a BMGImage to default values */
extern void InitBMGImage(struct BMGImageStruct *img);

/* frees memory allocated to a BMGImage */
extern void FreeBMGImage(struct BMGImageStruct *img);

/* allocates memory (bits & palette) for a BMGImage.
   returns 1 if successfull, 0 otherwise.
   width, height, bits_per_pixel, palette_size, 
   Assumes that all images with bits_per_pixel <= 8 requires a palette.
   will set bits_per_palette_entry, scan_width, bits, & palette */
extern BMGError AllocateBMGImage(struct BMGImageStruct *img);

/* reads image from PSD format */
extern BMGError ReadPSD(IGenLoad &crd, struct BMGImageStruct *img);

#endif
