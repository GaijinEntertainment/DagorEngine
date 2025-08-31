// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh
{
bool has_enough_vram_for_rt() { return false; }

bool is_available() { return false; }

void init(elem_rules_fn) {}

void teardown() {}

ContextId create_context(const char *, Features) { return InvalidContextId; }

void teardown(ContextId &) {}

void start_frame() {}

void add_terrain(ContextId, HeightProvider *) {}

void remove_terrain(ContextId) {}

void update_terrain(ContextId, const Point2 &) {}

void set_for_gpu_objects(ContextId) {}

void prepare_ri_extra_instances() {}

void set_ri_dist_mul(float) {}

void override_out_of_camera_ri_dist_mul(float) {}

void update_instances(ContextId, const Point3 &, const Frustum &, const Frustum &, dynrend::ContextId *, dynrend::ContextId *,
  RiGenVisibility *, threadpool::JobPriority)
{}

void update_instances(ContextId, const Point3 &, const Frustum &, const Frustum &, const dag::Vector<RiGenVisibility *> &,
  dynrend::BVHIterateCallback, threadpool::JobPriority)
{}

void add_object(ContextId, uint64_t, const ObjectInfo &) {}

void add_instance(ContextId, uint64_t, const mat43f &) {}

void remove_object(ContextId, uint64_t) {}

void build(ContextId, const TMatrix &, const TMatrix4 &, const Point3 &, const Point3 &) {}

void process_meshes(ContextId, BuildBudget) {}

void render_debug(ContextId, const IPoint2 &) {}

void bind_resources(ContextId, int) {}

void on_before_unload_scene(ContextId) {}

void on_before_settings_changed(ContextId) {}

void on_unload_scene(ContextId) {}

void on_load_scene(ContextId) {}

void on_scene_loaded(ContextId) {}

void reload_grass(ContextId, RandomGrass *) {}

void connect_fx(ContextId, fx_connect_callback) {}

void on_cables_changed(Cables *, ContextId) {}

void start_async_atmosphere_update(ContextId, const Point3 &, atmosphere_callback) {}

void finalize_async_atmosphere_update(ContextId) {}

bool is_building(ContextId) { return false; }

void set_grass_range(ContextId, float) {}

void set_debug_view_min_t(float) {}

void ChannelParser::enum_shader_channel(int, int, int, int, int, ChannelModifier, int) {}

void enable_per_frame_processing(bool) {}
void bind_tlas_stage(ContextId, ShaderStage) {}
void unbind_tlas_stage(ShaderStage) {}

} // namespace bvh