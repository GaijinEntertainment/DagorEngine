// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh
{
bool has_enough_vram_for_rt() { return false; }

bool is_available() { return false; }

BvhAvailabilityCode is_available_verbose() { return BvhAvailabilityCode::STUB; }

void init(elem_rules_fn, screenshot_fn, AdditionalSettings) {}

void teardown(bool, bool) {}

ContextId create_context(const char *, Features) { return InvalidContextId; }
bool has_features(ContextId, uint32_t) { return false; }

void teardown(ContextId &) {}

void start_frame() {}

void add_terrain(ContextId, HeightProvider *) {}

void remove_terrain(ContextId) {}

void update_terrain(ContextId, const Point2 &) {}

void set_for_gpu_objects(ContextId) {}

void add_bin_scene(ContextId, BaseStreamingSceneHolder &) {}

void add_water(ContextId, FFTWater &) {}

void prepare_ri_extra_instances() {}

void set_ri_dist_mul(float) {}

void override_out_of_camera_ri_dist_mul(float) {}

void update_instances(ContextId, const Point3 &, const Point3 &, const Frustum &, const Frustum &, dynrend::ContextId *,
  dynrend::ContextId *, RiGenVisibility *, threadpool::JobPriority)
{}

void update_instances(ContextId, const Point3 &, const Point3 &, const Frustum &, const Frustum &,
  const dag::Vector<RiGenVisibility *> &, dynrend::BVHIterateCallback, threadpool::JobPriority)
{}

void add_object(ContextId, uint64_t, const ObjectInfo &) {}

void add_instance(ContextId, uint64_t, const mat43f &) {}

void remove_object(ContextId, uint64_t) {}

void enable_dynamic_planar_decals(bool) {}

void build(ContextId, const TMatrix &, const TMatrix4 &, const Point3 &, const Point3 &) {}

void set_rigen_cpu_budget(int) {}

void process_meshes(ContextId, BuildBudget) {}

void render_debug(ContextId, const IPoint2 &) {}

void bind_gbuffer_textures(ContextId, Texture *, Texture *, Texture *, Texture *, Texture *) {}

void bind_fom_textures(ContextId, Texture *, Texture *, const d3d::SamplerHandle *) {}

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

void set_grass_fraction_to_keep(ContextId, float) {}

void set_debug_view_min_t(float) {}

void ChannelParser::enum_shader_channel(int, int, int, int, int, ChannelModifier, int) {}

void enable_per_frame_processing(bool) {}

void connect_dagdp(ContextId, dagdp_connect_callback callback) { callback(nullptr); }

void gpu_grass_make_meta(ContextId, const GPUGrassBase &) {}
void generate_gpu_grass_instances(ContextId, bool) {}

void gather_splinegen_instances(ContextId, Sbuffer *, eastl::vector<eastl::pair<uint32_t, MeshInfo>> &, uint32_t, uint32_t &) {}
void remove_spline_gen_instances(ContextId) {}

} // namespace bvh

size_t BVHInstanceMapper::getHWInstanceSize() { return 0; }