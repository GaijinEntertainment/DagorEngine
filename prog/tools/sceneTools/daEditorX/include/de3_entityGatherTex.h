//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class TextureIdSet;

class IEntityGatherTex
{
public:
  static constexpr unsigned HUID = 0x96E6D402u; // IEntityGatherTex

  virtual int gatherTextures(TextureIdSet &out_tex_id, int for_lod = -1) = 0;
};
