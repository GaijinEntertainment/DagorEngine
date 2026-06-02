// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "driver.h"
#include "tagged_types.h"

#include <drv/3d/dag_tex3d.h>


namespace drv3d_dx12
{
// stores formats and offers some utility members
struct FormatStore
{
  static constexpr uint32_t create_flags_format_shift = 24;
  // one additional entry for the default
  static constexpr uint32_t max_format_count =
    (TEXFMT_LAST_DEPTH >> create_flags_format_shift) - (TEXFMT_A2R10G10B10 >> create_flags_format_shift) + 1 + 1;
  static constexpr uint32_t bits_format = BitsNeeded<max_format_count>::VALUE;
  static constexpr uint32_t bits = bits_format + 2;
  static_assert(bits <= sizeof(uint8_t) * 8);

  union
  {
    struct
    {
      uint8_t linearFormat : bits_format;
      uint8_t srgbWrite : 1;
      uint8_t srgbRead : 1;
    };
    uint8_t index = 0;
  };

  struct CreateFlagFormat
  {
    uint32_t value = 0;

    uint32_t getLinearFormat() const { return (value & TEXFMT_MASK) >> create_flags_format_shift; }
    bool isSrgbRead() const { return 0 != (value & TEXCF_SRGBREAD); }
    bool isSrgbWrite() const { return 0 != (value & TEXCF_SRGBWRITE); }

