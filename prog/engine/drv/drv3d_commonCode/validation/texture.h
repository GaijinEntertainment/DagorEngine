// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>

#include <drv_log_defs.h>
#include <drv_assert_defs.h>

// Whitelist of TEXFMT_ values with a real SRGB hardware variant, extracted from the DX12 and Vulkan format tables.
inline void check_texture_srgb_format(uint32_t cflg, const char *name)
{
  if (!(cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)))
    return;

  const uint32_t fmt = cflg & TEXFMT_MASK;
  switch (fmt)
  {
    case TEXFMT_A8R8G8B8: // also TEXFMT_DEFAULT
    case TEXFMT_R8G8B8A8:
    case TEXFMT_DXT1:
    case TEXFMT_DXT3:
    case TEXFMT_DXT5:
    case TEXFMT_BC7:
    case TEXFMT_ASTC4:
    case TEXFMT_ASTC8:
    case TEXFMT_ASTC12:
    case TEXFMT_ETC2_RGBA: return;
    default:
      D3D_CONTRACT_ERROR("Texture <%s> has unsupported SRGB format 0x%08x. SRGB flag will be ignored. "
                         "Consider removing the SRGB flag.",
        name, fmt);
  }
}

inline void check_texture_creation_args(uint32_t width, uint32_t height, uint32_t flags, const char *name)
{
  if (is_bc_texformat(flags) && !(width % 4 == 0 && height % 4 == 0))
    D3D_CONTRACT_ERROR("BC texture format requires width and height to be multiple of 4, but got %u x %u for '%s'", width, height,
      name);
  check_texture_srgb_format(flags, name);
}
