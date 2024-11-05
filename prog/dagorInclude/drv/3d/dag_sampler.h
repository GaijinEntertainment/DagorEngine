//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_samplerHandle.h>
#include <math/dag_color.h>

//
// These familes are platform specific and are defined in dag_3dConst*.h header:
//   TEXADDR_
//   TEXFILTER_
//

/**
 * @brief MipMapMode enum
 */
enum
{
  TEXMIPMAP_DEFAULT = 0, ///< driver default
  TEXMIPMAP_NONE = 1,    ///< no mipmapping
  TEXMIPMAP_POINT = 2,   ///< point mipmapping
  TEXMIPMAP_LINEAR = 3,  ///< linear mipmapping
};

namespace d3d
{
/**
 * @brief MipMapMode enum class
 */
enum class MipMapMode : uint32_t
{
  Default = TEXMIPMAP_DEFAULT, ///< [DEPRECATED] driver default
  Disabled = TEXMIPMAP_NONE,   ///< [DEPRECATED] no mipmapping
  Point = TEXMIPMAP_POINT,     ///< point mipmapping
  Linear = TEXMIPMAP_LINEAR,   ///< linear mipmapping
};

/**
 * @brief FilterMode enum class
 */
enum class FilterMode : uint32_t
{
  Default = TEXFILTER_DEFAULT, ///< [DEPRECATED] driver default
  Disabled = TEXFILTER_NONE,   ///< [DEPRECATED]
  Point = TEXFILTER_POINT,     ///< point sampling
  Linear = TEXFILTER_LINEAR,   ///< linear sampling
  Best = TEXFILTER_BEST,       ///< [DEPRECATED] anisotropic and similar, if available
  Compare = TEXFILTER_COMPARE, ///< point comparasion for using in pcf
};

/**
 * @brief AddressMode enum class
 */
enum class AddressMode : uint32_t
{
  Wrap = TEXADDR_WRAP,             ///< Repeats the texture when texture coordinates are outside [0, 1]
  Mirror = TEXADDR_MIRROR,         ///< Mirrors the texture when texture coordinates are outside [0, 1]
  Clamp = TEXADDR_CLAMP,           ///< Clamps texture coordinates to [0, 1]
  Border = TEXADDR_BORDER,         ///< Uses the border color when texture coordinates are outside [0, 1]
  MirrorOnce = TEXADDR_MIRRORONCE, ///< Similar to Mirror and Clamp. Takes the absolute value of the texture coordinate (thus,
                                   ///< mirroring around 0), and then clamps to the maximum value. The most common usage is for volume
                                   ///< textures, where support for the full MirrorOnce texture-addressing mode is not necessary, but
                                   ///< the data is symmetrical around the one axis.
};

/**
 * @brief BorderColor struct
 */
struct BorderColor
{
  using UnderlyingT = uint32_t;
  enum class Color : UnderlyingT
  {
    TransparentBlack = 0x00000000, ///< Transparent black color
    OpaqueBlack = 0xFF000000,      ///< Opaque black color
    OpaqueWhite = 0xFFFFFFFF,      ///< Opaque white color
  } color;
  BorderColor(Color color = Color::TransparentBlack) : color(color) {}
  operator E3DCOLOR() const { return static_cast<E3DCOLOR>(eastl::to_underlying(color)); };
  operator UnderlyingT() const { return eastl::to_underlying(color); };
};

/**
 * @brief SamplerInfo struct
 *
 * This struct contains the information needed to create a sampler
 */
struct SamplerInfo
{
  MipMapMode mip_map_mode = MipMapMode::Default;  ///< MipMapMode
  FilterMode filter_mode = FilterMode::Default;   ///< FilterMode
  AddressMode address_mode_u = AddressMode::Wrap; ///< AddressMode for U coordinate
  AddressMode address_mode_v = AddressMode::Wrap; ///< AddressMode for V coordinate
  AddressMode address_mode_w = AddressMode::Wrap; ///< AddressMode for W coordinate
  BorderColor border_color;                       ///< Border color
  float anisotropic_max = 1.f;                    ///< Max anisotropic value. Only positive power of two values <= 16 are valid
  float mip_map_bias = 0.f;                       ///< MipMap bias

  // clang-format off

#define EQU(field) (this->field == rhs.field)
#define CMP(field) if (this->field != rhs.field) return field < rhs.field;
#define FOR_EACH_FIELD(op, sep) \
  op(mip_map_mode)              \
  sep op(filter_mode)           \
  sep op(address_mode_u)        \
  sep op(address_mode_v)        \
  sep op(address_mode_w)        \
  sep op(border_color)          \
  sep op(anisotropic_max)       \
  sep op(mip_map_bias)

  bool operator==(const SamplerInfo &rhs) const { return FOR_EACH_FIELD(EQU, &&); }
  bool operator!=(const SamplerInfo &rhs) const { return !(*this == rhs); }
  bool operator<(const SamplerInfo &rhs) const { FOR_EACH_FIELD(CMP, ;); return false; }

  // clang-format on

#undef EQU
#undef CMP
#undef FOR_EACH_FIELD
};

/**
 * @brief Request a sampler handle with the given sampler info
 *
 * Creates a sampler, when necessary. Identical infos should yield identical handles.
 * This call is thread-safe and does not require external synchronization.
 * @param sampler_info The information needed to create the sampler
 * @return SamplerHandle The handle to the sampler
 */
SamplerHandle request_sampler(const SamplerInfo &sampler_info);

/**
 * @brief Binds given sampler to the slot
 *
 * This call is not thread-safe, requires global gpu lock to be held
 *
 * @param shader_stage The shader stage to bind the sampler to. One of STAGE_XXX enum.
 * @param slot The slot to bind the sampler to
 * @param sampler The handle to the sampler to be bound
 */
void set_sampler(unsigned shader_stage, unsigned slot, SamplerHandle sampler);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline SamplerHandle request_sampler(const SamplerInfo &sampler_info) { return d3di.request_sampler(sampler_info); }
inline void set_sampler(unsigned shader_stage, unsigned slot, SamplerHandle sampler) { d3di.set_sampler(shader_stage, slot, sampler); }
} // namespace d3d
#endif
