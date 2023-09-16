/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)
 incorprorated into Dagor Engine by Aleksey Volynskov
 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/

#include <osApiWrappers/dag_files.h>
#include "rgbe.h"
#include <debug/dag_debug.h>
#include <ioSys/dag_genIo.h>

#include "loadRgbe.h"


typedef struct
{
  int valid;            /* indicate which fields are valid */
  char programtype[16]; /* listed at beginning of file to identify it
                         * after "#?".  defaults to "RGBE" */
  float gamma;          /* image has already been gamma corrected with
                         * given gamma.  defaults to 1.0 (no correction) */
  float exposure;       /* a value of 1.0 in an image corresponds to
                         * <exposure> watts/steradian/m^2.
                         * defaults to 1.0 */
} rgbe_header_info;

/* flags indicating which fields in an rgbe_header_info are valid */
#define RGBE_VALID_PROGRAMTYPE 0x01
#define RGBE_VALID_GAMMA       0x02
#define RGBE_VALID_EXPOSURE    0x04

/* return codes for rgbe routines */
#define RGBE_RETURN_SUCCESS 0
#define RGBE_RETURN_FAILURE -1

/* read or write headers */
/* you may set rgbe_header_info to null if you want to */
int RGBE_WriteHeader(file_ptr_t fp, int width, int height, rgbe_header_info *info);
int RGBE_ReadHeader(file_ptr_t fp, int *width, int *height, rgbe_header_info *info);

/* read or write pixels */
/* can read or write pixels in chunks of any size including single pixels*/
int RGBE_WritePixels(file_ptr_t fp, float *data, int numpixels);
int RGBE_ReadPixels(file_ptr_t fp, float *data, int numpixels);

/* read or write run length encoded files */
/* must be called to read or write whole scanlines */
int RGBE_WritePixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines);
int RGBE_ReadPixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines);


#include <stdio.h>
#include <ctype.h>

#define INLINE __forceinline

/* offsets to red, green, and blue components in a data (float) pixel */
#define RGBE_DATA_RED   0
#define RGBE_DATA_GREEN 1
#define RGBE_DATA_BLUE  2
/* number of floats per pixel */
#define RGBE_DATA_SIZE  3

enum rgbe_error_codes
{
  rgbe_read_error,
  rgbe_write_error,
  rgbe_format_error,
  rgbe_memory_error,
};

/* default error routine.  change this to change error handling */
static int rgbe_error(int rgbe_error_code, const char *msg)
{
  switch (rgbe_error_code)
  {
    case rgbe_read_error: debug("RGBE read error"); break;
    case rgbe_write_error: debug("RGBE write error"); break;
    case rgbe_format_error: debug("RGBE bad file format: %s", msg); break;
    default:
    case rgbe_memory_error: debug("RGBE error: %s", msg);
  }
  return RGBE_RETURN_FAILURE;
}

/* default minimal header. modify if you want more information in header */
int RGBE_WriteHeader(file_ptr_t fp, int width, int height, rgbe_header_info *info)
{
  const char *programtype = "RGBE";

  if (info && (info->valid & RGBE_VALID_PROGRAMTYPE))
    programtype = info->programtype;
  if (df_printf(fp, "#?%s\n", programtype) < 0)
    return rgbe_error(rgbe_write_error, NULL);
  /* The #? is to identify file type, the programtype is optional. */
  if (info && (info->valid & RGBE_VALID_GAMMA))
  {
    if (df_printf(fp, "GAMMA=%g\n", info->gamma) < 0)
      return rgbe_error(rgbe_write_error, NULL);
  }
  if (info && (info->valid & RGBE_VALID_EXPOSURE))
  {
    if (df_printf(fp, "EXPOSURE=%g\n", info->exposure) < 0)
      return rgbe_error(rgbe_write_error, NULL);
  }
  if (df_printf(fp, "FORMAT=32-bit_rle_rgbe\n\n") < 0)
    return rgbe_error(rgbe_write_error, NULL);
  if (df_printf(fp, "-Y %d +X %d\n", height, width) < 0)
    return rgbe_error(rgbe_write_error, NULL);
  return RGBE_RETURN_SUCCESS;
}

