//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace ground_holes
{
void holes_initialize(int &hmap_holes_scale_step_offset_varId, int &hmap_holes_temp_ofs_size_varId, bool &should_render_ground_holes,
  ecs::Point4List &holes);

void on_disappear(int hmap_holes_scale_step_offset_varId, UniqueTexHolder &hmapHolesTex);

void render(UniqueTexHolder &hmapHolesTex, UniqueTexHolder &hmapHolesTmpTex, UniqueBufHolder &hmapHolesBuf,
  PostFxRenderer &hmapHolesProcessRenderer, PostFxRenderer &hmapHolesMipmapRenderer, ShadersECS &hmapHolesPrepareRenderer,
  bool &should_render_ground_holes, int hmap_holes_scale_step_offset_varId, int hmap_holes_temp_ofs_size_varId, ecs::Point4List &holes,
  ecs::Point3List &invalidate_bboxes, const ComputeShader &heightmap_holes_process_cs);

void spawn_hole(ecs::Point4List &holes, const TMatrix &transform, bool roundShape, bool shapeIntersection);

void get_invalidation_bbox(ecs::Point3List &bboxes, const TMatrix &transform, bool shapeIntersection);

void convar_helper(bool &should_render_ground_holes);

void zones_before_render(UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones, Tab<Point3_vec4> &bboxes);

void zones_after_device_reset(UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones);

void zones_manager_on_disappear(UniqueBufHolder &hmapHolesZonesBuf);

bool get_debug_hide();
}; // namespace ground_holes