// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scene_api.h"
#include <math/dag_check_nan.h>
#include <EASTL/algorithm.h>
#include <math.h>
#include <string.h>

namespace dapcg
{
using dacsg::TMatrixD;

static bool is_finite(const DPoint3 &p) { return check_finite(p.x) && check_finite(p.y) && check_finite(p.z); }

static DPoint3 rotate(const TMatrixD &m, const DPoint3 &v) { return m.col[0] * v.x + m.col[1] * v.y + m.col[2] * v.z; }

static TMatrixD rigid_inverse(const TMatrixD &m)
{
  TMatrixD out;
  for (int j = 0; j < 3; ++j)
    out.col[j] = DPoint3(m.col[0][j], m.col[1][j], m.col[2][j]);
  out.col[3] = -rotate(out, m.col[3]);
  return out;
}

const char *pivot_frame(int kind, const DPoint3 &p, const DPoint3 &axis, TMatrixD &out)
{
  if (kind < PIVOT_MODEL || kind > PIVOT_LOCAL_Z)
    return "the pivot kind is not 0, 1 or 2";
  if (!is_finite(p))
    return "the pivot point is not finite";
  if (kind != PIVOT_MODEL && !is_finite(axis))
    return "the pivot axis is not finite";
  if (kind != PIVOT_MODEL && !(axis.lengthSq() > 0))
    return "the pivot axis is zero";
  out = TMD_IDENT;
  out.col[3] = p;
  if (kind == PIVOT_MODEL)
    return nullptr;
  const DPoint3 n = axis / axis.length();
  int k = 0;
  for (int i = 1; i < 3; ++i)
    if (fabs(n[i]) < fabs(n[k]))
      k = i;
  const DPoint3 x = TMD_IDENT.col[k] - n * n[k];
  out.col[0] = x / x.length();
  out.col[1] = kind == PIVOT_LOCAL_Y ? n : n % out.col[0];
  out.col[2] = kind == PIVOT_LOCAL_Y ? out.col[0] % n : n;
  return nullptr;
}

const char *check_node_name(dag::ConstSpan<SceneNode> nodes, const char *name)
{
  const size_t len = strlen(name);
  if (len < 1 || len > 64 || strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != len)
    return "the name is not 1 to 64 characters of A-Z, a-z, 0-9 and _";
  const bool used = eastl::any_of(nodes.begin(), nodes.end(), [&](const SceneNode &n) { return n.name == name; });
  return used ? "the name is used twice" : nullptr;
}

eastl::string node_script(const SceneNode &n)
{
  static const char *const KINDS[int(CollisionKind::Count)] = {"", "", "mesh", "box", "convex", "capsule", "sphere"};
  eastl::string s = n.animated ? "animated_node:b=yes" : "";
  if (n.collision == CollisionKind::None)
    s.append_sprintf("%scollidable:b=no", s.empty() ? "" : SCRIPT_EOL);
  else if (n.collision != CollisionKind::Default)
    s.append_sprintf("%scollidable:b=yes%scollision:t=\"%s\"", s.empty() ? "" : SCRIPT_EOL, SCRIPT_EOL, KINDS[int(n.collision)]);
  if (!n.model_script.empty())
    s.append_sprintf("%s%s", s.empty() ? "" : SCRIPT_EOL, n.model_script.c_str());
  return s;
}

TMatrixD local_tm(const TMatrixD &parent_wtm, const TMatrixD &wtm)
{
  const TMatrixD inv = rigid_inverse(parent_wtm);
  TMatrixD out;
  for (int j = 0; j < 3; ++j)
    out.col[j] = rotate(inv, wtm.col[j]);
  out.col[3] = rotate(inv, wtm.col[3]) + inv.col[3];
  return out;
}

void transform_mesh(dacsg::Mesh &m, const TMatrixD &tm)
{
  for (Point3 &p : m.pos)
    p = Point3(rotate(tm, DPoint3(p)) + tm.col[3]);
  if (dacsg::Channel *c = m.findChannel(dacsg::CH_NORMAL))
    for (uint32_t i = 0; i + 3 <= c->data.size(); i += 3)
    {
      const DPoint3 n = rotate(tm, DPoint3(c->data[i], c->data[i + 1], c->data[i + 2]));
      for (int j = 0; j < 3; ++j)
        c->data[i + j] = float(n[j]);
    }
}

static const char *frame_error(const TMatrixD &m)
{
  for (const DPoint3 &c : m.col)
    if (!is_finite(c))
      return "the frame is not finite";
  for (int i = 0; i < 3; ++i)
    for (int j = i; j < 3; ++j)
      if (!(fabs(m.col[i] * m.col[j] - (i == j ? 1 : 0)) <= 1e-6))
        return "the frame is not orthonormal to 1e-6";
  return m.col[0] * (m.col[1] % m.col[2]) > 0 ? nullptr : "the frame is mirrored";
}

static bool node_error(const SceneNode &n, const char *text, eastl::string &err)
{
  err.sprintf("node \"%s\"", n.name.c_str());
  if (!n.site.empty())
    err.append_sprintf(" (%s)", n.site.c_str());
  err.append_sprintf(": %s", text);
  return false;
}

bool build_scene(dacsg::Graph &g, dag::ConstSpan<SceneNode> nodes, const dacsg::ExecuteParams &p, dag::Vector<dacsg::Mesh> &out,
  dag::Vector<dacsg::ExecuteStats> *stats, eastl::string &err)
{
  out = dag::Vector<dacsg::Mesh>(nodes.size());
  if (stats)
    *stats = dag::Vector<dacsg::ExecuteStats>(nodes.size());
  if (nodes.empty())
  {
    err = "the scene has no node";
    return false;
  }
  eastl::string why;
  for (uint32_t i = 0; i < nodes.size(); ++i)
  {
    const SceneNode &n = nodes[i];
    if (n.parent < -1 || n.parent >= int(i))
      why.sprintf("the parent %d is not an earlier node", n.parent);
    else if (const char *bad_name = check_node_name(make_span_const(nodes.data(), i), n.name.c_str()))
      why = bad_name;
    else if (const char *bad_frame = frame_error(n.wtm))
      why = bad_frame;
    else if (n.parent == -1 && i != 0)
      why = "the node is not under the root";
    else
      continue;
    return node_error(n, why.c_str(), err);
  }
  for (uint32_t i = 0; i < nodes.size(); ++i)
    if (nodes[i].mesh.id &&
        !dacsg::execute(g, g.transform(nodes[i].mesh, rigid_inverse(nodes[i].wtm)), p, out[i], stats ? &(*stats)[i] : nullptr, &why))
      return node_error(nodes[i], why.c_str(), err);
  return true;
}
} // namespace dapcg
