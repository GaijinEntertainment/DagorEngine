// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_color.h>
#include <libTools/dtx/dtxHeader.h>

//==============================================================================

class DTXTextureSampler
{
public:
  DTXTextureSampler();
  ~DTXTextureSampler();

  bool construct(unsigned char *data, size_t size);
  bool isValid() const { return data_ != NULL; }
  void destroy();

  bool isRGBETexture() const;
  bool isFloatTexture() const;

  E3DCOLOR e3dcolorValueAt(float u, float v) const;
  Color4 floatValueAt(float u, float v) const;
  E3DCOLOR e3dcolorValueAt(int u, int v) const;
  Color4 floatValueAt(int u, int v) const;
  void applyAddressing(float &u, float &v) const;
  void applyAddressing(int &u, int &v) const;

  unsigned width() const { return width_; }
  unsigned height() const { return height_; }

  enum Format
  {
    FMT_RGBA,
    FMT_DXT1,
    FMT_DXT3,
    FMT_DXT5,
    FMT_A32B32G32R32F,
    FMT_RGBE,
    FMT_RGB_OTHERS,
    FMT_A16B16G16R16,
    FMT_A16B16G16R16F
  };

  bool ownData_;
  unsigned char *data_;
  size_t dataSize;

  Format format_;
  unsigned int width_;
  unsigned int height_;
  long pitch_;

  struct
  {
    struct
    {
      int r, g, b, a;
    } ofs; // for >> to base
    struct
    {
      int r, g, b, a;
    } shift; // for << to have 8 bit value
    int bpp; // byte per pixel
  } rgbOther;

private:
  int addr_mode_u, addr_mode_v;
};


//------------------------------------------------------------------------------

inline bool DTXTextureSampler::isFloatTexture() const { return (format_ == FMT_A32B32G32R32F); }


inline bool DTXTextureSampler::isRGBETexture() const { return (format_ == FMT_RGBE); }
