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
#include <ecs/render/renderPasses.h>
#include <render/heroData.h>
#include <render/fx/fx.h>
#include <render/daFrameGraph/daFG.h>
#include <ioSys/dag_dataBlock.h>
#include <daECS/core/componentTypes.h>
#include <generic/dag_fixedMoveOnlyFunction.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/render/renderEvent.h>
#include <render/dynmodelRenderer.h>
#include <render/resourceSlot/nodeHandleWithSlotsAccess.h>

struct RiGenVisibility;
struct CameraParams;
class Occlusion;
// all render events are called with broadcastImmediate. It is just generalized update stage.

class CameraNodesRegistratorStorage
{
  eastl::vector<dafg::NodeHandle> *nodes;
  uint32_t offset;
  uint32_t oldReservedSpace;
  uint32_t written = 0;
  bool hasOverflow = false;

public:
  CameraNodesRegistratorStorage(eastl::vector<dafg::NodeHandle> &nodes_storage, uint32_t range_offset, uint32_t range_size) :
    nodes(&nodes_storage), offset(range_size ? range_offset : uint32_t(nodes_storage.size())), oldReservedSpace(range_size)
  {}

  void push_back(dafg::NodeHandle &&node)
  {
    const bool hasReservedSpace = !hasOverflow && oldReservedSpace > 0;

    if (hasReservedSpace)
    {
      if (written < oldReservedSpace)
        (*nodes)[offset + written] = eastl::move(node);
      else
      {
        const uint32_t newOffset = (uint32_t)nodes->size();

        for (uint32_t i = 0; i < written; ++i)
        {
          nodes->push_back();
          nodes->back() = eastl::move((*nodes)[offset + i]);
        }
        nodes->push_back(eastl::move(node));

        hasOverflow = true;
        offset = newOffset;
      }
    }
    else
      nodes->push_back(eastl::move(node));

    ++written;
  }

  void clearUnused()
  {
    for (uint32_t i = written; i < oldReservedSpace; ++i)
      (*nodes)[offset + i] = {};
  }

  uint32_t getWritten() const { return written; }
  uint32_t getOffset() const { return offset; }
  bool isOverflowed() const { return hasOverflow; }
};

struct OnCameraNodeConstruction : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(OnCameraNodeConstruction)
  OnCameraNodeConstruction(eastl::vector<dafg::NodeHandle> *nodes_storage,
    const bool has_opaque_prepass,
    const bool gi_needs_reprojection,
    const bool need_depth_history,
    const bool is_bare_minimum,
    const bool has_motion_vectors) :
    ECS_EVENT_CONSTRUCTOR(OnCameraNodeConstruction),
    nodes(nodes_storage),
    hasOpaquePrepass(has_opaque_prepass),
    giNeedsReprojection(gi_needs_reprojection),
    needDepthHistory(need_depth_history),
    isBareMinimum(is_bare_minimum),
    hasMotionVectors(has_motion_vectors)
  {}
  eastl::vector<dafg::NodeHandle> *nodes;

  bool hasOpaquePrepass = true;
  bool giNeedsReprojection = true;
  bool needDepthHistory = true;
  bool isBareMinimum = false;
  bool hasMotionVectors = true;
};

struct OnCameraNodeWithSlotsConstruction : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(OnCameraNodeWithSlotsConstruction)
  OnCameraNodeWithSlotsConstruction(eastl::vector<resource_slot::NodeHandleWithSlotsAccess> *slot_nodes_storage) :
    ECS_EVENT_CONSTRUCTOR(OnCameraNodeWithSlotsConstruction), slotNodes(slot_nodes_storage)
  {}
  eastl::vector<resource_slot::NodeHandleWithSlotsAccess> *slotNodes;
};

struct OnCameraMainViewNodeConstruction : public ecs::Event
{
  ECS_UNICAST_EVENT_DECL(OnCameraMainViewNodeConstruction)
  OnCameraMainViewNodeConstruction(const char *view_ns_name, CameraNodesRegistratorStorage *nodes_storage, bool has_opaque_prepass) :
    ECS_EVENT_CONSTRUCTOR(OnCameraMainViewNodeConstruction),
    viewNsName(view_ns_name),
    nodes(nodes_storage),
    hasOpaquePrepass(has_opaque_prepass)
  {}
  const char *viewNsName;
  CameraNodesRegistratorStorage *nodes;
  bool hasOpaquePrepass;
};

