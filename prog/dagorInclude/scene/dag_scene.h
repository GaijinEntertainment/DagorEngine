//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stdint.h>
#include <scene/dag_occlusion.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_miscApi.h>

class IGenSave;
class IGenLoad;

namespace scene
{
#define KEEP_GENERATIONS (DAGOR_DBGLEVEL > 0)
typedef uint32_t node_index;
typedef uint16_t pool_index;

__forceinline pool_index get_node_pool(mat44f_cref node);
__forceinline uint32_t get_node_flags(mat44f_cref node);
__forceinline uint32_t check_node_flags(mat44f_cref node, const uint32_t flag); // if flag is compile time const, it is faster than
                                                                                // (get_node_flags(m)&flag)
__forceinline float get_node_bsphere_rad(mat44f_cref node);
__forceinline vec4f get_node_bsphere_vrad(mat44f_cref node);
__forceinline vec4f get_node_bsphere(mat44f_cref node);
__forceinline uint32_t &get_node_pool_flags_ref(mat44f &node);
__forceinline uint32_t get_node_pool_flags(mat44f_cref node);
__forceinline void set_node_flags(mat44f &node, uint16_t flags); // Adds flags to existing ones, doesn't replace all flag bits.
__forceinline void unset_node_flags(mat44f &node, uint16_t flags);

enum
{
  INVALID_POOL = 0xFFFF,
  INVALID_NODE = 0xFFFFFFFF,
  INVALID_INDEX = 0xFFFFFFFF
};

static const float defaultDisappearSq = 1000000.f; // one meter radius sphere to disappear in 1000m, if no pools.

class SimpleScene;

void save_simple_scene(IGenSave &cb, const SimpleScene &scene); // for debug purposes
template <typename Scene>
bool load_scene(IGenLoad &cb, Scene &scene); // for debug purposes
} // namespace scene

//
// maybe make template for pools?
class scene::SimpleScene
{
public:
  void reserve(int reservedNodesNumber);
  void term();

  const mat44f &getNode(node_index node) const { return nodes[getNodeIndex(node)]; }
  vec4f getNodeBSphere(node_index node) const; // outer (world) space apprixmate sphere
  inline pool_index getNodePool(node_index node) const;
  inline uint32_t getNodeFlags(node_index node) const;
  inline uint32_t getNodesCount() const { return (uint32_t)nodes.size(); }
  inline uint32_t getNodesCapacity() const { return (uint32_t)nodes.capacity(); }
  inline uint32_t getPoolsCount() const { return (uint32_t)poolBox.size(); }
  inline uint32_t getNodesAliveCount() const { return uint32_t(nodes.size() - firstAlive - freeIndices.size()); }
  inline uint32_t getNodeIndex(node_index node) const;
  inline bool isAliveNode(node_index node) const; // only for debug

  void setTransformNoScaleChange(node_index node, mat44f_cref transform); // not updating bounding sphere, i.e. assume transform is and
                                                                          // was without scale, or same uniform scale
  void setTransform(node_index node, mat44f_cref transform); // updating bounding sphere from pool, potentially slower, and requries
                                                             // pool to be valid
  void setFlags(node_index node, uint16_t flags);
  void unsetFlags(node_index node, uint16_t flags);
  void setPoolBBox(pool_index pool, bbox3f_cref box, bool update_nodes = false); // will create pool if missing. Requires 'pool' to be
                                                                                 // less than 65535
  void setPoolDistanceSqScale(pool_index pool, float dist_scale_sq); // will create pool if missing. Requires 'pool' to be less than
                                                                     // 65535
  inline float getPoolDistanceSqScale(pool_index pool) const { return v_extract_w(poolBox[pool].bmin); };
  inline float getPoolSphereRad(pool_index pool) const { return v_extract_w(poolBox[pool].bmax); }
  inline bbox3f_cref getPoolBbox(pool_index pool) const { return poolBox[pool]; }
  inline float getPoolSphereVerticalCenter(pool_index pool) const { return poolSphereVerticalCenter[pool]; }
  inline bbox3f calcNodeBox(node_index node) const;

  void clearNodes();

