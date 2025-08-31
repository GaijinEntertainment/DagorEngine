//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <shaders/dag_shaders.h>

void calc_sphere_vertex_face_count(uint32_t slices, uint32_t stacks, bool hemisphere, uint32_t &out_vertex_count,
  uint32_t &out_face_count);
void create_sphere_mesh(dag::Span<uint8_t> vertices, dag::Span<uint8_t> indices, float radius, uint32_t slices, uint32_t stacks,
  uint32_t stride, bool norm, bool tex, bool use_32_instead_of_16_indices, bool hemisphere);
float calc_sphere_max_radius_error(float radius, uint32_t slices, uint32_t stacks);

void calc_cylinder_vertex_face_count(uint32_t slices, uint32_t stacks, uint32_t &out_vertex_count, uint32_t &out_face_count);
void create_cylinder_mesh(dag::Span<uint8_t> vertices, dag::Span<uint8_t> indices, float radius, float height, uint32_t slices,
  uint32_t stacks, uint32_t stride, bool norm, bool tex, bool use_32_instead_of_16_indices);
void create_cubic_indices(dag::Span<uint8_t> indices, int count, bool use_32_instead_of_16_indices);
