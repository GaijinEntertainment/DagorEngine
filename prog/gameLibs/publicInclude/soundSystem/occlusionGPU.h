//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <soundSystem/handle.h>

class Point3;

namespace sndsys::occlusion_gpu
{
OcclusionBlobHandle create_blob(const Point3 &pos, float attach_radius, float occlusion_radius);
void delete_blob(OcclusionBlobHandle &blob_handle);
void set_pos(OcclusionBlobHandle blob_handle, const Point3 &pos);
void set_blob_active_range(OcclusionBlobHandle blob_handle, float active_radius);
void attach_to_blob(EventHandle event_handle, OcclusionBlobHandle blob_handle);
void drop_instance(EventHandle event_handle);
bool is_inside_active_range(const Point3 &pos);
void debug_render_3d();

typedef void (*external_factor_t)(const Point3 &listener, const Point3 &blob_pos, float distance_sq, float occlusion,
  bool occlusion_valid, Point3 &factor);
void set_external_factor(external_factor_t external_factor);
} // namespace sndsys::occlusion_gpu
