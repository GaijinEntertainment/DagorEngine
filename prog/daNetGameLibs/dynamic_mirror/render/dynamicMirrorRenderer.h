// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_plane3.h>
#include <math/dag_frustum.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_decl.h>

#define MIRROR_VARS_LIST     \
  VAR(dynamic_mirror_tex)    \
  VAR(mirror_proj_transform) \
  VAR(dynamic_texture_hdr_pass)

#define VAR(a) extern ShaderVariableInfo a##VarId;
MIRROR_VARS_LIST
#undef VAR

class DynamicRenderableSceneInstance;
struct RiGenVisibility;
class TexStreamingContext;
struct SkiesData;

struct AnimcharMirrorData
{
  dag::Vector<uint8_t> nodeFilter;
  DynamicRenderableSceneInstance *sceneInstance = nullptr;
  bool hasAdditionalData = false;
  ecs::Point4List additionalData;
  bool needsHighRenderPriority = false;

  const ecs::Point4List *getAdditionalData() const { return hasAdditionalData ? &additionalData : nullptr; }
};

class DynamicMirrorRenderer
{
public:
  DynamicMirrorRenderer();
  ~DynamicMirrorRenderer();

  DynamicMirrorRenderer(const DynamicMirrorRenderer &) = delete;
  DynamicMirrorRenderer &operator=(const DynamicMirrorRenderer &) = delete;

  struct MirrorData
  {
    int rendAnimcharNodeId = 0;
    BSphere3 boundingSphere = {};
  };

  struct CameraData
  {
    TMatrix4_vec4 globTm;
    TMatrix4_vec4 projTm;
    TMatrix4_vec4 prepassProjTm;
    TMatrix mirrorRenderViewItm;
    TMatrix viewItm;
    TMatrix viewTm;
    Frustum frustum;
    Driver3dPerspective persp;
    Driver3dPerspective prepassPersp;
    Point3 viewPos;
  };

  void init();
  [[nodiscard]] bool isInitialized() const;

  void setMirror(ecs::EntityId eid,
    DynamicRenderableSceneInstance *scene_instance,
    const ecs::Point4List *additional_data,
    bool animchar_render_priority);
  void unsetMirror(ecs::EntityId eid);
  [[nodiscard]] bool hasMirrorSet() const;
  [[nodiscard]] const AnimcharMirrorData *getAnimcharMirrorData() const { return hasCameraData ? &animcharMirrorData : nullptr; }

  [[nodiscard]] const dag::Vector<MirrorData> &getMirrors() const { return mirrors; }

  [[nodiscard]] RiGenVisibility *getVisibility() const { return riVisibility; }
  [[nodiscard]] const CameraData *getCameraData() const { return hasMirrorSet() && hasCameraData ? &currentCameraData : nullptr; }

  void prepareMirrorRIVisibilityAsync(int texture_height);
  void waitStaticsVisibility();

  void prepareCameraData(const TMatrix &view_itm, const Driver3dPerspective &persp, const Plane3 &mirror_plane);
  void clearCameraData();

  // This is set from the rendering code and meant to be read by the ECS handlers:
  // It's used for culling and determines the minimum height of an object to not cull it. No need for the current value,
  // 1 frame of delay is ok.
  IPoint2 getLatestResolution() const { return latestResolution; }
  void setLatestResolution(const IPoint2 &resolution) { latestResolution = resolution; }

  SkiesData *getSkiesData() { return skiesData; }

private:
  bool initialized = false;
  RiGenVisibility *riVisibility = nullptr;
  SkiesData *skiesData = nullptr;

  ecs::EntityId mirrorEntityId = {};
  dag::Vector<MirrorData> mirrors;
  AnimcharMirrorData animcharMirrorData;
  IPoint2 latestResolution = IPoint2(0, 0);

  bool hasCameraData = false;
  CameraData currentCameraData;

  [[nodiscard]] static CameraData get_common_camera_data(
    const TMatrix &view_itm, const Driver3dPerspective &persp, const Plane3 &mirror_plane);
};

void render_dynamic_mirrors(const DPoint3 &main_camera_pos,
  const AnimcharMirrorData &animchar_mirror_data,
  int render_pass,
  bool render_depth,
  TMatrix view_itm,
  const TexStreamingContext &tex_streaming_ctx);

ECS_DECLARE_BOXED_TYPE(DynamicMirrorRenderer);