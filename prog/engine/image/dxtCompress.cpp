// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stdint.h>
#include <stdio.h>

#if USE_SQUISHED
#include <squish/squish.h>
#else
#include <rygDXT.h>
#endif

#include <fastDXT.h>

#ifdef __GNUC__
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef void *LPVOID;
#endif

#include <math/dag_mathBase.h>
#include <3d/ddsFormat.h>
#include <3d/ddsxTex.h>
#include <image/dag_dxtCompress.h>
#include <image/dag_texPixel.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zlibIo.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <stdlib.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_adjpow2.h>

#define CV4(v) v.r, v.g, v.b, v.a
#define INLINE __forceinline

#define debug(...) logmessage(_MAKE4C('DXTC'), __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

static inline void AddColor(int *pSum, char *pAdd)
{
  for (int i = 0; i < 4; i++)
    pSum[i] += pAdd[i];
}

void GenNextLevelMipMap(TexPixel32 *pSrc, int srcW, int srcH, TexPixel32 *pDst)
{
  int dstW = srcW > 4 ? srcW / 2 : 4;
  int dstH = srcH > 4 ? srcH / 2 : 4;
  uint8_t *dst = reinterpret_cast<uint8_t *>(pDst);

  int iSumColor[4];
  for (int dstX = 0; dstX < dstW; dstX++)
  {
    for (int dstY = 0; dstY < dstH; dstY++)
    {
      memset(iSumColor, 0, 4 * sizeof(int));

      AddColor(iSumColor, reinterpret_cast<char *>(&pSrc[(dstY * srcH / dstH) * srcW + dstX * srcW / dstW]));
      AddColor(iSumColor, reinterpret_cast<char *>(&pSrc[((dstY * srcH + 1) / dstH) * srcW + dstX * srcW / dstW]));
      AddColor(iSumColor, reinterpret_cast<char *>(&pSrc[(dstY * srcH / dstH) * srcW + (dstX * srcW + 1) / dstW]));
      AddColor(iSumColor, reinterpret_cast<char *>(&pSrc[((dstY * srcH + 1) / dstH) * srcW + (dstX * srcW + 1) / dstW]));

      uint8_t *p = dst + (dstY * dstW + dstX) * 4;
      p[0] = iSumColor[0] / 4;
      p[1] = iSumColor[1] / 4;
      p[2] = iSumColor[2] / 4;
      p[3] = iSumColor[3] / 4;
    }
  }
}

void ManualDXT(int mode, const TexPixel32 *pImage, int iWidth, int iHeight, int dxt_pitch, char *pCompressed, int algorithm)
{
  if (algorithm != DXT_ALGORITHM_QUICK && mode != MODE_DXT5Alpha)
  {
    int algo = algorithm == DXT_ALGORITHM_PRECISE
                 ? rygDXT::STB_DXT_FAST
                 : (algorithm == DXT_ALGORITHM_PRODUCTION ? rygDXT::STB_DXT_NORMAL : rygDXT::STB_DXT_HIGHQUAL);
    if (mode == MODE_DXT1)
      rygDXT::CompressImageDXT1(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, algo, dxt_pitch);
    else if (mode == MODE_DXT5)
      rygDXT::CompressImageDXT5(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, algo, dxt_pitch);
    else
      rygDXT::CompressImageDXT3(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, algo, dxt_pitch);
    return;
  }
  G_ASSERT(is_pow_of2(iWidth) && is_pow_of2(iHeight));
  switch (mode)
  {
    case MODE_DXT1:
      fastDXT::CompressImageDXT1(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, dxt_pitch);
      break;
    case MODE_DXT5:
      fastDXT::CompressImageDXT5(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, dxt_pitch);
      break;
    case MODE_DXT5Alpha:
      fastDXT::CompressImageDXT5Alpha(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, dxt_pitch);
      break;
    case MODE_DXT3:
      rygDXT::CompressImageDXT3(reinterpret_cast<const unsigned char *>(pImage), reinterpret_cast<unsigned char *>(pCompressed),
        iWidth, iHeight, rygDXT::STB_DXT_NORMAL, dxt_pitch);
      break;
  }
}

void CompressBC4(const unsigned char *image, int width, int height, int dxt_pitch, char *pCompressed, int pixel_stride,
  int pixel_offset)
{
  fastDXT::CompressImageBC4(image, reinterpret_cast<unsigned char *>(pCompressed), width, height, dxt_pitch, pixel_stride,
    pixel_offset);
}

void CompressBC5(unsigned char *image, int width, int height, int dxt_pitch, char *pCompressed, int pixel_stride, int pixel_offset)
{
  fastDXT::CompressImageBC5(image, reinterpret_cast<unsigned char *>(pCompressed), width, height, dxt_pitch, pixel_stride,
    pixel_offset);
}

