/*
//  source file for the PSD file reader
//  The PSD is the Adobe Photoshop format
//  Copyright (C) 2001 M. Scott Heiman
//  All Rights Reserved
//
// You may use the software for any purpose you see fit.  You may modify
// it, incorporate it in a commercial application, use it for school,
// even turn it in as homework.  You must keep the Copyright in the
// header and source files.
// This software is not in the "Public Domain".
// You may use this software at your own risk.  I have made a reasonable
// effort to verify that this software works in the manner I expect it to;
// however,...
//
// THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS" AND
// WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE, INCLUDING
// WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR FITNESS FOR A
// PARTICULAR PURPOSE. IN NO EVENT SHALL MICHAEL S. HEIMAN BE LIABLE TO
// YOU OR ANYONE ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
// CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING
// WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE,
// OR THE CLAIMS OF THIRD PARTIES, WHETHER OR NOT MICHAEL S. HEIMAN HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
// POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
//
//  Modified by Nic Tiger (Dagor Technologies, Inc.) for DagorEngine3, 2006
//
//*/

#include "bmgImage.h"
#include "bmgUtils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <ioSys/dag_genIo.h>

union sigunion
{
  unsigned long val;
  char strng[4];
};

enum PSD_MODE {
  PSD_BITMAP        = 0,
  PSD_GRAYSCALE     = 1,
  PSD_INDEXED       = 2,
  PSD_RGB           = 3,
  PSD_CMYK          = 4,
  PSD_MULTICHANNEL  = 7,
  PSD_DUOTONE       = 8,
  PSD_LAB           = 9
};

#pragma pack( push, 1 )
struct PSDHeaderStruct
{
  unsigned int signature;  /* == "8PSD" */
  unsigned short version;   /* == 1 */
  char reserved[6];
  unsigned short channels; /* # of channels in the image (including alpha) 1-24 */
  unsigned int rows;  /* 1-30,000 */
  unsigned int columns; /* 1-30,000 */
  unsigned short depth; /* bits/channel: 1,8,16 */
  unsigned short mode;
};
#pragma pack(pop)

static int fread(void *buf, int elsz, int elcnt, IGenLoad &crd) { return crd.tryRead(buf, elsz*elcnt)/elsz; }
static int fseek(IGenLoad &crd, int pos, int whence)
{
  if (whence!=SEEK_CUR)
    return -1;
  crd.seekrel(pos);
  return 0;
}

static BMGError ReadCompressedChannel(struct BMGImageStruct *img,
                                      unsigned short offset,
                                      unsigned short channels, IGenLoad &file,
                                      unsigned int size, unsigned int row);
static BMGError ReadUncompressedChannel(struct BMGImageStruct *img,
                                        unsigned short offset,
                                        unsigned short channels, IGenLoad &file);

