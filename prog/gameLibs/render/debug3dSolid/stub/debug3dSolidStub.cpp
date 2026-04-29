// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debug3dSolid.h>
#include <render/debug3dSolidBuffered.h>
#include <math/dag_Point3.h>
void draw_debug_solid_mesh(const uint16_t *, int, const float *, int, int, const TMatrix &, const Color4 &, bool, DrawSolidMeshCull)
{
  G_ASSERT(0);
}
void draw_debug_solid_sphere(const Point3 &, float, const TMatrix &, const Color4 &, bool) { G_ASSERT(0); }
void draw_debug_solid_capsule(const Capsule &, const TMatrix &, const Color4 &, bool) { G_ASSERT(0); }
void draw_debug_solid_cube(const BBox3 &, const TMatrix &, const Color4 &, bool) { G_ASSERT(0); }
void draw_debug_solid_cone(const Point3, Point3, float, float, int, const Color4 &) { G_ASSERT(0); }
void draw_debug_solid_collision_node(int, const CollisionResource &, const TMatrix &, const Color4 &, bool, DrawSolidMeshCull,
  const GeomNodeTree *)
{
  G_ASSERT(0);
}
void init_debug_solid() { G_ASSERT(0); }
void close_debug_solid() { G_ASSERT(0); }

void flush_buffered_debug_meshes(bool) { G_ASSERT(0); }
void draw_debug_solid_mesh_buffered(const uint16_t *, int, const float *, int, int, const TMatrix &, Color4, size_t) { G_ASSERT(0); }
void draw_debug_solid_cube_buffered(const BBox3 &, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_triangle_buffered(Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_tehedron_buffered(const Point3 &, float, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_ball_buffered(const Point3 &, float, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_disk_buffered(const Point3, Point3, float, int, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_cone_buffered(const Point3, Point3, float, float, int, const Color4 &, size_t) { G_ASSERT(0); }
void clear_buffered_debug_solids() { G_ASSERT(0); }
