//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <debug/dag_assert.h>

class DataBlock;
class String;

// encoded file string format:
//   <file_path>?<meta_data_string>
//
// file_path is ordinary file path, e.g. tex/sample.dds
// meta_data_string is string in format:
//   ({DataSymbol}{DataValues})*
// symbols and values are:
//   A - addressing mode, values {w|c|m|b|o} repeated 1-3 times (e.g. Ac, Acw, Awwc; def=Aw)
//   N - anisotropy, values {i|m|d|a}{INT} (e.g. Ni2, Na8, Nm1; def=Nm1)
//   Q - quality, starting mip, values {INT} repeated 1-3 times separated with '-' for
//       high, medium, low quality  level (e.g. Q0-1-2, Q1, Q0-10; def=Q0-1-2 )
//   P - non-pow-2 flag, values {0|1} (e.g., P0; def=P0)
//   Z - allow-zlib-compression flag, values {0|1} (e.g., Z0; def=Z1)
//   T - optimize flag, values {0|1} (e.g., T0; def=T1)
//   F - filtering mode, values {p|s|b|n} (e.g., Fp; def=)
//   M - mip filtering mode, values {p|l|n} (e.g., Mp; def=)
//   B - border color, {32 bit HEX} (e.g., Bffffffff; def=B00000000)
//   L - lod bias, {INT} x1000 scaled (e.g., L500 means lod bias +0.5; def=L0)
//   D - premultiply color by alpha on image load
//   X - force override file's settings (e.g. in DDSx) with meta data, values {0|1} (e.g., X1; def=X0)
//   U - index of predefined stub texture that can be used in d3d, values {INT} (e.g., U1, U3; def=U0)
//   < - force TEXID_LQ (base quality of texture)
//   > - force TEXID_FQ (full quality of texture)
//   V - scaling factor for IES textures
//   R - rotation on/off for IES textures
//   S - string reference to other texture (for paired textures), always LAST data (def=S)
//
// sample file string: data/tex/sample.dds?AcNa4Q0-2-4Z0


#include <supp/dag_define_KRNLIMP.h>

struct TextureMetaData
{
  //! (logical) texture adderssing modes
  enum
  {
    ADDR_WRAP,       // w
    ADDR_MIRROR,     // m
    ADDR_CLAMP,      // c
    ADDR_BORDER,     // b
    ADDR_MIRRORONCE, // o
  };
  //! anisotropy functions
  enum
  {
    AFUNC_MIN, // i
    AFUNC_MUL, // m
    AFUNC_DIV, // d
    AFUNC_ABS, // a
  };
  //! filtering types
  enum
  {
    FILT_DEF,
    FILT_SMOOTH, // s
    FILT_BEST,   // b
    FILT_NONE,   // n
    FILT_POINT,  // p
    FILT_LINEAR, // l
  };
  //! misc flags
  enum
  {
    FLG_NONPOW2 = (1 << 0),  // P
    FLG_OPTIMIZE = (1 << 1), // T
    FLG_PACK = (1 << 2),     // Z
    FLG_OVERRIDE = (1 << 3), // X
    FLG_PREMUL_A = (1 << 4), // D
    FLG_IES_ROT = (1 << 5),  // R
  };

  uint8_t addrU : 4, addrV : 4, addrW : 4;
  uint8_t hqMip : 4, mqMip : 4, lqMip : 4;
  uint8_t anisoFunc;
  uint16_t anisoFactor;
  uint16_t flags;
  uint32_t borderCol;
  int16_t lodBias;
  uint8_t texFilterMode : 4, mipFilterMode : 4;
  uint8_t stubTexIdx : 4;
  uint8_t forceLQ : 1, forceFQ : 1;
  uint8_t _resv : 2;
  int16_t iesScalingFactor, _resv2 = 0, _resv3 = 0, _resv4 = 0;
  SimpleString baseTexName;

  TextureMetaData() { defaults(); }

  bool isDefault() const;

  void defaults()
  {
    addrU = addrV = addrW = ADDR_WRAP;
    hqMip = 0;
    mqMip = 1;
    lqMip = 2;
    anisoFunc = AFUNC_MUL;
    anisoFactor = 1;
    flags = FLG_OPTIMIZE | FLG_PACK;
    borderCol = 0;
    lodBias = 0;
    texFilterMode = FILT_DEF;
    mipFilterMode = FILT_DEF;
    stubTexIdx = 0;
    forceLQ = forceFQ = 0;
    _resv = 0;
    baseTexName = NULL;
    iesScalingFactor = 0;
  }

