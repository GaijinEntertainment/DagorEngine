//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/************************************************************************
  shader variable types
************************************************************************/

enum ShaderVarType
{
  SHVT_INT,
  SHVT_REAL,
  SHVT_COLOR4,
  SHVT_TEXTURE,
  SHVT_BUFFER,
  SHVT_INT4,
  SHVT_FLOAT4X4,
  SHVT_SAMPLER,
  SHVT_TLAS,
};

enum ShaderVarTextureType : int
{
  SHVT_TEX_UNKNOWN,
  SHVT_TEX_2D,
  SHVT_TEX_3D,
  SHVT_TEX_CUBE,
  SHVT_TEX_2D_ARRAY,
  SHVT_TEX_CUBE_ARRAY,
};