/* minimal header reading.  modify if you want to parse more information */
int RGBE_ReadHeader(file_ptr_t fp, int *width, int *height, rgbe_header_info *info)
{
  char buf[128];
  int found_format;
  float tempf;
  int i;

  found_format = 0;
  if (info)
  {
    info->valid = 0;
    info->programtype[0] = 0;
    info->gamma = info->exposure = 1.0;
  }
  if (df_gets(buf, sizeof(buf) / sizeof(buf[0]), fp) == NULL)
    return rgbe_error(rgbe_read_error, NULL);
  if ((buf[0] != '#') || (buf[1] != '?'))
  {
    /* if you want to require the magic token then uncomment the next line */
    /*return rgbe_error(rgbe_format_error,"bad initial token"); */
  }
  else if (info)
  {
    info->valid |= RGBE_VALID_PROGRAMTYPE;
    for (i = 0; i < sizeof(info->programtype) - 1; i++)
    {
      if ((buf[i + 2] == 0) || isspace(buf[i + 2]))
        break;
      info->programtype[i] = buf[i + 2];
    }
    info->programtype[i] = 0;
    if (df_gets(buf, sizeof(buf) / sizeof(buf[0]), fp) == 0)
      return rgbe_error(rgbe_read_error, NULL);
  }
  bool fmtok = false;
  for (;;)
  {
    if ((buf[0] == 0) || (buf[0] == '\n'))
    {
      if (!fmtok)
        return rgbe_error(rgbe_format_error, "no FORMAT specifier found");
      break;
    }
    else if (strcmp(buf, "FORMAT=32-bit_rle_rgbe\n") == 0)
      fmtok = true;
    else if (info && (sscanf(buf, "GAMMA=%g", &tempf) == 1))
    {
      info->gamma = tempf;
      info->valid |= RGBE_VALID_GAMMA;
    }
    else if (info && (sscanf(buf, "EXPOSURE=%g", &tempf) == 1))
    {
      info->exposure = tempf;
      info->valid |= RGBE_VALID_EXPOSURE;
    }
    if (df_gets(buf, sizeof(buf) / sizeof(buf[0]), fp) == 0)
      return rgbe_error(rgbe_read_error, NULL);
  }
  /*
  if (df_gets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
    return rgbe_error(rgbe_read_error,NULL);
  if (strcmp(buf,"\n") != 0)
    return rgbe_error(rgbe_format_error,
                      "missing blank line after FORMAT specifier");
  */
  if (df_gets(buf, sizeof(buf) / sizeof(buf[0]), fp) == 0)
    return rgbe_error(rgbe_read_error, NULL);
  if (sscanf(buf, "-Y %d +X %d", height, width) < 2)
    return rgbe_error(rgbe_format_error, "missing image size specifier");
  return RGBE_RETURN_SUCCESS;
}

int RGBE_ReadHeader(IGenLoad &crd, int datalen, int *width, int *height, rgbe_header_info *info)
{
  char buf[128];
  int found_format;
  float tempf;
  int i;

  found_format = 0;
  if (info)
  {
    info->valid = 0;
    info->programtype[0] = 0;
    info->gamma = info->exposure = 1.0;
  }
  crd.gets(buf, sizeof(buf) / sizeof(buf[0]));
  if ((buf[0] != '#') || (buf[1] != '?'))
  {
    /* if you want to require the magic token then uncomment the next line */
    /*return rgbe_error(rgbe_format_error,"bad initial token"); */
  }
  else if (info)
  {
    info->valid |= RGBE_VALID_PROGRAMTYPE;
    for (i = 0; i < sizeof(info->programtype) - 1; i++)
    {
      if ((buf[i + 2] == 0) || isspace(buf[i + 2]))
        break;
      info->programtype[i] = buf[i + 2];
    }
    info->programtype[i] = 0;
    crd.gets(buf, sizeof(buf) / sizeof(buf[0]));
  }
  bool fmtok = false;
  for (;;)
  {
    if ((buf[0] == 0) || (buf[0] == '\n'))
    {
      if (!fmtok)
        return rgbe_error(rgbe_format_error, "no FORMAT specifier found");
      break;
    }
    else if (strcmp(buf, "FORMAT=32-bit_rle_rgbe\n") == 0)
      fmtok = true;
    else if (info && (sscanf(buf, "GAMMA=%g", &tempf) == 1))
    {
      info->gamma = tempf;
      info->valid |= RGBE_VALID_GAMMA;
    }
    else if (info && (sscanf(buf, "EXPOSURE=%g", &tempf) == 1))
    {
      info->exposure = tempf;
      info->valid |= RGBE_VALID_EXPOSURE;
    }
    crd.gets(buf, sizeof(buf) / sizeof(buf[0]));
  }
  /*
  if (df_gets(buf,sizeof(buf)/sizeof(buf[0]),fp) == 0)
    return rgbe_error(rgbe_read_error,NULL);
  if (strcmp(buf,"\n") != 0)
    return rgbe_error(rgbe_format_error,
                      "missing blank line after FORMAT specifier");
  */
  crd.gets(buf, sizeof(buf) / sizeof(buf[0]));
  if (sscanf(buf, "-Y %d +X %d", height, width) < 2)
    return rgbe_error(rgbe_format_error, "missing image size specifier");
  return RGBE_RETURN_SUCCESS;
}

