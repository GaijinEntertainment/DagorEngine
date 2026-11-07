// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/meshCSG/meshCSG.h>
#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/io_api.h>
#include <gameLibs/daCSG/leaf_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <gameLibs/daCSG/validate_api.h>
#include <math/dag_bounds3.h>
#include <math/dag_check_nan.h>
#include <math/dag_mesh.h>
#include <math/dag_Point3.h>
#include <util/dag_bitArray.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/unique_ptr.h>
#include <string.h>

namespace mesh_csg
{
namespace
{
struct Part
{
  dacsg::Mesh mesh; // face[].srcFace: the input face
  eastl::string name;
  uint32_t input = 0;
};

class PartSource final : public dacsg::ILeafSource
{
public:
  explicit PartSource(const Part &p) : part(p)
  {
    for (const Point3 &v : p.mesh.pos)
      box += v;
  }

  bool tessellate(const TMatrix &, float, double, double, dacsg::Mesh &out, eastl::string &) override
  {
    out = part.mesh;
    return true;
  }
  uint64_t contentHash() const override { return uint64_t(uintptr_t(this)); }
  bool equals(const dacsg::ILeafSource &o) const override { return &o == this; }
  BBox3 bbox() const override { return box; }
  const char *name() const override { return part.name.c_str(); }

private:
  const Part &part;
  BBox3 box;
};

void flip(dacsg::Mesh &m)
{
  for (uint32_t f = 0; f < m.faceCount(); ++f)
  {
    eastl::swap(m.tri[3 * f + 1], m.tri[3 * f + 2]);
    for (dacsg::Channel &c : m.channels)
      for (uint32_t d = 0; d < c.dim; ++d)
        eastl::swap(c.data[(3 * f + 1) * c.dim + d], c.data[(3 * f + 2) * c.dim + d]);
  }
}

void drop_normals(dacsg::Mesh &m)
{
  for (uint32_t i = 0; i < m.channels.size(); ++i)
    if (m.channels[i].id == dacsg::CH_NORMAL)
    {
      m.channels.erase(m.channels.begin() + i);
      return;
    }
}

// false: a vertex or its float product is not finite, so the weld sort meets no NaN and no infinity.
// Normals: signed cofactors, the inverse transpose; in double, where a float determinant underflows
// and a squared cofactor overflows at world scales far from 1
bool to_world(dacsg::Mesh &m, const TMatrix &wtm, eastl::string &why)
{
  const DPoint3 c0(wtm.getcol(0)), c1(wtm.getcol(1)), c2(wtm.getcol(2));
  for (uint32_t i = 0; i < m.pos.size(); ++i)
  {
    const Point3 p = m.pos[i], w = wtm * p;
    if (!check_finite(p.x) || !check_finite(p.y) || !check_finite(p.z))
      return (why.sprintf("vertex %u is not finite", i), false);
    if (!check_finite(w.x) || !check_finite(w.y) || !check_finite(w.z))
      return (why.sprintf("vertex %u is outside the float range in world space", i), false);
    m.pos[i] = w;
  }
  const double sign = c0 * (c1 % c2) < 0 ? -1 : 1;
  if (dacsg::Channel *n = m.findChannel(dacsg::CH_NORMAL))
  {
    const DPoint3 cx = (c1 % c2) * sign, cy = (c2 % c0) * sign, cz = (c0 % c1) * sign;
    for (uint32_t i = 0; i + 2 < n->data.size(); i += 3)
    {
      const DPoint3 v = cx * n->data[i] + cy * n->data[i + 1] + cz * n->data[i + 2];
      const double len = length(v);
      const DPoint3 u = len > 0 ? v / len : v;
      const float out[3] = {float(u.x), float(u.y), float(u.z)};
      memcpy(&n->data[i], out, sizeof(out));
    }
  }
  if (sign < 0)
    flip(m);
  return true;
}

// keeps face order, drops unused vertices
void take_faces(const Part &src, dag::ConstSpan<uint32_t> faces, Part &dst)
{
  const dacsg::Mesh &s = src.mesh;
  dacsg::Mesh &d = dst.mesh;
  d = dacsg::Mesh();
  dst.input = src.input;
  dst.name = src.name;
  dag::Vector<uint32_t> remap(s.pos.size(), dacsg::NO_INDEX);
  for (uint32_t f : faces)
  {
    for (uint32_t k = 0; k < 3; ++k)
    {
      const uint32_t v = s.tri[3 * f + k];
      if (remap[v] == dacsg::NO_INDEX)
      {
        remap[v] = d.pos.size();
        d.pos.push_back(s.pos[v]);
      }
      d.tri.push_back(remap[v]);
    }
    d.face.push_back(s.face[f]);
  }
  for (const dacsg::Channel &c : s.channels)
  {
    dacsg::Channel &dc = d.channels.push_back();
    dc.id = c.id;
    dc.dim = c.dim;
    dc.data.reserve(faces.size() * 3 * c.dim);
    for (uint32_t f : faces)
      dc.data.insert(dc.data.end(), c.data.begin() + 3 * f * c.dim, c.data.begin() + 3 * (f + 1) * c.dim);
  }
}

// zero area; dropping keeps neighbours paired
void drop_repeated(const Part &src, Part &dst)
{
  dag::Vector<uint32_t> keep;
  keep.reserve(src.mesh.faceCount());
  for (uint32_t f = 0; f < src.mesh.faceCount(); ++f)
  {
    const uint32_t *t = &src.mesh.tri[3 * f];
    if (t[0] != t[1] && t[1] != t[2] && t[2] != t[0])
      keep.push_back(f);
  }
  take_faces(src, keep, dst);
}

void weld_equal_positions(dacsg::Mesh &m)
{
  dag::Vector<uint32_t> order(m.pos.size());
  for (uint32_t i = 0; i < order.size(); ++i)
    order[i] = i;
  const auto less = [&](uint32_t a, uint32_t b) {
    const Point3 &p = m.pos[a], &q = m.pos[b];
    return p.x != q.x ? p.x < q.x : p.y != q.y ? p.y < q.y : p.z != q.z ? p.z < q.z : a < b;
  };
  eastl::sort(order.begin(), order.end(), less);
  dag::Vector<uint32_t> rep(m.pos.size());
  for (uint32_t i = 0; i < order.size(); ++i)
  {
    const Point3 &p = m.pos[order[i]];
    const bool same = i > 0 && p.x == m.pos[order[i - 1]].x && p.y == m.pos[order[i - 1]].y && p.z == m.pos[order[i - 1]].z;
    rep[order[i]] = same ? rep[order[i - 1]] : order[i];
  }
  for (uint32_t &v : m.tri)
    v = rep[v];
}

uint32_t defects(const dacsg::ValidationReport &v)
{
  return v.nonFiniteVerts + v.unpairedHalfedges + v.misorientedEdges + v.duplicateFaces + v.zeroAreaFaces + v.misalignedArrays +
         v.extraFans;
}

uint32_t find_root(dag::Vector<uint32_t> &parent, uint32_t i)
{
  while (parent[i] != i)
    i = parent[i] = parent[parent[i]];
  return i;
}

// the faces of each vertex-connected shell, in face order
void shell_faces(const dacsg::Mesh &m, dag::Vector<dag::Vector<uint32_t>> &shellFaces)
{
  dag::Vector<uint32_t> parent(m.pos.size());
  for (uint32_t i = 0; i < parent.size(); ++i)
    parent[i] = i;
  for (uint32_t f = 0; f < m.faceCount(); ++f)
    for (uint32_t k = 1; k < 3; ++k)
      parent[find_root(parent, m.tri[3 * f + k])] = find_root(parent, m.tri[3 * f]);
  dag::Vector<uint32_t> shellOfRoot(m.pos.size(), dacsg::NO_INDEX);
  shellFaces.clear();
  for (uint32_t f = 0; f < m.faceCount(); ++f)
  {
    uint32_t &shell = shellOfRoot[find_root(parent, m.tri[3 * f])];
    if (shell == dacsg::NO_INDEX)
    {
      shell = shellFaces.size();
      shellFaces.push_back();
    }
    shellFaces[shell].push_back(f);
  }
}

DPoint3 corner_of(const dacsg::Mesh &m, uint32_t f, uint32_t k) { return DPoint3(m.pos[m.tri[3 * f + k]]); }

// from a vertex of the shell, as dacsg::signed_volume: from the origin, a shell far away cancels to noise
double shell_volume(const dacsg::Mesh &m, dag::ConstSpan<uint32_t> faces)
{
  const DPoint3 o = corner_of(m, faces[0], 0);
  double v = 0;
  for (uint32_t f : faces)
    v += (corner_of(m, f, 0) - o) * ((corner_of(m, f, 1) - o) % (corner_of(m, f, 2) - o));
  return v / 6;
}

// the generalized winding number about p: 1 inside an outward closed shell, 0 outside
double winding(const dacsg::Mesh &m, dag::ConstSpan<uint32_t> faces, const DPoint3 &p)
{
  constexpr double FOUR_PI = 12.566370614359172;
  double sum = 0;
  for (uint32_t f : faces)
  {
    const DPoint3 a = corner_of(m, f, 0) - p, b = corner_of(m, f, 1) - p, c = corner_of(m, f, 2) - p;
    const double la = length(a), lb = length(b), lc = length(c);
    sum += 2 * atan2(a * (b % c), la * lb * lc + (a * b) * lc + (a * c) * lb + (b * c) * la);
  }
  return sum / FOUR_PI;
}

// the outward shells; from two, each one's faces by shell index, with the cavities it is the smallest shell around
uint32_t outward_shells(const dacsg::Mesh &m, dag::Vector<dag::Vector<uint32_t>> &shellFaces)
{
  dag::Vector<dag::Vector<uint32_t>> shells;
  shell_faces(m, shells);
  dag::Vector<double> volume(shells.size());
  dag::Vector<BBox3> box(shells.size());
  uint32_t outward = 0, largest = 0;
  for (uint32_t i = 0; i < shells.size(); ++i)
  {
    volume[i] = shell_volume(m, make_span_const(shells[i]));
    for (uint32_t f : shells[i])
      for (uint32_t k = 0; k < 3; ++k)
        box[i] += m.pos[m.tri[3 * f + k]];
    outward += volume[i] > 0;
    largest = volume[i] > volume[largest] ? i : largest;
  }
  if (outward < 2)
    return outward;
  shellFaces.resize(shells.size());
  for (uint32_t i = 0; i < shells.size(); ++i)
  {
    uint32_t holder = volume[i] > 0 ? i : dacsg::NO_INDEX;
    if (holder == dacsg::NO_INDEX)
    {
      const uint32_t f = shells[i][0];
      const DPoint3 inside = (corner_of(m, f, 0) + corner_of(m, f, 1) + corner_of(m, f, 2)) / 3;
      for (uint32_t j = 0; j < shells.size(); ++j)
        if (volume[j] > 0 && (holder == dacsg::NO_INDEX || volume[j] < volume[holder]) && (box[j] & box[i].lim[0]) &&
            (box[j] & box[i].lim[1]) && winding(m, make_span_const(shells[j]), inside) > 0.5)
          holder = j;
      // none holds a valid solid's cavity: then the largest
      holder = holder == dacsg::NO_INDEX ? largest : holder;
    }
    shellFaces[holder].insert(shellFaces[holder].end(), shells[i].begin(), shells[i].end());
  }
  for (dag::Vector<uint32_t> &faces : shellFaces)
    eastl::sort(faces.begin(), faces.end());
  return outward;
}

void take_shells(const Part &part, dag::ConstSpan<dag::Vector<uint32_t>> shellFaces, dag::Vector<Part> &out)
{
  for (uint32_t i = 0; i < shellFaces.size(); ++i)
    if (!shellFaces[i].empty())
    {
      Part &p = out.push_back();
      take_faces(part, make_span_const(shellFaces[i]), p);
      p.name.append_sprintf(", shell %u", i);
    }
}

// one operand of nested outward shells would keep the inner one's faces through the union
void split_outward(Part &part, dag::Vector<Part> &parts, Result &r)
{
  dag::Vector<dag::Vector<uint32_t>> shellFaces;
  const uint32_t outward = outward_shells(part.mesh, shellFaces);
  if (outward < 2)
  {
    parts.push_back(eastl::move(part));
    return;
  }
  take_shells(part, make_span_const(shellFaces), parts);
  r.warnings.push_back().sprintf("%s: %u outward shells, an operand each", part.name.c_str(), outward);
}

// a self-intersecting input splits when each outward shell with its cavities is valid alone
bool split_shells(const Part &part, dag::Vector<Part> &parts, Result &r)
{
  dag::Vector<dag::Vector<uint32_t>> shellFaces;
  dag::Vector<Part> shells;
  if (outward_shells(part.mesh, shellFaces) < 2)
    return false;
  take_shells(part, make_span_const(shellFaces), shells);
  for (const Part &shell : shells)
  {
    dacsg::Solid s;
    dacsg::mesh_to_solid(shell.mesh, s);
    if (!dacsg::validate(s, dacsg::VALIDATE_SELF_INTERSECTION).valid)
      return false;
  }
  r.warnings.push_back().sprintf("%s: self-intersecting, split into %u shells", part.name.c_str(), uint32_t(shells.size()));
  for (Part &shell : shells)
    parts.push_back(eastl::move(shell));
  return true;
}

// artist meshes the import rule would refuse
bool prepare(const Input &in, uint32_t input, dag::Vector<Part> &parts, Result &r)
{
  eastl::string &why = r.refused[input];
  if (!in.mesh || in.mesh->face.empty())
  {
    why = "no faces";
    return false;
  }
  for (const float v : in.wtm.array)
    if (!check_finite(v))
    {
      why = "the node transform is not finite";
      return false;
    }
  Part raw;
  raw.input = input;
  raw.name = in.name ? in.name : "";
  if (!dacsg::from_dag_mesh(*in.mesh, raw.mesh, why))
    return false;
  if (!to_world(raw.mesh, in.wtm, why))
    return false;

  Part part;
  drop_repeated(raw, part);
  dacsg::Solid s;
  dacsg::mesh_to_solid(part.mesh, s);
  if (!s.closed)
  {
    weld_equal_positions(raw.mesh);
    drop_repeated(raw, part);
    dacsg::mesh_to_solid(part.mesh, s);
  }
  if (const char *breach = dacsg::mesh_contract_breach(part.mesh))
    return (why = breach, false);
  dacsg::ValidationReport v = dacsg::validate(s);
  if (defects(v))
    return (why = v.error, false);
  v = dacsg::validate(s, dacsg::VALIDATE_SELF_INTERSECTION);
  if (!v.valid && !v.selfIntersectingPairs && v.signedVolume < 0)
  {
    // a mixed-winding input stays invalid flipped, and only v.error is read after it
    flip(part.mesh);
    drop_normals(part.mesh);
    dacsg::mesh_to_solid(part.mesh, s);
    if (dacsg::validate(s, dacsg::VALIDATE_SELF_INTERSECTION).valid)
    {
      r.warnings.push_back().sprintf("%s: inside out, flipped", raw.name.c_str());
      split_outward(part, parts, r);
      return true;
    }
  }
  if (v.valid)
  {
    split_outward(part, parts, r);
    return true;
  }
  if (v.selfIntersectingPairs && split_shells(part, parts, r))
    return true;
  why = v.error;
  return false;
}
} // namespace

bool evaluate(dag::ConstSpan<Input> a, Op op, dag::ConstSpan<Input> b, Result &r)
{
  r = Result();
  const uint32_t inputs = a.size() + b.size();
  r.refused.resize(inputs);
  dag::Vector<Part> parts;
  bool uv = false, color = false;
  for (uint32_t i = 0; i < inputs; ++i)
  {
    const Input &in = i < a.size() ? a[i] : b[i - a.size()];
    if (!prepare(in, i, parts, r))
      continue;
    uv |= in.mesh->tface[0].size() == in.mesh->face.size() && !in.mesh->tvert[0].empty();
    color |= in.mesh->cface.size() == in.mesh->face.size() && !in.mesh->cvert.empty();
  }

  dag::Vector<eastl::unique_ptr<PartSource>> sources; // they outlive the graph
  dacsg::Graph g;
  dag::Vector<uint32_t> partOfNode;
  dacsg::OpResult side[2];
  for (uint32_t i = 0; i < parts.size(); ++i)
  {
    sources.push_back(eastl::make_unique<PartSource>(parts[i]));
    const dacsg::OpResult leaf = g.leaf(sources.back().get());
    g.setName(leaf, parts[i].name.c_str());
    if (partOfNode.size() <= leaf.id)
      partOfNode.resize(leaf.id + 1, dacsg::NO_INDEX);
    partOfNode[leaf.id] = i;
    dacsg::OpResult &s = side[parts[i].input < a.size() ? 0 : 1];
    s = s == g.empty() ? leaf : g.op(dacsg::NodeKind::Union, s, leaf);
  }
  dacsg::OpResult root;
  switch (op)
  {
    case Op::Union: root = g.op(dacsg::NodeKind::Union, side[0], side[1]); break;
    case Op::Subtract: root = g.op(dacsg::NodeKind::Subtract, side[0], side[1]); break;
    case Op::Intersect: root = g.op(dacsg::NodeKind::Intersect, side[0], side[1]); break;
    case Op::Xor:
      // a union, not a concat: remainders on one face share it, and disjoint ones optimize to a concat
      root = g.op(dacsg::NodeKind::Union, g.op(dacsg::NodeKind::Subtract, side[0], side[1]),
        g.op(dacsg::NodeKind::Subtract, side[1], side[0]));
      break;
  }

  dacsg::ExecuteParams params;
  params.validateLeaves = false;
  dacsg::ExecuteStats stats;
  const bool ok = dacsg::execute(g, root, params, r.mesh, &stats, &r.error);
  for (eastl::string &w : stats.warnings)
    r.warnings.push_back(eastl::move(w));
  if (!ok)
    return false;
  dag::Vector<dacsg::Channel> &ch = r.mesh.channels;
  ch.erase(eastl::remove_if(ch.begin(), ch.end(),
             [uv, color](const dacsg::Channel &c) { return c.id == dacsg::CH_UV ? !uv : c.id == dacsg::CH_COLOR && !color; }),
    ch.end());

  const uint32_t faceCount = r.mesh.faceCount();
  r.faceInput.assign(faceCount, Result::NO_INPUT);
  r.faceSource.assign(faceCount, Result::NO_INPUT);
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    const dacsg::FaceInfo &fi = r.mesh.face[f];
    const uint32_t part = fi.prim < partOfNode.size() ? partOfNode[fi.prim] : dacsg::NO_INDEX;
    if (part == dacsg::NO_INDEX || fi.srcFace >= parts[part].mesh.faceCount())
      continue;
    r.faceInput[f] = parts[part].input;
    r.faceSource[f] = parts[part].mesh.face[fi.srcFace].srcFace;
  }
  return true;
}

