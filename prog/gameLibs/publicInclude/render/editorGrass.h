//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/randomGrass.h>
#include <render/landMask.h>

struct IRandomGrassRenderHelper : public IRandomGrassHelper, public ILandMaskRenderHelper
{};

class EditorGrass : public RandomGrass
{
public:
  EditorGrass(const DataBlock &level_grass_blk, const DataBlock &params_blk);
  ~EditorGrass();

  void beforeRender(const Point3 &center_pos, IRandomGrassRenderHelper &render_helper, const Point3 &view_dir, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, const Frustum &frustum, bool force_update = false, Occlusion *occlusion = NULL);
  void renderDepth();
  void renderTrans();
  void renderOpaque(bool optimization_prepass_needed = false);

  void resetLayersVB();
  void setLayerDensity(int layer_i, float new_density);

  int getGrassLayersCount();
  GrassLayerInfo *getGrassLayer(int layer_i);

  int addDefaultLayer();
  bool removeLayer(int layer_i);
  void removeAllLayers();
  void reload(const DataBlock &grass_blk, const DataBlock &params_blk);

  bool changeLayerResource(int layer_i, const char *resName);
  void updateLayerVbo(int layer_i);

protected:
  LandMask *landMask;
};