/* simple write routine that does not use run length encoding */
/* These routines can be made faster by allocating a larger buffer and
   fread-ing and fwrite-ing the data in larger chunks */
int RGBE_WritePixels(file_ptr_t fp, float *data, int numpixels)
{
  unsigned char rgbe[4];

  while (numpixels-- > 0)
  {
    float2rgbe(rgbe, data[RGBE_DATA_RED], data[RGBE_DATA_GREEN], data[RGBE_DATA_BLUE]);
    data += RGBE_DATA_SIZE;
    if (df_write(fp, rgbe, sizeof(rgbe)) != sizeof(rgbe))
      return rgbe_error(rgbe_write_error, NULL);
  }
  return RGBE_RETURN_SUCCESS;
}

/* simple read routine.  will not correctly handle run length encoding */
int RGBE_ReadPixels(file_ptr_t fp, float *data, int numpixels)
{
  unsigned char rgbe[4];

  while (numpixels-- > 0)
  {
    if (df_read(fp, rgbe, sizeof(rgbe)) != sizeof(rgbe))
      return rgbe_error(rgbe_read_error, NULL);
    rgbe2float(&data[RGBE_DATA_RED], &data[RGBE_DATA_GREEN], &data[RGBE_DATA_BLUE], rgbe);
    data += RGBE_DATA_SIZE;
  }
  return RGBE_RETURN_SUCCESS;
}

int RGBE_ReadPixels(IGenLoad &crd, int datalen, float *data, int numpixels)
{
  unsigned char rgbe[4];

  while (numpixels-- > 0)
  {
    crd.read(rgbe, sizeof(rgbe));
    rgbe2float(&data[RGBE_DATA_RED], &data[RGBE_DATA_GREEN], &data[RGBE_DATA_BLUE], rgbe);
    data += RGBE_DATA_SIZE;
  }
  return RGBE_RETURN_SUCCESS;
}

/* The code below is only needed for the run-length encoded files. */
/* Run length encoding adds considerable complexity but does */
/* save some space.  For each scanline, each channel (r,g,b,e) is */
/* encoded separately for better compression. */

static int RGBE_WriteBytes_RLE(file_ptr_t fp, unsigned char *data, int numbytes)
{
#define MINRUNLENGTH 4
  int cur, beg_run, run_count, old_run_count, nonrun_count;
  unsigned char buf[2];

  cur = 0;
  while (cur < numbytes)
  {
    beg_run = cur;
    /* find next run of length at least 4 if one exists */
    run_count = old_run_count = 0;
    while ((run_count < MINRUNLENGTH) && (beg_run < numbytes))
    {
      beg_run += run_count;
      old_run_count = run_count;
      run_count = 1;
      while ((beg_run + run_count < numbytes) && (run_count < 127) && (data[beg_run] == data[beg_run + run_count]))
        run_count++;
    }
    /* if data before next big run is a short run then write it as such */
    if ((old_run_count > 1) && (old_run_count == beg_run - cur))
    {
      buf[0] = 128 + old_run_count; /*write short run*/
      buf[1] = data[cur];
      if (df_write(fp, buf, sizeof(buf[0]) * 2) != sizeof(buf[0]) * 2)
        return rgbe_error(rgbe_write_error, NULL);
      cur = beg_run;
    }
    /* write out bytes until we reach the start of the next run */
    while (cur < beg_run)
    {
      nonrun_count = beg_run - cur;
      if (nonrun_count > 128)
        nonrun_count = 128;
      buf[0] = nonrun_count;
      if (df_write(fp, buf, sizeof(buf[0])) != sizeof(buf[0]))
        return rgbe_error(rgbe_write_error, NULL);
      if (df_write(fp, &data[cur], sizeof(data[0]) * nonrun_count) != sizeof(data[0]) * nonrun_count)
        return rgbe_error(rgbe_write_error, NULL);
      cur += nonrun_count;
    }
    /* write out next run if one was found */
    if (run_count >= MINRUNLENGTH)
    {
      buf[0] = 128 + run_count;
      buf[1] = data[beg_run];
      if (df_write(fp, buf, sizeof(buf[0]) * 2) != sizeof(buf[0]) * 2)
        return rgbe_error(rgbe_write_error, NULL);
      cur += run_count;
    }
  }
  return RGBE_RETURN_SUCCESS;
#undef MINRUNLENGTH
}

