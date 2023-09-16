//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>

class TMatrix;
class Point3;
struct Capsule;
class BBox3;
struct Color4;

void draw_debug_solid_mesh(const uint16_t *indices, int faceCount, const float *xyz_pos, int v_stride, int v_cnt, const TMatrix &tm,
  const Color4 &color, bool shaded = false);
void draw_debug_solid_sphere(const Point3 &sphere_c, float sphere_rad, const TMatrix &tm, const Color4 &color, bool shaded = false);
void draw_debug_solid_capsule(const Capsule &capsule, const TMatrix &tm, const Color4 &color, bool shaded = false);
void draw_debug_solid_cube(const BBox3 &cube, const TMatrix &tm, const Color4 &color, bool shaded = false);
void draw_debug_solid_cone(const Point3 pos, Point3 norm, float radius, float height, int segments, const Color4 &color);

void close_debug_solid();
