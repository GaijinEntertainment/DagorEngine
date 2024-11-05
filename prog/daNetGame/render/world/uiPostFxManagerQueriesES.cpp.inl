// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/uiPostFxManager.h>
#include <drv/3d/dag_resId.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/samplerHandle.h>


ECS_DECLARE_RELOCATABLE_TYPE(TEXTUREID)

template <typename Callable>
static void get_ui_blur_texid_ecs_query(Callable);

template <typename Callable>
static void get_ui_blur_sdr_texid_ecs_query(Callable);

template <typename Callable>
static void get_ui_blur_sampler_ecs_query(Callable);

template <typename Callable>
static void get_ui_blur_sdr_sampler_ecs_query(Callable);

TEXTUREID UiPostFxManager::getUiBlurTexId()
{
  TEXTUREID blurTexId = BAD_TEXTUREID;
  get_ui_blur_texid_ecs_query([&](TEXTUREID &blurred_ui__texid) { blurTexId = blurred_ui__texid; });
  return blurTexId;
}

d3d::SamplerHandle UiPostFxManager::getUiBlurSampler()
{
  d3d::SamplerHandle smp = d3d::INVALID_SAMPLER_HANDLE;
  get_ui_blur_sampler_ecs_query([&](d3d::SamplerHandle blurred_ui__smp) { smp = blurred_ui__smp; });
  return smp;
}

TEXTUREID UiPostFxManager::getUiBlurSdrTexId()
{
  TEXTUREID blurSdrTexId = BAD_TEXTUREID;
  get_ui_blur_sdr_texid_ecs_query([&](TEXTUREID &blurred_ui_sdr__texid) { blurSdrTexId = blurred_ui_sdr__texid; });
  return blurSdrTexId;
}

d3d::SamplerHandle UiPostFxManager::getUiBlurSdrSampler()
{
  d3d::SamplerHandle smp = d3d::INVALID_SAMPLER_HANDLE;
  get_ui_blur_sdr_sampler_ecs_query([&](d3d::SamplerHandle blurred_ui_sdr__smp) { smp = blurred_ui_sdr__smp; });
  return smp;
}
