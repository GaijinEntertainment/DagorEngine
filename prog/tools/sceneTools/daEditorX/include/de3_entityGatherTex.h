//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class TextureIdSet;

class IEntityGatherTex
{
public:
  static constexpr unsigned HUID = 0x96E6D402u; // IEntityGatherTex

  virtual int gatherTextures(TextureIdSet &out_tex_id, int for_lod = -1) = 0;
};
