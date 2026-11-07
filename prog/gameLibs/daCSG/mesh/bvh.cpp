// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/bvh.h"
#include <EASTL/sort.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
static constexpr uint32_t LEAF_TRIS = 4;

// every position is within +-FLT_MAX
static float float_below(double d)
{
  const float f = float(max(d, -double(FLT_MAX)));
  return double(f) > d ? nextafterf(f, -FLT_MAX) : f;
}

static float float_above(double d)
{
  const float f = float(min(d, double(FLT_MAX)));
  return double(f) < d ? nextafterf(f, FLT_MAX) : f;
}

bbox3f outward_box(const DPoint3 &lo, const DPoint3 &hi)
{
  bbox3f b;
  b.bmin = v_make_vec4f(float_below(lo.x), float_below(lo.y), float_below(lo.z), 0);
  b.bmax = v_make_vec4f(float_above(hi.x), float_above(hi.y), float_above(hi.z), 0);
  return b;
}

static float lane(vec4f v, int axis) { return axis == 0 ? v_extract_x(v) : axis == 1 ? v_extract_y(v) : v_extract_z(v); }

bbox3f face_box(const Solid &s, uint32_t f, double grow)
{
  const DPoint3 &p0 = s.pos[s.faceVert(f, 0)], &p1 = s.pos[s.faceVert(f, 1)], &p2 = s.pos[s.faceVert(f, 2)], g(grow, grow, grow);
  return outward_box(min(min(p0, p1), p2) - g, max(max(p0, p1), p2) + g);
}

void Bvh::build(const Solid &s)
{
  dag::Vector<bbox3f> box(s.faceCount());
  for (uint32_t f = 0; f < s.faceCount(); ++f)
    box[f] = face_box(s, f);
  build(make_span_const(box));
}

void Bvh::build(dag::ConstSpan<bbox3f> box, dag::ConstSpan<uint32_t> ids)
{
  const uint32_t faceCount = box.size();
  nodes.clear();
  nodes.reserve(faceCount); // leaves hold 2 or more: fewer nodes
  tris.resize(faceCount);
  slotItem.clear();
  dag::Vector<vec4f> center(faceCount);
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    center[f] = v_bbox3_center(box[f]);
    tris[f] = f;
  }

  struct Item
  {
    uint32_t first, count, parent;
  };
  dag::Vector<Item> stack;
  if (faceCount)
    stack.push_back(Item{0, faceCount, NO_INDEX});
  while (!stack.empty())
  {
    const Item it = stack.back();
    stack.pop_back();
    const uint32_t idx = nodes.size();
    if (it.parent != NO_INDEX)
      nodes[it.parent].right = idx;
    bbox3f centerBox;
    v_bbox3_init_empty(centerBox);
    nodes.push_back(Node{centerBox, it.first, it.count, NO_INDEX}); // refit sets the box
    if (it.count <= LEAF_TRIS)
      continue;
    // the split axis: the longest side of the centers' box
    for (uint32_t i = it.first; i < it.first + it.count; ++i)
      v_bbox3_add_pt(centerBox, center[tris[i]]);
    const vec3f size = v_bbox3_size(centerBox);
    const float sx = v_extract_x(size), sy = v_extract_y(size), sz = v_extract_z(size);
    const int axis = sx >= sy && sx >= sz ? 0 : sy >= sz ? 1 : 2;
    const uint32_t mid = it.first + it.count / 2;
    eastl::nth_element(tris.begin() + it.first, tris.begin() + mid, tris.begin() + it.first + it.count, [&](uint32_t a, uint32_t b) {
      const float ca = lane(center[a], axis), cb = lane(center[b], axis);
      return ca < cb || (ca == cb && a < b);
    });
    stack.push_back(Item{mid, it.first + it.count - mid, idx});
    stack.push_back(Item{it.first, mid - it.first, NO_INDEX});
  }

  triBox.resize(faceCount);
  slotOf.clear();
  refit(box);
  if (ids.empty())
    return;
  slotItem = tris;
  for (uint32_t &t : tris)
    t = ids[t];
}

void Bvh::refitNode(uint32_t idx)
{
  Node &n = nodes[idx];
  if (n.right != NO_INDEX)
    n.box = v_bbox3_sum(nodes[n.right].box, nodes[idx + 1].box);
  else
    for (uint32_t i = n.first; i < n.first + n.count; ++i)
      n.box = i == n.first ? triBox[i] : v_bbox3_sum(n.box, triBox[i]);
}

