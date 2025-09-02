//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>
#include <math/integer/dag_IPoint3.h>
#include <render/toroidalHelper.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <generic/dag_tab.h>

class DataBlock;
struct RiGenVisibility;

class TreesAbove
{
public:
  TreesAbove(const DataBlock &settings);
  ~TreesAbove();

  void invalidateTrees2d(bool force);
  void invalidateTrees2d(const BBox3 &box, const TMatrix &tm);
  void renderInvalidBboxes(const TMatrix &view_itm, float minZ, float maxZ);
  void prepareTrees2d(const Point3 &origin, const TMatrix &view_itm, float minZ, float maxZ);
  void renderAlbedo();
  void renderAlbedoLit();
  void renderAlbedo(int w, int h, int maxRes);
  void renderAlbedoLit(int w, int h, const IPoint3 &res);

private:
  void initTrees2d(const DataBlock &settings);
  void renderRegion(ToroidalQuadRegion &reg, float texelSize, float minZ, float maxZ, const TMatrix &view_itm);
  void closeTrees2d();

  ToroidalHelper trees2dHelper;
  TextureIDHolderWithVar trees2d;
  TextureIDHolderWithVar trees2dDepth; // to be removed!
  Tab<bbox3f> invalidBoxes;
  float trees2dDist = 384;
  RiGenVisibility *rendinst_trees_visibility = nullptr;
  PostFxRenderer voxelize_trees_albedo, voxelize_trees_lit;
  int world_to_trees_tex_ofsVarId;
  int world_to_trees_tex_mulVarId;
  int gbuffer_for_treesaboveVarId;
};
