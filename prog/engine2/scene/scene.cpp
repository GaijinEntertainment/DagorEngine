#include <vecmath/dag_vecMath.h>
#include <scene/dag_scene.h>
#include <startup/dag_globalSettings.h>

static __forceinline vec4f v_get_max_scale_sq(vec4f col0, vec4f col1, vec4f col2)
{
  vec4f res = v_max(v_max(v_length3_sq_x(col0), v_length3_sq_x(col1)), v_length3_sq_x(col2));
  res = v_add(res, v_add(v_abs(v_dot3(col0, col1)), v_add(v_abs(v_dot3(col0, col2)), v_abs(v_dot3(col1, col2)))));
  return res;
}

void scene::SimpleScene::reserve(int reservedNodesNumber) { nodes.reserve(reservedNodesNumber); }

void scene::SimpleScene::term()
{
  clearNodes();

  nodes.clear();
  poolBox.clear();
  poolSphereVerticalCenter.clear();
  freeIndices.clear();
  writingThread = -1;
}

void scene::SimpleScene::setTransformNoScaleChange(node_index node, mat44f_cref transform)
{
  uint32_t index = getNodeIndex(node);
  G_ASSERTF_RETURN(index < nodes.size(), , "invalid node index=%d nodes.size()=%d", index, nodes.size());
  G_ASSERTF_RETURN(isInWritingThread(), , "%s", "not in writing thread");
  // todo: check if scale has not increased in dev mode
  mat44f &m = nodes[index];
  m.col0 = v_perm_xyzd(transform.col0, m.col0);
  m.col1 = v_perm_xyzd(transform.col1, m.col1);
  m.col2 = v_perm_xyzd(transform.col2, m.col2);
  m.col3 = v_perm_xyzd(transform.col3, m.col3);
}

void scene::SimpleScene::setTransform(node_index node, mat44f_cref transform)
{
  uint32_t index = getNodeIndex(node);
  G_ASSERTF_RETURN(index < nodes.size(), , "invalid node index=%d nodes.size()=%d", index, nodes.size());
  G_ASSERTF_RETURN(isInWritingThread(), , "%s", "not in writing thread");
  mat44f &m = nodes[index];
  // no pool available, just rescale existing sphere radius based on maximum scale
  vec4f maxScaleSq = v_get_max_scale_sq(transform.col0, transform.col1, transform.col2);
  vec3f cradius = m.col3;
  pool_index pool = getIndexPool(index);
  vec4f disappearDistSq;
  if (pool < poolBox.size())
  {
    cradius = v_mul(poolBox.data()[pool].bmax, v_splat_x(v_sqrt_x(maxScaleSq))); // we keep lbbox sphere radius in bmax.w
    disappearDistSq = v_mul(v_mul(cradius, cradius), v_splat_w(poolBox.data()[pool].bmin));
  }
  else
  {
    vec4f oldScaleSq = v_get_max_scale_sq(m.col0, m.col1, m.col2);
    cradius = v_mul(cradius, v_splat_x(v_sqrt_x(v_div(maxScaleSq, v_max(oldScaleSq, V_C_EPS_VAL))))); // avoid division by zero
    disappearDistSq = v_mul(v_mul(cradius, cradius), v_splats(defaultDisappearSq));
  }
  m.col3 = v_perm_xyzd(transform.col3, cradius);
  m.col0 = v_perm_xyzd(transform.col0, disappearDistSq);
  m.col1 = v_perm_xyzd(transform.col1, m.col1);
  m.col2 = v_perm_xyzd(transform.col2, m.col2);
}

void scene::SimpleScene::resizePools(pool_index pool)
{
  if (pool < poolBox.size())
    return;
  G_ASSERT_RETURN(isInWritingThread(), );
  bbox3f empty;
  empty.bmin = V_C_UNIT_1110;
  empty.bmax = v_zero();
  poolBox.resize(pool + 1, empty);
  poolSphereVerticalCenter.resize(pool + 1, 0.f);
}

void scene::SimpleScene::setPoolBBox(pool_index pool, bbox3f_cref box, bool update_nodes)
{
  G_ASSERT_RETURN(isInWritingThread(), );
  G_ASSERT_RETURN(pool < INVALID_POOL, );
  G_ASSERT_RETURN(v_test_vec_x_le(box.bmin, box.bmax), );
  resizePools(pool);
  bbox3f oldPoolBox = poolBox[pool];
  vec3f boxCenter = v_bbox3_center(box);
  vec3f sphVerticalCenter = v_and(boxCenter, v_cast_vec4f(V_CI_MASK0100));
  vec3f maxRad = v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 0)));
  maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 4))));
  maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 1))));
  maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 7))));
  // half of bbox points are intentionally omitted, since we choose sphere center located at y of bbox3 center, i.e. radius is
  // symetrical around
  maxRad = v_splat_x(v_sqrt_x(maxRad));

  sphVerticalCenter = v_splat_y(sphVerticalCenter);
  poolBox[pool].bmin = v_perm_xyzd(box.bmin, v_splats(defaultDisappearSq));
  poolBox[pool].bmax = v_perm_xyzd(box.bmax, maxRad);
  poolSphereVerticalCenter[pool] = v_extract_x(sphVerticalCenter);

  if (update_nodes && !v_test_vec_x_lt(oldPoolBox.bmax, oldPoolBox.bmin))
  {
    for (auto node : *this)
    {
      uint32_t index = getNodeIndex(node);
      if (getIndexPool(node) == pool)
      {
        mat44f &m = nodes[index];
        m.col1 = v_perm_xyzd(m.col1, sphVerticalCenter);
        vec4f rad = v_mul(maxRad, v_splat_x(v_sqrt_x(v_get_max_scale_sq(m.col0, m.col1, m.col2))));
        m.col0 = v_perm_xyzd(m.col3, v_mul(v_mul(rad, rad), v_splat_w(poolBox[pool].bmin)));
        m.col3 = v_perm_xyzd(m.col3, rad);
      }
    }
  }
}