  bool isValid() const
  {
#ifndef __GNUC__
    if (addrU < ADDR_WRAP || addrU > ADDR_MIRRORONCE)
      return false;
    if (addrV < ADDR_WRAP || addrV > ADDR_MIRRORONCE)
      return false;
    if (addrW < ADDR_WRAP || addrW > ADDR_MIRRORONCE)
      return false;
#endif
    if (anisoFunc > AFUNC_ABS)
      return false;
    if (anisoFactor > 16)
      return false;
    if (anisoFunc == AFUNC_DIV && anisoFactor == 0)
      return false;
    if (flags & ~(FLG_NONPOW2 | FLG_OPTIMIZE | FLG_PACK | FLG_OVERRIDE | FLG_PREMUL_A | FLG_IES_ROT))
      return false;
    if (texFilterMode == FILT_LINEAR)
      return false;
    if (mipFilterMode == FILT_SMOOTH || mipFilterMode == FILT_BEST)
      return false;
    return true;
  }

  int calcAnisotropy(int base_aniso) const
  {
    switch (anisoFunc)
    {
      case AFUNC_MIN: return anisoFactor < base_aniso ? anisoFactor : base_aniso;
      case AFUNC_MUL: return min(base_aniso * anisoFactor, 16);
      case AFUNC_DIV: return base_aniso / anisoFactor;
      case AFUNC_ABS: return anisoFactor;
    }
    return base_aniso; // should not get here though
  }

  bool needSetBorder() const { return addrU == ADDR_BORDER || addrV == ADDR_BORDER || addrW == ADDR_BORDER; }
  bool needSetTexFilter() const
  {
    return texFilterMode == FILT_SMOOTH || texFilterMode == FILT_BEST || texFilterMode == FILT_NONE || texFilterMode == FILT_POINT;
  }
  bool needSetMipFilter() const { return mipFilterMode == FILT_NONE || mipFilterMode == FILT_POINT || mipFilterMode == FILT_LINEAR; }

  // This multiplier allows iesScale value to be up to 32 while encoding it as 16 bit uint
  static constexpr float IES_SCALE_MUL = float(1 << 11);

  float getIesScale() const
  {
    float scale = static_cast<float>(iesScalingFactor);
    scale /= IES_SCALE_MUL;
    scale += 1.f;
    return scale;
  }

  void setIesScale(float scale)
  {
    scale -= 1.f; // scale of 1 => iesScaleFcator = 0
    G_ASSERT(scale < 32 && scale > -32);
    scale *= IES_SCALE_MUL;
    iesScalingFactor = static_cast<uint16_t>(scale);
  }

  //! encodes and returns file string for given path and current metadata [uses highly temporary buffer when storage==NULL]
  //! returns NULL on invalid metadata
  KRNLIMP const char *encode(const char *fpath, String *storage = NULL) const;

  //! decodes metadata and returns file path for given file string [uses highly temporary buffer when storage==NULL]
  //! returns NULL when decoded invalid metadata
  [[nodiscard("Use decodeData method instead.")]] KRNLIMP const char *decode(const char *fstring, String *storage = NULL);

  //! decodes metadata from given file string; returns false when decoded invalid metadata
  //! allows to avoid String allocations for file path and baseTexName when they are not needed
  KRNLIMP bool decodeData(const char *fstring, bool dec_bt_name = true);

  //! read metadata from DataBlock (single standard for texture asset metadata)
  KRNLIMP void read(const DataBlock &blk, const char *spec_target_str);

  //! write metadata to DataBlock (single standard for texture asset metadata)
  KRNLIMP void write(DataBlock &blk);


  //! returns converts d3d constant for logical tex filtering mode
  KRNLIMP int d3dTexFilter() const;

  //! returns converts d3d constant for logical mip filtering mode
  KRNLIMP int d3dMipFilter() const;


  //! converts logical addressing mode to d3d constant
  KRNLIMP static int d3dTexAddr(unsigned addr);

  //! returns file path for given file string [uses highly temporary buffer when storage==NULL]
  KRNLIMP static const char *decodeFileName(const char *fstring, String *storage = NULL);

  //! returns pointer to baseTex name (inside fstring) or NULL if absent;
  //! baseTex is always encoded last, so it is safe to treat pointer as string
  KRNLIMP static const char *decodeBaseTexName(const char *fstring);
};

#include <supp/dag_undef_KRNLIMP.h>
