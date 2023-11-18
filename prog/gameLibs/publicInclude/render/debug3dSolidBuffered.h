//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "debug3dSolid.h"
#include <math/dag_color.h>

void flush_buffered_debug_meshes(bool game_is_paused);
void draw_debug_solid_mesh_buffered(const uint16_t *indices, int faces_count, const float *xyz_pos, int vertex_size,
  int vertices_count, const TMatrix &tm, Color4 color, size_t frames);

void draw_debug_solid_cube_buffered(const BBox3 &cube, const TMatrix &tm, const Color4 &color, size_t frames);
void draw_debug_solid_triangle_buffered(Point3 a, Point3 b, Point3 c, const Color4 &color, size_t frames);
void draw_debug_solid_quad_buffered(Point3 half_vec_i, Point3 half_vec_j, const TMatrix &tm, const Color4 &color, size_t frames);
void draw_debug_solid_quad_buffered(Point3 top_left, Point3 bottom_left, Point3 bottom_right, Point3 top_right, const Color4 &color,
  size_t frames);
// Use negative sphere_r to draw the other side of the ball
void draw_debug_ball_buffered(const Point3 &sphere_c, float sphere_r, const Color4 &color, size_t frames);
// Use negative norm to draw the other side of the disk
void draw_debug_solid_disk_buffered(const Point3 pos, Point3 norm, float radius, int segments, const Color4 &color, size_t frames);
void draw_debug_solid_cone_buffered(const Point3 pos, Point3 norm, float radius, float height, int segments, const Color4 &color,
  size_t frames);
void clear_buffered_debug_solids();
