//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <rendInst/rendInstGen.h>
#include <gameRes/dag_collisionResource.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_bitArray.h>

namespace rendinst
{
struct RendinstVertexDataCbBase : public rendinst::RendInstCollisionCB
{
  Bitarray &pools;
  ska::flat_hash_map<int, uint32_t> &obstaclePools;
  ska::flat_hash_map<int, uint32_t> &materialPools;
  ska::flat_hash_map<int, float> &navMeshOffsetPools;

  Tab<Point3> &vertices;
  Tab<int> &indices;
  int vertNum = 0;
  int indNum = 0;

  struct RiData
  {
    rendinst::RendInstDesc desc;
    Tab<vec4f> vertices; // with ident instance tm
    Tab<int> indices;

    bool operator==(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) const
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
        indices.push_back(idxBase + int(node->indices[i * 3 + 2]));
        indices.push_back(idxBase + int(node->indices[i * 3 + 1]));
        indices.push_back(idxBase + int(node->indices[i * 3 + 0]));
      }
    }

    void build(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info)
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
  Tab<rendinst::RendInstCollisionCB::CollisionInfo> collCache;

  RendinstVertexDataCbBase(Tab<Point3> &verts, Tab<int> &inds, Bitarray &pools, ska::flat_hash_map<int, uint32_t> &obstaclePools,
    ska::flat_hash_map<int, uint32_t> &materialPools, ska::flat_hash_map<int, float> &navMeshOffsetPools) :
    vertices(verts),
    indices(inds),
    pools(pools),
    obstaclePools(obstaclePools),
    materialPools(materialPools),
    navMeshOffsetPools(navMeshOffsetPools)
  {}
  ~RendinstVertexDataCbBase() { clear_all_ptr_items(riCache); }

  void pushCollision(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info)
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
    for (const rendinst::RendInstCollisionCB::CollisionInfo &coll : collCache)
      processCollision(coll);
  }

  virtual void processCollision(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) = 0;

  virtual void addCollisionCheck(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) { pushCollision(coll_info); }
  virtual void addTreeCheck(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info) { pushCollision(coll_info); }
};
} // namespace rendinst