/*
    ReadPSD - Reads the contents of a PSD file and stores the contents into
              BMGImageStruct

    Inputs:
        filename    - the name of the file to be opened

    Outputs:
        img         - the BMGImageStruct containing the image data

    Returns:
        BMGError - if the file could not be read or a resource error occurred
        BMG_OK   - if the file was read and the data was stored in img

    this function was based upon the "Photoshop File Format" document obtained 
    from www.wotsit.org.

    PSD files are stored in big endian format - so byte-swapping is necessary.

    I have only 2 test images for this format (1 PSD_INDEXED and 1 PSD_RGB ).
    All other modes have not been tested.
*/
BMGError ReadPSD(IGenLoad &file, struct BMGImageStruct *img, bool read_pixels)
{
  struct PSDHeaderStruct header;
  union sigunion sigstring;
  unsigned int tmp_uint, i, j;
  unsigned short compression;
  unsigned short *sizes = NULL;
  int dest_channels = 0;

  jmp_buf err_jmp;
  int error;
  BMGError tmp;

  /* error handler */
  error = setjmp(err_jmp);
  if (error != 0)
  {
    if (sizes != NULL)
      memfree(sizes, tmpmem);
    FreeBMGImage(img);
    return (BMGError)error;
  }

  memcpy(sigstring.strng, "8BPS", 4);

  /* read the header */
  if (fread((void*)& header, sizeof(struct PSDHeaderStruct), 1, file) != 1)
    longjmp(err_jmp, (int)errFileRead);

  header.version = SwapUShort(header.version);
  header.channels = SwapUShort(header.channels);
  header.rows = SwapULong(header.rows);
  header.columns = SwapULong(header.columns);
  header.depth = SwapUShort(header.depth);
  header.mode = SwapUShort(header.mode);

  /* hiccup if the ID is bad */
  if (header.signature != sigstring.val || header.version != 1)
  {
    longjmp(err_jmp, (int)errCorruptFile);
  }

  dest_channels = (header.channels < 4) ? header.channels : 4;

  /* hiccup if this is an unsupported format */
  if (dest_channels > 5 || header.depth == 16 ||
      /* need an example to work with */
      header.mode == PSD_CMYK || header.mode == PSD_BITMAP ||
      /* need an example to work with */
      header.mode == PSD_MULTICHANNEL || /* need an example to work with */
      header.mode == PSD_LAB)
  {
    longjmp(err_jmp, (int)errUnsupportedFileFormat);
  }

  /* determine the bits/pixel */
  if (header.mode == PSD_RGB)
  {
    img->bits_per_pixel = (unsigned char)(dest_channels * header.depth);
  }
  else
    img->bits_per_pixel = (unsigned char)header.depth;


  /* allocate memory for the image */
  img->height = header.rows;
  img->width = header.columns;
  img->palette_size = header.mode == PSD_RGB ? 0 : 256;
  if (!read_pixels)
    return BMG_OK;

  tmp = AllocateBMGImage(img);
  if (tmp != BMG_OK)
    longjmp(err_jmp, (int)tmp);


  /* Read the color mode data section (aka palette information)
  // read the size of the palette information */
  if (fread((void*)& tmp_uint, 4, 1, file) != 1)
    longjmp(err_jmp, (int)errFileRead);
  tmp_uint = SwapULong(tmp_uint);

  /*  only PSD_INDEXED & PSD_DUOTONE formats have color data.  PSD_DUOTONE
  // is undocumented - so treat it like grayscale.  PSD_INDEXED is always
  // 256 colors */
  if (tmp_uint > 0)
  {
    if (header.mode == PSD_DUOTONE)
    {
      if (fseek(file, tmp_uint, SEEK_CUR) != 0)
        longjmp(err_jmp, (int)errFileRead);
      memset((void*)img->palette, 0,
             img->bytes_per_palette_entry * img->palette_size);
    }
    else if (header.mode == PSD_INDEXED)
    {
      if (img->bytes_per_palette_entry == 3)
      {
        if (fread((void*)img->palette, tmp_uint, 1, file) != 1)
          longjmp(err_jmp, (int)errFileRead);
      }
      else
      {
        memset((void*)img->palette, 0,
               img->bytes_per_palette_entry * img->palette_size);
        for (i = 0; i < (unsigned int)img->palette_size; i++)
        {
          if (fread((void*)(img->palette + img->bytes_per_palette_entry * i), 3,
                    1, file) != 1)
          {
            longjmp(err_jmp, (int)errFileRead);
          }
        }
      }
    }
    else
      longjmp(err_jmp, (int)errUnsupportedFileFormat);
  }

  /* create grayscale palette */
  if (header.mode == PSD_DUOTONE || header.mode == PSD_GRAYSCALE)
  {
    for (i = 0; i < (unsigned int)img->palette_size; i++)
      memset((void*)(img->palette + img->bytes_per_palette_entry * i), i, 3);
  }

  /* read the image resource section - which we ignore for now */
  if (fread((void*)& tmp_uint, 4, 1, file) != 1)
    longjmp(err_jmp, (int)errFileRead);
  tmp_uint = SwapULong(tmp_uint);

  if (tmp_uint > 0)
  {
    if (fseek(file, tmp_uint, SEEK_CUR) != 0)
      longjmp(err_jmp, (int)errFileRead);
  }

  /* read the mask and layer information - which we ignore for now */
  if (fread((void*)& tmp_uint, 4, 1, file) != 1)
    longjmp(err_jmp, (int)errFileRead);
  tmp_uint = SwapULong(tmp_uint);

  if (tmp_uint > 0)
  {
    if (fseek(file, tmp_uint, SEEK_CUR) != 0)
      longjmp(err_jmp, (int)errFileRead);
  }

  /* read the pixel values. 
  // read the compression flag 
  // compression: 0 - uncompressed, 1 - RLE compression */
  if (fread((void*)& compression, 2, 1, file) != 1)
    longjmp(err_jmp, (int)errFileRead);
  compression = SwapUShort(compression);

  /* read uncompressed data */
  if (compression == 0)
  {
    /*  PSD_RGB data is stored by channel: red bytes first, green bytes next, 
    //  blue bytes third, and (if present) alpha bytes last */
    if (header.mode == PSD_RGB)
    {
      if (header.channels < 3)
        longjmp(err_jmp, (int)errUnsupportedFileFormat);

      /* red channel */
      tmp = ReadUncompressedChannel(img, 2, header.channels, file);
      if (tmp != BMG_OK)
        longjmp(err_jmp, (int)tmp);

      /* green channel */
      tmp = ReadUncompressedChannel(img, 1, header.channels, file);
      if (tmp != BMG_OK)
        longjmp(err_jmp, (int)tmp);

      /* blue channel */
      tmp = ReadUncompressedChannel(img, 0, header.channels, file);
      if (tmp != BMG_OK)
        longjmp(err_jmp, (int)tmp);

      /* alpha channel (if present) */
      if (header.channels >= 4)
      {
        tmp = ReadUncompressedChannel(img, 3, header.channels, file);
        if (tmp != BMG_OK)
          longjmp(err_jmp, (int)tmp);
      }
    }
    else /* palleted data  */
    {
      unsigned int i;
      unsigned int numBytes = (8 * img->width + 7) / img->bits_per_pixel;
      for (i = 0; i < header.rows; i++)
      {
        if (fread((void*)(img->bits + i * img->scan_width), 1, numBytes,
                  file) != numBytes)
        {
          longjmp(err_jmp, (int)errFileRead);
        }
      }
    }
  }
  else /* read compressed data */
  {
    unsigned short offset;
    unsigned int i, bias;
    unsigned int size = header.rows *header.channels;

    /* allocate memory to hold the size of the compressed scan lines */
    sizes = (unsigned short*)memalloc(size * sizeof(unsigned short), tmpmem);
    memset(sizes, 0, size * sizeof(unsigned short));
    if (sizes == NULL)
      longjmp(err_jmp, (int)errMemoryAllocation);

    /* read the sizes of the compressed scan lines */
    for (i = 0; i < size; i++)
    {
      if (fread((void*)& sizes[i], 2, 1, file) != 1)
        longjmp(err_jmp, (int)errFileRead);
      sizes[i] = SwapUShort(sizes[i]);
    }

    /* read each compressed channel */
    for (i = 0; i < header.channels; i++)
    {
      bias = i * header.rows;

      switch (i)
      {
        case 0:
          /* red or palette indices */
          offset = header.mode == PSD_RGB ? 2 : 0;
          break;
        case 1:
          /* green */
          offset = 1;
          break;
        case 2:
          /* blue */
          offset = 0;
          break;
        case 3:
          /* red */
          offset = 3;
          break;
        case 4:
          /* alpha */
          offset = 3;
          break;
      }

      /* read each scan line for a given channel */
      for (j = 0; j < header.rows; j++)
      {
        tmp = ReadCompressedChannel(img, offset, dest_channels, file,
                                    sizes[bias + j], j);
        if (tmp != BMG_OK)
          longjmp(err_jmp, (int)tmp);
      }
    }

    memfree(sizes, tmpmem);
  }

  return BMG_OK;
}

