//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstdint>
#include <math/dag_Point3.h>
#include <math/dag_plane3.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>


class ShaderElement;
class ShaderMaterial;

namespace frx
{

struct DestrMesh;
struct DestrContext;
struct DestrSystem;


void mesh_normalize_transform(DestrContext &, DestrMesh &mesh);

void mesh_prepare_convex_hull(const DestrContext &ctx, const DestrMesh &mesh, float min_dist,
  dag::Vector<Point3, framemem_allocator> &out_points);

void dbg_file_save(const DestrContext &ctx, const DestrSystem &sys, const char *fname);
void dbg_file_load(DestrContext &ctx, DestrSystem &sys, const char *fname, ShaderElement *sh_elem, ShaderMaterial *sh_mat);

} // namespace frx
