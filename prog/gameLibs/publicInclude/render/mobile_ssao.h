//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "ssao.h"

// Following class implements the most lightweight algorithm that was
// present in dagor before this class was added
// It's done completely in screen space (sampling in depth based radius)
class MobileSSAORenderer : public ISSAORenderer
{
public:
  using ISSAORenderer::render;

  MobileSSAORenderer() = delete;
  MobileSSAORenderer(int w, int h, int num_views, uint32_t flags = SSAO_NONE);
  ~MobileSSAORenderer();

  void reset() override{};

  void setCurrentView(int) override;

  Texture *getSSAOTex() override;
  TEXTUREID getSSAOTexId() override;

private:
  void render(BaseTexture *depth_tex_to_use, const ManagedTex *ssaoTex, const ManagedTex *prevSsaoTex, const ManagedTex *tmpTex,
    const DPoint3 *, SubFrameSample sub_sample = SubFrameSample::Single) override;

  void setFrameNo();
  void renderSSAO(BaseTexture *depth_tex_to_use);
  void applyBlur();

  void setFactor();

  static constexpr const char *ssao_sh_name = "mobile_ssao";
  static constexpr const char *blur_sh_name = "mobile_ssao_blur";

  static constexpr int renderId = 0;
  static constexpr int blurId = 1;

  // To apply blur we have to have at least 1 extra texture to store
  // intermediate result
  eastl::array<UniqueTex, 2> ssaoTex;

  eastl::unique_ptr<PostFxRenderer> ssaoBlurRenderer{nullptr};
};
