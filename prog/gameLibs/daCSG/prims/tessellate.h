// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/mesh_api.h>
#include <EASTL/string.h>

namespace dacsg
{
// The 16 parametric primitives are closed by construction, with uv, color, smgr and srcFace set, but some can fold through
// themselves: the caller runs the self-intersection test on a type prim_can_fold names, a PrimType::Mesh result being the file's
// mesh as read. sag in the primitive's frame, spec.md 4; on failure err names the node and out holds an empty Mesh. prim must
// name a Prim node (unchecked in a release build); a PrimType::Leaf fails with "a leaf tessellates through its source"
bool tessellate(const Graph &g, OpResult prim, double sag, Mesh &out, eastl::string &err, MeshLoader loader = nullptr);
// The checks of tessellate at the node's own seg and segMinor, at the fewest segments for an automatic count, an
// import's without the file: false with err naming the node; a PrimType::Leaf fails as above
bool check_prim(const Graph &g, OpResult prim, MeshLoader loader, eastl::string &err);
} // namespace dacsg
