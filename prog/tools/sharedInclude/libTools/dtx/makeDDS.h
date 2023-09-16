#pragma once

#include <util/dag_stdint.h>
#include <3d/dag_tex3d.h>
#include <3d/ddsFormat.h>
#include <debug/dag_debug.h>

inline int dds_header_size() { return sizeof(DDSURFACEDESC2) + 4; }

inline bool create_dds_header(void *data, int size, int w, int h, int bpp, int mip_levels, int fmt, bool astc)
{
  if (size < sizeof(DDSURFACEDESC2))
    return false;
  *(uint32_t *)data = MAKEFOURCC('D', 'D', 'S', ' ');
  DDSURFACEDESC2 &dsc = *(DDSURFACEDESC2 *)((uint32_t *)data + 1);
  memset(&dsc, 0, sizeof(dsc));
  dsc.dwSize = sizeof(DDSURFACEDESC2);
  dsc.dwWidth = w;
  dsc.dwHeight = h;
  dsc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
  dsc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  dsc.ddpfPixelFormat.dwRGBBitCount = bpp;
  dsc.dwMipMapCount = mip_levels < 1 ? 1 : mip_levels;
  dsc.ddsCaps.dwCaps = DDSCAPS2_CUBEMAP_POSITIVEY;

  if (fmt == TEXFMT_A4R4G4B4) // FIXME: Other formats may not export for xbox.
  {
    dsc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    dsc.ddpfPixelFormat.dwRBitMask = 0x00000F00;
    dsc.ddpfPixelFormat.dwGBitMask = 0x000000F0;
    dsc.ddpfPixelFormat.dwBBitMask = 0x0000000F;
    dsc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000F000;
  }
  else if (fmt == TEXFMT_A8)
  {
    dsc.ddpfPixelFormat.dwFlags = DDPF_ALPHA | DDPF_ALPHAPIXELS;
    dsc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x000000FF;
  }
  else if (fmt == TEXFMT_L8)
  {
    dsc.ddpfPixelFormat.dwFlags = DDPF_LUMINANCE;
    dsc.ddpfPixelFormat.dwLuminanceBitCount = bpp;
    dsc.ddpfPixelFormat.dwLuminanceBitMask = 0xFF;
  }
  else
  {
    DWORD dfmt = D3DFMT_UNKNOWN;
    switch (fmt)
    {
      case TEXFMT_A2R10G10B10: dfmt = D3DFMT_A2R10G10B10; break;
      case TEXFMT_A2B10G10R10: dfmt = D3DFMT_A2B10G10R10; break;
      case TEXFMT_A16B16G16R16: dfmt = D3DFMT_A16B16G16R16; break;
      case TEXFMT_A16B16G16R16F: dfmt = D3DFMT_A16B16G16R16F; break;
      case TEXFMT_A32B32G32R32F: dfmt = D3DFMT_A32B32G32R32F; break;
      case TEXFMT_G16R16: dfmt = D3DFMT_G16R16; break;
      case TEXFMT_G16R16F: dfmt = D3DFMT_G16R16F; break;
      case TEXFMT_G32R32F: dfmt = D3DFMT_G32R32F; break;
      case TEXFMT_R16F: dfmt = D3DFMT_R16F; break;
      case TEXFMT_R32F: dfmt = D3DFMT_R32F; break;
      case TEXFMT_DXT1:
        dfmt = astc ? (MAKEFOURCC('A', 'S', 'T', '8')) : D3DFMT_DXT1;
        if (astc)
          dsc.ddpfPixelFormat.dwRGBBitCount = 2;
        break;
      case TEXFMT_DXT3: dfmt = D3DFMT_DXT3; break;
      case TEXFMT_DXT5:
        dfmt = astc ? (MAKEFOURCC('A', 'S', 'T', '4')) : D3DFMT_DXT5;
        if (astc)
          dsc.ddpfPixelFormat.dwRGBBitCount = 8;
        break;
      case TEXFMT_A8R8G8B8: dfmt = D3DFMT_A8R8G8B8; break;
      case TEXFMT_V16U16: dfmt = D3DFMT_V16U16; break;
      case TEXFMT_L16: dfmt = D3DFMT_L16; break;
      case TEXFMT_A8: dfmt = D3DFMT_A8; break;
      case TEXFMT_L8: dfmt = D3DFMT_L8; break;
      case TEXFMT_A1R5G5B5: dfmt = D3DFMT_A1R5G5B5; break;
      case TEXFMT_A4R4G4B4: dfmt = D3DFMT_A4R4G4B4; break;
      default: return false;
    }
    dsc.ddpfPixelFormat.dwFlags |= DDPF_FOURCC;
    dsc.ddpfPixelFormat.dwFourCC = dfmt;
  }
  return true;
}
