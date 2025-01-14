// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../assetStats.h"
#include <3d/dag_texIdSet.h>
#include <generic/dag_tab.h>

class CollisionResource;
class DynamicRenderableSceneInstance;
class ICompositObj;
class IObjEntity;
class Point3;
class RenderableInstanceLodsResource;
class ShaderMaterial;
struct CollisionNode;

class AssetStatsFiller
{
public:
  explicit AssetStatsFiller(AssetStats &in_stats);

  void fillAssetStatsFromObjEntity(IObjEntity &entity, const Point3 &camera_pos);
  int fillAssetStatsFromRenderableInstanceLodsResource(const RenderableInstanceLodsResource &res, float distance);
  void fillAssetCollisionStats(CollisionResource &collision_resource);

  // This must be called to calculate the final statistics for materials and textures.
  void finalizeStats();

private:
  static int getLodFromDistance(const RenderableInstanceLodsResource &res, float distance);
  static float getLodDistance(const Point3 &camera_pos, const IObjEntity &entity);
  static void fillAssetCollisionNodeStats(AssetStats::GeometryStat &geometry, const CollisionNode &collision_node);

  int fillAssetStatsFromDynamicRenderableSceneInstance(DynamicRenderableSceneInstance &scene_instance);
  void fillCompositeAssetStats(ICompositObj &composite, const Point3 &camera_pos, bool &firstLodSet);

  AssetStats &stats;
  Tab<ShaderMaterial *> materials;
  TextureIdSet textures;
};
