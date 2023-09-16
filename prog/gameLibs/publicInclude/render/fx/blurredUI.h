//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_textureIDHolder.h>

class IBBox2;
struct BlurredUI
{
  TextureIDHolder final;
  TextureIDPair intermediate;
  bool ownsIntermediate = false;
  TextureIDHolder intermediate2; // always owned
  PostFxRenderer initial_downsample, initial_downsample_and_blend, subsequent_downsample, blur;
  void setIntermediate(int w, int h, TextureIDPair tex);
  void closeIntermediate();
  void ensureIntermediate2(int w, int h, int mips);
  void init(int w, int h, int mips, TextureIDPair intermediate);
  void close();
  ~BlurredUI() { close(); }
  void updateFinalBlurred(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, int max_mip, bool bw = false);
  void updateFinalBlurred(const TextureIDPair &src, const TextureIDPair &bkg, const IBBox2 *begin, const IBBox2 *end, int max_mip,
    bool bw = false);
};
