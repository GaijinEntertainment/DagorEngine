// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

class Mesh;
class Point3;

namespace meshopt
{

// vimax<<vimin;
enum
{
  SEAM_EDGE_01 = 1,
  SEAM_EDGE_02 = 2,
  SEAM_EDGE_12 = 4
};

// create face seam flags.
// face_flags should be allocated to at least mesh face.size()

void create_face_flags(Mesh &m, uint8_t *face_flags);

struct Vsplit /* (v1, v2) -> vf(vx, vy, vz) */
{
  int v1;
  int v2;
  float alpha;
  unsigned vf : 31;       // v2*alpha + v1*(1-alpha)
  unsigned newVertex : 1; // alpha != 0 && alpha != 1
};

bool make_vsplits(const uint8_t *face_indices, int stride, bool short_indices, int fn,
  const uint8_t *face_flags, // optional parameter, can be NULL or created with crate_face_flags
  const Point3 *verts, int vn, Tab<Vsplit> &vsplits, int target_num_faces, bool onlyExistingVerts);

int progressive_optimize(Mesh &mesh, const Vsplit *vsplits, int vcnt, bool update_existing);

} // namespace meshopt
