//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <dag/dag_vector.h>

#include <ecs/anim/anim.h>
#include <daECS/core/componentTypes.h>

#include <outline/outline_buffer_size.hlsli>

enum OutlineMode
{
  ZTEST_FAIL,
  ZTEST_PASS,
  NO_ZTEST,
  OVERRIDES_COUNT
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineMode);

struct OutlineElement
{
  vec4f clipBox;
  E3DCOLOR int_color, ext_color;
  OutlineElement(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color);
  bool operator<(const OutlineElement &rhs) const;
};

struct OutlinedAnimchar : OutlineElement
{
  const AnimV20::AnimcharRendComponent *animchar;
  const ecs::Point4List *additionalData;
  OutlinedAnimchar(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, const AnimV20::AnimcharRendComponent *animchar,
    const ecs::Point4List *additionalData);
};

struct OutlinedRendinst : OutlineElement
{
  TMatrix transform;
  uint32_t riIdx;
  OutlinedRendinst(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, const TMatrix &transform, uint32_t ri_idx);
};

struct OutlineContext
{
  void addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox,
    const AnimV20::AnimcharRendComponent &animchar, const ecs::Point4List *additional_data);
  void addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox, const TMatrix &transform,
    uint32_t ri_idx);
  Point4 calculate_box() const;
  void clear();
  bool empty() const;

  dag::Vector<OutlinedAnimchar> animcharElements;
  dag::Vector<OutlinedRendinst> riElements;
};

struct OutlineContexts
{
  eastl::array<OutlineContext, OVERRIDES_COUNT> context;

  bool anyVisible() const;
  Point4 calculateBox() const;
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineContexts);

using ColorBuffer = eastl::fixed_vector<Color4, COLOR_BUF_SIZE>;
struct OutlineRenderer
{
  TMatrix4_vec4 tm;
  int clipLeft = 0, clipTop = 0, clipWidth = 0, clipHeight = 0;
  eastl::array<shaders::UniqueOverrideStateId, OVERRIDES_COUNT> zTestOverrides;

  int width = 0, height = 0;
  PostFxRenderer finalRender, fillDepthRender;
  UniqueBuf colorsCB;
  UniqueBuf rendInstTransforms;
  ColorBuffer colors;

  void resetColorBuffer();
  void uploadColorBufferToGPU();
  void updateScaleVar();
  void setClippedViewport();
  void calculateProjection(Point4 clipBox);
  shaders::OverrideStateId create_override(int override_type);
  void init(float outline_brightness);
  void changeResolution(int w, int h);
  void initBlurWidth(float outline_blur_width, int display_width, int display_height);
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineRenderer);

struct StencilColorState
{
  uint32_t prevIntColor = 0, prevExtColor = 0, stencil = 0;
};

// Helper function that iteraters across OutlineElements and batches elements with common int_color/ext_color.
template <class Elements, class AddElement, class FlushElements>
void render_outline_elements(Elements &elems, StencilColorState &stencil, ColorBuffer &colors, AddElement add, FlushElements flush)
{
  eastl::sort(elems.begin(), elems.end());

  bool hasPendingFlush = false;
  for (const auto &e : elems)
  {
    if (stencil.prevIntColor != e.int_color || stencil.prevExtColor != e.ext_color)
    {
      if (hasPendingFlush)
      {
        flush();
        hasPendingFlush = false;
      }
      colors.emplace_back(color4(e.int_color));
      colors.emplace_back(color4(e.ext_color));
      stencil.prevIntColor = e.int_color;
      stencil.prevExtColor = e.ext_color;
      stencil.stencil++;
    }
    if (!hasPendingFlush) // first draw with new stencil value.
      shaders::set_stencil_ref(stencil.stencil);

    add(e);
    hasPendingFlush = true;
  }
  if (hasPendingFlush)
    flush();
}

// This function should be called after animchar_before_render_es at UpdateStageInfoBeforeRender.
void outline_update_contexts(OutlineContexts &outline_ctxs, const TMatrix4 &globtm, const Occlusion *occlusion);

// This functions should be implemented in project-specific manner!
void outline_render_elements(OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs, const TMatrix &view_tm,
  const TMatrix4_vec4 &proj_tm);

// Requires:
// outline_render_elements to be implemented!
// Outline depth binded as depth.
// Gbuffer depth binded at shaderVar "depth_gbuf".
void outline_prepare(OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs, Texture *outline_depth, const TMatrix &view_tm,
  const TMatrix4_vec4 &proj_tm);

// Requires:
// Outline depth binded at shaderVar outline_depth
void outline_apply(OutlineRenderer &outline_renderer, Texture *outline_depth);