int RGBE_WritePixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines)
{
  unsigned char rgbe[4];
  unsigned char *buffer;
  int i, err;

  if ((scanline_width < 8) || (scanline_width > 0x7fff))
    /* run length encoding is not allowed so write flat*/
    return RGBE_WritePixels(fp, data, scanline_width * num_scanlines);
  buffer = (unsigned char *)memalloc(sizeof(unsigned char) * 4 * scanline_width, tmpmem);
  if (buffer == NULL)
    /* no buffer space so write flat */
    return RGBE_WritePixels(fp, data, scanline_width * num_scanlines);
  while (num_scanlines-- > 0)
  {
    rgbe[0] = 2;
    rgbe[1] = 2;
    rgbe[2] = scanline_width >> 8;
    rgbe[3] = scanline_width & 0xFF;
    if (df_write(fp, rgbe, sizeof(rgbe)) != sizeof(rgbe))
    {
      memfree(buffer, tmpmem);
      return rgbe_error(rgbe_write_error, NULL);
    }
    for (i = 0; i < scanline_width; i++)
    {
      float2rgbe(rgbe, data[RGBE_DATA_RED], data[RGBE_DATA_GREEN], data[RGBE_DATA_BLUE]);
      buffer[i] = rgbe[0];
      buffer[i + scanline_width] = rgbe[1];
      buffer[i + 2 * scanline_width] = rgbe[2];
      buffer[i + 3 * scanline_width] = rgbe[3];
      data += RGBE_DATA_SIZE;
    }
    /* write out each of the four channels separately run length encoded */
    /* first red, then green, then blue, then exponent */
    for (i = 0; i < 4; i++)
    {
      if ((err = RGBE_WriteBytes_RLE(fp, &buffer[i * scanline_width], scanline_width)) != RGBE_RETURN_SUCCESS)
      {
        memfree(buffer, tmpmem);
        return err;
      }
    }
  }
  memfree(buffer, tmpmem);
  return RGBE_RETURN_SUCCESS;
}

