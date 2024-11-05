// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_sampler.h>

#include "bitfield.h"
#include "half_float.h"
#include "d3d12_d3d_translation.h"


namespace drv3d_dx12
{

BEGIN_BITFIELD_TYPE(SamplerState, uint32_t)
  enum
  {
    BIAS_BITS = 16,
    BIAS_OFFSET = 0,
    MIP_BITS = 1,
    MIP_SHIFT = BIAS_OFFSET + BIAS_BITS,
    FILTER_BITS = 1,
    FILTER_SHIFT = MIP_SHIFT + MIP_BITS,
    // Instead of using N bits per coord, we store all coords in one value, this safes 2 bits
    COORD_VALUE_COUNT = (D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE - D3D12_TEXTURE_ADDRESS_MODE_WRAP) + 1,
    COORD_MAX_VALUE = COORD_VALUE_COUNT * COORD_VALUE_COUNT * COORD_VALUE_COUNT,
    COORD_BITS = BitsNeeded<COORD_MAX_VALUE>::VALUE,
    COORD_SHIFT = FILTER_SHIFT + FILTER_BITS,
    ANISO_BITS = 3,
    ANISO_SHIFT = COORD_SHIFT + COORD_BITS,
    BORDER_BITS = 2,
    BORDER_SHIFT = ANISO_SHIFT + ANISO_BITS,
    IS_COMPARE_BITS = 1,
    IS_COMPARE_SHIFT = BORDER_BITS + BORDER_SHIFT,
    FLOAT_EXP_BASE = 127,
    FLOAT_EXP_SHIFT = 23,
  };
  ADD_BITFIELD_MEMBER(mipMapMode, MIP_SHIFT, MIP_BITS)
  ADD_BITFIELD_MEMBER(filterMode, FILTER_SHIFT, FILTER_BITS)
  ADD_BITFIELD_MEMBER(borderColorMode, BORDER_SHIFT, BORDER_BITS)
  ADD_BITFIELD_MEMBER(anisotropicValue, ANISO_SHIFT, ANISO_BITS)
  ADD_BITFIELD_MEMBER(coordModes, COORD_SHIFT, COORD_BITS)
  ADD_BITFIELD_MEMBER(biasBits, BIAS_OFFSET, BIAS_BITS)
  ADD_BITFIELD_MEMBER(isCompare, IS_COMPARE_SHIFT, IS_COMPARE_BITS)

  BEGIN_BITFIELD_TYPE(iee754Float, uint32_t)
    float asFloat;
    uint32_t asUint;
    int32_t asInt;
    ADD_BITFIELD_MEMBER(mantissa, 0, 23)
    ADD_BITFIELD_MEMBER(exponent, 23, 8)
    ADD_BITFIELD_MEMBER(sign, 31, 1)
  END_BITFIELD_TYPE()