struct OnCameraPerViewNodeConstruction : public ecs::Event
{
  ECS_UNICAST_EVENT_DECL(OnCameraPerViewNodeConstruction)
  OnCameraPerViewNodeConstruction(
    bool is_main_view, const char *view_ns_name, CameraNodesRegistratorStorage *nodes_storage, bool has_opaque_prepass) :
    ECS_EVENT_CONSTRUCTOR(OnCameraPerViewNodeConstruction),
    isMainView(is_main_view),
    viewNsName(view_ns_name),
    nodes(nodes_storage),
    hasOpaquePrepass(has_opaque_prepass)
  {}
  bool isMainView;
  const char *viewNsName;
  CameraNodesRegistratorStorage *nodes;
  bool hasOpaquePrepass;
};

struct QueryShooterCamDistanceMultipliers : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(QueryShooterCamDistanceMultipliers)
  QueryShooterCamDistanceMultipliers(float *ri_distance_mul, float *impostor_distance_mul) :
    ECS_EVENT_CONSTRUCTOR(QueryShooterCamDistanceMultipliers), riDistanceMul(ri_distance_mul), impostorDistMul(impostor_distance_mul)
  {}
  float *riDistanceMul;
  float *impostorDistMul;
};

// Broadcast right before the world's framegraph multiplexing extents are
// finalized, seeded with the renderer's defaults. A feature may raise any
// dimension's extent to multiplex extra passes over it (e.g. the AA benchmark
// iterating ground-truth accumulation over the sub-sample dimension). A consumer
// should only override a dimension left unused by the renderer (extent == 1).
// TODO: make screenshot nodes use this override mechanism instead of the current approach
struct QueryMultiplexingExtents : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(QueryMultiplexingExtents)
  QueryMultiplexingExtents(dafg::multiplexing::Extents *extents) : ECS_EVENT_CONSTRUCTOR(QueryMultiplexingExtents), extents(extents) {}
  dafg::multiplexing::Extents *extents;
};