int RGBE_ReadPixels_RLE(file_ptr_t fp, float *data, int scanline_width, int num_scanlines)
{
  unsigned char rgbe[4], *scanline_buffer, *ptr, *ptr_end;
  int i, count;
  unsigned char buf[2];

  if ((scanline_width < 8) || (scanline_width > 0x7fff))
    /* run length encoding is not allowed so read flat*/
    return RGBE_ReadPixels(fp, data, scanline_width * num_scanlines);
  scanline_buffer = NULL;
  /* read in each successive scanline */
  while (num_scanlines > 0)
  {
    if (df_read(fp, rgbe, sizeof(rgbe)) != sizeof(rgbe))
    {
      memfree(scanline_buffer, tmpmem);
      return rgbe_error(rgbe_read_error, NULL);
    }
    if ((rgbe[0] != 2) || (rgbe[1] != 2) || (rgbe[2] & 0x80))
    {
      /* this file is not run length encoded */
      rgbe2float(&data[0], &data[1], &data[2], rgbe);
      data += RGBE_DATA_SIZE;
      memfree(scanline_buffer, tmpmem);
      return RGBE_ReadPixels(fp, data, scanline_width * num_scanlines - 1);
    }
    if ((((int)rgbe[2]) << 8 | rgbe[3]) != scanline_width)
    {
      memfree(scanline_buffer, tmpmem);
      return rgbe_error(rgbe_format_error, "wrong scanline width");
    }
    if (scanline_buffer == NULL)
      scanline_buffer = (unsigned char *)memalloc(sizeof(unsigned char) * 4 * scanline_width, tmpmem);
    if (scanline_buffer == NULL)
      return rgbe_error(rgbe_memory_error, "unable to allocate buffer space");

    ptr = &scanline_buffer[0];
    /* read each of the four channels for the scanline into the buffer */
    for (i = 0; i < 4; i++)
    {
      ptr_end = &scanline_buffer[(i + 1) * scanline_width];
      while (ptr < ptr_end)
      {
        if (df_read(fp, buf, sizeof(buf[0]) * 2) != sizeof(buf[0]) * 2)
        {
          memfree(scanline_buffer, tmpmem);
          return rgbe_error(rgbe_read_error, NULL);
        }
        if (buf[0] > 128)
        {
          /* a run of the same value */
          count = buf[0] - 128;
          if (count > ptr_end - ptr)
          {
            memfree(scanline_buffer, tmpmem);
            return rgbe_error(rgbe_format_error, "bad scanline data");
          }
          while (count-- > 0)
            *ptr++ = buf[1];
        }
        else
        {
          /* a non-run */
          count = buf[0];
          if ((count == 0) || (count > ptr_end - ptr))
          {
            memfree(scanline_buffer, tmpmem);
            return rgbe_error(rgbe_format_error, "bad scanline data");
          }
          *ptr++ = buf[1];
          if (--count > 0)
          {
            if (df_read(fp, ptr, sizeof(*ptr) * count) != sizeof(*ptr) * count)
            {
              memfree(scanline_buffer, tmpmem);
              return rgbe_error(rgbe_read_error, NULL);
            }
            ptr += count;
          }
        }
      }
    }
    /* now convert data from buffer into floats */
    for (i = 0; i < scanline_width; i++)
    {
      rgbe[0] = scanline_buffer[i];
      rgbe[1] = scanline_buffer[i + scanline_width];
      rgbe[2] = scanline_buffer[i + 2 * scanline_width];
      rgbe[3] = scanline_buffer[i + 3 * scanline_width];
      rgbe2float(&data[RGBE_DATA_RED], &data[RGBE_DATA_GREEN], &data[RGBE_DATA_BLUE], rgbe);
      data += RGBE_DATA_SIZE;
    }
    num_scanlines--;
  }
  memfree(scanline_buffer, tmpmem);
  return RGBE_RETURN_SUCCESS;
}


int RGBE_ReadPixels_RLE(IGenLoad &crd, int datalen, float *data, int scanline_width, int num_scanlines)
{
  unsigned char rgbe[4], *scanline_buffer, *ptr, *ptr_end;
  int i, count;
  unsigned char buf[2];

  if ((scanline_width < 8) || (scanline_width > 0x7fff))
    /* run length encoding is not allowed so read flat*/
    return RGBE_ReadPixels(crd, datalen, data, scanline_width * num_scanlines);
  scanline_buffer = NULL;
  /* read in each successive scanline */
  while (num_scanlines > 0)
  {
    crd.read(rgbe, sizeof(rgbe));
    if ((rgbe[0] != 2) || (rgbe[1] != 2) || (rgbe[2] & 0x80))
    {
      /* this file is not run length encoded */
      rgbe2float(&data[0], &data[1], &data[2], rgbe);
      data += RGBE_DATA_SIZE;
      memfree(scanline_buffer, tmpmem);
      return RGBE_ReadPixels(crd, datalen, data, scanline_width * num_scanlines - 1);
    }
    if ((((int)rgbe[2]) << 8 | rgbe[3]) != scanline_width)
    {
      memfree(scanline_buffer, tmpmem);
      return rgbe_error(rgbe_format_error, "wrong scanline width");
    }
    if (scanline_buffer == NULL)
      scanline_buffer = (unsigned char *)memalloc(sizeof(unsigned char) * 4 * scanline_width, tmpmem);
    if (scanline_buffer == NULL)
      return rgbe_error(rgbe_memory_error, "unable to allocate buffer space");

    ptr = &scanline_buffer[0];
    /* read each of the four channels for the scanline into the buffer */
    for (i = 0; i < 4; i++)
    {
      ptr_end = &scanline_buffer[(i + 1) * scanline_width];
      while (ptr < ptr_end)
      {
        crd.read(buf, sizeof(buf[0]) * 2);
        if (buf[0] > 128)
        {
          /* a run of the same value */
          count = buf[0] - 128;
          if (count > ptr_end - ptr)
          {
            memfree(scanline_buffer, tmpmem);
            return rgbe_error(rgbe_format_error, "bad scanline data");
          }
          while (count-- > 0)
            *ptr++ = buf[1];
        }
        else
        {
          /* a non-run */
          count = buf[0];
          if ((count == 0) || (count > ptr_end - ptr))
          {
            memfree(scanline_buffer, tmpmem);
            return rgbe_error(rgbe_format_error, "bad scanline data");
          }
          *ptr++ = buf[1];
          if (--count > 0)
          {
            crd.read(ptr, sizeof(*ptr) * count);
            ptr += count;
          }
        }
      }
    }
    /* now convert data from buffer into floats */
    for (i = 0; i < scanline_width; i++)
    {
      rgbe[0] = scanline_buffer[i];
      rgbe[1] = scanline_buffer[i + scanline_width];
      rgbe[2] = scanline_buffer[i + 2 * scanline_width];
      rgbe[3] = scanline_buffer[i + 3 * scanline_width];
      rgbe2float(&data[RGBE_DATA_RED], &data[RGBE_DATA_GREEN], &data[RGBE_DATA_BLUE], rgbe);
      data += RGBE_DATA_SIZE;
    }
    num_scanlines--;
  }
  memfree(scanline_buffer, tmpmem);
  return RGBE_RETURN_SUCCESS;
}