  static SamplerState make_default()
  {
    SamplerState desc;
    desc.setMip(D3D12_FILTER_TYPE_LINEAR);
    desc.setFilter(D3D12_FILTER_TYPE_LINEAR);
    desc.setU(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setV(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setW(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setBias(0.f);
    desc.setAniso(1.f);
    desc.setBorder(0);
    return desc;
  }

  D3D12_SAMPLER_DESC asDesc() const
  {
    D3D12_SAMPLER_DESC result;

    result.MaxAnisotropy = getAnisoInt();
    if (result.MaxAnisotropy > 1)
      result.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(static_cast<uint32_t>(isCompare));
    else
      result.Filter = D3D12_ENCODE_BASIC_FILTER(getFilter(), getFilter(), getMip(), static_cast<uint32_t>(isCompare));

    result.AddressU = getU();
    result.AddressV = getV();
    result.AddressW = getW();
    result.MipLODBias = getBias();
    result.ComparisonFunc = isCompare ? D3D12_COMPARISON_FUNC_LESS_EQUAL : D3D12_COMPARISON_FUNC_ALWAYS;
    result.MinLOD = 0;
    result.MaxLOD = FLT_MAX;
    result.BorderColor[0] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[1] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[2] = static_cast<uint32_t>(borderColorMode) & 1 ? 1.f : 0.f;
    result.BorderColor[3] = static_cast<uint32_t>(borderColorMode) & 2 ? 1.f : 0.f;

    return result;
  }

  void setMip(D3D12_FILTER_TYPE mip) { mipMapMode = (uint32_t)mip; }
  D3D12_FILTER_TYPE getMip() const { return static_cast<D3D12_FILTER_TYPE>(static_cast<uint32_t>(mipMapMode)); }
  void setFilter(D3D12_FILTER_TYPE filter) { filterMode = filter; }
  D3D12_FILTER_TYPE getFilter() const { return static_cast<D3D12_FILTER_TYPE>(static_cast<uint32_t>(filterMode)); }
  void setCoordModes(D3D12_TEXTURE_ADDRESS_MODE u, D3D12_TEXTURE_ADDRESS_MODE v, D3D12_TEXTURE_ADDRESS_MODE w)
  {
    auto rawU = static_cast<uint32_t>(u) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    auto rawV = static_cast<uint32_t>(v) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    auto rawW = static_cast<uint32_t>(w) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes = rawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT + rawV * COORD_VALUE_COUNT + rawU;
  }
  void setU(D3D12_TEXTURE_ADDRESS_MODE u)
  {
    auto oldRawU = coordModes % COORD_VALUE_COUNT;
    auto newRawU = static_cast<uint32_t>(u) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawU;
    coordModes += newRawU;
  }
  void setV(D3D12_TEXTURE_ADDRESS_MODE v)
  {
    auto oldRawV = (coordModes / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    auto newRawV = static_cast<uint32_t>(v) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawV * COORD_VALUE_COUNT;
    coordModes += newRawV * COORD_VALUE_COUNT;
  }
  void setW(D3D12_TEXTURE_ADDRESS_MODE w)
  {
    auto oldRawW = (coordModes / COORD_VALUE_COUNT / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    auto newRawW = static_cast<uint32_t>(w) - D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    coordModes -= oldRawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT;
    coordModes += newRawW * COORD_VALUE_COUNT * COORD_VALUE_COUNT;
  }
  D3D12_TEXTURE_ADDRESS_MODE getU() const
  {
    auto rawValue = coordModes % COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  D3D12_TEXTURE_ADDRESS_MODE getV() const
  {
    auto rawValue = (coordModes / COORD_VALUE_COUNT) % COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  D3D12_TEXTURE_ADDRESS_MODE getW() const
  {
    auto rawValue = (coordModes / COORD_VALUE_COUNT) / COORD_VALUE_COUNT;
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(D3D12_TEXTURE_ADDRESS_MODE_WRAP + rawValue);
  }
  void setBias(float b) { biasBits = half_float::convert_from_float(b); }
  float getBias() const { return half_float::convert_to_float(biasBits); }
  void setAniso(float a)
  {
    // some float magic, falls flat on its face if it is not ieee-754
    // extracts exponent and subtracts the base
    // clamps the result into range from  0 to 4 which represents 1,2,4,8 and 16 as floats
    // negative values are treated as positive
    // values from range 0 - 1 are rounded up
    // everything else is rounded down
    iee754Float f;
    f.asFloat = a;
    int32_t value = f.exponent - FLOAT_EXP_BASE;
    // clamp from 1 to 16
    value = clamp(value, 0, 4);
    anisotropicValue = value;
  }
  float getAniso() const
  {
    iee754Float f;
    f.exponent = FLOAT_EXP_BASE + anisotropicValue;
    return f.asFloat;
  }
  uint32_t getAnisoInt() const { return 1u << static_cast<uint32_t>(anisotropicValue); }
  // Same restrictions as with vulkan, either color is white or black and its either fully
  // transparent or opaque
  void setBorder(E3DCOLOR color) { borderColorMode = ((color.r || color.g || color.b) ? 1 : 0) | (color.a ? 2 : 0); }
  E3DCOLOR getBorder() const
  {
    E3DCOLOR result;
    result.r = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.g = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.b = static_cast<uint32_t>(borderColorMode) & 1 ? 0xFF : 0;
    result.a = static_cast<uint32_t>(borderColorMode) & 2 ? 0xFF : 0;
    return result;
  }

  bool needsBorderColor() const
  {
    return (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getU()) || (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getV()) ||
           (D3D12_TEXTURE_ADDRESS_MODE_BORDER == getW());
  }

  bool normalizeSelf()
  {
    bool wasNormalized = false;
    if (!needsBorderColor())
    {
      setBorder(0);
      wasNormalized = true;
    }
    return wasNormalized;
  }

  SamplerState normalize() const
  {
    // normalization is when border color is not needed we default to color 0
    SamplerState copy = *this;
    copy.normalizeSelf();
    return copy;
  }

  static SamplerState fromSamplerInfo(const d3d::SamplerInfo &info);
END_BITFIELD_TYPE()


inline SamplerState SamplerState::fromSamplerInfo(const d3d::SamplerInfo &info)
{
  SamplerState state;
  state.isCompare = info.filter_mode == d3d::FilterMode::Compare;
  state.setFilter(translate_filter_type_to_dx12(static_cast<int>(info.filter_mode)));
  state.setMip(translate_mip_filter_type_to_dx12(static_cast<int>(info.mip_map_mode)));
  state.setU(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_u)));
  state.setV(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_v)));
  state.setW(translate_texture_address_mode_to_dx12(static_cast<int>(info.address_mode_w)));
  state.setBias(info.mip_map_bias);
  state.setAniso(info.anisotropic_max);
  state.setBorder(info.border_color);
  return state;
}

struct SamplerDescriptorAndState
{
  D3D12_CPU_DESCRIPTOR_HANDLE sampler;
  SamplerState state;
};

} // namespace drv3d_dx12
