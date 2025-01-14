// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/gpuGrass.h>
#include <webui/editVarPlugin.h>
#include <render/world/rendInstHeightmap.h>
#include <render/grassify.h>

struct GrassRenderer
{
  eastl::unique_ptr<EditableVariablesNotifications> grassEditVarNotification;
  eastl::unique_ptr<Grassify> grassify;
  GPUGrass grass;
  dag::Vector<Point4> grassErasers;
  uint32_t grassErasersIndexToAdd = 0;
  uint32_t grassErasersActualSize = 0;
  bool grassErasersModified = false;
  UniqueBufHolder grassEraserBuffer;
  DynamicShaderHelper grassEraserShader;
  eastl::unique_ptr<RendInstHeightmap> rendInstHeightmap;
  GrassRenderer()
  {
#if DAGOR_DBGLEVEL > 0
    grassEditVarNotification = eastl::make_unique<EditableVariablesNotifications>();
#endif
  }
  void renderGrassPrepassInternal();
  void renderGrassPrepass();
  void renderGrass();
  void generateGrass(const TMatrix &itm, const Driver3dPerspective &perspective);
  void setGrassErasers(int count, const Point4 *erasers);
  void addGrassEraser(const Point3 &world_pos, float radius);
  void clearGrassErasers();
  void initGrass(const DataBlock &grass_settings_fallback);
  void initGrassRendinstClipmap();
  void invalidateGrass(bool regenerate);
  void invalidateGrassBoxes(const dag::ConstSpan<BBox2> &boxes);
  void invalidateGrassBoxes(const dag::ConstSpan<BBox3> &boxes);

  void resetGrass();
};


ECS_DECLARE_RELOCATABLE_TYPE(GrassRenderer);