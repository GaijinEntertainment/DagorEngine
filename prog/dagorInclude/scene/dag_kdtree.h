//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <debug/dag_assert.h>
#include <vecmath/dag_vecMathDecl.h>
#include <dag/dag_vector.h>
// simple KDtree implementation.
// allows building
class Point3;
namespace kdtree
{
struct KDNode
{
  union
  {
    struct
    {
      vec4f bmin_start, bmax_count;
    };
    uint32_t start_count[8];
  };
  const bbox3f &getBox() const { return *(const bbox3f *)this; }
  enum
  {
    NODE_SHIFT = 18,
    INDEX_COUNT = (1 << NODE_SHIFT),
    NODES_COUNT = 1 << (32 - NODE_SHIFT),
    COUNT_MASK = (1 << NODE_SHIFT) - 1
  };
  inline uint32_t getStart() const { return start_count[3] & COUNT_MASK; }
  inline uint32_t getCount() const { return start_count[7] & COUNT_MASK; }
  inline uint32_t getLeftToParent(int parent) const { return parent + (start_count[3] >> NODE_SHIFT); } // offset to parent
  inline uint32_t getRightToLeft(int left) const { return left + (start_count[7] >> NODE_SHIFT); }      // offset to left
  inline bool isLeaf() const { return getRightToLeft(0) == 0; } // right subtree is left subtree

  inline void setStart(uint32_t v) { start_count[3] = v; }
  inline void setCount(uint32_t v) { start_count[7] = v; }
  inline void setLeft(uint32_t v)
  {
    G_ASSERTF(v < NODES_COUNT, "%d", v);
    start_count[3] |= (v << NODE_SHIFT);
  } // offset related to parent
  inline void setRight(uint32_t v)
  {
    G_ASSERTF(v < NODES_COUNT, "%d", v);
    start_count[7] |= (v << NODE_SHIFT);
  }
};

/// returns position of inserted element into nodes. Should be just equal to nodes.size() before calling
/// if boxes count in node is bigger than min_to_split_geom and size is bigger than min_box_to_split_geom - we will try to split
/// if boxes count in node is bigger than max_to_split_count and box size is bigger than max_box_to_split_count - we will try to split
/// indices are referencing to boxes
extern int make_nodes(KDNode *extern_root, const bbox3f *boxes, const float *box_centers_x, const float *box_centers_y,
  const float *box_centers_z, int from, int to, uint32_t *indices, dag::Vector<KDNode> &nodes, int min_to_split_geom = 16,
  int max_to_split_count = 64, float min_box_to_split_geom = 32.f, float max_box_to_split_count = 8.f);

enum UpdateResult
{
  NOT_UPDATED,
  UPDATED_COARSE,
  UPDATED_TIGHT
};
/// this is fast, but potentially not optimal update.
/// fast update kd-tree, if one node moved/changed
/// will return either:
/// NOT_UPDATED(taget node is not in the tree),
/// UPDATED_COARSE (kdtree is now valid, but may be not optimal, so rebuild can help)
/// UPDATED_TIGHT (kdtree is now valid, and optimal, if node bbox was within current leaf box, i.e. rebuild will unlikely change
/// anything)
template <typename Callable>
inline UpdateResult update_nodes(kdtree::KDNode *nodes, const uint32_t root, int node_id, const Callable &get_leaf_bbox)
{
  const uint32_t start = nodes[root].getStart(), end = nodes[root].getCount() + start;
  if (root < start || root >= end)
    return NOT_UPDATED;
  bbox3f box;
  UpdateResult result;
  if (nodes[root].isLeaf())
  {
    box = nodes[root].getBox();
    get_leaf_bbox(start, end, box);
    result = v_bbox3_test_box_inside(nodes[root].getBox(), box) ? UPDATED_TIGHT : UPDATED_COARSE;
  }
  else
  {
    const uint32_t leftNode = nodes[root].getLeftToParent(root), rightNode = nodes[root].getRightToLeft(leftNode);
    result = update_nodes(nodes, leftNode, node_id, get_leaf_bbox);
    if (result == NOT_UPDATED)
      result = update_nodes(nodes, rightNode, node_id, get_leaf_bbox);
    box = nodes[leftNode].getBox();
    v_bbox3_add_box(box, nodes[rightNode].getBox());
  }
  nodes[root].bmin_start = v_perm_xyzd(box.bmin, nodes[root].bmin_start);
  nodes[root].bmax_count = v_perm_xyzd(box.bmax, nodes[root].bmax_count);

  G_ASSERT(result != NOT_UPDATED);
  return result;
}
// simplest usage, int nodeToUpdate, bbox3f nodeBox
// UpdateResult ret = update_nodes(kdnodes, root, nodeToUpdate, [&nodeBox](int , int , bbox3f& cbox){v_bbox3_add_box(cbox, nodeBox);});
// more complicated usage, int nodeToUpdate, bbox3f nodeBox
// UpdateResult ret = update_nodes(kdnodes, root, nodeToUpdate, [&](int start, int end, bbox3f& cbox)
//  {v_bbox3_init_empty(cbox);for (int i = start; i < end; ++i) v_bbox3_add_box(cbox, get_node_box(i));});

inline void leaves_only(kdtree::KDNode *nodes, const kdtree::KDNode &node, int root, int &write_to)
{
  if (node.isLeaf())
  {
    nodes[write_to++] = node;
  }
  else
  {
    const uint32_t leftNode = node.getLeftToParent(root), rightNode = node.getRightToLeft(leftNode);
    const kdtree::KDNode left = nodes[leftNode], right = nodes[rightNode]; // copy
    leaves_only(nodes, left, leftNode, write_to);
    leaves_only(nodes, right, rightNode, write_to);
  }
}
}; // namespace kdtree
