#pragma once
#include "../assetStats.h"

class CollisionResource;
class ICompositObj;
class IObjEntity;
class Point3;
class RenderableInstanceLodsResource;
struct CollisionNode;

class AssetStatsFiller
{
public:
  static void fillAssetStatsFromObjEntity(AssetStats &stats, IObjEntity &entity, const Point3 &camera_pos);
  static int fillAssetStatsFromRenderableInstanceLodsResource(AssetStats &stats, const RenderableInstanceLodsResource &res,
    float distance);
  static void fillAssetCollisionStats(AssetStats &stats, CollisionResource &collision_resource);

private:
  static int getLodFromDistance(const RenderableInstanceLodsResource &res, float distance);
  static float getLodDistance(const Point3 &camera_pos, const IObjEntity &entity);
  static void fillAssetCollisionNodeStats(AssetStats::GeometryStat &geometry, const CollisionNode &collision_node);
  static void fillCompisiteAssetStats(AssetStats &stats, ICompositObj &composite, const Point3 &camera_pos, bool &firstLodSet);
};