// a node precedes its children, the left one next to it; the slots stay, so slotOf does
void Bvh::refit(dag::ConstSpan<bbox3f> box)
{
  for (uint32_t i = 0; i < tris.size(); ++i)
    if (itemAt(i) != NO_INDEX)
      triBox[i] = box[itemAt(i)];
  for (uint32_t idx = nodes.size(); idx-- > 0;)
    refitNode(idx);
}

void Bvh::markPath(uint32_t slot)
{
  for (uint32_t idx = 0;;)
  {
    dirty.push_back(idx);
    if (nodes[idx].right == NO_INDEX)
      return;
    idx = slot < nodes[nodes[idx].right].first ? idx + 1 : nodes[idx].right;
  }
}

// children first, so each node reads fresh ones
void Bvh::refitMarked()
{
  eastl::sort(dirty.begin(), dirty.end(), [](uint32_t a, uint32_t b) { return a > b; });
  for (uint32_t i = 0; i < dirty.size(); ++i)
    if (!i || dirty[i] != dirty[i - 1])
      refitNode(dirty[i]);
  dirty.clear();
}

void Bvh::refit(dag::ConstSpan<bbox3f> box, dag::ConstSpan<uint32_t> items)
{
  // by item id, whose domain is the boxes given: a renumber can put an id past the slot count
  if (slotOf.size() < box.size())
  {
    slotOf.assign(box.size(), NO_INDEX);
    for (uint32_t slot = 0; slot < tris.size(); ++slot)
      if (itemAt(slot) < slotOf.size())
        slotOf[itemAt(slot)] = slot;
  }
  // many items: one pass over all nodes beats sorting their paths
  const bool few = items.size() * 32 < nodes.size();
  for (uint32_t item : items)
  {
    if (item >= slotOf.size() || slotOf[item] == NO_INDEX)
      continue;
    triBox[slotOf[item]] = box[item];
    if (few)
      markPath(slotOf[item]);
  }
  if (few)
    refitMarked();
  else
    for (uint32_t idx = nodes.size(); idx-- > 0;)
      refitNode(idx);
}

void Bvh::renumber(dag::ConstSpan<uint32_t> new_id)
{
  slotOf.clear();
  dag::Vector<uint32_t> dropped;
  for (uint32_t slot = 0; slot < tris.size(); ++slot)
  {
    if (itemAt(slot) == NO_INDEX)
      continue;
    const uint32_t now = new_id[itemAt(slot)];
    (slotItem.empty() ? tris : slotItem)[slot] = now;
    if (now != NO_INDEX)
      continue;
    tris[slot] = NO_INDEX;
    v_bbox3_init_empty(triBox[slot]);
    dropped.push_back(slot);
  }
  if (dropped.size() * 32 >= nodes.size())
    for (uint32_t idx = nodes.size(); idx-- > 0;)
      refitNode(idx);
  else
  {
    for (uint32_t slot : dropped)
      markPath(slot);
    refitMarked();
  }
}

void Bvh::collect(const Bvh &a, const Bvh &b, bool self, bool sorted, dag::Vector<TriPair> &pairs, uint32_t limit)
{
  pairs.clear();
  if (a.nodes.empty() || b.nodes.empty())
    return;
  dag::Vector<TriPair> stack;
  stack.push_back(TriPair(0, 0));
  while (!stack.empty() && pairs.size() <= limit)
  {
    const TriPair top = stack.back();
    stack.pop_back();
    const Node &na = a.nodes[top.first], &nb = b.nodes[top.second];
    const bool aLeaf = na.right == NO_INDEX, bLeaf = nb.right == NO_INDEX;
    if (self && top.first == top.second)
    {
      if (aLeaf)
      {
        for (uint32_t i = na.first; i < na.first + na.count; ++i)
          for (uint32_t j = i + 1; j < na.first + na.count; ++j)
            if (a.tris[i] != NO_INDEX && a.tris[j] != NO_INDEX && v_bbox3_test_box_intersect(a.triBox[i], a.triBox[j]))
              pairs.push_back(TriPair(eastl::min(a.tris[i], a.tris[j]), eastl::max(a.tris[i], a.tris[j])));
        continue;
      }
      stack.push_back(TriPair(top.first + 1, top.first + 1));
      stack.push_back(TriPair(na.right, na.right));
      stack.push_back(TriPair(top.first + 1, na.right));
      continue;
    }
    if (!v_bbox3_test_box_intersect(na.box, nb.box))
      continue;
    if (aLeaf && bLeaf)
    {
      for (uint32_t i = na.first; i < na.first + na.count; ++i)
        for (uint32_t j = nb.first; j < nb.first + nb.count; ++j)
          if (a.tris[i] != NO_INDEX && b.tris[j] != NO_INDEX && v_bbox3_test_box_intersect(a.triBox[i], b.triBox[j]))
            pairs.push_back(
              self ? TriPair(eastl::min(a.tris[i], b.tris[j]), eastl::max(a.tris[i], b.tris[j])) : TriPair(a.tris[i], b.tris[j]));
    }
    else if (!aLeaf && (bLeaf || na.count >= nb.count))
    {
      stack.push_back(TriPair(top.first + 1, top.second));
      stack.push_back(TriPair(na.right, top.second));
    }
    else
    {
      stack.push_back(TriPair(top.first, top.second + 1));
      stack.push_back(TriPair(top.first, nb.right));
    }
  }
  if (sorted)
    eastl::sort(pairs.begin(), pairs.end());
}

