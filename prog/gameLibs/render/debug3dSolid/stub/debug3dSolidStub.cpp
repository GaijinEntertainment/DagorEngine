// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/debug3dSolidBuffered.h>
#include <math/dag_Point3.h>
void flush_buffered_debug_meshes(bool) { G_ASSERT(0); }
void draw_debug_solid_mesh_buffered(const uint16_t *, int, const float *, int, int, const TMatrix &, Color4, size_t) { G_ASSERT(0); }

void draw_debug_solid_cube_buffered(const BBox3 &, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_triangle_buffered(Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, const TMatrix &, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_quad_buffered(Point3, Point3, Point3, Point3, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_ball_buffered(const Point3 &, float, const Color4 &, size_t) { G_ASSERT(0); }
void draw_debug_solid_disk_buffered(const Point3, Point3, float, int, const Color4 &, size_t) { G_ASSERT(0); }
void clear_buffered_debug_solids() { G_ASSERT(0); }
