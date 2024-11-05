// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/bfg.h>
#include <render/denoiser.h>

#include <shaders/dag_computeShaders.h>

class PostFxRenderer;

class SSRDenoiser final
{
public:
  SSRDenoiser(uint32_t w, uint32_t h, bool halfRes);
  ~SSRDenoiser() { denoiser::teardown(); }
  void setShaderVars() const;
  void drawValidation();

#if DAGOR_DBGLEVEL > 0
  void drawImguiWindow();
#endif

  dabfg::NodeHandle makeDenoiserPrepareNode() const;
  dabfg::NodeHandle makeDenoiseUnpackNode() const;
  dabfg::NodeHandle makeDenoiseNode() const;

  Point4 getHitDistParams() const { return Point4(gloss_ray_length, distance_factor, scatter_factor, roughness_factor); }
  denoiser::ReflectionMethod getReflectionMethod() const { return output_type; }

private:
  eastl::unique_ptr<PostFxRenderer> validation_renderer;
  eastl::unique_ptr<ComputeShaderElement> denoisedUnpackCS;
  denoiser::ReflectionMethod output_type = denoiser::ReflectionMethod::Reblur;
  UniqueTex denoised_reflection;
  UniqueTex decoded_denoised_reflection;
  UniqueTex validation_texture;
  uint32_t width, height = 0;
  float water_ray_length = 5000;
  float gloss_ray_length = 1000;
  float rough_ray_length = 1000;
  float distance_factor = 0;
  float scatter_factor = 1;
  float roughness_factor = 0;
  bool show_validation = false;
  bool performance_mode = true;
  bool halfRes = false;
};

void teardown_ssr_denoiser();
SSRDenoiser *init_ssr_denoiser(int w, int h, bool isHalfRes);
SSRDenoiser *get_ssr_denoiser();