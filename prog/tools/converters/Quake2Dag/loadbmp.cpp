// Copyright (C) Gaijin Games KFT.  All rights reserved.

// ############################################################################
// ##                                                                        ##
// ##  LOADBMP.CPP                                                           ##
// ##                                                                        ##
// ##  Utility routines, reads and writes BMP files.  This is *windows*      ##
// ##  specific, the only piece of code that relies on windows and would     ##
// ##  need to be ported for other OS implementations.                       ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################

#include "loadbmp.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

#include "fload.h"

unsigned char *Bmp::LoadBMP(const StdString &fname, int &wid, int &hit, int &bpp)
{
  Fload data(fname);

  unsigned char *mem = (unsigned char *)data.GetData();
  if (!mem)
    return 0;
  return LoadBMP(mem, wid, hit, bpp);
}


unsigned char *Bmp::LoadBMP(const unsigned char *data, int &wid, int &hit, int &Bpp)
{
  // First things first, init working variables
  if (!data)
    return NULL;
  const unsigned char *p = data; // working pointer into DIB
  wid = 0;
  hit = 0;
  Bpp = 0;

  // Note: We declare and use a BITMAPINFOHEADER, even though the header
  //   may be of one of the newer types, because we don't need to know the
  //   actual header size in order to find the bitmap bits.  (The offset
  //   is given by bfOffBits)
  BITMAPFILEHEADER *filehdr;
  BITMAPINFOHEADER *infohdr;

  filehdr = (BITMAPFILEHEADER *)p;
  infohdr = (BITMAPINFOHEADER *)(p + sizeof(BITMAPFILEHEADER));
  char *blah = (char *)(infohdr + sizeof(infohdr));

  if (infohdr->biSize == sizeof(BITMAPCOREHEADER))
  {
    // Old-style OS/2 bitmap header, we don't support it
    return NULL;
  }
  else
  {
    wid = infohdr->biWidth;
    hit = infohdr->biHeight;
    Bpp = (infohdr->biBitCount / 8);
  }

  //  if ( Bpp != 1 && Bpp != 3 ) return NULL;    //We only support 8bit and 24bit files


  // Set pointer to beginning of packed pixel data
  p = data + filehdr->bfOffBits;

  // FIXME: This assumes a non-compressed bitmap (no RLE)
  long siz;
#if 0
  int remainder = wid % 4;
  if (remainder != 0)
    siz = (wid + (4-remainder)) * hit * Bpp;
  else
    siz = wid * hit * Bpp;
#else
  int linesize = ((((Bpp * wid) - 1) / 4) + 1) * 4;
  siz = hit * linesize;
#endif
  unsigned char *mem = new unsigned char[siz];

  assert(mem);

  if (Bpp == 1)
  {

    const unsigned char *base_source = &p[(hit - 1) * wid];
    unsigned char *base_dest = mem;

    for (int y = 0; y < hit; y++)
    {
      unsigned char *dest = base_dest;
      const unsigned char *source = base_source;
      memcpy(dest, source, wid);
      base_dest += (wid);
      base_source -= (wid);
    }
  }
  else
  {
    const unsigned char *base_source = &p[(hit - 1) * wid * 3];
    unsigned char *base_dest = mem;

    for (int y = 0; y < hit; y++)
    {
      unsigned char *dest = base_dest;
      const unsigned char *source = base_source;

      for (int x = 0; x < wid; x++)
      {
        dest[0] = source[2];
        dest[1] = source[1];
        dest[2] = source[0];
        dest += 3;
        source += 3;
      }

      base_dest += (wid * 3);
      base_source -= (wid * 3);
    }
  }
  return mem;
}


