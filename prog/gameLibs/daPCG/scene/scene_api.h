// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/mesh_api.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/string.h>

namespace dapcg
{
inline const dacsg::TMatrixD TMD_IDENT = {{DPoint3(1, 0, 0), DPoint3(0, 1, 0), DPoint3(0, 0, 1), DPoint3(0, 0, 0)}};
// as DataBlock text and dag exporters write
inline constexpr const char *SCRIPT_EOL = "\r\n";

// Default writes no key: the engine default holds
enum class CollisionKind : uint8_t
{
  Default,
  None,
  Mesh,
  Box,
  Convex,
  Capsule,
  Sphere,
  Count,
};

struct SceneNode
{
  eastl::string name;
  eastl::string model_script;      // the model's lines, joined by SCRIPT_EOL
  eastl::string site;              // <file>:<line> of the model call
  int parent = -1;                 // an earlier node, -1 for the root
  dacsg::TMatrixD wtm = TMD_IDENT; // pivot frame in model space, orthonormal, right-handed
  dacsg::OpResult mesh;            // in model space; may be empty
  CollisionKind collision = CollisionKind::Default;
  bool animated = false; // keeps its node and mesh in the engine
};

enum PivotKind
{
  PIVOT_MODEL,
  PIVOT_LOCAL_Y,
  PIVOT_LOCAL_Z,
};

// right-handed; X: the least parallel model axis
const char *pivot_frame(int kind, const DPoint3 &p, const DPoint3 &axis, dacsg::TMatrixD &out);
const char *check_node_name(dag::ConstSpan<SceneNode> nodes, const char *name);
eastl::string node_script(const SceneNode &n);
dacsg::TMatrixD local_tm(const dacsg::TMatrixD &parent_wtm, const dacsg::TMatrixD &wtm);
// in double; tm orthonormal, not mirrored
void transform_mesh(dacsg::Mesh &m, const dacsg::TMatrixD &tm);
// out[i]: node i's local mesh; partial on false
bool build_scene(dacsg::Graph &g, dag::ConstSpan<SceneNode> nodes, const dacsg::ExecuteParams &p, dag::Vector<dacsg::Mesh> &out,
  dag::Vector<dacsg::ExecuteStats> *stats, eastl::string &err);
} // namespace dapcg
