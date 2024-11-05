// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h> // for TEXADDR_*
#include <util/dag_string.h>


class IGenSave;


//------------------------------------------------------------------------------
namespace ddstexture
{
class Converter
{
public:
  enum Format
  {
    fmtCurrent = 0,
    fmtARGB,
    fmtRGB,
    fmtDXT1,
    fmtDXT1a,
    fmtDXT3,
    fmtDXT5,
    fmtA8L8,
    fmtL8,
    fmtASTC4,
    fmtASTC8,
    fmtRGBA4444,
    fmtRGBA5551,
    fmtRGB565,
  };

  enum TextureType
  {
    type2D = 0,
    typeCube,
    typeVolume,
    typeDefault,
  };

  enum MipmapType
  {
    mipmapNone = 0,
    mipmapUseExisting,
    mipmapGenerate
  };

  enum MipmapFilter
  {
    filterPoint = 0,
    filterBox,
    filterTriangle,
    filterQuadratic,
    filterCubic,

    filterCatrom,
    filterMitchell,

    filterGaussian,
    filterSinc,
    filterBessel,

    filterHanning,
    filterHamming,
    filterBlackman,
    filterKaiser
  };
  enum QualityType
  {
    QualityFastest,
    QualityNormal,
    QualityProduction,
    QualityHighest
  };

  inline Converter() :
    type(type2D),
    format(fmtDXT1),
    volTexDepth(0),
    mipmapType(mipmapUseExisting),
    mipmapCount(AllMipMaps),
    mipmapFilter(filterKaiser),
    mipmapFilterWidth(0.0f),
    mipmapFilterBlur(1.0f),
    mipmapFilterGamma(2.2f), // default from nvdxt
    quality(QualityProduction)
  {}

  bool convert(const char *src_filename, const char *dst_filename, bool overwrite = true, bool make_pow_2 = false,
    int pow2_strategy = 1);

  //! convert image, optionally rescaling it to pow-of-2 (strategy: -1=SmallestPower2, 0=NearestPower2, 1=BiggestPower2, 2=no-rescale)
  bool convertImage(IGenSave &cb, TexPixel32 *pixels, int width, int height, int stride_bytes, bool make_pow_2 = false,
    int pow2_strategy = 1);
  bool convertImageFast(IGenSave &cb, TexPixel32 *pixels, int width, int height, int stride_bytes, bool make_pow_2 = false,
    int pow2_strategy = 1);

  static const char *getLastError();

  static const unsigned AllMipMaps = 100;

  TextureType type;
  Format format;
  //! volTexDepth>0 means depth=volTexDepth and vertical strip of slices in source image
  //! volTexDepth<0 means depth=-volTexDepth and horizontal strip of slices in source image
  //! volTexDepth=0 (default) is not allowed when converting to typeVolume
  int volTexDepth;

  MipmapType mipmapType;
  unsigned mipmapCount;
  MipmapFilter mipmapFilter;
  float mipmapFilterWidth; // 0.0f means default
  float mipmapFilterBlur;
  float mipmapFilterGamma;
  QualityType quality;
  static bool copyTosrcLocation;

protected:
  void fillCompressionOptions(void *dest_nv_options);
};


bool getDdsInfo(const char *filename, int *src_type, int *src_format, int *src_mipmaps, int *width = NULL, int *height = NULL);
} // namespace ddstexture

// FIX: need move from dtx..
bool tgaWithAlpha(const char *filename);
