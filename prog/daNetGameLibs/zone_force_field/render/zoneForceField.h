// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/core/entityManager.h>

#include <scene/dag_occlusion.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/world/partitionSphere.h>

// Framegraph v2
#include <render/daBfg/bfg.h>

struct ZoneForceFieldRenderer
{
  DynamicShaderHelper forceFieldShader;
  DynamicShaderHelper forceFieldApplierShader;
  PostFxRenderer forceFieldManyApplier;
  UniqueBuf sphereVb, sphereIb;
  uint32_t v_count = 0, f_count = 0, texHt = 1;
  shaders::OverrideStateId zonesInState, zonesOutState;
  eastl::vector<Point4> frameZones, frameZonesOut;
  bool bilateral_upscale = true;

  enum
  {
    SLICES = 33
  };

  ~ZoneForceFieldRenderer();
  ZoneForceFieldRenderer();

  dabfg::NodeHandle createRenderingNode(const char *rendering_shader_name, uint32_t render_target_fmt);

  dabfg::NodeHandle createApplyingNode(const char *applying_shader_name, const char *fullscreen_applying_shader_name);


  void start() const;
  void startZoneIn() const;
  void startZoneOut() const;
  void render(const Point4 *sph, int count) const;

  void applyOne(const Point4 &sph, bool cull_ccw) const;
  void applyMany() const;

  void gatherForceFields(const TMatrix &view_itm, const Frustum &culling_frustum, const Occlusion *occlusion);
  PartitionSphere getClosestForceField(const Point3 &camera_pos) const;

  void renderForceFields(Texture *downsampled_depth, const ManagedTex *render_target);
  void apply() const;
  void fillBuffers();
};

ECS_DECLARE_BOXED_TYPE(ZoneForceFieldRenderer);