static BMGError ReadUncompressedChannel(struct BMGImageStruct *img,
                                        unsigned short offset,
                                        unsigned short channels, IGenLoad &file)
{
  unsigned int i, j;
  for (i = 0; i < img->height; i++)
  {
    unsigned char *p = img->bits + i *img->scan_width + offset;
    for (j = 0; j < img->width; j++)
    {
      if (fread((void*)p, 1, 1, file) != 1)
        return errFileRead;
      p += channels;
    }
  }

  return BMG_OK;
}

static BMGError ReadCompressedChannel(struct BMGImageStruct *img,
                                      unsigned short offset,
                                      unsigned short channels, IGenLoad &file,
                                      unsigned int channel_size,
                                      unsigned int row)
{
  unsigned char *buf = NULL, *p, *q, *end;
  unsigned int byte_cnt = 0;

  jmp_buf err_jmp;
  int error;

  /* error handler */
  error = setjmp(err_jmp);
  if (error != 0)
  {
    if (buf != NULL)
      memfree(buf, tmpmem);
    return (BMGError)error;
  }

  /* allocate enough memory to store the data */
  buf = (unsigned char*)memalloc(img->width, tmpmem);
  if (buf == NULL)
    longjmp(err_jmp, (int)errMemoryAllocation);

  /* decompress an entire channel of data into the buffer */
  p = buf;
  end = buf + img->width;
  while (p < end)
  {
    signed char cnt;
    int count;

    if (fread((void*)& cnt, 1, 1, file) != 1)
      longjmp(err_jmp, (int)errFileRead);
    byte_cnt++;

    /* if cnt >= 0 then we must read cnt + 1 bytes from the files */
    if (cnt >= 0)
    {
      count = (int)cnt + 1;
      if (fread((void*)p, count, 1, file) != 1)
        longjmp(err_jmp, (int)errFileRead);
      p += count;
      byte_cnt += count;
    }
    /* if cnt < 0 the read 1 byte from the file and repeat abs(cnt) + 1 times */
    else
    {
      count = (int)(- cnt) + 1;
      if (fread((void*)& cnt, 1, 1, file) != 1)
        longjmp(err_jmp, (int)errFileRead);
      memset((void*)p, (int)cnt, count);
      byte_cnt++;
      p += count;
    }

    /* hiccup here if we exceed the size of the channel */
    if (byte_cnt > channel_size)
      longjmp(err_jmp, (int)errCorruptFile);
  }

  /* copy the buffer contents into the bits array */
  if (img->bits_per_pixel > 1)
  {
    q = buf;
    p = img->bits + row * img->scan_width;
    end = p + img->width * channels;
    p += offset;
    while (p < end)
    {
      * p = * q;
      p += channels;
      q++;
    }
  }
  else /* I need test cases - if they exist */
  {
    longjmp(err_jmp, (int)errUnsupportedFileFormat);
  }

  memfree(buf, tmpmem);

  /* make sure that the number of bytes read from the file matches the 
  // expected number of bytes (channel_size) */
  return byte_cnt == channel_size ? BMG_OK : errCorruptFile;
}
