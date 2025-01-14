// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "basetexture.h"
#include <3d/ddsFormat.h>

uint32_t d3dformat_to_texfmt(/*D3DFORMAT*/ uint32_t fmt)
{
  switch (fmt)
  {
    case D3DFMT_A8R8G8B8: return TEXFMT_A8R8G8B8; //-V1037
    case D3DFMT_A8B8G8R8: return TEXFMT_R8G8B8A8;
    case D3DFMT_X8R8G8B8: return TEXFMT_A8R8G8B8; //?
    case D3DFMT_A2R10G10B10: return TEXFMT_A2R10G10B10;
    case D3DFMT_A2B10G10R10: return TEXFMT_A2B10G10R10;
    case D3DFMT_A16B16G16R16: return TEXFMT_A16B16G16R16;
    case D3DFMT_A16B16G16R16F: return TEXFMT_A16B16G16R16F;
    case D3DFMT_A32B32G32R32F: return TEXFMT_A32B32G32R32F;
    case D3DFMT_G16R16: return TEXFMT_G16R16;
    case D3DFMT_L16: return TEXFMT_L16;
    case D3DFMT_A8: return TEXFMT_A8;
    case D3DFMT_L8: return TEXFMT_R8;
    case D3DFMT_G16R16F: return TEXFMT_G16R16F;
    case D3DFMT_G32R32F: return TEXFMT_G32R32F;
    case D3DFMT_R16F: return TEXFMT_R16F;
    case D3DFMT_R32F: return TEXFMT_R32F;
    case D3DFMT_DXT1: return TEXFMT_DXT1;
    case D3DFMT_DXT3: return TEXFMT_DXT3;
    case D3DFMT_DXT5: return TEXFMT_DXT5;
    case D3DFMT_A4R4G4B4: return TEXFMT_A4R4G4B4; // dxgi 1.2 //-V1037
    case D3DFMT_X4R4G4B4: return TEXFMT_A4R4G4B4;
    case D3DFMT_A1R5G5B5: return TEXFMT_A1R5G5B5;
    case D3DFMT_R5G6B5: return TEXFMT_R5G6B5;
#if _TARGET_IOS | _TARGET_TVOS | _TARGET_C3 | _TARGET_ANDROID
    case D3DFMT_R4G4B4A4: return TEXFMT_A4R4G4B4;
    case D3DFMT_R5G5B5A1: return TEXFMT_A1R5G5B5;
#endif
    case _MAKE4C('ATI1'): return TEXFMT_ATI1N;
    case _MAKE4C('ATI2'): return TEXFMT_ATI2N;
    case _MAKE4C('BC6H'): return TEXFMT_BC6H;
    case _MAKE4C('BC7 '): return TEXFMT_BC7;
    case _MAKE4C('AST4'): return TEXFMT_ASTC4;
    case _MAKE4C('AST8'): return TEXFMT_ASTC8;
    case _MAKE4C('ASTC'): return TEXFMT_ASTC12;

    case TEXFMT_A16B16G16R16S:
    case TEXFMT_ATI1N:
    case TEXFMT_ATI2N:
    case TEXFMT_A16B16G16R16UI:
    case TEXFMT_A32B32G32R32UI:
    case TEXFMT_R32G32UI:
    case TEXFMT_R32UI:
    case TEXFMT_R32SI:
    case TEXFMT_R11G11B10F:
    case TEXFMT_R9G9B9E5:
    case TEXFMT_R8G8:
    case TEXFMT_R8G8S: return fmt;
  }
  G_ASSERTF(0, "can't convert d3d format");
  return TEXFMT_A8R8G8B8; // TEXFMT_DEFAULT;
}

uint32_t texfmt_to_d3dformat(/*D3DFORMAT*/ uint32_t fmt)
{
  switch (fmt)
  {
    case TEXFMT_A8R8G8B8: return D3DFMT_A8R8G8B8;
    case TEXFMT_R8G8B8A8: return D3DFMT_A8B8G8R8;
    case TEXFMT_A2B10G10R10: return D3DFMT_A2B10G10R10;
    case TEXFMT_A16B16G16R16: return D3DFMT_A16B16G16R16;
    case TEXFMT_A16B16G16R16F: return D3DFMT_A16B16G16R16F;
    case TEXFMT_A32B32G32R32F: return D3DFMT_A32B32G32R32F;
    case TEXFMT_G16R16: return D3DFMT_G16R16;
    case TEXFMT_L16: return D3DFMT_L16;
    case TEXFMT_A8: return D3DFMT_A8;
    case TEXFMT_R8: return D3DFMT_L8;
    case TEXFMT_G16R16F: return D3DFMT_G16R16F;
    case TEXFMT_G32R32F: return D3DFMT_G32R32F;
    case TEXFMT_R16F: return D3DFMT_R16F;
    case TEXFMT_R32F: return D3DFMT_R32F;
    case TEXFMT_DXT1: return D3DFMT_DXT1;
    case TEXFMT_DXT3: return D3DFMT_DXT3;
    case TEXFMT_DXT5: return D3DFMT_DXT5;
    case TEXFMT_A4R4G4B4: return D3DFMT_A4R4G4B4; // dxgi 1.2
    case TEXFMT_A1R5G5B5: return D3DFMT_A1R5G5B5;
    case TEXFMT_R5G6B5: return D3DFMT_R5G6B5;
    case TEXFMT_ATI1N: return _MAKE4C('ATI1');
    case TEXFMT_ATI2N: return _MAKE4C('ATI2');
    case TEXFMT_BC6H: return _MAKE4C('BC6H');
    case TEXFMT_BC7: return _MAKE4C('BC7 ');
  }
  G_ASSERTF(0, "can't convert tex format: %d", fmt);
  return D3DFMT_A8R8G8B8;
}
