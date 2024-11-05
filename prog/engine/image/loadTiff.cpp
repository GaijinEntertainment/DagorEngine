// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_tiff.h>
#include <image/dag_texPixel.h>
#include <image/tiff-4.4.0/tiff.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <ioSys/dag_fileIo.h>
#include <EASTL/utility.h>

static tsize_t tiff_ReadProc(thandle_t fp, void *buf, tsize_t size) { return reinterpret_cast<IGenLoad *>(fp)->tryRead(buf, size); }
static tsize_t tiff_WriteProc(thandle_t fp, void *buf, tsize_t size) { return -1; }
static toff_t tiff_SeekProc(thandle_t fp, toff_t off, int whence)
{
  auto &crd = *reinterpret_cast<IGenLoad *>(fp);
  switch (whence)
  {
    case SEEK_CUR: crd.seekrel(off); return crd.tell();
    case SEEK_SET: crd.seekto(off); return crd.tell();
    case SEEK_END: crd.seekto(crd.getTargetDataSize() + off); return crd.tell();
  }
  return -1;
}
static toff_t tiff_SizeProc(thandle_t fp) { return reinterpret_cast<IGenLoad *>(fp)->getTargetDataSize(); }
static int tiff_CloseProc(thandle_t fp) { return 0; }
static int tiff_nullMapProc(thandle_t, void **, toff_t *) { return 0; }
static void tiff_nullUnmapProc(thandle_t, void *, toff_t) {}


TexImage32 *load_tiff32(const char *fn, IMemAlloc *mem, bool *out_used_alpha)
{
  FullFileLoadCB crd(fn);
  if (crd.fileHandle)
    return load_tiff32(crd, mem, out_used_alpha);
  return NULL;
}
TexImage32 *load_tiff32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha)
{
  uint32_t width, height;
  uint16_t bps, spp;
  TIFF *tif = TIFFClientOpen(crd.getTargetName(), "r", reinterpret_cast<thandle_t>(&crd), &tiff_ReadProc, &tiff_WriteProc,
    &tiff_SeekProc, &tiff_CloseProc, &tiff_SizeProc, &tiff_nullMapProc, &tiff_nullUnmapProc);

  if (!tif)
  {
    logerr("%s: could not open image", crd.getTargetName());
    return nullptr;
  }

  // Do whatever it is we do with the buffer -- we dump it in hex
  if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) == 0)
  {
    logerr("%s: no image width", crd.getTargetName());
    TIFFClose(tif);
    return nullptr;
  }
  if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) == 0)
  {
    logerr("%s: no image height", crd.getTargetName());
    TIFFClose(tif);
    return nullptr;
  }

  // Check that it is of a type that we support
  if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps) == 0)
  {
    logerr("%s: undefined bits-per-sample", crd.getTargetName());
    TIFFClose(tif);
    return nullptr;
  }
  if (bps != 1 && bps != 2 && bps != 4 && bps != 8 && bps != 16 && bps != 32)
  {
    logerr("%s: unsupported bits-per-sample: %d", crd.getTargetName(), bps);
    TIFFClose(tif);
    return nullptr;
  }

  if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0)
  {
    logerr("%s: undefined samples-per-pixel", crd.getTargetName());
    TIFFClose(tif);
    return nullptr;
  }

  if (spp != 1 && spp != 2 && spp != 3 && spp != 4)
  {
    logerr("%s: unsupported samples-per-pixel: %d", crd.getTargetName(), spp);
    TIFFClose(tif);
    return nullptr;
  }

  if (spp <= 4 && bps <= 8) //-V560
  {
    TexImage32 *im32 = TexImage32::create(width, height, mem);

    // set extra channels to EXTRASAMPLE_UNSPECIFIED to treat alpha as ordinary (non-premultiplied) channel
    uint16_t *sampleinfo, extrasamples = 0, new_sampleinfo[16];
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
    for (int i = 0; i < spp - 3; i++) //-V1008
      new_sampleinfo[i] = EXTRASAMPLE_UNSPECIFIED;
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extrasamples, new_sampleinfo);

    if (!TIFFReadRGBAImageOriented(tif, width, height, (unsigned *)im32->getPixels(), ORIENTATION_TOPLEFT, 0))
    {
      mem->free(im32);
      TIFFClose(tif);
      return nullptr;
    }
    TIFFClose(tif);

    for (TexPixel32 *p = im32->getPixels(), *pe = p + width * height; p < pe; p++)
      eastl::swap(p->b, p->r); // TexPixel32 is BGRA while TIFFRGBAImage is RGBA

    if (out_used_alpha)
    {
      *out_used_alpha = false;
      if (spp >= 4)
        for (TexPixel32 *p = im32->getPixels(), *pe = p + width * height; p < pe; p++)
          if (p->a != 255)
          {
            *out_used_alpha = true;
            break;
          }
    }
    return im32;
  }
  logerr("%s: unsupported samples-per-pixel=%d and bits-per-sample=%d", crd.getTargetName(), spp, bps);
  return nullptr;
}