  void destroy(node_index node);
  inline node_index allocate(const mat44f &transform, pool_index pool, uint16_t flags);
  void reallocate(node_index node, const mat44f &transform, pool_index pool, uint16_t flags); // replace instead of delete, to avoid
                                                                                              // messing with freeNodes
  // node_index       allocate(const mat44f& transform, uint16_t pool, uint16_t flags, float sph_radius, float y_offset, float
  // disappearDistance);//if no pools
  void setWritingThread(int64_t thread)
  {
    G_ASSERT_RETURN(isInWritingThread(), );
    writingThread = thread;
  }
  bool checkConsistency() const;

  template <bool use_flags, typename VisibleNodesFunctor>
  inline // VisibleNodesFunctor(scene::node_index, mat44f_cref, vec4f distsqScale)
    void
    nodesInRange(vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes,
      int start_index = 0, int end_index = INT_MAX) const;

  template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesFunctor>
  inline // VisibleNodesFunctor(scene::node_index, mat44f_cref, vec4f distsqScale)
    void
    frustumCull(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion,
      VisibleNodesFunctor visible_nodes) const;

  template <bool use_flags, bool use_pools, typename VisibleNodesFunctor> // VisibleNodesFunctor(scene::node_index, mat44f_cref)
  void boxCull(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const;


  class iterator
  {
  public:
    __forceinline iterator(const SimpleScene &ss, uint32_t index);
    bool operator==(const iterator &it) const { return &it.mSs == &mSs && it.nodeIndex == nodeIndex; }
    bool operator!=(const iterator &it) const { return !(*this == it); }

    node_index operator*() const { return mSs.getNodeFromIndex(nodeIndex); }

    __forceinline const iterator &operator++();
    __forceinline iterator operator++(int);
    __forceinline const iterator &operator+=(uint32_t cnt);

  private:
    const SimpleScene &mSs;
    uint32_t nodeIndex;
  };


  iterator begin() const { return iterator(*this, firstAlive); }
  iterator end() const { return iterator(*this, getNodesCount()); }

protected:
  friend class iterator;

  void resizePools(pool_index for_pool);
  inline bool isInWritingThread() const { return writingThread == -1 || get_current_thread_id() == writingThread; }
  inline uint32_t getIndexPoolFlags(uint32_t index) const { return get_node_pool_flags(nodes.data()[index]); }
  inline pool_index getIndexPool(uint32_t index) const { return getIndexPoolFlags(index) & 0xFFFF; }
  inline uint32_t getIndexFlags(uint32_t index) const { return getIndexPoolFlags(index) >> 16; }
  inline bool isInvalidIndex(uint32_t index) const { return ((uint32_t *)(char *)&nodes.data()[index].col0)[3] == 0xFFFFFFFF; }
  inline bool isFreeIndex(uint32_t index) const { return index < firstAlive || index >= nodes.size() || isInvalidIndex(index); }
  inline bbox3f calcNodeBox(mat44f_cref m) const;
  inline bbox3f calcNodeBoxFromSphere(mat44f_cref m) const;
  inline bbox3f_cref getPoolBboxInternal(pool_index pool) const { return poolBox.data()[pool]; }

  node_index allocateOne();

  __forceinline void invalidateIndexInternal(uint32_t index) { nodes[index].col0 = v_cast_vec4f(V_CI_MASK0001); }
#if KEEP_GENERATIONS
  dag::Vector<uint8_t> generations;
  __forceinline void invalidateIndex(uint32_t index)
  {
    invalidateIndexInternal(index);
    generations[index]++;
  }
  __forceinline node_index getNodeFromIndex(uint32_t index) const
  {
    return (index << 8) | (index < generations.size() ? generations[index] : 0);
  }

  __forceinline uint32_t getNodeGen(node_index node) const { return node & 0xFF; }
  __forceinline uint32_t getNodeIndexInternal(node_index node) const { return node >> 8; }
#else
  __forceinline void invalidateIndex(uint32_t i) { invalidateIndexInternal(i); }
  __forceinline node_index getNodeFromIndex(uint32_t index) const { return index; }
  __forceinline uint32_t getNodeIndexInternal(node_index node) const { return node; }
#endif
#if DAGOR_DBGLEVEL > 0
  __forceinline void debugCheckConsistency() const { checkConsistency(); }
#else
  __forceinline void debugCheckConsistency() const {}
#endif
  const mat44f &getNodeInternal(node_index node) const { return nodes.data()[getNodeIndexInternal(node)]; } // fast, no checks at all!

  //
  dag::Vector<mat44f> nodes;
  dag::Vector<bbox3f> poolBox;                 // can be empty, if only invalid pools used
  dag::Vector<float> poolSphereVerticalCenter; // can be empty, if only invalid pools used
  // Will be empty if nothing ever deleted, or all deleted nodes are in the begining and in the end.
  dag::Vector<uint32_t> freeIndices;

  int64_t writingThread = -1;
  uint32_t firstAlive = 0; // first not dead node
};

inline uint32_t scene::SimpleScene::getNodeIndex(node_index node) const
{
  uint32_t index = getNodeIndexInternal(node);
  G_ASSERTF_RETURN(!isFreeIndex(index), INVALID_INDEX, "invalid index %d (%d..%d)", index, firstAlive, nodes.size());
#if KEEP_GENERATIONS
  G_ASSERTF_RETURN(index < generations.size() && generations[index] == getNodeGen(node), INVALID_INDEX, "old index=%d (deleted)",
    index);
#endif
  return index;
}

inline bool scene::SimpleScene::isAliveNode(node_index node) const
{
#if KEEP_GENERATIONS
  uint32_t index = getNodeIndexInternal(node);
  if (index < generations.size() && generations[index] != getNodeGen(node))
    return false;
  return !isFreeIndex(index);
#endif
  return !isFreeIndex(getNodeIndexInternal(node));
}


inline uint32_t &scene::get_node_pool_flags_ref(mat44f &node) { return (((uint32_t *)(char *)&node.col2)[3]); }

inline uint32_t scene::get_node_pool_flags(mat44f_cref node) { return (((uint32_t *)(char *)&node.col2)[3]); }

inline scene::pool_index scene::get_node_pool(mat44f_cref node) { return get_node_pool_flags(node) & 0xFFFF; }

inline uint32_t scene::get_node_flags(mat44f_cref node) { return get_node_pool_flags(node) >> 16; }
__forceinline uint32_t scene::check_node_flags(mat44f_cref node, const uint32_t flag)
{
  return get_node_pool_flags(node) & (flag << 16);
}

inline vec4f scene::get_node_bsphere_vrad(mat44f_cref node) { return v_splat_w(node.col3); }

inline float scene::get_node_bsphere_rad(mat44f_cref node) { return v_extract_w(node.col3); }

inline vec4f scene::get_node_bsphere(mat44f_cref node)
{
  vec4f center_rad = node.col3, up = node.col1;
  vec4f center = v_madd(up, v_splat_w(up), center_rad);
  return v_perm_xyzd(center, center_rad);
}

inline void scene::set_node_flags(mat44f &node, uint16_t flags) { get_node_pool_flags_ref(node) |= uint32_t(flags) << 16; }

inline void scene::unset_node_flags(mat44f &node, uint16_t flags) { get_node_pool_flags_ref(node) &= ~(uint32_t(flags) << 16); }

inline scene::SimpleScene::iterator::iterator(const SimpleScene &ss, uint32_t index) : mSs(ss), nodeIndex(index)
{
  if (nodeIndex < mSs.getNodesCount() && mSs.isFreeIndex(nodeIndex))
    ++*this;
}

inline const scene::SimpleScene::iterator &scene::SimpleScene::iterator::operator+=(uint32_t cnt)
{
  uint32_t size = mSs.getNodesCount();
  if (size == mSs.getNodesAliveCount())
  {
    nodeIndex += min(size - nodeIndex, cnt);
    return *this;
  }
  for (; cnt; cnt--)
  {
    for (++nodeIndex; nodeIndex < size && mSs.isFreeIndex(nodeIndex);)
      ++nodeIndex;
  }
  return *this;
}

inline const scene::SimpleScene::iterator &scene::SimpleScene::iterator::operator++()
{
  uint32_t size = mSs.getNodesCount();
  for (++nodeIndex; nodeIndex < size && mSs.isFreeIndex(nodeIndex);)
    ++nodeIndex;
  return *this;
}

inline scene::SimpleScene::iterator scene::SimpleScene::iterator::operator++(int)
{
  iterator temp = *this;
  ++*this;
  return temp;
}

inline vec4f scene::SimpleScene::getNodeBSphere(node_index node) const
{
  const uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return v_zero();
  return get_node_bsphere(nodes.data()[index]);
}

inline bbox3f scene::SimpleScene::calcNodeBox(mat44f_cref m) const
{
  bbox3f box;
  const pool_index pool = get_node_pool(m);
  if (pool < poolBox.size())
    v_bbox3_init(box, m, getPoolBboxInternal(pool));
  else
  {
    box = calcNodeBoxFromSphere(m);
  }
  return box;
}

inline bbox3f scene::SimpleScene::calcNodeBoxFromSphere(mat44f_cref m) const
{
  bbox3f box;
  vec4f sphere = get_node_bsphere(m);
  box.bmin = v_sub(sphere, v_splat_w(sphere));
  box.bmax = v_add(sphere, v_splat_w(sphere));
  return box;
}

inline bbox3f scene::SimpleScene::calcNodeBox(node_index node) const
{
  const uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
  {
    bbox3f box;
    v_bbox3_init_empty(box);
    return box;
  }
  return calcNodeBox(nodes.data()[index]);
}

inline scene::pool_index scene::SimpleScene::getNodePool(node_index node) const
{
  uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return INVALID_POOL;
  return getIndexPool(index);
}
inline uint32_t scene::SimpleScene::getNodeFlags(node_index node) const
{
  uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return 0;
  return getIndexFlags(index);
}

inline scene::node_index scene::SimpleScene::allocate(const mat44f &transform, pool_index pool, uint16_t flags)
{
  G_ASSERT_RETURN(isInWritingThread(), INVALID_NODE);
  node_index node = allocateOne();
  reallocate(node, transform, pool, flags);
  return node;
}

template <bool use_flags, typename VisibleNodesFunctor>
void scene::SimpleScene::nodesInRange(vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags,
  VisibleNodesFunctor visible_nodes, int start_index, int end_index) const
{
  test_flags <<= 16;
  equal_flags <<= 16;
  const bool hasFree = freeIndices.size() != 0;
  for (int i = firstAlive, ei = (node_index)nodes.size(), localIndex = 0; i < ei; ++i)
  {
    if (hasFree && isInvalidIndex(i))
      continue;
    if (localIndex < start_index)
    {
      localIndex++;
      continue;
    }
    if (localIndex >= end_index)
      break;
    localIndex++;

    const mat44f &m = nodes.data()[i];

    uint32_t poolFlags = get_node_pool_flags(m);
    if (use_flags && ((test_flags & poolFlags) != equal_flags))
      continue;

    vec4f sphere = get_node_bsphere(m);
    vec3f distToSphereSqScaled = v_mul_x(v_length3_sq_x(v_sub(pos_distscale, sphere)), v_splat_w(pos_distscale));
    if (v_test_vec_x_lt(v_splat_w(m.col0), distToSphereSqScaled))
      continue;

    visible_nodes(getNodeFromIndex(i), m, distToSphereSqScaled);
  }
}

template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesFunctor> // CulledContainer has to have push_back
void scene::SimpleScene::frustumCull(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags,
  const Occlusion *occlusion, VisibleNodesFunctor visible_nodes) const
{
  if (use_occlusion)
  {
    G_ASSERT_RETURN(occlusion, );
  }
  test_flags <<= 16;
  equal_flags <<= 16;
  vec3f plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W;
  v_construct_camplanes(globtm, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y);
  plane03X = v_norm3(plane03X);
  plane03Y = v_norm3(plane03Y);
  plane03Z = v_norm3(plane03Z);
  plane03W = v_norm3(plane03W);
  plane47X = v_norm3(plane47X);
  plane47Y = v_norm3(plane47Y);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);
  plane47Z = plane47X, plane47W = plane47Y; // we can use some useful planes instead of replicating
  v_mat44_transpose(plane47X, plane47Y, plane47Z, plane47W);
  const bool hasFree = freeIndices.size() != 0;

