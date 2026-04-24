//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
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
  float depthDiffBlendStart, depthDiffBlendEnd;
  OutlineElement(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, float depthDiffBlendStart, float depthDiffBlendEnd);
  bool operator<(const OutlineElement &rhs) const;
};

struct OutlinedDynModel : OutlineElement
{
  const DynamicRenderableSceneInstance *dynModel;
  const ecs::Point4List *animcharAdditionalData;
  OutlinedDynModel(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, float depthDiffBlendStart, float depthDiffBlendEnd,
    const DynamicRenderableSceneInstance *dynModel, const ecs::Point4List *animcharAdditionalData);
};

struct OutlinedRendinst : OutlineElement
{
  TMatrix transform;
  uint32_t riIdx;
  OutlinedRendinst(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, float depthDiffBlendStart, float depthDiffBlendEnd,
    const TMatrix &transform, uint32_t ri_idx);
};

struct OutlinedBox : OutlineElement
{
  TMatrix transform;
  OutlinedBox(vec4f clipBox, E3DCOLOR int_color, E3DCOLOR ext_color, float depthDiffBlendStart, float depthDiffBlendEnd,
    const TMatrix &transform);
};

struct OutlineContext
{
  void addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox,
    const DynamicRenderableSceneInstance *dynModel, const ecs::Point4List *additional_data, float depthDiffBlendStart = 1,
    float depthDiffBlendEnd = 0);
  void addOutline(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, bbox3f_cref wbox, const TMatrix &transform,
    uint32_t ri_idx, float depthDiffBlendStart = 1, float depthDiffBlendEnd = 0);
  void addOutlineBox(E3DCOLOR int_color, E3DCOLOR ext_color, mat44f_cref globtm, const TMatrix &transform,
    float depthDiffBlendStart = 1, float depthDiffBlendEnd = 0);
  Point4 calculate_box() const;
  void clear();
  bool empty() const;

  dag::Vector<OutlinedDynModel> dynModelElements;
  dag::Vector<OutlinedRendinst> riElements;
  dag::Vector<OutlinedBox> boxElements;
};

struct OutlineContexts
{
  eastl::array<OutlineContext, OVERRIDES_COUNT> context;

  bool anyVisible() const;
  Point4 calculateBox() const;
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineContexts);

struct StencilState
{
  uint32_t stencil = 0;
};

using ColorBuffer = eastl::fixed_vector<eastl::array<Color4, 3>, COLOR_BUF_SIZE / 3>;
struct OutlineRenderer
{
  OutlineRenderer() = default;
  ~OutlineRenderer() { close(); }
  TMatrix4_vec4 tm;
  int clipLeft = 0, clipTop = 0, clipWidth = 0, clipHeight = 0;
  eastl::array<shaders::UniqueOverrideStateId, OVERRIDES_COUNT> zTestOverrides;

  int width = 0, height = 0;
  PostFxRenderer finalRender, fillDepthRender;
  UniqueBuf colorsCB;
  UniqueBuf rendInstTransforms;
  ColorBuffer colors;

  DynamicShaderHelper outlineBoxShader;
  bool inited = false;

  void resetColorBuffer();
  void uploadColorBufferToGPU();
  void updateScaleVar();
  void setClippedViewport(float dynamic_resolution_scale = 1);
  void calculateProjection(Point4 clipBox);
  shaders::OverrideStateId create_override(int override_type);
  void init(float outline_brightness);
  void close();
  void changeResolution(int w, int h);
  void initBlurWidth(float outline_blur_width, int display_width, int display_height);

  // Common render functions
  void renderOutlineBoxes(dag::Vector<OutlinedBox> &boxElements);
};

ECS_DECLARE_RELOCATABLE_TYPE(OutlineRenderer);

// Helper function that iteraters across OutlineElements and batches elements with common int_color/ext_color.
template <class Elements, class AddElement, class FlushElements>
void render_outline_elements(Elements &elems, ColorBuffer &colors, AddElement add, FlushElements flush)
{
  eastl::sort(elems.begin(), elems.end());

  uint32_t curStencil = -1;
  bool hasPendingFlush = false;
  for (const auto &e : elems)
  {
    eastl::array colorData = {color4(e.int_color), color4(e.ext_color), Color4(e.depthDiffBlendStart, e.depthDiffBlendEnd, 0, 0)};

    auto rIt = eastl::find(colors.rbegin(), colors.rend(), colorData);
    if (rIt == colors.rend())
    {
      colors.emplace_back(colorData);
      rIt = colors.rbegin();
    }
    int stencilReq = eastl::distance(colors.begin(), rIt.base()) - 1;

    if (curStencil != stencilReq)
    {
      if (hasPendingFlush)
      {
        flush();
        hasPendingFlush = false;
      }
      curStencil = stencilReq;
    }

    if (!hasPendingFlush) // first draw with new stencil value.
      shaders::set_stencil_ref(curStencil);

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
void outline_apply(OutlineRenderer &outline_renderer, Texture *outline_depth, float dynamic_resolution_scale = 1);