TexImageF *load_rgbe(const char *fn, IMemAlloc *mem, HDRImageInfo *ii)
{
  file_ptr_t fh = df_open((char *)fn, DF_READ);
  if (!fh)
    return NULL;
  rgbe_header_info hdr;
  int w, h;
  if (RGBE_ReadHeader(fh, &w, &h, &hdr) == RGBE_RETURN_FAILURE)
  {
    df_close(fh);
    return NULL;
  }
  TexImageF *im = (TexImageF *)memalloc(sizeof(Color3) * w * h + sizeof(TexImageF), mem);
  if (!im)
  {
    df_close(fh);
    return NULL;
  }
  im->w = w;
  im->h = h;
  if (RGBE_ReadPixels_RLE(fh, (float *)im->getPixels(), w, h) == RGBE_RETURN_FAILURE)
  {
    memfree(im, mem);
    df_close(fh);
    return NULL;
  }
  df_close(fh);
  if (ii)
  {
    if (hdr.valid & RGBE_VALID_EXPOSURE)
      ii->exposure = hdr.exposure;
    else
      ii->exposure = 1;
    if (hdr.valid & RGBE_VALID_GAMMA)
      ii->gamma = hdr.gamma;
    else
      ii->gamma = 1;
  }
  return im;
}

TexImageF *load_rgbe(IGenLoad &crd, int datalen, IMemAlloc *mem, HDRImageInfo *ii)
{
  int startPos = crd.tell();
  rgbe_header_info hdr;
  int w, h;
  if (RGBE_ReadHeader(crd, datalen, &w, &h, &hdr) == RGBE_RETURN_FAILURE)
  {
    crd.seekto(startPos + datalen);
    return NULL;
  }
  TexImageF *im = (TexImageF *)memalloc(sizeof(Color3) * w * h + sizeof(TexImageF), mem);
  if (!im)
  {
    crd.seekto(startPos + datalen);
    return NULL;
  }
  im->w = w;
  im->h = h;
  if (RGBE_ReadPixels_RLE(crd, datalen, (float *)im->getPixels(), w, h) == RGBE_RETURN_FAILURE)
  {
    memfree(im, mem);
    crd.seekto(startPos + datalen);
    return NULL;
  }
  if (ii)
  {
    if (hdr.valid & RGBE_VALID_EXPOSURE)
      ii->exposure = hdr.exposure;
    else
      ii->exposure = 1;
    if (hdr.valid & RGBE_VALID_GAMMA)
      ii->gamma = hdr.gamma;
    else
      ii->gamma = 1;
  }

  int stopPos = crd.tell();
  crd.seekto(startPos + datalen);

  return im;
}
