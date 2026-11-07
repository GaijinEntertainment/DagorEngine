// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "mesh/solidUtil.h"
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <vecmath/dag_vecMath.h>

namespace dacsg
{
// Conservative float box of a double range
bbox3f outward_box(const DPoint3 &lo, const DPoint3 &hi);
bbox3f face_box(const Solid &s, uint32_t f, double grow = 0);

// query results are sorted unless named Unsorted
class Bvh
{
public:
  void build(const Solid &s);
  // item i gets boxes[i]; queries return ids[i], or i when ids is empty
  void build(dag::ConstSpan<bbox3f> boxes, dag::ConstSpan<uint32_t> ids = {});
  // new boxes by item id, boxes spanning every id a renumber gave; the tree keeps its shape, a dropped item stays out
  void refit(dag::ConstSpan<bbox3f> boxes);
  // as refit, for the listed items only
  void refit(dag::ConstSpan<bbox3f> boxes, dag::ConstSpan<uint32_t> items);
  // item i becomes new_id[i], new_id spanning every current id, keeping its query id when built with ids; NO_INDEX
  // drops it from the box queries
  void renumber(dag::ConstSpan<uint32_t> new_id);
  void overlaps(const Bvh &other, dag::Vector<TriPair> &pairs) const;
  // first < second
  void selfOverlaps(dag::Vector<TriPair> &pairs) const;
  // as selfOverlaps, in the walk's order; the walk stops once pairs passes cap
  void selfOverlapsUnsorted(dag::Vector<TriPair> &pairs, uint32_t cap = ~0u) const;
  void overlapping(const bbox3f &box, dag::Vector<uint32_t> &out) const;
  // s is the Solid the items index; the query id of the closest, NO_INDEX when empty
  uint32_t closest(const Solid &s, const DPoint3 &p, double &dist_sq) const;

private:
  struct Node
  {
    bbox3f box;
    uint32_t first, count;
    uint32_t right; // NO_INDEX: leaf, else left child follows
  };

  static void collect(const Bvh &a, const Bvh &b, bool self, bool sorted, dag::Vector<TriPair> &pairs, uint32_t limit = ~0u);
  void refitNode(uint32_t idx);
  void markPath(uint32_t slot);
  void refitMarked();
  uint32_t itemAt(uint32_t slot) const { return slotItem.empty() ? tris[slot] : slotItem[slot]; }

  dag::Vector<Node> nodes;
  // tris: the query ids; slotItem: the items, when the ids differ
  dag::Vector<uint32_t> tris, slotItem;
  dag::Vector<bbox3f> triBox;          // in the order of tris
  dag::Vector<uint32_t> slotOf, dirty; // of the partial refits
};
} // namespace dacsg
