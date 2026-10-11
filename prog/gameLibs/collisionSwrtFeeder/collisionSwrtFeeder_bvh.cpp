// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <daSWRT/swBVH.h>
#include <daSWRT/swBLASBoxResemblance.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <debug/dag_assert.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMath.h>


// Distance under which a BLAS this box-like renders as an analytic box (SWRT LOD knob).
// encoding must match the tree bytes: the splice names it.
static float scoreDimAsBoxDist(const uint8_t *tree, int tree_bytes, bbox3f_cref box, float dim_min, float dim_max,
  daSWRT::BlasBoxEncoding encoding)
{
  if (dim_max <= dim_min)
    return dim_max;
  const float boxLike = daSWRT::computeBlasBoxResemblanceVoxel(tree, 0, tree_bytes, box, encoding);
  return lerp(dim_max, dim_min, powf(boxLike, 1.5f));
}

bool CollisionGeometryFeeder::buildSwrtBLAS(const CollisionResource &coll_res, const PhysMatFilter &node_filter, float dim_as_box_min,
  float dim_as_box_max, const PhysMatLeafUser &leaf_user, bool box_user, daSWRT::BuiltBLAS &built)
{
  const auto allNodes = coll_res.getAllNodes();

  // The game's physmat filter, asked per FACE (one collision node can carry several materials); an
  // empty filter keeps everything.
  auto faceMaterialPasses = [&node_filter](int phys_mat_id) { return !node_filter || node_filter(phys_mat_id); };

  // Eligibility is the trace's own early-out plus a well-formedness guard, and the node's material
  // set answers "any face kept" (eligibility) and "all faces kept" (the splice filter) in one walk.
  Tab<int> nodeIndices(framemem_ptr());
  bool allFacesKept = true;
  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!node || !node->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    // a mirrored/singular live pose hides the node from CPU traces; SWRT must match
    if (!coll_res.getDefaultInstance().isNodeTraceable(node->nodeIndex))
      continue;
    if (node->type != COLLISION_NODE_TYPE_MESH && node->type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    if (coll_res.getNodeVertCount(ni) == 0 || coll_res.getNodeFaceCount(ni) == 0)
      continue;
    bool anyKept = !node_filter, allKept = true;
    if (node_filter)
      // The own-authority / material-less dispatch lives on the resource, one home with the
      // per-face enumeration.
      coll_res.classifyNodeMaterials(*node, faceMaterialPasses, anyKept, allKept);
    if (!anyKept)
      continue;
    nodeIndices.push_back(ni);
    allFacesKept &= allKept;
  }
  if (nodeIndices.empty())
    return false;

  ChunkSpliceInfo info;
  if (!buildSwrtChunkSpliceBLAS(coll_res, nodeIndices, built, info, allFacesKept ? PhysMatFilter{} : node_filter, leaf_user, box_user))
    return false;
  // The producer names its byte format, so the score decoder cannot drift from the bytes. A carved model is
  // scored on its carved tree too: at its box distance the shadow of a carved-out window does not matter.
  built.dimAsBoxDist =
    scoreDimAsBoxDist(built.data.data(), (int)built.treeBytes, built.box, dim_as_box_min, dim_as_box_max, info.boxEncoding);
  return true;
}

int CollisionGeometryFeeder::buildSwrtBLASFromCollisionResource(RenderSWRT &swrt, const CollisionResource &coll_res,
  const PhysMatFilter &node_filter, float dim_as_box_min, float dim_as_box_max, const PhysMatLeafUser &leaf_user)
{
  G_ASSERTF_RETURN(swrt.blasVertsFp16, -1, "swrtFeeder: the chunk splice writes fp16 verts only");
  daSWRT::BuiltBLAS built;
  if (!buildSwrtBLAS(coll_res, node_filter, dim_as_box_min, dim_as_box_max, leaf_user, swrt.tlasBoxUser, built))
    return -1;
  return swrt.addBuiltModel(eastl::move(built));
}
