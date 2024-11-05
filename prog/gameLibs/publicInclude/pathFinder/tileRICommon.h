//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <rendInst/rendInstCollision.h>
#include <gameRes/dag_collisionResource.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_bitArray.h>
#include <ioSys/dag_dataBlock.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <EASTL/string.h>

namespace rendinst
{
struct RiObstacleSetup
{
  bool overridePadding = false;
  float overridePaddingValue = 0.f;
  bool overrideType = false;
  int overrideTypeValue = 0;
};
struct obstacle_settings_t
{
  eastl::vector<RiObstacleSetup> setups;
  ska::flat_hash_map<uint32_t, int> mapPools;
  ska::flat_hash_map<eastl::string, int> mapNames;
  const RiObstacleSetup *find(int resIdx);
};
bool load_obstacle_settings(const char *obstacle_settings_path, rendinst::obstacle_settings_t &obstacle_settings);

struct RendinstVertexDataCbBase : public rendinst::RendInstCollisionCB
{
  Bitarray &pools;
  ska::flat_hash_map<int, uint32_t> &obstaclePools;
  ska::flat_hash_map<int, uint32_t> &materialPools;
  rendinst::obstacle_settings_t &obstaclesSettings;

  Tab<Point3> &vertices;
  Tab<int> &indices;
  int vertNum = 0;
  int indNum = 0;

  struct RiData
  {
    rendinst::RendInstDesc desc;
    Tab<vec4f> vertices; // with ident instance tm
    Tab<int> indices;

    bool operator==(const rendinst::CollisionInfo &coll_info) const
    {
      return coll_info.desc.isRiExtra() == desc.isRiExtra() && coll_info.desc.pool == desc.pool;
    }

    void buildMeshNode(const CollisionNode *node)
    {
      int idxBase = vertices.size();
      mat44f nodeTm;
      v_mat44_make_from_43cu_unsafe(nodeTm, node->tm.array);
      vertices.reserve(idxBase + node->vertices.size());
      for (int i = 0, size = node->vertices.size(); i < size; ++i)
        vertices.push_back(v_mat44_mul_vec3p(nodeTm, v_ldu(&node->vertices[i].x)));
      indices.reserve(indices.size() + node->indices.size());
      for (int i = 0, size = node->indices.size() / 3; i < size; ++i)
      {
        indices.push_back(idxBase + int(node->indices[i * 3 + 0]));
        indices.push_back(idxBase + int(node->indices[i * 3 + 1]));
        indices.push_back(idxBase + int(node->indices[i * 3 + 2]));
      }
    }

    void build(const rendinst::CollisionInfo &coll_info)
    {
      if (!coll_info.collRes)
        return;
      desc = coll_info.desc;
      for (const CollisionNode *node = coll_info.collRes->meshNodesHead; node; node = node->nextNode)
        if (node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          buildMeshNode(node);
      for (const CollisionNode *node = coll_info.collRes->boxNodesHead; node; node = node->nextNode)
      {
        if (!node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        int idxBase = vertices.size();
        vertices.reserve(vertices.size() + 8);
        for (int i = 0; i < 8; ++i)
        {
          Point3_vec4 boxPt = node->modelBBox.point(i);
          vertices.push_back(v_ld(&boxPt.x));
        }
        /*
              7+------+3
              /|     /|
            / |   2/ |
          6+------+  |
            | 5+---|--+1
            | /    | /
            |/     |/
          4+------+0
        */
        indices.reserve(indices.size() + 6 * 2 * 3);
        indices.push_back(idxBase + 0);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 1);
        indices.push_back(idxBase + 0);
        indices.push_back(idxBase + 4);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 0);
        indices.push_back(idxBase + 1);
        indices.push_back(idxBase + 4);

        indices.push_back(idxBase + 3);
        indices.push_back(idxBase + 1);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 3);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 7);
        indices.push_back(idxBase + 3);
        indices.push_back(idxBase + 7);
        indices.push_back(idxBase + 1);

        indices.push_back(idxBase + 5);
        indices.push_back(idxBase + 1);
        indices.push_back(idxBase + 7);
        indices.push_back(idxBase + 5);
        indices.push_back(idxBase + 4);
        indices.push_back(idxBase + 1);
        indices.push_back(idxBase + 5);
        indices.push_back(idxBase + 7);
        indices.push_back(idxBase + 4);

