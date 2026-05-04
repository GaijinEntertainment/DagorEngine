// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assetStatsFiller.h"
#include "../assetStats.h"
#include <de3_composit.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_objEntity.h>
#include <3d/dag_texIdSet.h>
#include <gameRes/dag_collisionResource.h>
#include <shaders/dag_rendInstRes.h>

extern int rimgr_get_force_lod_no();

AssetStatsFiller::AssetStatsFiller(AssetStats &in_stats) : stats(in_stats) {}

int AssetStatsFiller::getLodFromDistance(const RenderableInstanceLodsResource &res, float distance)
{
  for (int i = 0; i < res.lods.size(); ++i)
    if (distance <= res.lods[i].range)
      return i;

  return -1;
}

float AssetStatsFiller::getLodDistance(const Point3 &camera_pos, const IObjEntity &entity)
{
  TMatrix entityTm;
  entity.getTm(entityTm);
  const Point3 entityPos = entityTm.getcol(3);
  return (camera_pos - entityPos).length();
}

void AssetStatsFiller::fillAssetStatsFromShaderMesh(const ShaderMesh &mesh)
{
  auto meshesResult = processedMeshes.emplace(&mesh, 0);
  if (meshesResult.second) // inserted
  {
    meshesResult.first->second = mesh.calcTotalFaces();

    // gatherUsedMat() uses a linear search to check for duplicates, so we deduplicate the material list ourself.
    // This also allows us to only gather textures for each material once.
    dag::ConstSpan<ShaderMesh::RElem> elems = mesh.getAllElems();
    for (const ShaderMesh::RElem &elem : elems)
      if (elem.mat)
      {
        auto materialsResult = materials.emplace(elem.mat);
        if (materialsResult.second) // inserted
          elem.mat->gatherUsedTex(textures);
      }
  }

  stats.trianglesRenderable += meshesResult.first->second;
}

int AssetStatsFiller::fillAssetStatsFromRenderableInstanceLodsResource(const RenderableInstanceLodsResource &res, float distance)
{
  if (res.lods.empty())
    return -1;

  int lod = rimgr_get_force_lod_no();
  if (lod < 0) // automatic LOD
  {
    lod = getLodFromDistance(res, distance);
    if (lod < 0) // no need to draw
      return -1;
  }
  else if (lod >= res.lods.size())
    lod = res.lods.size() - 1;

  if (!res.lods[lod].scene)
    return -1;

  const InstShaderMeshResource *meshResource = res.lods[lod].scene->getMesh();
  const InstShaderMesh *mesh = meshResource ? meshResource->getMesh() : nullptr;
  if (!mesh)
    return -1;

  fillAssetStatsFromShaderMesh(*mesh->getMesh());

  return lod;
}

int AssetStatsFiller::fillAssetStatsFromDynamicRenderableSceneInstance(DynamicRenderableSceneInstance &scene_instance)
{
  DynamicRenderableSceneLodsResource *res = scene_instance.getLodsResource();
  if (!res || res->lods.empty())
    return -1;

  int lod = scene_instance.getCurrentLodNo();
  if (lod < 0) // no need to draw
    return -1;
  else if (lod >= res->lods.size())
    lod = res->lods.size() - 1;

  DynamicRenderableSceneResource *meshResource = res->lods[lod].scene;
  if (!meshResource)
    return -1;

  Tab<ShaderMeshResource *> rigidMeshes;
  Tab<int> rigidNodeIds;
  meshResource->getMeshes(rigidMeshes, rigidNodeIds, nullptr);
  G_ASSERT(rigidNodeIds.size() == rigidMeshes.size());

  for (int i = 0; i < rigidMeshes.size(); ++i)
  {
    if (scene_instance.isNodeHidden(rigidNodeIds[i]))
      continue;

    const ShaderMeshResource *shaderMeshResource = rigidMeshes[i];
    if (!shaderMeshResource)
      continue;

    const ShaderMesh *shaderMesh = shaderMeshResource->getMesh();
    if (!shaderMesh)
      continue;

    fillAssetStatsFromShaderMesh(*shaderMesh);
  }

  const dag::ConstSpan<PatchablePtr<ShaderSkinnedMeshResource>> skins = meshResource->getSkins();
  const dag::ConstSpan<int> skinNodeIds = meshResource->getSkinNodes();
  G_ASSERT(skinNodeIds.size() == skins.size());

  for (int i = 0; i < skins.size(); ++i)
  {
    if (scene_instance.isNodeHidden(skinNodeIds[i]))
      continue;

    const ShaderSkinnedMeshResource *shaderSkinnedMeshResource = skins[i];
    if (!shaderSkinnedMeshResource)
      continue;

    const ShaderSkinnedMesh *shaderSkinnedMesh = shaderSkinnedMeshResource->getMesh();
    if (!shaderSkinnedMesh)
      continue;

    const ShaderMesh &shaderMesh = shaderSkinnedMesh->getShaderMesh();
    fillAssetStatsFromShaderMesh(shaderMesh);
  }

  return lod;
}

