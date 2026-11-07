// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class Point3;
struct Frustum;
class TMatrix;
class TMatrix4;
struct BVHInstanceMapper;
namespace dafg
{
class NodeHandle;
}

void bvh_update_dyn_instances(const Point3 &cameraPos,
  const Point3 &lightDirection,
  const TMatrix &itm,
  const TMatrix4 &viewRotTm,
  const TMatrix4 &projTm,
  const TMatrix4 &prevViewRotTm,
  const TMatrix4 &prevProjTm);
void bvh_start_ri_jobs(const Point3 &cameraPos, const Frustum &cullingFrustum, float realDt);
void bvh_wait_ri_jobs(); // A frame that skips WorldRenderer::draw after bvh_start_ri_jobs must call it.
void prepareFXForBVH(const Point3 &cameraPos);
bool is_bvh_enabled();
bool is_bvh_usable();
bool is_bvh_dyn_models_enabled();
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
bool is_bvh_dagdp_enabled();
void draw_rtr_validation();
void draw_ptgi_validation();
void bvh_cables_changed();
bool is_rt_supported();
bool is_rt_supported_on_disk();
int rt_support_error_code();
void bvh_release_bindlessly_held_textures();
bool should_delay_pufd_until_bvh_jobs_done();
void set_bvh_on_parallel_jobs_finished_cb(void (*cb)());
TMatrix4 get_bvh_culling_matrix(const Point3 &cameraPos);
BVHInstanceMapper *get_bvh_dagdp_instance_mapper();
void bvh_bind_resources(int render_width);
void bvh_unbind_resources();
dafg::NodeHandle make_rtsm_dynamic_node();
void recreate_water_rt_node();
void toggle_rtsm_dynamic(bool enable);
bool bvh_do_early_occlusion_culling();
float get_bvh_animchar_lod_dist_mul();