// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_Point3.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>
#include <3d/dag_textureIDHolder.h>
#include "rendererFeatures.h"
#include <3d/dag_texStreamingContext.h>
#include <ecs/render/transformHolder.h>
#include <render/heroData.h>
#include <render/fx/fx.h>
#include <render/daBfg/bfg.h>
#include <ioSys/dag_dataBlock.h>


class Occlusion;
// all render events are called with broadcastImmediate. It is just generalized update stage.
struct UpdateBlurredUI : public ecs::Event
{
  const IBBox2 *begin;
  const IBBox2 *end;
  int max_mip;
  ECS_BROADCAST_EVENT_DECL(UpdateBlurredUI)
  UpdateBlurredUI(const IBBox2 *begin, const IBBox2 *end, int max_mip) :
    ECS_EVENT_CONSTRUCTOR(UpdateBlurredUI), begin(begin), end(end), max_mip(max_mip)
  {}
};
struct OnLevelLoaded : public ecs::Event
{
  const DataBlock &level_blk;
  ECS_BROADCAST_EVENT_DECL(OnLevelLoaded)
  OnLevelLoaded(const DataBlock &lev_blk) : ECS_EVENT_CONSTRUCTOR(OnLevelLoaded), level_blk(lev_blk) {}
};
struct BeforeDraw : public ecs::Event
{
  Driver3dPerspective persp;
  Frustum frustum;
  Point3 camPos;
  float dt;
  ECS_BROADCAST_EVENT_DECL(BeforeDraw)
  BeforeDraw(const Driver3dPerspective &persp, const Frustum &frustum, const Point3 &cam_pos, float dt) :
    ECS_EVENT_CONSTRUCTOR(BeforeDraw), persp(persp), frustum(frustum), camPos(cam_pos), dt(dt)
  {}
};
struct SetResolutionEvent : public ecs::Event
{
  enum class Type
  {
    SETTINGS_CHANGED,
    DYNAMIC_RESOLUTION
  } type;
  IPoint2 displayResolution, renderingResolution, postFxResolution;
  ECS_BROADCAST_EVENT_DECL(SetResolutionEvent)
  SetResolutionEvent(Type type, const IPoint2 &dr, const IPoint2 &rr, const IPoint2 &pr) :
    ECS_EVENT_CONSTRUCTOR(SetResolutionEvent), type(type), displayResolution(dr), renderingResolution(rr), postFxResolution(pr)
  {}
};

struct ChangeRenderFeatures : public ecs::Event
{
  FeatureRenderFlagMask newFeatureFlags;
  FeatureRenderFlagMask changedFeatureFlags;
  bool hasFeature(FeatureRenderFlags f) const { return newFeatureFlags.test(f); }
  bool isFeatureChanged(FeatureRenderFlags f) const { return changedFeatureFlags.test(f); }
  ECS_BROADCAST_EVENT_DECL(ChangeRenderFeatures)
  ChangeRenderFeatures(const FeatureRenderFlagMask &new_feature_flags, const FeatureRenderFlagMask &changed_feature_flags) :
    ECS_EVENT_CONSTRUCTOR(ChangeRenderFeatures), newFeatureFlags(new_feature_flags), changedFeatureFlags(changed_feature_flags)
  {}
};

struct SetFxQuality : public ecs::Event
{
  FxQuality fxQualityConfig;
  ECS_BROADCAST_EVENT_DECL(SetFxQuality)
  SetFxQuality(FxQuality fx_quality_config) : ECS_EVENT_CONSTRUCTOR(SetFxQuality), fxQualityConfig(fx_quality_config) {}
};

struct BeforeDrawPostFx : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(BeforeDrawPostFx)
  BeforeDrawPostFx() : ECS_EVENT_CONSTRUCTOR(BeforeDrawPostFx) {}
};

struct RenderPostFx : public ecs::Event
{
  TextureIDPair downsampledColor, prevRTColor;
  TextureIDPair closedDepth, targetDepth;
  float zNear, zFar, fovScale;
  ECS_BROADCAST_EVENT_DECL(RenderPostFx)
  RenderPostFx(TextureIDPair downsampled_color,
    TextureIDPair prev_rt_color,
    TextureIDPair closed_depth,
    TextureIDPair target_depth,
    float z_near,
    float z_far,
    float fov_scale) :
    ECS_EVENT_CONSTRUCTOR(RenderPostFx),
    downsampledColor(downsampled_color),
    prevRTColor(prev_rt_color),
    closedDepth(closed_depth),
    targetDepth(target_depth),
    zNear(z_near),
    zFar(z_far),
    fovScale(fov_scale)
  {}
};

struct AfterRenderPostFx : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(AfterRenderPostFx)
  AfterRenderPostFx() : ECS_EVENT_CONSTRUCTOR(AfterRenderPostFx) {}
};

struct AfterRenderWorld : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(AfterRenderWorld)
  Driver3dPerspective persp;
  AfterRenderWorld(const Driver3dPerspective &persp) : ECS_EVENT_CONSTRUCTOR(AfterRenderWorld), persp(persp) {}
};

