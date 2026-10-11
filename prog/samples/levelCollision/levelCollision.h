// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <gameRes/dag_collResDump.h>
#include <gameRes/dag_collisionResource.h>
#include <generic/dag_span.h>
#include <scene/dag_tiledScene.h>
#include <util/dag_bitArray.h>

class IGenLoad;

// handle: (scene pool << 32) | index in pool
class LevelCollision
{
public:
  static constexpr uint32_t POOL_SPAN = 65536; // node flags hold the index in pool

  explicit LevelCollision(float tile_size = 128.f);

  // finds the dump kind by its magic
  bool load(IGenLoad &cb);

  // instances must be live; finalize() must follow
  void addResource(Ptr<CollisionResource> res, dag::Vector<mat43f> &&instances, const char *name = nullptr);
  void finalize();
  void clear();

  uint32_t resourceCount() const { return resources.size(); }
  uint32_t instanceCount() const { return tms.size(); }
  const CollisionResource *resource(uint32_t r) const { return resources[r].get(); }
  const char *resourceName(uint32_t r) const { return names[r].str(); }
  // instances of r: [firstInstance(r), firstInstance(r + 1))
  uint32_t firstInstance(uint32_t r) const { return resFirst[r]; }
  const mat43f &transform(uint32_t i) const { return tms[i]; }
  vec4f sphere(uint32_t i) const { return spheres[i]; } // radius in w
  bbox3f worldBox() const { return box; }

  // by PhysMat id; empty for ri_collisions
  const dag::Vector<CollResDump::Material> &materials() const { return mats; }
  bool isLightTransparent(int phys_mat) const
  {
    return (uint32_t)phys_mat < lightTransparentMats.size() && lightTransparentMats[phys_mat];
  }

  static uint64_t nodeHandle(mat44f_cref node) { return (uint64_t(scene::get_node_pool(node)) << 32) | scene::get_node_flags(node); }
  uint32_t handleResource(uint64_t h) const { return poolResource[h >> 32]; }
  uint32_t handleInstance(uint64_t h) const { return poolFirst[h >> 32] + uint32_t(h); }
  bbox3f nodeBox(mat44f_cref node) const { return scene.calcNodeBox(node); } // the world box of a culled node

  // cb(uint64_t handle, mat44f_cref node), for the nodes whose world box meets cull_box (a multi-tile scene tests the sphere too)
  template <class Cb>
  void boxCull(bbox3f_cref cull_box, Cb cb) const
  {
    scene.boxCull<false, true>(cull_box, 0, 0, [&](scene::node_index, mat44f_cref node) { cb(nodeHandle(node), node); });
  }
  // cb(uint64_t handle, mat44f_cref node, vec4f dist)
  template <class Cb>
  void frustumCull(mat44f_cref globtm, vec4f pos_distscale, Cb cb) const
  {
    scene.frustumCull<false, false, false>(globtm, pos_distscale, 0, 0, nullptr,
      [&](scene::node_index, mat44f_cref node, vec4f dist) { cb(nodeHandle(node), node, dist); });
  }
  // LRU draws runs of one resource
  void sortByPool(dag::Span<uint64_t> handles) const;

private:
  bool loadCollRes(IGenLoad &cb);
  bool loadRiCollisions(IGenLoad &cb);

  float tileSize;
  scene::TiledScene scene;
  dag::Vector<Ptr<CollisionResource>> resources;
  dag::Vector<SimpleString> names;
  dag::Vector<uint32_t> resFirst;
  dag::Vector<mat43f> tms;
  dag::Vector<vec4f> spheres;
  dag::Vector<uint32_t> poolFirst;
  dag::Vector<uint32_t> poolResource;
  bbox3f box;
  dag::Vector<CollResDump::Material> mats;
  Bitarray lightTransparentMats;
};

// the lru_collision_get_* hooks read this level
void set_lru_collision_level(const LevelCollision *level);

// the mesh and convex nodes a trace sees
template <class Cb> // cb(int node_index)
inline void for_each_traced_node(const CollisionResource &res, Cb cb)
{
  for (int ni = 0, ne = res.getAllNodes().size(); ni < ne; ++ni)
  {
    const CollisionNode *n = res.getNode(ni);
    if (n && n->checkBehaviorFlags(CollisionNode::TRACEABLE) && res.getDefaultInstance().isNodeTraceable(ni) &&
        (n->type == COLLISION_NODE_TYPE_MESH || n->type == COLLISION_NODE_TYPE_CONVEX) && res.getNodeFaceCount(ni) > 0)
      cb(ni);
  }
}