void AssetStatsFiller::fillCompositeAssetStats(ICompositObj &composite, const Point3 &camera_pos, bool &firstLodSet)
{
  dag::Span<IObjEntity *> subEntities = composite.getCompositSubEntities();
  for (IObjEntity *subEntity : subEntities)
  {
    if (!subEntity)
      continue;

    if (ICompositObj *compositeSubEntity = subEntity->queryInterface<ICompositObj>())
      fillCompositeAssetStats(*compositeSubEntity, camera_pos, firstLodSet);

    if (const IEntityGetRISceneLodsRes *sceneLodRes = subEntity->queryInterface<IEntityGetRISceneLodsRes>())
    {
      if (const RenderableInstanceLodsResource *res = sceneLodRes->getSceneLodsRes())
      {
        const float distance = getLodDistance(camera_pos, *subEntity);
        const int lod = fillAssetStatsFromRenderableInstanceLodsResource(*res, distance);
        if (firstLodSet)
        {
          if (lod != stats.currentLod)
            stats.mixedLod = true;
        }
        else
        {
          firstLodSet = true;
          stats.currentLod = lod;
        }
      }
    }
    else if (IEntityGetDynSceneLodsRes *dynSceneLodsRes = subEntity->queryInterface<IEntityGetDynSceneLodsRes>())
    {
      if (DynamicRenderableSceneInstance *sceneInstance = dynSceneLodsRes->getSceneInstance())
      {
        const int lod = fillAssetStatsFromDynamicRenderableSceneInstance(*sceneInstance);
        if (firstLodSet)
        {
          if (lod != stats.currentLod)
            stats.mixedLod = true;
        }
        else
        {
          firstLodSet = true;
          stats.currentLod = lod;
        }
      }
    }
  }
}

void AssetStatsFiller::fillAssetCollisionNodeStats(AssetStats::GeometryStat &geometry, const CollisionResource &collision_resource,
  int node_id)
{
  G_STATIC_ASSERT(NUM_COLLISION_NODE_TYPES == 6);

  const CollisionNode *node = collision_resource.getNode(node_id);
  if (!node)
    return;

  switch (node->type)
  {
    case COLLISION_NODE_TYPE_MESH:
      ++geometry.meshNodeCount;
      geometry.meshTriangleCount += collision_resource.getNodeFaceCount(node_id);
      break;

    case COLLISION_NODE_TYPE_CONVEX:
    case COLLISION_NODE_TYPE_POINTS:
      ++geometry.convexNodeCount;
      geometry.convexVertexCount += collision_resource.getNodeVertCount(node_id);
      break;

    case COLLISION_NODE_TYPE_BOX: ++geometry.boxNodeCount; break;

    case COLLISION_NODE_TYPE_SPHERE: ++geometry.sphereNodeCount; break;

    case COLLISION_NODE_TYPE_CAPSULE: ++geometry.capsuleNodeCount; break;

    default: logwarn("Unknown collision node type: %d", (int)node->type); break;
  }
}

void AssetStatsFiller::fillAssetCollisionStats(CollisionResource &collision_resource)
{
  const auto allNodes = collision_resource.getAllNodes();
  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode &collisionNode = allNodes[ni];
    if (collisionNode.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
      fillAssetCollisionNodeStats(stats.physGeometry, collision_resource, ni);
    if (collisionNode.checkBehaviorFlags(CollisionNode::TRACEABLE))
      fillAssetCollisionNodeStats(stats.traceGeometry, collision_resource, ni);
  }
}

void AssetStatsFiller::fillAssetStatsFromObjEntity(IObjEntity &entity, const Point3 &camera_pos)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    bool firstLodSet = false;
    fillCompositeAssetStats(*compositObj, camera_pos, firstLodSet);
  }
  else
  {
    if (IEntityGetRISceneLodsRes *sceneLodRes = entity.queryInterface<IEntityGetRISceneLodsRes>())
    {
      if (const RenderableInstanceLodsResource *res = sceneLodRes->getSceneLodsRes())
      {
        const float distance = getLodDistance(camera_pos, entity);
        stats.currentLod = fillAssetStatsFromRenderableInstanceLodsResource(*res, distance);
      }
    }
    else if (IEntityGetDynSceneLodsRes *dynSceneLodsRes = entity.queryInterface<IEntityGetDynSceneLodsRes>())
    {
      if (DynamicRenderableSceneInstance *sceneInstance = dynSceneLodsRes->getSceneInstance())
        stats.currentLod = fillAssetStatsFromDynamicRenderableSceneInstance(*sceneInstance);
    }
  }
}

void AssetStatsFiller::finalizeStats()
{
  stats.materialCount += materials.size();
  stats.textureCount += textures.size();
}