void Bvh::overlaps(const Bvh &other, dag::Vector<TriPair> &pairs) const { collect(*this, other, false, true, pairs); }

void Bvh::selfOverlaps(dag::Vector<TriPair> &pairs) const { collect(*this, *this, true, true, pairs); }

void Bvh::selfOverlapsUnsorted(dag::Vector<TriPair> &pairs, uint32_t cap) const { collect(*this, *this, true, false, pairs, cap); }

void Bvh::overlapping(const bbox3f &pointBox, dag::Vector<uint32_t> &out) const
{
  out.clear();
  if (nodes.empty())
    return;
  uint32_t stack[64]; // median splits keep the depth under 33
  int top = 0;
  stack[top++] = 0;
  while (top)
  {
    const uint32_t idx = stack[--top];
    const Node &n = nodes[idx];
    if (!v_bbox3_test_box_intersect(n.box, pointBox))
      continue;
    if (n.right == NO_INDEX)
    {
      for (uint32_t i = n.first; i < n.first + n.count; ++i)
        if (tris[i] != NO_INDEX && v_bbox3_test_box_intersect(triBox[i], pointBox))
          out.push_back(tris[i]);
      continue;
    }
    stack[top++] = idx + 1;
    stack[top++] = n.right;
  }
  eastl::sort(out.begin(), out.end());
}

static double box_dist_sq(const bbox3f &b, const DPoint3 &p)
{
  alignas(16) float lo[4], hi[4];
  v_st(lo, b.bmin);
  v_st(hi, b.bmax);
  double d = 0;
  for (int i = 0; i < 3; ++i)
  {
    const double e = max(max(lo[i] - p[i], p[i] - hi[i]), 0.0);
    d += e * e;
  }
  return d;
}

uint32_t Bvh::closest(const Solid &s, const DPoint3 &p, double &dist_sq) const
{
  struct Item
  {
    uint32_t node;
    double distSq;
  };
  Item stack[64]; // median splits keep the depth under 33
  int top = 0;
  uint32_t best = NO_INDEX;
  dist_sq = DBL_MAX;
  if (!nodes.empty())
    stack[top++] = Item{0, 0};
  while (top)
  {
    const Item it = stack[--top];
    if (it.distSq >= dist_sq)
      continue;
    const Node &n = nodes[it.node];
    if (n.right == NO_INDEX)
    {
      for (uint32_t i = n.first; i < n.first + n.count; ++i)
      {
        const uint32_t f = itemAt(i);
        if (f == NO_INDEX)
          continue;
        const double d = point_triangle_dist_sq(p, s.pos[s.faceVert(f, 0)], s.pos[s.faceVert(f, 1)], s.pos[s.faceVert(f, 2)]);
        if (d < dist_sq)
        {
          dist_sq = d;
          best = tris[i];
        }
      }
      continue;
    }
    const Item l{it.node + 1, box_dist_sq(nodes[it.node + 1].box, p)}, r{n.right, box_dist_sq(nodes[n.right].box, p)};
    stack[top++] = l.distSq < r.distSq ? r : l;
    stack[top++] = l.distSq < r.distSq ? l : r;
  }
  return best;
}
} // namespace dacsg
