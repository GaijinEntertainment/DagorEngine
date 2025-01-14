// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "driver.h"
#include "tagged_types.h"

#include <drv/3d/dag_tex3d.h>
#include <util/dag_globDef.h>


namespace drv3d_dx12
{
// stores formats and offers some utility members
BEGIN_BITFIELD_TYPE(FormatStore, uint8_t)
  enum
  {
    CREATE_FLAGS_FORMAT_SHIFT = 24,
    // one additional entry for the default
    MAX_FORMAT_COUNT = (TEXFMT_LAST_DEPTH >> CREATE_FLAGS_FORMAT_SHIFT) - (TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT) + 1 + 1,
    BITS = BitsNeeded<MAX_FORMAT_COUNT>::VALUE
  };
  ADD_BITFIELD_MEMBER(isSrgb, BITS, 1)
  ADD_BITFIELD_MEMBER(linearFormat, 0, BITS)
  ADD_BITFIELD_MEMBER(index, 0, BITS + 1)

  BEGIN_BITFIELD_TYPE(CreateFlagFormat, uint32_t)
    ADD_BITFIELD_MEMBER(format, 24, 8)
    ADD_BITFIELD_MEMBER(srgbRead, 22, 1)
    ADD_BITFIELD_MEMBER(srgbWrite, 21, 1)
  END_BITFIELD_TYPE()

  FormatStore(DXGI_FORMAT fmt) = delete;
  // only valid to call if fmt was tested with canBeStored
  static FormatStore fromDXGIFormat(DXGI_FORMAT fmt);
  static FormatStore fromDXGIDepthFormat(DXGI_FORMAT fmt);
  static FormatStore fromCreateFlags(uint32_t cflg)
  {
    FormatStore fmt;
    fmt.setFromFlagTexFlags(cflg);
    return fmt;
  }
  DXGI_FORMAT asDxGiTextureCreateFormat() const;
  DXGI_FORMAT asDxGiBaseFormat() const;
  DXGI_FORMAT asDxGiFormat() const;
  DXGI_FORMAT asLinearDxGiFormat() const;
  DXGI_FORMAT asSrgbDxGiFormat() const;
  bool isSrgbCapableFormatType() const;
  FormatStore getLinearVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 0;
    return r;
  }
  FormatStore getSRGBVariant() const
  {
    FormatStore r = *this;
    r.isSrgb = 1;
    return r;
  }
  void setFromFlagTexFlags(uint32_t flags)
  {
    CreateFlagFormat fmt(flags);
    linearFormat = fmt.format;
    isSrgb = fmt.srgbRead | fmt.srgbWrite;
  }
  uint32_t asTexFlags() const
  {
    CreateFlagFormat fmt;
    fmt.format = linearFormat;
    fmt.srgbRead = isSrgb;
    fmt.srgbWrite = isSrgb;
    return fmt.wrapper.value;
  }
  bool isSampledAsFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_DEFAULT >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2R10G10B10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A2B10G10R10 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT1 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT3 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DXT5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_L16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A1R5G5B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A4R4G4B4 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R5G6B5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A16B16G16R16S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI1N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_ATI2N >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8B8A8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R8G8S >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH24 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH16 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32 >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_DEPTH32_S8 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
    }
    return false;
  }
  bool isFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_A16B16G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_A32B32G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G16R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_G32R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R16F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R32F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R11G11B10F >> CREATE_FLAGS_FORMAT_SHIFT:
      case TEXFMT_R9G9B9E5 >> CREATE_FLAGS_FORMAT_SHIFT: return true;
      default: return false;
    }
  }
  // Format has to have RGB channels with more than 8 bits each
  bool isHDRFormat() const;
  bool isColor() const;
  bool isDepth() const;
  bool isStencil() const;
  bool isBlockCompressed() const;
  uint32_t getBytesPerPixelBlock(uint32_t *block_x = NULL, uint32_t *block_y = NULL) const;
  const char *getNameString() const;
  // returns true if the format can be represented with this storage
  static bool canBeStored(DXGI_FORMAT fmt);
#if _TARGET_PC_WIN
  static void patchFormatTalbe(ID3D12Device * device, uint32_t vendor);
#endif
  FormatPlaneCount getPlanes() const;
  uint32_t getChannelMask() const;
  // returns true if a format can be mem copied by the GPU
  bool isCopyConvertible(FormatStore other) const;
END_BITFIELD_TYPE()
inline bool operator==(FormatStore l, FormatStore r) { return l.index == r.index; }
inline bool operator!=(FormatStore l, FormatStore r) { return l.index != r.index; }
} // namespace drv3d_dx12