bool subtract_in_turn(dag::ConstSpan<Input> a, dag::ConstSpan<Input> b, uint32_t keep, Result &r)
{
  bool ok = evaluate(a, Op::Subtract, b, r);
  for (uint32_t i = 0; i < a.size() + keep && i < r.refused.size(); ++i)
    if (!r.refused[i].empty())
    {
      const char *name = (i < a.size() ? a[i] : b[i - a.size()]).name;
      r.error.sprintf("%s: %s", name ? name : "", r.refused[i].c_str());
      return false;
    }
  if (ok || b.size() <= keep)
    return ok;
  dag::Vector<Input> kept(b.begin(), b.begin() + keep);
  dag::Vector<uint32_t> index;
  // r by b's inputs: a left-out cutter is refused with its failure and gives no face
  dag::Vector<eastl::string> refused(a.size() + b.size());
  for (uint32_t i = 0; i < keep; ++i)
    index.push_back(i);
  for (uint32_t i = keep; i < b.size(); ++i)
  {
    kept.push_back(b[i]);
    index.push_back(i);
    ok = evaluate(a, Op::Subtract, kept, r);
    if (!ok)
    {
      refused[a.size() + i] = "its cut fails on top of the ones before it: " + r.error;
      kept.pop_back();
      index.pop_back();
    }
  }
  if (!ok)
    ok = evaluate(a, Op::Subtract, kept, r);
  for (uint32_t i = 0; i < r.refused.size(); ++i)
    refused[i < a.size() ? i : a.size() + index[i - a.size()]] = eastl::move(r.refused[i]);
  for (uint32_t &in : r.faceInput)
    in = in != Result::NO_INPUT && in >= a.size() ? a.size() + index[in - a.size()] : in;
  r.refused = eastl::move(refused);
  return ok;
}

void to_engine_mesh(const Result &r, Mesh &out) { dacsg::to_dag_mesh(r.mesh, out); }

void surface_faces(const Result &r, uint32_t input, uint32_t face_count, Bitarray &used)
{
  used.resize(face_count);
  if (input < r.refused.size() && !r.refused[input].empty())
  {
    used.set();
    return;
  }
  used.reset();
  for (uint32_t f = 0; f < r.faceInput.size(); ++f)
    if (r.faceInput[f] == input && r.faceSource[f] < face_count)
      used.set(r.faceSource[f]);
}

void keep_surface_faces(const Result &r, uint32_t input, Mesh &mesh)
{
  Bitarray used;
  surface_faces(r, input, mesh.face.size(), used);
  mesh.removeFacesFast(used);
}
} // namespace mesh_csg
