#pragma once


#include <util/dag_stdint.h>
#include <util/dag_baseDef.h>

namespace ddsx
{
inline unsigned max(unsigned a, unsigned b) { return a > b ? a : b; }

struct Header
{
  uint32_t label;     // 'DDSx'
  uint32_t d3dFormat; // D3DFMT_xxx

  uint32_t flags;
  uint16_t w, h;

  uint8_t levels, hqPartLevels;
  uint16_t depth;
  uint16_t bitsPerPixel;

  uint8_t lQmip : 4, mQmip : 4;
  uint8_t dxtShift : 4, uQmip : 4;

  uint32_t memSz, packedSz;

  enum
  {
    FLG_CONTIGUOUS_MIP = 0x0100,
    FLG_NONPACKED = 0x0200,
    FLG_HASBORDER = 0x0400,
    FLG_CUBTEX = 0x0800,
    FLG_VOLTEX = 0x1000,

    FLG_GENMIP_BOX = 0x2000,
    FLG_GENMIP_KAIZER = 0x4000,
    FLG_GAMMA_EQ_1 = 0x8000,

    FLG_HOLD_SYSMEM_COPY = 0x00010000,
    FLG_NEED_PAIRED_BASETEX = 0x00020000,

    FLG_REV_MIP_ORDER = 0x00040000,
    FLG_HQ_PART = 0x00080000,

    FLG_MOBILE_TEXFMT = 0x00100000,
    FLG_ARRTEX = 0x00200000,

    FLG_COMPR_MASK = 0xE0000000,
    FLG_7ZIP = 0x40000000,
    FLG_ZLIB = 0x80000000,
    FLG_ZSTD = 0x20000000,
    FLG_OODLE = 0x60000000,

    FLG_ADDRU_MASK = 0x000F,
    FLG_ADDRV_MASK = 0x00F0,
  };


  int getAddrU() const
  {
    int a = flags & FLG_ADDRU_MASK;
    return a ? a : 1;
  }
  int getAddrV() const
  {
    int a = (flags & FLG_ADDRV_MASK) >> 4;
    return a ? a : 1;
  }
  void setAddr(unsigned u, unsigned v)
  {
    flags = (flags & ~(FLG_ADDRU_MASK | FLG_ADDRV_MASK)) | (u & FLG_ADDRU_MASK) | ((v << 4) & FLG_ADDRV_MASK);
  }

  unsigned compressionType() const { return flags & FLG_COMPR_MASK; }
  bool isCompression(unsigned f) const { return (flags & FLG_COMPR_MASK) == f; }
  bool isCompressionZLIB() const { return isCompression(FLG_ZLIB); }
  bool isCompression7ZIP() const { return isCompression(FLG_7ZIP); }
  bool isCompressionZSTD() const { return isCompression(FLG_ZSTD); }
  bool isCompressionOODLE() const { return isCompression(FLG_OODLE); }

  bool checkLabel() const { return label == _MAKE4C('DDSx'); }

  //! returns size of surface in bytes
  inline uint32_t calcSurfaceSz(int sw, int sh) const
  {
    if (!dxtShift)
      return sh * sw * bitsPerPixel / 8;

    if (flags & FLG_MOBILE_TEXFMT)
    {
      int tcType = dxtShift;
      if (tcType == 5 || tcType == 6 || tcType == 7) // ASTC
      {
        if (tcType == 5)
          sw /= 4, sh /= 4;
        else if (tcType == 6)
          sw /= 8, sh /= 8;
        else if (tcType == 7)
          sw /= 12, sh /= 12;
        if (sw < 1)
          sw = 1;
        if (sh < 1)
          sh = 1;
        return (sw * sh) * 128 / 8;
      }
      return 0;
    }

    return (max(sw >> 2, 1) * max(sh >> 2, 1)) << dxtShift;
  }
  //! returns size of level-th surface in bytes
  inline uint32_t getSurfaceSz(int level) const { return calcSurfaceSz(max(w >> level, 1), max(h >> level, 1)); }
  inline uint32_t getSurfacePitch(int level) const
  {
    if (!dxtShift)
    {
      uint32_t bw = w >> level;
      if (bw < 1)
        bw = 1;
      return bw * bitsPerPixel / 8;
    }

    if (flags & FLG_MOBILE_TEXFMT)
    {
      int tcType = dxtShift;
      if (tcType == 5 || tcType == 6 || tcType == 7) // ASTC
      {
        uint32_t bw = w >> level;
        if (tcType == 5)
          bw /= 4;
        else if (tcType == 6)
          bw /= 8;
        else if (tcType == 7)
          bw /= 12;
        if (bw < 1)
          bw = 1;
        return bw * 128 / 8;
      }
      return 0;
    }

    uint32_t bw = w >> (level + 2);
    if (bw < 1)
      bw = 1;
    return bw << dxtShift;
  }
  inline uint32_t getSurfaceScanlines(int level) const
  {
    if (!dxtShift)
    {
      uint32_t bh = h >> level;
      if (bh < 1)
        bh = 1;
      return bh;
    }

    if (flags & FLG_MOBILE_TEXFMT)
    {
      int tcType = dxtShift;
      if (tcType == 5 || tcType == 6 || tcType == 7) // ASTC
      {
        uint32_t bh = h >> level;
        if (tcType == 5)
          bh /= 4;
        else if (tcType == 6)
          bh /= 8;
        else if (tcType == 7)
          bh /= 12;
        return bh >= 1 ? bh : 1;
      }
    }

    uint32_t bh = h >> (level + 2);
    if (bh < 1)
      bh = 1;
    return bh;
  }

  //! returns number of top levels to skip
  inline int getSkipLevelsFromQ(int quality_id) const
  {
    switch (quality_id)
    {
      case 0: return 0;
      case 1: return mQmip;
      case 2: return lQmip;
      case 3: return uQmip;
      default: break;
    }
    return levels - 1;
  }

  //! returns number of top levels to skip and validates inout_levels
  inline int getSkipLevels(int skip_levels, int &inout_levels) const
  {
    if (flags & FLG_HQ_PART)
    {
      if (inout_levels <= 0)
        inout_levels = levels - skip_levels;
      return skip_levels;
    }

    if (skip_levels >= levels)
      skip_levels = levels - 1;

    if (inout_levels <= 0 || inout_levels > levels - skip_levels)
      inout_levels = levels - skip_levels;
    return skip_levels;
  }
};

static constexpr int VALID_HEADER = sizeof(char[(sizeof(Header) == 32) * 2 - 1]);
} // namespace ddsx