struct UpdateBlurredUI : public ecs::Event
{
  const IBBox2 *begin;
  const IBBox2 *end;
  int max_mip;
  BaseTexture *uiTex;
  ECS_BROADCAST_EVENT_DECL(UpdateBlurredUI)
  UpdateBlurredUI(const IBBox2 *begin, const IBBox2 *end, int max_mip, BaseTexture *ui_tex) :
    ECS_EVENT_CONSTRUCTOR(UpdateBlurredUI), begin(begin), end(end), max_mip(max_mip), uiTex(ui_tex)
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
  IPoint2 displayResolution, renderingResolution, postFxResolution, maxPossibleRenderResolution;
  ECS_BROADCAST_EVENT_DECL(SetResolutionEvent)
  SetResolutionEvent(Type type, const IPoint2 &dr, const IPoint2 &rr, const IPoint2 &pr, const IPoint2 &mpr) :
    ECS_EVENT_CONSTRUCTOR(SetResolutionEvent),
    type(type),
    displayResolution(dr),
    renderingResolution(rr),
    postFxResolution(pr),
    maxPossibleRenderResolution(mpr)
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
  BaseTexture *downsampledColor;
  BaseTexture *prevRTColor;
  BaseTexture *closedDepth;
  BaseTexture *targetDepth;
  float zNear, zFar, fovScale;
  ECS_BROADCAST_EVENT_DECL(RenderPostFx)
  RenderPostFx(BaseTexture *downsampled_color,
    BaseTexture *prev_rt_color,
    BaseTexture *closed_depth,
    BaseTexture *target_depth,
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

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

struct GatherSplinegenBVHDataEvent : public ecs::Event
{
  bvh::ContextId contextId;

  ECS_BROADCAST_EVENT_DECL(GatherSplinegenBVHDataEvent)
  GatherSplinegenBVHDataEvent(bvh::ContextId contextId) : ECS_EVENT_CONSTRUCTOR(GatherSplinegenBVHDataEvent), contextId(contextId) {}
};

struct BVHConnection;
struct GatherSmokeTracersBVHDataEvent : public ecs::Event
{
  BVHConnection *connection;

  ECS_BROADCAST_EVENT_DECL(GatherSmokeTracersBVHDataEvent)
  GatherSmokeTracersBVHDataEvent(BVHConnection *connection) :
    ECS_EVENT_CONSTRUCTOR(GatherSmokeTracersBVHDataEvent), connection(connection)
  {}
};

struct RemoveSplinegenBVHEvent : public ecs::Event
{
  ECS_BROADCAST_EVENT_DECL(RemoveSplinegenBVHEvent)
  RemoveSplinegenBVHEvent() : ECS_EVENT_CONSTRUCTOR(RemoveSplinegenBVHEvent) {}
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
  TexStreamingContext texCtx;
  ECS_BROADCAST_EVENT_DECL(VehicleCockpitPrepass)
  VehicleCockpitPrepass(const TMatrix &view_tm, const TexStreamingContext &tex_ctx) :
    ECS_EVENT_CONSTRUCTOR(VehicleCockpitPrepass), viewTm(view_tm), texCtx(tex_ctx)
  {}
};

struct RenderSetExposure : public ecs::Event
{
  bool value;
  ECS_BROADCAST_EVENT_DECL(RenderSetExposure)
  RenderSetExposure(bool value) : ECS_EVENT_CONSTRUCTOR(RenderSetExposure), value(value) {}
};

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
  dynrend::ContextId ctx;
  const GlobalVariableStates *globVarsState;
  const Occlusion *occlusion;
  const Frustum cullingFrustum;
  const animchar_visbits_t add_vis_bits, check_bits;
  const uint8_t filterMask;
  const bool needPrevious;
  const AnimcharRenderAsyncFilter eidFilter;
  TexStreamingContext texCtx;
  ECS_INSIDE_EVENT_DECL(AnimcharRenderAsyncEvent, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  AnimcharRenderAsyncEvent(dynrend::ContextId ctx_,
    const GlobalVariableStates *gvars_state,
    const Occlusion *occlusion_,
    const Frustum &frustum_,
    animchar_visbits_t add_vis_bits_,
    animchar_visbits_t check_bits_,
    uint8_t filter_mask,
    bool needPrevious_,
    AnimcharRenderAsyncFilter eid_filter,
    TexStreamingContext tex_context = TexStreamingContext(0)) :
    ECS_EVENT_CONSTRUCTOR(AnimcharRenderAsyncEvent),
    ctx(ctx_),
    globVarsState(gvars_state),
    occlusion(occlusion_),
    cullingFrustum(frustum_),
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
  Texture *prevFrameTex;
  ECS_BROADCAST_EVENT_DECL(RenderLateTransEvent)
  RenderLateTransEvent(
    const TMatrix &view_tm, const Point3 &camera_world_pos, const TexStreamingContext &tex_ctx, Texture *prev_frame_tex) :
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

struct UpdateStageInfoNeedDistortion : public ecs::Event, public TransformHolder
{
  mutable bool needed = false;
  TMatrix viewItm;
  ECS_BROADCAST_EVENT_DECL(UpdateStageInfoNeedDistortion);
  UpdateStageInfoNeedDistortion(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &itm) :
    ECS_EVENT_CONSTRUCTOR(UpdateStageInfoNeedDistortion), TransformHolder(view_tm, proj_tm), viewItm(itm)
  {}
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

struct RenderStaticSceneEvent : public ecs::Event
{
  Frustum cullingFrustum;
  Point3 mainCamPos;
  int renderPass = RENDER_UNKNOWN;
  ECS_BROADCAST_EVENT_DECL(RenderStaticSceneEvent);
  RenderStaticSceneEvent(const Frustum &culling_frustum, const Point3 &main_cam_pos, int render_pass) :
    ECS_EVENT_CONSTRUCTOR(RenderStaticSceneEvent), cullingFrustum(culling_frustum), mainCamPos(main_cam_pos), renderPass(render_pass)
  {}
};

struct RenderDecalsOnDynamic : public ecs::Event
{
  TMatrix viewTm;
  TMatrix4 projTm; // must match whatever the broadcaster set as TM_PROJ
  Point3 mainCamPos;
  Frustum cullingFrustum;
  const Occlusion *occlusion;
  TexStreamingContext texCtx;
  ECS_BROADCAST_EVENT_DECL(RenderDecalsOnDynamic);
  RenderDecalsOnDynamic(const TMatrix &view_tm,
    const TMatrix4 &proj_tm,
    const Point3 &main_cam_pos,
    const Frustum &culling_frustum,
    const Occlusion *occlusion_,
    TexStreamingContext tex_ctx) :
    ECS_EVENT_CONSTRUCTOR(RenderDecalsOnDynamic),
    viewTm(view_tm),
    projTm(proj_tm),
    mainCamPos(main_cam_pos),
    cullingFrustum(culling_frustum),
    occlusion(occlusion_),
    texCtx(tex_ctx)
  {}
};

#define DEF_RENDER_PROFILE_EVENTS                                                                        \
  DEF_RENDER_PROF_EVENT(RenderEventUI, TMatrix /* viewTm */, TMatrix /* viewItm */, mat44f /* globtm */, \
    Driver3dPerspective /* persp */)                                                                     \
  DEF_RENDER_PROF_EVENT(RenderXray, TMatrix /* viewTm */, TMatrix /* viewItm */, mat44f /* globtm */,    \
    Driver3dPerspective /* persp */)                                                                     \
  DEF_RENDER_PROF_EVENT(RenderDebugWithJitter)                                                           \
  DEF_RENDER_PROF_EVENT(RenderEventDebugGUI)

#define DEF_RENDER_EVENTS                                                                              \
  DEF_RENDER_EVENT(OnWorldRendererCreated)                                                             \
  DEF_RENDER_EVENT(UnloadLevel)                                                                        \
  DEF_RENDER_EVENT(OnRenderDecals, TMatrix /*viewTm*/, TMatrix /*viewItm*/, Point3 /*cameraWorldPos*/, \
    TexStreamingContext /*texCtx*/, const RiGenVisibility * /*rendinstMainVisibility*/)                \
  DEF_RENDER_EVENT(RenderDecalsOnGlass)                                                                \
  DEF_RENDER_EVENT(RenderDecalsOnGlassDynamic)                                                         \
  DEF_RENDER_EVENT(RenderDecalsGlassMask)                                                              \
  DEF_RENDER_EVENT(RegisterPostfxResources, dafg::Registry)                                            \
  DEF_RENDER_EVENT(RegisterDistortionModifiersResources, dafg::Registry)                               \
  DEF_RENDER_EVENT(AfterShaderReload)


#define DEF_RENDER_EVENT              ECS_BROADCAST_EVENT_TYPE
#define DEF_RENDER_PROF_EVENT(K, ...) ECS_BASE_DECL_EVENT_TYPE(K, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE, ##__VA_ARGS__)
DEF_RENDER_PROFILE_EVENTS
DEF_RENDER_EVENTS
#undef DEF_RENDER_PROF_EVENT
#undef DEF_RENDER_EVENT

ECS_UNICAST_EVENT_TYPE(CustomSkyRender, TMatrix /* viewTm */, TMatrix4 /* projTm */, Driver3dPerspective /* persp */)
ECS_BROADCAST_EVENT_TYPE(CustomDmPanelRender, RectInt /*rect*/)
// this event ensures that WorldRenderer is created
ECS_BROADCAST_EVENT_TYPE(BeforeLoadLevel)
ECS_BROADCAST_EVENT_TYPE(InvalidateClipmapBox, BBox2 /*box*/);
ECS_BROADCAST_EVENT_TYPE(InvalidateBoxAfterHeightmapChange, BBox3 /*box*/);
ECS_BROADCAST_EVENT_TYPE(OnClipmapTileRender, Frustum /*frustum*/);
ECS_BROADCAST_EVENT_TYPE(AfterHeightmapChange);
ECS_BROADCAST_EVENT_TYPE(RendinstLodRangeIncreasedEvent, bool /*impostor*/, bool /*rendinst*/);

class TextureIDPair;
ECS_UNICAST_EVENT_TYPE(CustomEnviProbeRender, const ManagedTex *, int)
ECS_BROADCAST_EVENT_TYPE(CustomEnviProbeGetSphericalHarmonics, eastl::vector<Color4>)
ECS_BROADCAST_EVENT_TYPE(CustomEnviProbeLogSphericalHarmonics, const Color4 *)

class DynamicRenderableSceneInstance;
class DynamicRenderableSceneResource;
using BVHAdditionalAnimcharIterateCallback = dag::FixedMoveOnlyFunction<32,
  void(ecs::EntityId,
    DynamicRenderableSceneInstance *,
    DynamicRenderableSceneResource *,
    animchar_additional_data::AnimcharAdditionalDataView,
    const animchar_visbits_t &)>;
ECS_BROADCAST_EVENT_TYPE(BVHAdditionalAnimcharIterate, BVHAdditionalAnimcharIterateCallback &)
ECS_BROADCAST_EVENT_TYPE(BVHDagdpChanged)

ECS_BROADCAST_EVENT_TYPE(ChangeRenderFeaturesEarly)
ECS_BROADCAST_EVENT_TYPE(SetAntialiasing)

ECS_BROADCAST_EVENT_TYPE(RenderAlbedoVoxelization, BBox3 /*cullBox*/, bool /*outRenderedAnything*/)
ECS_BROADCAST_EVENT_TYPE(RenderSdfVoxelization, BBox3 /*cullBox*/, bool /*outRenderedAnything*/)
ECS_BROADCAST_EVENT_TYPE(SetGrassSdfEareaser, bool /* enable */)
