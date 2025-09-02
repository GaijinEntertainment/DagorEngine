// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;
struct Frustum;

void bvh_update_instances(const Point3 &cameraPos, const Frustum &viewFrustum);
void prepareFXForBVH(const Point3 &cameraPos);
bool is_bvh_enabled();
bool is_rtsm_enabled();
bool is_rtsm_dynamic_enabled();
bool is_rtr_enabled();
bool is_rttr_enabled();
bool is_rtao_enabled();
bool is_ptgi_enabled();
bool is_rt_water_enabled();
bool is_rtgi_enabled();
bool is_denoiser_enabled();
bool is_rr_enabled();
void draw_rtr_validation();
void bvh_cables_changed();
bool is_rt_supported();
void bvh_release_bindlessly_held_textures();
bool delay_PUFD_after_bvh();