struct UpdateEffectRestrictionBoxes : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(UpdateEffectRestrictionBoxes)
  UpdateEffectRestrictionBoxes() : ECS_EVENT_CONSTRUCTOR(UpdateEffectRestrictionBoxes) {}
};

struct OcclusionExclusion : public ecs::Event
{
  bool rendered = false;
  TMatrix viewTm;
  ECS_BROADCAST_EVENT_DECL(OcclusionExclusion)
  OcclusionExclusion(const TMatrix &view_tm) : ECS_EVENT_CONSTRUCTOR(OcclusionExclusion), viewTm(view_tm) {}
};


struct RenderHmapDeform : public ecs::Event
{
  vec4f hmapDeformRect;
  vec4f negRoundedCamPos, negRemainderCamPos;
  //-(roundedCamPos+remainderCamPos) - same as camPos, but more precise, as calculated in doubles!

  TMatrix viewTm;
  TMatrix viewItm;
  Point3 mainCamPos;
  ECS_BROADCAST_EVENT_DECL(RenderHmapDeform)
  RenderHmapDeform(const Point4 &hmap_deform_rect,
    vec4f neg_rounded_cam_pos,
    vec4f neg_remainder_cam_pos,
    const TMatrix &view_tm,
    const TMatrix &itm,
    const Point3 &main_cam) :
    ECS_EVENT_CONSTRUCTOR(RenderHmapDeform),
    viewTm(view_tm),
    viewItm(itm),
    mainCamPos(main_cam),
    negRoundedCamPos(neg_rounded_cam_pos),
    negRemainderCamPos(neg_remainder_cam_pos)
  {
    hmapDeformRect = v_make_vec4f(hmap_deform_rect.z, hmap_deform_rect.w, -hmap_deform_rect.x, -hmap_deform_rect.y);
  }
};

struct VehicleCockpitPrepass : public ecs::Event
{
  TMatrix viewTm;
  ECS_BROADCAST_EVENT_DECL(VehicleCockpitPrepass)
  VehicleCockpitPrepass(const TMatrix &view_tm) : ECS_EVENT_CONSTRUCTOR(VehicleCockpitPrepass), viewTm(view_tm) {}
};

struct RenderReinitCube : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(RenderReinitCube)
  RenderReinitCube() : ECS_EVENT_CONSTRUCTOR(RenderReinitCube) {}
};

struct RenderSetExposure : public ecs::Event
{
  bool value;
  ECS_BROADCAST_EVENT_DECL(RenderSetExposure)
  RenderSetExposure(bool value) : ECS_EVENT_CONSTRUCTOR(RenderSetExposure), value(value) {}
};

namespace dynmodel_renderer
{
struct DynModelRenderingState;
}
class AnimCharShadowOcclusionManager;
class Occlusion;
class GlobalVariableStates;

enum AnimcharRenderAsyncFilter : int8_t
{
  ARF_ANY_IDX = -1,
  ARF_ONLY_IDX0,
  ARF_ONLY_IDX1,
  ARF_ONLY_IDX2,
  ARF_ONLY_IDX3,
  ARF_IDX_COUNT,
  ARF_IDX_MASK = ARF_IDX_COUNT - 1,
};
static_assert(is_pow2(AnimcharRenderAsyncFilter::ARF_IDX_COUNT), "AnimcharRenderAsyncFilter::EACH_ENTITY should be pow of 2");

