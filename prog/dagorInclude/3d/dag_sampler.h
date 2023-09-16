//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_3dConst_base.h"
#include <math/dag_color.h>
#include <util/dag_stdint.h>

//
// These familes are platform specific and are defined in dag_3dConst*.h header:
//   TEXADDR_
//   TEXFILTER_
//

enum
{
  TEXMIPMAP_DEFAULT = 0, // driver default
  TEXMIPMAP_NONE = 1,    // no mipmapping
  TEXMIPMAP_POINT = 2,   // point mipmapping
  TEXMIPMAP_LINEAR = 3,  // linear mipmapping
};

namespace d3d
{
// Unortunately, the `None` macro has already been defined in /usr/include/X11/X.h:115 as
// #define None                 0L /* universal null resource or null atom */
// Therefore, we cannot use this identifier as an alias for TEXMIPMAP_NONE
// clang-format off
enum class MipMapMode : uint32_t
{
  Default  = TEXMIPMAP_DEFAULT, // [DEPRECATED] driver default
  Disabled = TEXMIPMAP_NONE,    // [DEPRECATED] no mipmapping
  Point    = TEXMIPMAP_POINT,   // point mipmapping
  Linear   = TEXMIPMAP_LINEAR,  // linear mipmapping
};

enum class FilterMode : uint32_t
{
  Default  = TEXFILTER_DEFAULT, // [DEPRECATED] driver default
  Disabled = TEXFILTER_NONE,    // [DEPRECATED]
  Point    = TEXFILTER_POINT,
  Linear   = TEXFILTER_SMOOTH,
  Best     = TEXFILTER_BEST,    // [DEPRECATED] anisotropic and similar, if available
  Compare  = TEXFILTER_COMPARE, // point comparasion for using in pcf
};

enum class AddressMode : uint32_t
{
  Wrap       = TEXADDR_WRAP,
  Mirror     = TEXADDR_MIRROR,
  Clamp      = TEXADDR_CLAMP,
  Border     = TEXADDR_BORDER,
  MorrorOnce = TEXADDR_MIRRORONCE,
};
// clang-format on

struct SamplerInfo
{
  MipMapMode mip_map_mode = MipMapMode::Default;
  FilterMode filter_mode = FilterMode::Default;
  AddressMode address_mode_u = AddressMode::Wrap;
  AddressMode address_mode_v = AddressMode::Wrap;
  AddressMode address_mode_w = AddressMode::Wrap;
  E3DCOLOR border_color;
  // only positive power of two values <= 16 are valid
  float anisotropic_max = 1.f;
  float mip_map_bias = 0.f;
};

#if _TARGET_64BIT
// This way we have type safety!
struct SamplerObjectType;
using SamplerHandle = SamplerObjectType *;
#else
// Have always 64bits - this allows the drivers to use pointers, packed data structures of API objects to be returned
using SamplerHandle = uint64_t;
#endif

} // namespace d3d