    void setLinearFormat(uint32_t fmt) { value = (value & ~TEXFMT_MASK) | (fmt << create_flags_format_shift); }
    void setSrgbRead(bool enable) { value = enable ? (value | TEXCF_SRGBREAD) : (value & ~TEXCF_SRGBREAD); }
    void setSrgbWrite(bool enable) { value = enable ? (value | TEXCF_SRGBWRITE) : (value & ~TEXCF_SRGBWRITE); }
  };
  // only valid to call if fmt was tested with canBeStored
  static FormatStore fromDXGIFormat(DXGI_FORMAT fmt);
  static FormatStore fromDXGIDepthFormat(DXGI_FORMAT fmt);
  static FormatStore fromCreateFlags(uint32_t cflg)
  {
    FormatStore fmt;
    fmt.setFromFlagTexFlags(cflg);
    return fmt;
  }
  DXGI_FORMAT asDxGiResourceCreateFormat() const;
  DXGI_FORMAT asDxGiBaseFormat() const;
  template <bool use_for_read>
  DXGI_FORMAT asDxGiFormat() const;
  DXGI_FORMAT asLinearDxGiFormat() const;
  DXGI_FORMAT asSrgbDxGiFormat() const;
  bool isSrgbCapableFormatType() const;
  FormatStore getLinearVariant() const
  {
    FormatStore r = *this;
    r.srgbRead = r.srgbWrite = 0;
    return r;
  }
  FormatStore getSRGBVariant() const
  {
    FormatStore r = *this;
    r.srgbRead = r.srgbWrite = 1;
    return r;
  }
  void setFromFlagTexFlags(uint32_t flags)
  {
    CreateFlagFormat fmt{flags};
    linearFormat = fmt.getLinearFormat();
    srgbRead = fmt.isSrgbRead();
    srgbWrite = fmt.isSrgbWrite();
  }
  uint32_t asTexFlags() const
  {
    CreateFlagFormat fmt;
    fmt.setLinearFormat(linearFormat);
    fmt.setSrgbRead(srgbRead);
    fmt.setSrgbWrite(srgbWrite);
    return fmt.value;
  }
  bool isSampledAsFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_DEFAULT >> create_flags_format_shift:
      case TEXFMT_A2R10G10B10 >> create_flags_format_shift:
      case TEXFMT_A2B10G10R10 >> create_flags_format_shift:
      case TEXFMT_A16B16G16R16 >> create_flags_format_shift:
      case TEXFMT_A16B16G16R16F >> create_flags_format_shift:
      case TEXFMT_A32B32G32R32F >> create_flags_format_shift:
      case TEXFMT_G16R16 >> create_flags_format_shift:
      case TEXFMT_G16R16F >> create_flags_format_shift:
      case TEXFMT_G32R32F >> create_flags_format_shift:
      case TEXFMT_R16F >> create_flags_format_shift:
      case TEXFMT_R32F >> create_flags_format_shift:
      case TEXFMT_DXT1 >> create_flags_format_shift:
      case TEXFMT_DXT3 >> create_flags_format_shift:
      case TEXFMT_DXT5 >> create_flags_format_shift:
      case TEXFMT_L16 >> create_flags_format_shift:
      case TEXFMT_A8 >> create_flags_format_shift:
      case TEXFMT_R8 >> create_flags_format_shift:
      case TEXFMT_A1R5G5B5 >> create_flags_format_shift:
      case TEXFMT_A4R4G4B4 >> create_flags_format_shift:
      case TEXFMT_R5G6B5 >> create_flags_format_shift:
      case TEXFMT_A16B16G16R16S >> create_flags_format_shift:
      case TEXFMT_ATI1N >> create_flags_format_shift:
      case TEXFMT_ATI2N >> create_flags_format_shift:
      case TEXFMT_R8G8B8A8 >> create_flags_format_shift:
      case TEXFMT_R11G11B10F >> create_flags_format_shift:
      case TEXFMT_R9G9B9E5 >> create_flags_format_shift:
      case TEXFMT_R8G8 >> create_flags_format_shift:
      case TEXFMT_R8G8S >> create_flags_format_shift:
      case TEXFMT_DEPTH24 >> create_flags_format_shift:
      case TEXFMT_DEPTH16 >> create_flags_format_shift:
      case TEXFMT_DEPTH32 >> create_flags_format_shift:
      case TEXFMT_DEPTH32_S8 >> create_flags_format_shift: return true;
    }
    return false;
  }
  bool isFloat() const
  {
    switch (linearFormat)
    {
      case TEXFMT_A16B16G16R16F >> create_flags_format_shift:
      case TEXFMT_A32B32G32R32F >> create_flags_format_shift:
      case TEXFMT_G16R16F >> create_flags_format_shift:
      case TEXFMT_G32R32F >> create_flags_format_shift:
      case TEXFMT_R16F >> create_flags_format_shift:
      case TEXFMT_R32F >> create_flags_format_shift:
      case TEXFMT_R11G11B10F >> create_flags_format_shift:
      case TEXFMT_R9G9B9E5 >> create_flags_format_shift: return true;
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
  uint32_t getBytesPerPixelBlockPerPlane(uint32_t *block_x, uint32_t *block_y, uint32_t plane_index) const;
  template <bool use_for_read>
  const char *getNameString() const;
  // returns true if the format can be represented with this storage
  static bool canBeStored(DXGI_FORMAT fmt);
  static void configureFormatTable(ID3D12Device *device, uint32_t vendor = 0);
  FormatPlaneCount getPlanes() const;
  uint32_t getChannelMask() const;
  // isSupported section
  bool isSupported(D3D12_FORMAT_SUPPORT1 flags) const;
  bool isSupported(D3D12_FORMAT_SUPPORT2 flags) const;
  bool isSupportedTexture2D() const;
  bool isSupportedTexture3D() const;
  bool isSupportedTextureCube() const;
  bool isSupportedDepthStencil() const;
  bool isSupportedRenderTarget() const;
  bool isSupportedMultisampleRenderTarget() const;
  bool isSupportedMultisampleResolve() const;
  bool isSupportedMultisampleLoad() const;
  bool isSupportedBlendable() const;
  bool isSupportedShaderSample() const;
  bool isSupportedTypedUav() const;
  bool isSupportedUavTypedLoad() const;
  bool isSupportedTiled() const;
  // msaa section
  bool isSampleCountSupported(int32_t sampleCount) const;
  int32_t getMaxSampleCount() const;
  // returns true if a format can be mem copied by the GPU
  bool isCopyConvertible(FormatStore other) const;
};
inline bool operator==(FormatStore l, FormatStore r) { return l.index == r.index; }
inline bool operator!=(FormatStore l, FormatStore r) { return l.index != r.index; }
} // namespace drv3d_dx12