struct AnimcharRenderAsyncEvent : public ecs::Event
{
  dynmodel_renderer::DynModelRenderingState &state;
  const GlobalVariableStates *globVarsState;
  const Occlusion *occlusion;
  const Frustum cullingFrustum;
  const uint8_t add_vis_bits, check_bits, filterMask;
  const bool needPrevious;
  const AnimcharRenderAsyncFilter eidFilter;
  TexStreamingContext texCtx;
  ECS_INSIDE_EVENT_DECL(AnimcharRenderAsyncEvent, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  AnimcharRenderAsyncEvent(dynmodel_renderer::DynModelRenderingState &state_,
    const GlobalVariableStates *gvars_state,
    const Occlusion *occlusion_,
    const Frustum &frustum_,
    // uint32_t hints_,
    uint8_t add_vis_bits_,
    uint8_t check_bits_,
    uint8_t filter_mask,
    bool needPrevious_,
    AnimcharRenderAsyncFilter eid_filter,
    TexStreamingContext tex_context = TexStreamingContext(0)) :
    ECS_EVENT_CONSTRUCTOR(AnimcharRenderAsyncEvent),
    state(state_),
    globVarsState(gvars_state),
    occlusion(occlusion_),
    cullingFrustum(frustum_),
    // hints(hints_),
    add_vis_bits(add_vis_bits_),
    check_bits(check_bits_),
    filterMask(filter_mask),
    needPrevious(needPrevious_),
    eidFilter(eid_filter),
    texCtx(tex_context)
  {}
};

struct RenderLateTransEvent : public ecs::Event
{
  TMatrix viewTm;
  Point3 cameraWorldPos;
  TexStreamingContext texCtx;
  ManagedTexView prevFrameTex;
  ECS_BROADCAST_EVENT_DECL(RenderLateTransEvent)
  RenderLateTransEvent(
    const TMatrix &view_tm, const Point3 &camera_world_pos, const TexStreamingContext &tex_ctx, ManagedTexView prev_frame_tex) :
    viewTm(view_tm),
    cameraWorldPos(camera_world_pos),
    texCtx(tex_ctx),
    prevFrameTex(prev_frame_tex),
    ECS_EVENT_CONSTRUCTOR(RenderLateTransEvent)
  {}
};

struct QueryUnexpectedAltitudeChange : public ecs::Event
{
  bool enabled = false;
  ECS_BROADCAST_EVENT_DECL(QueryUnexpectedAltitudeChange)
  QueryUnexpectedAltitudeChange() : ECS_EVENT_CONSTRUCTOR(QueryUnexpectedAltitudeChange) {}
};

struct ResetAoEvent : public ecs::Event
{
  IPoint2 aoResolution;
  enum State
  {
    INIT,
    CLOSE
  } state;
  ECS_BROADCAST_EVENT_DECL(ResetAoEvent)
  ResetAoEvent(IPoint2 ao_resolution, State state) : ECS_EVENT_CONSTRUCTOR(ResetAoEvent), aoResolution(ao_resolution), state(state) {}
};

struct UpdateStageInfoRenderDistortion : public ecs::Event, public TransformHolder
{
  TMatrix viewItm;
  TexStreamingContext texCtx;
  ECS_BROADCAST_EVENT_DECL(UpdateStageInfoRenderDistortion);
  UpdateStageInfoRenderDistortion(
    const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &itm, const TexStreamingContext &tex_ctx) :
    ECS_EVENT_CONSTRUCTOR(UpdateStageInfoRenderDistortion), TransformHolder(view_tm, proj_tm), viewItm(itm), texCtx(tex_ctx)
  {}
};

struct QueryHeroWtmAndBoxForRender : public ecs::Event, public HeroWtmAndBox
{
  ECS_UNICAST_EVENT_DECL(QueryHeroWtmAndBoxForRender)
  QueryHeroWtmAndBoxForRender(bool weap_only = false) : ECS_EVENT_CONSTRUCTOR(QueryHeroWtmAndBoxForRender) { onlyWeapons = weap_only; }
};
static_assert(alignof(QueryHeroWtmAndBoxForRender) < sizeof(vec4f)); // Cause das-aot can't provide vector alignment


#define DEF_RENDER_EVENTS                                                                                                            \
  DEF_RENDER_EVENT(RenderEventUI, TMatrix /* viewTm */, TMatrix /* viewItm */, mat44f /* globtm */, Driver3dPerspective /* persp */) \
  DEF_RENDER_EVENT(RenderEventAfterUI, TMatrix /* viewTm */, TMatrix /* viewItm */, mat44f /* globtm */,                             \
    Driver3dPerspective /* persp */)                                                                                                 \
  DEF_RENDER_EVENT(RenderDebugWithJitter)                                                                                            \
  DEF_RENDER_EVENT(RenderEventDebugGUI)                                                                                              \
  DEF_RENDER_EVENT(UnloadLevel)                                                                                                      \
  DEF_RENDER_EVENT(OnRenderDecals, TMatrix /* viewTm */, TMatrix4 /* projTm */, Point3 /* cameraWorldPos */)                         \
  DEF_RENDER_EVENT(RenderDecalsOnDynamic)                                                                                            \
  DEF_RENDER_EVENT(RenderDecalsOnGlass)                                                                                              \
  DEF_RENDER_EVENT(AfterDeviceReset, bool /*fullReset*/)                                                                             \
  DEF_RENDER_EVENT(RegisterPostfxResources, dabfg::Registry)                                                                         \
  DEF_RENDER_EVENT(AfterShaderReload)


#define DEF_RENDER_EVENT ECS_BROADCAST_EVENT_TYPE
DEF_RENDER_EVENTS
#undef DEF_RENDER_EVENT

ECS_UNICAST_EVENT_TYPE(CustomSkyRender)
ECS_BROADCAST_EVENT_TYPE(CustomDmPanelRender, RectInt /*rect*/)
// this event ensures that WorldRenderer is created
ECS_BROADCAST_EVENT_TYPE(BeforeLoadLevel)
ECS_BROADCAST_EVENT_TYPE(SkiesLoaded)
ECS_BROADCAST_EVENT_TYPE(InvalidateClipmapBox, BBox2 /*box*/);
ECS_BROADCAST_EVENT_TYPE(InvalidateBoxAfterHeightmapChange, BBox3 /*box*/);
ECS_BROADCAST_EVENT_TYPE(OnClipmapTileRender, Frustum /*frustum*/);
ECS_BROADCAST_EVENT_TYPE(AfterHeightmapChange);

class TextureIDPair;
ECS_UNICAST_EVENT_TYPE(CustomEnviProbeRender, const ManagedTex *, int)
ECS_BROADCAST_EVENT_TYPE(CustomEnviProbeGetSphericalHarmonics, eastl::vector<Color4>)
ECS_BROADCAST_EVENT_TYPE(CustomEnviProbeLogSphericalHarmonics, const Color4 *)
