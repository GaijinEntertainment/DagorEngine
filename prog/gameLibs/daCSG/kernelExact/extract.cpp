// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include "mesh/solidUtil.h"

namespace dacsg::exact
{
// 1 for a leaf below an odd number of Subtract right children (spec 5.4)
static void leaf_flips(const BoolTree &tree, dag::Vector<uint8_t> &flip)
{
  dag::Vector<uint8_t> nodeFlip(tree.nodes.size(), 0);
  flip.assign(tree.leaves.size(), 0);
  for (uint32_t i = tree.nodes.size(); i-- > 0;)
  {
    const BoolTree::Node &n = tree.nodes[i];
    if (n.kind == BoolTree::Kind::Leaf)
      flip[n.leaf] = nodeFlip[i];
    else
    {
      nodeFlip[n.child[0]] = nodeFlip[i];
      nodeFlip[n.child[1]] = nodeFlip[i] ^ (n.kind == BoolTree::Kind::Subtract);
    }
  }
}

static bool cell_inside(const BoolTree &tree, const int32_t *w, dag::Vector<uint8_t> &value)
{
  value.resize(tree.nodes.size());
  for (uint32_t i = 0; i < tree.nodes.size(); ++i)
  {
    const BoolTree::Node &n = tree.nodes[i];
    const bool a = n.kind == BoolTree::Kind::Leaf ? false : value[n.child[0]],
               b = n.kind == BoolTree::Kind::Leaf ? false : value[n.child[1]];
    switch (n.kind)
    {
      case BoolTree::Kind::Leaf: value[i] = w[n.leaf] > 0; break;
      case BoolTree::Kind::Subtract: value[i] = a && !b; break;
      case BoolTree::Kind::Intersect: value[i] = a && b; break;
      // Concat operands never touch, spec 5.2
      case BoolTree::Kind::Union:
      case BoolTree::Kind::Concat: value[i] = a || b; break;
    }
  }
  return value.back() != 0;
}

// the first member in leaf order whose face, flipped by its subtract parity, faces the out cell
static LeafRef provenance(const Arrangement &a, const Cells &c, const Face &f, dag::ConstSpan<uint8_t> flip, bool out_front)
{
  const Tri *best = nullptr;
  for (uint32_t m = f.firstMember; m < f.firstMember + f.memberCount; ++m)
  {
    const Tri &t = a.tris[c.members[m]];
    const bool faces = (t.orient > 0) != (flip[t.leaf] != 0);
    if (faces == out_front && (!best || t.leaf < best->leaf))
      best = &t;
  }
  if (!best)
    best = &a.tris[c.members[f.firstMember]];
  return LeafRef{best->leaf, best->face};
}

bool extract_solid(const Arrangement &a, const BoolTree &tree, const Cells &c, double tolerance, Solid &out, eastl::string &err)
{
  out = Solid();
  dag::Vector<uint8_t> flip, value, inside(c.shellCount);
  leaf_flips(tree, flip);
  for (uint32_t s = 0; s < c.shellCount; ++s)
    inside[s] = cell_inside(tree, &c.shellWinding[size_t(s) * c.leafCount], value);

  dag::Vector<OutFace> faces;
  for (uint32_t i = 0; i < c.faces.size(); ++i)
  {
    const bool inFront = inside[c.faceShell[i * 2]], inBack = inside[c.faceShell[i * 2 + 1]];
    if (inFront == inBack)
      continue;
    OutFace &f = faces.push_back();
    f.face = i;
    for (int k = 0; k < 3; ++k)
      f.v[k] = c.faces[i].v[inBack ? k : 2 - k];
    f.ref = provenance(a, c, c.faces[i], make_span_const(flip), inBack);
  }
  if (!output_pass(a, c, tolerance, faces, out, err))
    return false;
  out.face.resize(faces.size());
  out.leafRef.resize(faces.size());
  for (uint32_t i = 0; i < faces.size(); ++i)
  {
    out.leafRef[i] = faces[i].ref;
    out.face[i] = tree.leaves[faces[i].ref.leaf].face[faces[i].ref.leafFace];
  }
  eastl::string closedErr;
  if (!build_halfedges(out, closedErr))
  {
    err.sprintf("the arrangement output is %s", closedErr.c_str());
    return false;
  }
  compute_vertex_normals(out);
  return true;
}
} // namespace dacsg::exact