        indices.push_back(idxBase + 6);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 4);
        indices.push_back(idxBase + 6);
        indices.push_back(idxBase + 7);
        indices.push_back(idxBase + 2);
        indices.push_back(idxBase + 6);
        indices.push_back(idxBase + 4);
        indices.push_back(idxBase + 7);
      }
    }
  };

  Tab<RiData *> riCache;
  Tab<rendinst::CollisionInfo> collCache;

  RendinstVertexDataCbBase(Tab<Point3> &verts, Tab<int> &inds, Bitarray &pools, ska::flat_hash_map<int, uint32_t> &obstaclePools,
    ska::flat_hash_map<int, uint32_t> &materialPools, rendinst::obstacle_settings_t &obstaclesSettings) :
    vertices(verts),
    indices(inds),
    pools(pools),
    obstaclePools(obstaclePools),
    materialPools(materialPools),
    obstaclesSettings(obstaclesSettings)
  {}
  ~RendinstVertexDataCbBase() { clear_all_ptr_items(riCache); }

  void pushCollision(const rendinst::CollisionInfo &coll_info)
  {
    if (!coll_info.collRes)
      return;

    if (pools.size() && pools[coll_info.desc.pool])
      return;

    collCache.push_back(coll_info);
    for (const CollisionNode *node = coll_info.collRes->meshNodesHead; node; node = node->nextNode)
    {
      if (!node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      vertNum += node->vertices.size();
      indNum += node->indices.size();
    }
    for (const CollisionNode *node = coll_info.collRes->boxNodesHead; node; node = node->nextNode)
    {
      if (!node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      vertNum += 8;
      indNum += 6 * 2 * 3;
    }
  }

  void procAllCollision()
  {
    for (const rendinst::CollisionInfo &coll : collCache)
      processCollision(coll);
  }

  template <typename FilterCallback>
  void procFilteredCollision(FilterCallback filter)
  {
    for (const rendinst::CollisionInfo &coll : collCache)
      if (filter(coll))
        processCollision(coll);
  }

  virtual void processCollision(const rendinst::CollisionInfo &coll_info) = 0;

  virtual void addCollisionCheck(const rendinst::CollisionInfo &coll_info) { pushCollision(coll_info); }
  virtual void addTreeCheck(const rendinst::CollisionInfo &coll_info) { pushCollision(coll_info); }
};

inline bool load_obstacle_settings(const char *obstacle_settings_path, rendinst::obstacle_settings_t &obstacle_settings)
{
  DataBlock obstacleSettingsBlk;
  if (!dblk::load(obstacleSettingsBlk, obstacle_settings_path, dblk::ReadFlag::ROBUST))
    return false;

  obstacle_settings = rendinst::obstacle_settings_t();
  const int reserveCount = obstacleSettingsBlk.blockCount();
  obstacle_settings.setups.reserve(reserveCount);
  obstacle_settings.mapPools.reserve(reserveCount);
  obstacle_settings.mapNames.reserve(reserveCount);

  for (int blkIt = 0; blkIt < obstacleSettingsBlk.blockCount(); blkIt++)
  {
    const DataBlock *blk = obstacleSettingsBlk.getBlock(blkIt);
    const char *riName = blk->getBlockName();

    rendinst::RiObstacleSetup setup;
    bool hasOverrides = false;
    int setupIdx = -1;

    auto it = obstacle_settings.mapNames.find(riName);
    if (it != obstacle_settings.mapNames.end())
    {
      setupIdx = it->second;
      setup = obstacle_settings.setups[setupIdx];
      hasOverrides = true;
    }

    if (blk->findParam("navMeshBoxOffset") >= 0)
    {
      setup.overridePadding = true;
      setup.overridePaddingValue = blk->getReal("navMeshBoxOffset", 0.f);
      hasOverrides = true;
    }
    if (blk->findParam("obstacleType") >= 0)
    {
      setup.overrideType = true;
      setup.overrideTypeValue = blk->getInt("obstacleType", 0);
      hasOverrides = true;
    }

    if (hasOverrides)
    {
      if (setupIdx < 0)
      {
        setupIdx = (int)obstacle_settings.setups.size();
        obstacle_settings.setups.push_back(setup);
        obstacle_settings.mapNames.emplace(riName, setupIdx);
        const int resIdx = rendinst::getRIGenExtraResIdx(riName);
        if (resIdx >= 0)
          obstacle_settings.mapPools.emplace(resIdx, setupIdx);
      }
      else
      {
        obstacle_settings.setups[setupIdx] = setup;
      }
    }
  }

  return true;
}

inline const RiObstacleSetup *obstacle_settings_t::find(int resIdx)
{
  if (resIdx < 0)
    return nullptr;
  auto it = mapPools.find(resIdx);
  if (it != mapPools.end())
    return &setups[it->second];
  rendinst::RendInstDesc desc(rendinst::make_handle(resIdx, 0));
  const char *riName = rendinst::getRIGenResName(desc);
  if (!riName || !*riName)
    return nullptr;
  auto it2 = mapNames.find(riName);
  if (it2 == mapNames.end())
    return nullptr;
  const int setupIdx = it2->second;
  mapPools.emplace(resIdx, setupIdx);
  return &setups[setupIdx];
}
} // namespace rendinst
