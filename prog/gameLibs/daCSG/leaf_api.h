// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/mesh_api.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <EASTL/string.h>

namespace dacsg
{
// A primitive another library tessellates; its owner outlives the graph.
// Its mesh meets the Mesh contract whatever ExecuteParams::validateLeaves is, and bbox() and reach() are finite on every call.
// Execute checks the bounds of every reachable leaf before any prune, and the mesh of each leaf it tessellates
struct ILeafSource
{
  virtual ~ILeafSource() = default;
  // The mesh in world space by world_tm, its winding flipped by the leaf when det < 0: daCSG uses it as is. quality and
  // tolerance are the execute's; sag is the chord error the leaf keeps, in world units. Instances under a similarity share
  // one tessellation made with world_tm = s * I, and daCSG applies the rest of each instance map to it.
  // out: overwritten whole; the leaf sets each face's smgr, mat and attrSet, the executor its prim and srcFace
  virtual bool tessellate(const TMatrix &world_tm, float quality, double tolerance, double sag, Mesh &out, eastl::string &err) = 0;
  virtual uint64_t contentHash() const = 0; // without the transform; equals() confirms
  // false for an object of another implementation
  virtual bool equals(const ILeafSource &other) const = 0;
  virtual BBox3 bbox() const = 0; // local, conservative
  // how far the mesh may lie outside world_tm(bbox()), per unit of the mean world scale cbrt(|det|): a skin's thickness
  virtual float reach() const { return 0; }
  virtual const char *name() const = 0;
};
} // namespace dacsg