void Bmp::SaveBMP(const char *fname, const unsigned char *inputdata, int wid, int hit, int Bpp)
{
  FILE *fph = fopen(fname, "wb");
  if (!fph)
    return;

  unsigned char *data = 0;
  if (Bpp == 1)
  {
    data = (unsigned char *)inputdata;
  }
  else
  {
    const unsigned char *source;
    unsigned char *dest = new unsigned char[wid * hit * 3];
    data = dest; // data to save has flipped RGB order.

    int bwid = wid * 3;

    source = &inputdata[(hit - 1) * bwid];

    for (int y = 0; y < hit; y++)
    {
      memcpy(dest, source, bwid);
      dest += bwid;
      source -= bwid;
    }

    // swap RGB!!
    if (1)
    {
      unsigned char *swap = data;
      int size = wid * hit;
      for (int i = 0; i < size; i++)
      {
        unsigned char c = swap[0];
        swap[0] = swap[2];
        swap[2] = c;
        swap += 3;
      }
    }
  }


  BITMAPFILEHEADER filehdr;
  BITMAPINFOHEADER infohdr;

  DWORD offset = sizeof(filehdr) + sizeof(infohdr);
  if (Bpp == 1)
  {
    // Leave room in file for the color table
    offset += (256 * sizeof(RGBQUAD));
  }

  DWORD sizeImage;
  int remainder = wid % 4;
  int linesize;
  if (remainder != 0)
    linesize = wid + (4 - remainder);
  else
    linesize = wid;
  sizeImage = linesize * hit * Bpp;

  filehdr.bfType = *((WORD *)"BM");
  filehdr.bfSize = offset + sizeImage;
  filehdr.bfReserved1 = 0;
  filehdr.bfReserved2 = 0;
  filehdr.bfOffBits = offset;

  infohdr.biSize = sizeof(infohdr);
  infohdr.biWidth = wid;
  infohdr.biHeight = hit;
  infohdr.biPlanes = 1;
  infohdr.biBitCount = Bpp * 8;
  infohdr.biCompression = BI_RGB;
  infohdr.biSizeImage = sizeImage;
  infohdr.biClrUsed = 0;
  infohdr.biClrImportant = 0;
  infohdr.biXPelsPerMeter = 72;
  infohdr.biYPelsPerMeter = 72;

  unsigned int writtencount = 0;

  fwrite(&filehdr, sizeof(filehdr), 1, fph);

  writtencount += sizeof(filehdr);

  fwrite(&infohdr, sizeof(infohdr), 1, fph);
  writtencount += sizeof(infohdr);

  if (Bpp == 1)
  {
    // Generate a greyscale color table for this image
    RGBQUAD rgbq;
    rgbq.rgbReserved = 0;
    for (int i = 0; i < 256; i++)
    {
      rgbq.rgbBlue = i;
      rgbq.rgbGreen = i;
      rgbq.rgbRed = i;
      fwrite(&rgbq, sizeof(RGBQUAD), 1, fph);
      writtencount += sizeof(RGBQUAD);
    }
  }

  unsigned char *p = data;
  for (unsigned int i = 0; i < sizeImage; i += linesize)
  {
    fwrite((void *)p, linesize, 1, fph);
    p += linesize;
    writtencount += linesize;
  }

  assert(writtencount == filehdr.bfSize);

  if (Bpp == 3)
  {
    delete data;
  }

  fclose(fph);
}


void Bmp::SwapRGB(unsigned char *dest, const unsigned char *src, unsigned int size)
{
  const unsigned char *s = src;
  unsigned char *d = dest;

  for (unsigned int i = 0; i < size; i += 3)
  {
    unsigned char tmp = s[2]; // in case src and dest point to the same place
    d[2] = s[0];
    d[1] = s[1];
    d[0] = tmp;
    d += 3;
    s += 3;
  }
}


void Bmp::SwapVertically(unsigned char *dest, const unsigned char *src, int wid, int hit, int Bpp)
{
  const unsigned char *s = src;

  int rowSize = wid * Bpp;

  unsigned char *d = dest + (rowSize * (hit - 1));

  for (int r = 0; r < hit; ++r)
  {
    memcpy(d, s, rowSize);
    s += rowSize;
    d -= rowSize;
  }
}