  for (int i = firstAlive, ei = (node_index)nodes.size(); i < ei; ++i)
  {
    if (hasFree && isInvalidIndex(i))
      continue;
    const mat44f &m = nodes.data()[i];

    uint32_t poolFlags = get_node_pool_flags(m);
    if (use_flags && ((test_flags & poolFlags) != equal_flags))
      continue;
    vec4f sphere = get_node_bsphere(m); // broad phase
    vec4f sphereRad = v_splat_w(sphere);
    int sphereVis;
    if (!use_pools || use_occlusion)
      sphereVis =
        v_is_visible_sphere(sphere, sphereRad, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W);
    else
      sphereVis =
        v_sphere_intersect(sphere, sphereRad, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W);
    if (!sphereVis)
      continue;
    // more correct is using z-distance, but it will be view dependent. We avoid that by using eucledian distance.
    vec3f distToSphereSqScaled = v_mul_x(v_length3_sq_x(v_sub(pos_distscale, sphere)), v_splat_w(pos_distscale));
    // vec3f invClipSpaceRadius = v_mul_x(v_mul_x(distToSphereSq, v_splat_w(pos_distscale)), v_rcp_x(v_mul_x(sphereRad, sphereRad)));
    if (v_test_vec_x_lt(v_splat_w(m.col0), distToSphereSqScaled))
      continue;

    poolFlags &= 0xFFFF;
    if (use_pools && (use_occlusion || sphereVis == 2) && poolFlags < poolBox.size())
    {
      // narrow check
      mat44f clipTm;
      v_mat44_mul43(clipTm, globtm, m);
      bbox3f pool = poolBox.data()[poolFlags];
      // if (v_test_vec_x_lt(v_splat_w(pool.bmin), invClipSpaceRadius))
      //   continue;
      if (use_occlusion)
      {
        if (!occlusion->isVisibleBox(pool.bmin, pool.bmax, clipTm))
          continue;
      }
      else
      {
        if (sphereVis == 2 && !v_is_visible_b_fast_8planes(pool.bmin, pool.bmax, clipTm))
          continue;
      }
    }
    else if (use_occlusion && !occlusion->isVisibleSphere(sphere, sphereRad))
      continue;
    visible_nodes(getNodeFromIndex(i), m, distToSphereSqScaled);
  }
}

