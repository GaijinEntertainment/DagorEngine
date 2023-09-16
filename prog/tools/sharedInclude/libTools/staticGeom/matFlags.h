// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_TOOLS_MATFLAGS_H
#define _GAIJIN_TOOLS_MATFLAGS_H
#pragma once


namespace MatFlags
{
enum
{
  FLG_2SIDED = 1 << 0,      /// both sides are light emitters
  FLG_REAL_2SIDED = 1 << 1, /// real2sided - one side is invisible
  FLG_USE_LM = 1 << 2,      /// lightmap, only one
  FLG_USE_VLM = 1 << 3,     /// vertex light, only one

  FLG_USE_IN_GI = 1 << 10, /// shadow

  FLG_GI_MUL_DIFF_COLOR = 1 << 11, /// vertex color
  FLG_GI_MUL_DIFF_TEX = 1 << 12,   /// diff * diff.texture.rgb

  FLG_GI_MUL_EMIS_ALPHA = 1 << 13, /// emission * diff.texture.alpha
  FLG_GI_MUL_EMIS_ANGLE = 1 << 14,
  FLG_GI_MUL_EMIS_TEX = 1 << 15, /// emission * diff.texture.rgb

  FLG_GI_MUL_TRANS_TEX = 1 << 16,   /// trans * diff.texture.rgb
  FLG_GI_MUL_TRANS_ALPHA = 1 << 17, /// trans * diff.texture.alpha
  FLG_GI_MUL_TRANS_INV_A = 1 << 18, /// trans *  1-diff.texture.alpha

  FLG_BILLBOARD_MASK = 3 << 19, /// billboard type mask

  FLG_BILLBOARD_NONE = 0 << 19,     /// no billboarding
  FLG_BILLBOARD_VERTICAL = 1 << 19, /// billboard rotating around vertical axis
  FLG_BILLBOARD_FACING = 2 << 19,   /// billboard fixed to view space orientation
};
};


namespace ObjFlags
{
enum
{
  FLG_CAST_SHADOWS = 1 << 0,               /// cast shadows
  FLG_CAST_SHADOWS_ON_SELF = 1 << 1,       /// cast shadows on self
  FLG_OBJ_FADE = 1 << 2,                   /// object fade
  FLG_OBJ_FADENULL = 1 << 3,               /// object completely fade
  FLG_OBJ_USE_VSS = 1 << 4,                /// calculates VLTMAP using VSS technology
  FLG_OBJ_DONT_USE_VSS = 1 << 5,           /// explicitly disable VSS technology
  FLG_BILLBOARD_MESH = 1 << 6,             /// object is billboard mesh - special mesh data preparation
  FLG_OCCLUDER = 1 << 7,                   /// invisible occluder
  FLG_BACK_FACE_DYNAMIC_LIGHTING = 1 << 8, /// small bumped objects will be lighted from all sides
  FLG_DO_NOT_MIX_LODS = 1 << 9,            /// do not mix LODs, switch thay simultaniously
};
};


#endif
