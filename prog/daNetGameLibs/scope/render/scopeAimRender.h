// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <3d/dag_texStreamingContext.h>
#include <daECS/core/event.h>
#include <render/dof/dofProperties.h>
#include <render/world/aimRender.h>
#include <render/world/cameraParams.h>

class PostFxRenderer;

namespace dabfg
{
class NodeHandle;
}

struct ScopeAimRenderingData
{
  int lensNodeId = -1;
  int crosshairNodeId = -1;
  ecs::EntityId entityWithScopeLensEid;
  ecs::EntityId gunEid;
  bool isAiming = false;
  bool isAimingThroughScope = false;
  bool nightVision = false;
  bool nearDofEnabled = false;
  bool simplifiedAimDof = false;
  dag::RelocatableFixedVector<ecs::EntityId, 31> scopeLensCockpitEntities;
  TMatrix4_vec4 jitterProjTm;
  TMatrix4_vec4 noJitterProjTm;
};

struct AimDofSettings
{
  DOFProperties focus;
  float minCheckDistance = 0.0f;
  bool on = false;
  bool changed = false;
};

ECS_BROADCAST_EVENT_TYPE(UpdateAimDofSettings, bool /*aim_near_dof_enabled*/, bool /*simplified_aim_dof*/);

dabfg::NodeHandle makeSetupScopeAimRenderingDataNode();
dabfg::NodeHandle makeAimDofPrepareNode();

inline int get_scope_mask_format() { return TEXFMT_R8; }

void restore_scope_aim_dof(const AimDofSettings &savedDofSettings);
void prepare_aim_dof(const ScopeAimRenderingData &scopeAimData,
  const AimRenderingData &aim_data,
  AimDofSettings &dof_settings,
  Texture *aim_dof_depth,
  const TexStreamingContext &tex_ctx);

void render_scope_prepass(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope_lens_mask(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);
void render_scope_lens_hole(const ScopeAimRenderingData &scopeAimData, const bool by_mask, const TexStreamingContext &texCtx);
void render_scope_trans_forward(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx);

void render_lens_frame(
  const ScopeAimRenderingData &scopeAimData, TEXTUREID frameTexId, TEXTUREID lensSourceTexId, const TexStreamingContext &texCtx);
void render_lens_optics(
  const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx, const PostFxRenderer &fadingRenderer);

class ComputeShaderElement;
void prepare_scope_vrs_mask(ComputeShaderElement *scope_vrs_mask_cs, Texture *scope_vrs_mask_tex, TEXTUREID depth);