void *CompressDXT(int mode, TexPixel32 *image, int /*stride_bytes*/, int width, int height, int levels, int *len, int /*algorithm*/,
  int zlib_lev)
{
  __int64 t0Total = ref_time_ticks_qpc();
  if (!is_pow_of2(width) || !is_pow_of2(height))
    DAG_FATAL("Texture for DXT compression sizes must be powers of 2, not %d x %d", width, height);
  // The size is same for both compression types
  // int iCompressedSize = width*height/16*(64+64)/8;

  int compressedW = width > 4 ? width : 4;
  int compressedH = height > 4 ? height : 4;
  int iCompressedSize = (mode == MODE_DXT1 ? compressedW * compressedH / 2 : compressedW * compressedH);

  G_ASSERT(levels >= 0);

  if (!levels)
  {
    int iSize = max(width, height);

    while (iSize > 0)
    {
      levels++;
      iSize >>= 1;
    }
    debug("Levels count for %dx%d = %d", width, height, levels);
  }

  int iCurW = width;
  int iCurH = height;
  for (int iL = 1; iL < levels; iL++)
  {
    int mipW = iCurW > 1 ? iCurW / 2 : iCurW;
    int mipH = iCurH > 1 ? iCurH / 2 : iCurH;
    int compressedW = mipW > 4 ? mipW : 4;
    int compressedH = mipH > 4 ? mipH : 4;

    iCompressedSize += (mode == MODE_DXT1 ? compressedW * compressedH / 2 : compressedW * compressedH);

    iCurW = mipW;
    iCurH = mipH;
  }

  // Total data length
  int iResultLen = sizeof(ddsx::Header) + iCompressedSize;

  // Compressed data
  char *pData = reinterpret_cast<char *>(memalloc(iResultLen, tmpmem));

  if (!pData)
    return NULL;

  //--- Fill in DDSx header ------------------
  int dataStartOffs = sizeof(ddsx::Header);

  ddsx::Header *hdr = reinterpret_cast<ddsx::Header *>(pData);
  memset(hdr, 0, sizeof(ddsx::Header));
  hdr->label = _MAKE4C('DDSx');
  hdr->flags = ddsx::Header::FLG_CONTIGUOUS_MIP | 0x11;
  hdr->w = width;
  hdr->h = height;
  hdr->levels = levels;
  hdr->bitsPerPixel = 0;
  hdr->depth = 1;
  hdr->memSz = iCompressedSize;
  hdr->packedSz = 0;

  if (mode == MODE_DXT1)
    hdr->d3dFormat = _MAKE4C('DXT1'), hdr->dxtShift = 3;
  else if (mode == MODE_DXT3)
    hdr->d3dFormat = _MAKE4C('DXT3'), hdr->dxtShift = 4;
  else if (mode == MODE_DXT5)
    hdr->d3dFormat = _MAKE4C('DXT5'), hdr->dxtShift = 4;
  else
    DAG_FATAL("Unsupported mode");
  hdr->lQmip = 0;
  hdr->mQmip = 0;
  hdr->uQmip = 0;

  //--- Process -----------------------------

  __int64 t0 = ref_time_ticks_qpc();

  int pitch = (mode == MODE_DXT1 ? width * 2 : width * 4);
  ManualDXT(mode, image, width, height, pitch, pData + dataStartOffs);
  debug("Manual DXT for %dx%d (level 0): %d usec", width, height, get_time_usec_qpc(t0));

  t0 = ref_time_ticks_qpc();

  // Mipmaps and their compression -----------------------------------------------
  if (levels > 1)
  {
    TexPixel32 *pMipMapBuf = reinterpret_cast<TexPixel32 *>(memalloc(width * height * 4, tmpmem));

    int iCurW = width;
    int iCurH = height;

    TexPixel32 *currentUncompressed = pMipMapBuf;
    char *currentCompressed = pData + dataStartOffs + (mode == MODE_DXT1 ? width * height / 2 : width * height);

    TexPixel32 *pPrevTex = image;

    for (int iLevel = 1; iLevel < levels; iLevel++)
    {
      int mipW = iCurW > 1 ? iCurW / 2 : iCurW;
      int mipH = iCurH > 1 ? iCurH / 2 : iCurH;
      int compressedW = mipW > 4 ? mipW : 4;
      int compressedH = mipH > 4 ? mipH : 4;

      // debug("building mimpap level %d", iLevel);

      GenNextLevelMipMap(pPrevTex, iCurW, iCurH, currentUncompressed);

      //      extern int save_tga32(const char *fn,TexPixel32 *ptr,int wd,int ht,int stride);
      //      save_tga32(String(64, "mip%02d.tga", iLevel), pMipMapBuf+iMipMapBufPos, iCurW/2, iCurH/2, iCurW/2*4);

      int pitch = (mode == MODE_DXT1 ? mipW * 2 : mipW * 4);
      ManualDXT(mode, currentUncompressed, mipW, mipH, pitch, currentCompressed);

      // debug("Compressing image=%X to buffer=%X", pMipMapBuf+iMipMapBufPos, pCompressedMipMaps);

      // Setup next level sizes
      // Save current texture as base for next level
      pPrevTex = currentUncompressed;
      currentUncompressed += mipW * mipH;
      currentCompressed += (mode == MODE_DXT1 ? compressedW * compressedH / 2 : compressedW * compressedH);

      iCurW = mipW;
      iCurH = mipH;
    }

    memfree(pMipMapBuf, tmpmem);
  }

  debug("Manual DXT for %dx%d (%d mipmap levels): %d usec", width, height, levels - 1, get_time_usec_qpc(t0));

  if (zlib_lev)
  {
    MemorySaveCB mcwr(128 << 10);

    ZlibSaveCB z_cwr(mcwr, ZlibSaveCB::CL_BestSpeed + zlib_lev);
    z_cwr.write(hdr + 1, hdr->memSz);
    z_cwr.finish();
    if (mcwr.getSize() < hdr->memSz)
    {
      hdr->flags |= ddsx::Header::FLG_ZLIB;
      hdr->packedSz = mcwr.getSize();
      MemoryLoadCB crd(mcwr.getMem(), false);
      crd.read(hdr + 1, hdr->packedSz);
      debug("zlib_lev=%d, packRatio=%.1f : 1", zlib_lev, float(hdr->memSz) / hdr->packedSz);
    }
  }

  if (hdr->packedSz)
  {
    *len = hdr->packedSz + sizeof(ddsx::Header);
    pData = reinterpret_cast<char *>(tmpmem->realloc(pData, *len));
  }
  else
    *len = hdr->memSz + sizeof(ddsx::Header);

  debug("Total compression routine working time is %d usec", get_time_usec_qpc(t0Total));

  return pData;
}
