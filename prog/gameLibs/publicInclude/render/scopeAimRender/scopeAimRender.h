//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_texStreamingContext.h>
#include <daECS/core/event.h>
#include <render/dof/dofProperties.h>
#include <render/world/aimRender.h>
#include <render/world/cameraParams.h>

class PostFxRenderer;

namespace dafg
{
class NodeHandle;
class Registry;
} // namespace dafg

constexpr int SCOPE_CROSSHAIR_LAST_IDX = 254;
constexpr int SCOPE_CROSSHAIR_INVALID_IDX = 255;
constexpr int SCOPE_CROSSHAIR_MAX_COUNT = 8;

inline uint64_t pack_crosshair_node_id(uint64_t node_ids, const uint64_t node_id, const int crosshair_idx)
{
  node_ids &= ~(0xFFull << (crosshair_idx * 8));
  node_ids |= node_id << (crosshair_idx * 8);
  return node_ids;
}

class ScopeCrosshairNodeIterator
{
  int64_t nodeIds;
  uint8_t idx = 0;
  uint8_t endIdx = SCOPE_CROSSHAIR_MAX_COUNT;

  uint8_t get(const int i) const { return (nodeIds >> (i * 8)) & 0xFF; }
  uint8_t findEndIdx() const
  {
    for (uint8_t i = 0; i < SCOPE_CROSSHAIR_MAX_COUNT; ++i)
    {
      if (get(i) == SCOPE_CROSSHAIR_INVALID_IDX)
        return i;
    }
    return SCOPE_CROSSHAIR_MAX_COUNT;
  }

  ScopeCrosshairNodeIterator(int64_t v, uint8_t idx, uint8_t end_idx) : nodeIds(v), idx(idx), endIdx(end_idx) {}

public:
  ScopeCrosshairNodeIterator(int64_t v) : nodeIds(v), idx(0), endIdx(findEndIdx()) {}

  uint8_t operator*() const { return get(idx); }
  ScopeCrosshairNodeIterator &operator++()
  {
    ++idx;
    return *this;
  }
  bool operator!=(const ScopeCrosshairNodeIterator &o) const { return idx != o.idx; }

  ScopeCrosshairNodeIterator begin() const { return *this; }
  ScopeCrosshairNodeIterator end() const { return ScopeCrosshairNodeIterator(-1, endIdx, endIdx); }
};

struct ScopeAimRenderingData
{
  int lensNodeId = -1;
  int lensCollisionNodeId = -1;
  int64_t crosshairNodeIds = -1;
  float scopeWeaponLensZoomFactor = 1.0f;
  ecs::EntityId entityWithScopeLensEid;
  ecs::EntityId gunEid;
  bool isAiming = false;
  bool isAimingThroughScope = false;
  bool nightVision = false;
  bool nearDofEnabled = false;
  bool simplifiedAimDof = false;
  dag::RelocatableFixedVector<ecs::EntityId, 31> scopeLensCockpitEntities;
};

struct AimDofSettings
{
  DOFProperties focus;
  float minCheckDistance = 0.0f;
  bool on = false;
  bool changed = false;
};

ECS_BROADCAST_EVENT_TYPE(UpdateAimDofSettings, bool /*aim_near_dof_enabled*/, bool /*simplified_aim_dof*/);

dafg::NodeHandle makeSetupScopeAimRenderingDataNode();
dafg::NodeHandle makeAimDofPrepareNode();

inline int get_scope_mask_format() { return TEXFMT_R8; }

void restore_scope_aim_dof(const AimDofSettings &savedDofSettings);
void prepare_aim_dof(const ScopeAimRenderingData &scopeAimData, const AimRenderingData &aim_data, AimDofSettings &dof_settings,
  Texture *aim_dof_depth, const TexStreamingContext &tex_ctx);

void render_scope_prepass(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope_trans_except_lens(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope_lens_mask(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx,
  const bool use_depth_as_val = false);
void render_scope_lens_hole(const ScopeAimRenderingData &scopeAimData, const bool by_mask, const TexStreamingContext &texCtx);
void render_scope_trans_forward(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope_crosshair_gui(const AimRenderingData &scopeAimData, const CameraParams &view, const IPoint2 &viewport_res);

void render_lens_frame(const ScopeAimRenderingData &scopeAimData, BaseTexture *frameTex, BaseTexture *lensSourceTex,
  const TexStreamingContext &texCtx);
void render_lens_optics(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx,
  const PostFxRenderer &fadingRenderer);

class ComputeShaderElement;
void prepare_scope_vrs_mask(ComputeShaderElement *scope_vrs_mask_cs, Texture *scope_vrs_mask_tex, Texture *depth);

void render_scope(const ecs::EntityId entity_with_scope, const int node_id, const TexStreamingContext &tex_ctx,
  ShaderElement *shader_elem, const int block_id = -1);