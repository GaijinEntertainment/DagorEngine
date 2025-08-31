// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/gpuGrass.h>
#include <render/fast_grass.h>
#include <webui/editVarPlugin.h>
#include <render/world/rendInstHeightmap.h>
#include <render/grassify.h>
#include <shaders/dag_postFxRenderer.h>

struct GrassRenderer
{
  eastl::unique_ptr<EditableVariablesNotifications> grassEditVarNotification;
  eastl::unique_ptr<Grassify> grassify;
  GPUGrass grass;
  FastGrassRenderer fastGrass;
  dag::Vector<Point4> grassErasers;
  uint32_t grassErasersIndexToAdd = 0;
  uint32_t grassErasersActualSize = 0;
  bool grassErasersModified = false;
  bool fastGrassChanged = false;
  UniqueBufHolder grassEraserBuffer;
  DynamicShaderHelper grassEraserShader;
  PostFxRenderer grassSdfEraser;
  dag::Vector<BBox3> lastSdfBoxes;
  dag::Vector<BBox2> sdfBoxesToUpdate;
  int sdfUpdatePeriod = 3; // 3x3 = 9 frames
  eastl::unique_ptr<RendInstHeightmap> rendInstHeightmap;
  GrassRenderer()
  {
#if DAGOR_DBGLEVEL > 0
    grassEditVarNotification = eastl::make_unique<EditableVariablesNotifications>();
#endif
  }
  void renderGrassPrepassInternal(const GrassView view);
  void renderGrassPrepass(const GrassView view);
  void renderGrassVisibilityPass(const GrassView view);
  void resolveGrassVisibility(const GrassView view);
  void renderGrass(const GrassView view);
  void renderFastGrass(const TMatrix4 &globtm, const Point3 &view_pos);
  void generateGrassPerCamera(const TMatrix &itm);
  void generateGrassPerView(const GrassView view, const Frustum &frustum, const TMatrix &itm, const Driver3dPerspective &perspective);
  void setGrassErasers(int count, const Point4 *erasers);
  void addGrassEraser(const Point3 &world_pos, float radius);
  void clearGrassErasers();
  void initGrass(const DataBlock &grass_settings_fallback);
  void initGrassRendinstClipmap();
  void initOrUpdateFastGrass();
  void invalidateGrass(bool regenerate);
  void invalidateGrassBoxes(const dag::ConstSpan<BBox2> &boxes);
  void invalidateGrassBoxes(const dag::ConstSpan<BBox3> &boxes);
  void toggleCameraInCamera(const bool v);

private:
  void updateSdfEraser();
};


ECS_DECLARE_RELOCATABLE_TYPE(GrassRenderer);