void scene::SimpleScene::setPoolDistanceSqScale(pool_index pool, float dist_scale_sq)
{
  G_ASSERT_RETURN(pool < INVALID_POOL, );
  resizePools(pool);
  G_ASSERT_RETURN(pool < poolBox.size(), );
  poolBox[pool].bmin = v_perm_xyzd(poolBox[pool].bmin, v_splats(dist_scale_sq));
}

void scene::SimpleScene::reallocate(node_index node, const mat44f &transform, pool_index pool, uint16_t flags)
{
  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndexInternal(node);
  if (index >= nodes.size())
    return;
  mat44f m = transform;
  m.col2 = v_perm_xyzd(m.col2, v_cast_vec4f(v_splatsi(pool | (flags << 16))));
  vec4f sphereRadius, sphereVertical, disappearDistSq;
  if (pool >= poolBox.size())
  {
    sphereRadius = v_splat_x(v_sqrt_x(v_get_max_scale_sq(m.col0, m.col1, m.col2)));
    disappearDistSq = v_mul(v_mul(sphereRadius, sphereRadius), v_splats(defaultDisappearSq));
    sphereVertical = v_zero();
  }
  else
  {
    //== fixme: only optimal if uniform scale!
    // this is just simplication for uniform scale!
    // for non-uniform scale we'd better make all calculations
    sphereRadius = v_mul(v_splat_w(poolBox[pool].bmax), v_splat_x(v_sqrt_x(v_get_max_scale_sq(m.col0, m.col1, m.col2))));
    sphereVertical = v_splats(poolSphereVerticalCenter[pool]);
    disappearDistSq = v_mul(v_mul(sphereRadius, sphereRadius), v_splat_w(poolBox.data()[pool].bmin));
  }
  m.col3 = v_perm_xyzd(m.col3, v_max(sphereRadius, V_C_EPS_VAL));
  m.col1 = v_perm_xyzd(m.col1, sphereVertical);
  m.col0 = v_perm_xyzd(m.col0, disappearDistSq);
  nodes[index] = m;
}

void scene::SimpleScene::clearNodes()
{
  G_ASSERT_RETURN(isInWritingThread(), );
  nodes.resize(0);
  freeIndices.resize(0);
  firstAlive = 0;
#if KEEP_GENERATIONS
  generations.resize(0);
#endif
}

void scene::SimpleScene::destroy(node_index node)
{
  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndex(node);
  if (index >= nodes.size())
    return;
  invalidateIndex(index);
  // simple case: node in the end of list
  if (index == nodes.size() - 1)
    nodes.pop_back();           // simple
  else if (index == firstAlive) // simple case: node in the begining of alive nodes
    firstAlive++;               // simple remove
  else
    freeIndices.push_back(index);
  if (firstAlive == nodes.size())
    clearNodes();
}

scene::node_index scene::SimpleScene::allocateOne()
{
  if (firstAlive > 0)
  {
    firstAlive--;
    return getNodeFromIndex(firstAlive);
  }
  if (freeIndices.size() == 0)
  {
    // Use more conservative growth strategy after 128K elements to avoid wasting too much memory
    if (nodes.capacity() >= (128 << 10) && nodes.capacity() == nodes.size())
      nodes.reserve(nodes.capacity() + (32 << 10));
    nodes.emplace_back();
#if KEEP_GENERATIONS
    if (generations.size() < nodes.size())
      generations.emplace_back(0);
    G_ASSERT(generations.size() >= nodes.size());
#endif
    return getNodeFromIndex(uint32_t(nodes.size() - 1));
  }
  uint32_t index = freeIndices.back();
  freeIndices.pop_back();
  G_ASSERT(index > firstAlive && index < nodes.size());
  return getNodeFromIndex(index);
}

bool scene::SimpleScene::checkConsistency() const
{
  for (int i = 0; i < nodes.size(); ++i)
  {
    bool is_free = (i < firstAlive || eastl::find(freeIndices.begin(), freeIndices.end(), i) != freeIndices.end());
    if (isInvalidIndex(i) != is_free)
    {
      logerr("broken node %d ivalid= %d, isFree", i, isInvalidIndex(i), is_free);
      return false;
    }
  }
  return true;
}

void scene::SimpleScene::setFlags(node_index node, uint16_t flags)
{
  uint32_t nodeIdx = getNodeIndexInternal(node);
  if (nodeIdx < nodes.size())
    set_node_flags(nodes[nodeIdx], flags);
}

void scene::SimpleScene::unsetFlags(node_index node, uint16_t flags)
{
  uint32_t nodeIdx = getNodeIndexInternal(node);
  if (nodeIdx < nodes.size())
    unset_node_flags(nodes[nodeIdx], flags);
}
