// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <EASTL/fixed_string.h>
#include <dag/dag_vector.h>
#include <drv/3d/dag_texFlags.h>
#include <ecs/core/entitySystem.h>
#include <render/world/cameraParams.h>
#include <render/daBfg/nodeHandle.h>
#include "../shaders/dagdp_constants.hlsli"

// Common daGDP terminology:
//
// - Placer: an instance of a particular placement algorithm. Determines where the objects can be located each frame. Has ECS
// representation.
// - Object group: an instance of a particular rendering algorithm. Determines what types of objects can be used. Has ECS
// representation.
// - Rule: connects one or more placers, and one or more object groups. Has ECS representation.
//
// - Placeable: something which has an independent placing weight (which affects probability of its appearance). Each placer+view
// combination has its own set of placeables.
// - Renderable: something which has an independent instance counter, and instance data memory region. All rules share one set of all
// possible renderables.
//
// - View: something that requires objects to be rendered into a separate target. Contains one or more viewports.
// - Viewport: a specific frustum belonging to a view.
//

namespace dagdp
{

#define DAGDP_DEBUG DAGOR_DBGLEVEL != 0

using PlaceableId = uint32_t;
using RenderableId = uint32_t;
using ViewId = uint32_t;

constexpr unsigned int perInstanceFormat = TEXFMT_A32B32G32R32F;
using PerInstanceData = TMatrix4; // TODO: support float4 and float4x3.

struct InstanceRegion
{
  uint32_t baseIndex;
  uint32_t maxCount;
};

struct PlaceableRange
{
  float baseDrawDistance; // Should be modified based on scale.
  RenderableId rId;
};

// Assumption: GPU objects are relatively small, and should not ever be drawn above some distance.
static constexpr float GLOBAL_MAX_DRAW_DISTANCE = 256.0f;

struct PlaceableParams
{
  float weight = 1.0f;
  float slopeFactor = 0.0f;
  uint32_t flags = 0; // See dagdp_common.hlsli
  uint32_t riPoolOffset = 0;
  Point2 scaleMidDev = Point2(1.0f, 0.0f);
  Point2 yawRadiansMidDev = Point2(0.0f, M_PI);
  Point2 pitchRadiansMidDev = Point2(0.0f, 0.0f);
  Point2 rollRadiansMidDev = Point2(0.0f, 0.0f);
};

inline float md_min(const Point2 &mid_dev) { return mid_dev.x - mid_dev.y; }
inline float md_max(const Point2 &mid_dev) { return mid_dev.x + mid_dev.y; }

struct PlacerInfo
{
  dag::RelocatableFixedVector<ecs::EntityId, 4> objectGroupEids;
};

struct ObjectGroupPlaceable
{
  PlaceableParams params;
  dag::RelocatableFixedVector<PlaceableRange, 4> ranges;
};

struct ObjectGroupInfo
{
  dag::Vector<ObjectGroupPlaceable> placeables;
  float maxPlaceableBoundingRadius = 0.0f;
  float maxDrawDistance = 0.0f;
};

struct RulesBuilder
{
  NameMap objectGroupNameMap;
  NameMap placerNameMap;
  dag::RelocatableFixedVector<ecs::EntityId, 32> objectGroupsWithNames;
  dag::VectorMap<ecs::EntityId, ObjectGroupInfo> objectGroups;
  dag::VectorMap<ecs::EntityId, PlacerInfo> placers;
  RenderableId nextRenderableId = 0;
  uint32_t maxObjects = 0; // 0 means no limit.
  bool useHeightmapDynamicObjects = false;
};

// Supposed to be immutable after building phase.
struct ViewBuilder
{
  dag::RelocatableFixedVector<uint32_t, 64> renderablesMaxInstances;
  dag::RelocatableFixedVector<InstanceRegion, 64> renderablesInstanceRegions;
  uint32_t totalMaxInstances = 0; // static for all viewports, and dynamic.
  uint32_t maxStaticInstancesPerViewport = 0;
  uint32_t numRenderables = 0;
  bool hasDynamicPlacers = false;

  // Additional, GPU-suballocated memory. Located directly after static regions.
  InstanceRegion dynamicInstanceRegion = {};
};

enum class ViewKind
{
  MAIN_CAMERA, // DNG: main camera.
  DYN_SHADOWS, // DNG: dynamic light shadows.
  // TODO: other types like CSM, probes, etc...
  COUNT
};

static inline void view_multiplex(dabfg::Registry registry, ViewKind kind)
{
  auto mode = dabfg::multiplexing::Mode::None;

  // Shadow rendering nodes in DNG are currently multiplexed (although they probably should not be?), and we have to account for that.
  if (kind == ViewKind::DYN_SHADOWS)
    mode = dabfg::multiplexing::Mode::FullMultiplex;

  registry.multiplex(mode);
}

static constexpr size_t VIEW_KIND_COUNT = eastl::to_underlying(ViewKind::COUNT);

struct ViewInfo
{
  eastl::fixed_string<char, 16> uniqueName;
  ViewKind kind;
  uint32_t maxViewports;
  float maxDrawDistance;
};

struct Viewport
{
  DPoint3 worldPos;
  float maxDrawDistance; // Must be clipped, if greater than the maxDrawDistance of its View.
  Frustum frustum;
};

struct ViewPerFrameData
{
  dag::RelocatableFixedVector<Viewport, DAGDP_MAX_VIEWPORTS, false> viewports;
};

bool set_common_params(const ecs::Object &object, ecs::EntityId eid, PlaceableParams &result);
bool set_object_params(const ecs::ChildComponent &child, ecs::EntityId eid, const eastl::string &field_name, PlaceableParams &result);

using ViewInserter = eastl::fixed_function<sizeof(void *), ViewInfo &()>;
using NodeInserter = eastl::fixed_function<sizeof(void *), void(dabfg::NodeHandle)>;

struct GlobalConfig;

ECS_BROADCAST_EVENT_TYPE(EventObjectGroupProcess, RulesBuilder *);
ECS_BROADCAST_EVENT_TYPE(EventInitialize);
ECS_BROADCAST_EVENT_TYPE(EventRecreateViews, const GlobalConfig *, ViewInserter);
ECS_BROADCAST_EVENT_TYPE(EventInvalidateViews);
ECS_BROADCAST_EVENT_TYPE(EventViewProcess, const RulesBuilder &, const ViewInfo &, ViewBuilder *);
ECS_BROADCAST_EVENT_TYPE(EventViewFinalize, const ViewInfo &, const ViewBuilder &, NodeInserter);
ECS_BROADCAST_EVENT_TYPE(EventFinalize, NodeInserter);

} // namespace dagdp
