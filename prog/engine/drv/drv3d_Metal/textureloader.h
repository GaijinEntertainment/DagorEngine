// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include "render.h"

enum TextureFormat
{
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_A8 = 28,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_G16R16 = 34,
  D3DFMT_A16B16G16R16 = 36,

  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,

  D3DFMT_DXT1 = _MAKE4C('DXT1'),
  D3DFMT_DXT3 = _MAKE4C('DXT3'),
  D3DFMT_DXT5 = _MAKE4C('DXT5'),

  D3DFMT_L16 = 81,

  // Floating point surface formats

  // s10e5 formats (16-bits per channel)
  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,

  // IEEE s23e8 formats (32-bits per channel)
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,

  D3DFMT_ATI1 = _MAKE4C('ATI1'),
  D3DFMT_ATI2 = _MAKE4C('ATI2'),
  D3DFMT_BC6H = _MAKE4C('BC6H'),
  D3DFMT_BC7 = _MAKE4C('BC7'),

  D3DFMT_ASTC4 = _MAKE4C('AST4'),
  D3DFMT_ASTC8 = _MAKE4C('AST8'),
  D3DFMT_ASTC12 = _MAKE4C('ASTC')
};
