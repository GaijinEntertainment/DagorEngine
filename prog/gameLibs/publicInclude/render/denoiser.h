//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_textureIDHolder.h>

namespace denoiser
{

struct FrameParams
{
  Point3 viewPos = Point3::ZERO;
  Point3 prevViewPos = Point3::ZERO;
  Point3 viewDir = Point3::ZERO;
  Point3 prevViewDir = Point3::ZERO;
  TMatrix4 viewItm = TMatrix4::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT;
  TMatrix4 prevViewItm = TMatrix4::IDENT;
  TMatrix4 prevProjTm = TMatrix4::IDENT;
  Point2 jitter = Point2::ZERO;
  Point2 prevJitter = Point2::ZERO;
  Point3 motionMultiplier = Point3::ZERO;
  BaseTexture *motionVectors = nullptr;
  BaseTexture *halfMotionVectors = nullptr;
  TextureIDPair halfNormals;
  bool reset = false;
};

enum class ReflectionMethod
{
  Reblur,
  Relax,
};

void initialize(int width, int height);
void teardown();

void prepare(const FrameParams &params);

void make_shadow_maps(UniqueTex &shadow_value, UniqueTex &denoised_shadow);
void make_shadow_maps(UniqueTex &shadow_value, UniqueTex &shadow_translucency, UniqueTex &denoised_shadow);

void make_ao_maps(UniqueTex &ao_value, UniqueTex &denoised_ao, bool half_res);

void make_reflection_maps(UniqueTex &reflection_value, UniqueTex &denoised_reflection, ReflectionMethod method, bool half_res);

void get_memory_stats(int &common_size_mb, int &ao_size_mb, int &reflection_size_mb, int &transient_size_mb);

struct ShadowDenoiser
{
  Texture *denoisedShadowMap = nullptr;
  Texture *shadowValue = nullptr;
  Texture *shadowTranslucency = nullptr;
  Texture *csmTexture = nullptr;
  d3d::SamplerHandle csmSampler = d3d::INVALID_SAMPLER_HANDLE;
};

struct AODenoiser
{
  Point4 hitDistParams = Point4::ZERO;
  Point4 antilagSettings = Point4(2, 2, 1, 1);
  Texture *denoisedAo = nullptr;
  Texture *aoValue = nullptr;
  bool performanceMode = true;
};

struct ReflectionDenoiser
{
  Point4 hitDistParams = Point4::ZERO;
  Point4 antilagSettings = Point4(2, 2, 1, 1);
  Texture *denoisedReflection = nullptr;
  Texture *reflectionValue = nullptr;
  Texture *validationTexture = nullptr;
  bool antiFirefly = false;
  bool performanceMode = true;
  bool checkerboard = true;
  bool highSpeedMode = false;
  bool useNRDLib = false;
};

void denoise_shadow(const ShadowDenoiser &params);
void denoise_ao(const AODenoiser &params);
void denoise_reflection(const ReflectionDenoiser &params);

int get_frame_number();

TEXTUREID get_nr_texId(int w);
TEXTUREID get_viewz_texId(int w);

} // namespace denoiser