template <bool use_flags, bool use_pools, typename VisibleNodesFunctor> // VisibleNodesFunctor(scene::node_index, mat44f_cref)
void scene::SimpleScene::boxCull(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const
{
  test_flags <<= 16;
  equal_flags <<= 16;
  const bool hasFree = freeIndices.size() != 0;

  for (int i = firstAlive, ei = (node_index)nodes.size(); i < ei; ++i)
  {
    if (hasFree && isInvalidIndex(i))
      continue;
    const mat44f &m = nodes.data()[i];

    uint32_t poolFlags = get_node_pool_flags(m);
    if (use_flags && ((test_flags & poolFlags) != equal_flags))
      continue;
    vec4f sphere = get_node_bsphere(m); // broad phase
    vec4f sphereRad = v_splat_w(sphere);
    bbox3f sphereBox;
    sphereBox.bmin = v_sub(sphere, sphereRad);
    sphereBox.bmax = v_add(sphere, sphereRad);
    if (!v_bbox3_test_box_intersect(box, sphereBox))
      continue;
    const bool fullyInside = v_bbox3_test_box_inside(box, sphereBox);
    if (!fullyInside && !use_pools && !v_bbox3_test_sph_intersect(box, sphere, v_splat_w(v_mul(sphere, sphere))))
      continue;
    if (use_pools && !fullyInside)
    {
      poolFlags &= 0xFFFF;
      if (poolFlags < poolBox.size())
      {
        bbox3f transformed;
        bbox3f pool = poolBox.data()[poolFlags];
        v_bbox3_init(transformed, m, pool);
        if (!v_bbox3_test_box_intersect(transformed, box))
          continue;
      }
    }
    visible_nodes(getNodeFromIndex(i), m);
  }
}
