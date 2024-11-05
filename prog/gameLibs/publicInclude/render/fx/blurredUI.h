//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resPtr.h>

class IBBox2;
class BlurredUI
{
public:
  void init(int w, int h, int mips, TextureIDPair intermediate);
  void initSdrTex(int w, int h, int mips);
  ~BlurredUI() { close(); }
  void close();

  void updateFinalBlurred(const TextureIDPair &src, const IBBox2 *begin, const IBBox2 *end, int max_mip, bool bw = false);
  void updateFinalBlurred(const TextureIDPair &src, const TextureIDPair &bkg, const IBBox2 *begin, const IBBox2 *end, int max_mip,
    bool bw = false);

  void updateSdrBackBufferTex();

  const TextureIDHolder &getFinal() const { return final; }
  const TextureIDHolder &getFinalSdr() const { return finalSdr; }
  d3d::SamplerHandle getFinalSampler() const { return finalSampler; }
  d3d::SamplerHandle getFinalSdrSampler() const { return finalSdrSampler; }

private:
  TextureIDHolder final;
  TextureIDHolder finalSdr;
  d3d::SamplerHandle finalSampler = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle finalSdrSampler = d3d::INVALID_SAMPLER_HANDLE;
  TextureIDPair intermediate;
  bool ownsIntermediate = false;
  TextureIDHolder intermediate2; // always owned
  ExternalTex sdrBackbufferTex;
  PostFxRenderer initialDownsample;
  PostFxRenderer initialDownsampleAndBlend;
  PostFxRenderer subsequentDownsample;
  PostFxRenderer blur;

  void setIntermediate(int w, int h, TextureIDPair tex);
  void closeIntermediate();
  void ensureIntermediate2(int w, int h, int mips);
};
