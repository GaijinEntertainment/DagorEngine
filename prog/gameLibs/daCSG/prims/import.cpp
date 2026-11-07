// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "prims/prims.h"
#include <gameLibs/daCSG/graph_api.h>

namespace dacsg
{
// Opens the path as given; callers resolve relative ones
bool make_mesh(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  if (!a.loader)
    return fail(err, "mesh import needs a loader: ExecuteParams::meshLoader");
  if (a.checkOnly)
    return true;
  if (!a.loader(a.g.path(a.d.path), b.m, err))
    return false;
  // the client's Mesh is checked before anything reads it; the channels it left out take their defaults
  if (const char *breach = mesh_contract_breach(b.m))
    return fail(err, breach);
  b.m.addDefaultChannels();
  for (uint32_t f = 0; f < b.m.faceCount(); ++f)
  {
    b.m.face[f].prim = b.prim;
    b.m.face[f].srcFace = f;
  }
  return true;
}
} // namespace dacsg
