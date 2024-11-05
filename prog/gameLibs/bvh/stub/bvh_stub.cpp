// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh
{
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

void set_ri_extra_range(ContextId, float) {}

void prepare_ri_extra_instances() {}

void update_instances(ContextId, const Point3 &, const Frustum &, dynrend::ContextId *, RiGenVisibility *) {}

void add_mesh(ContextId, uint64_t, const MeshInfo &) {}

void add_instance(ContextId, uint64_t, const mat43f &) {}

void remove_mesh(ContextId, uint64_t) {}

void build(ContextId, const TMatrix &, const TMatrix4 &, const Point3 &, const Point3 &) {}

void process_meshes(ContextId, BuildBudget) {}

void render_debug(ContextId, const IPoint2 &) {}

void bind_resources(ContextId, int) {}

void on_before_unload_scene(ContextId) {}

void on_unload_scene(ContextId) {}

void on_load_scene(ContextId) {}

void on_scene_loaded(ContextId) {}

void reload_grass(ContextId, RandomGrass *) {}

void connect_fx(ContextId, fx_connect_callback) {}

void on_cables_changed(Cables *, ContextId) {}

void start_async_atmosphere_update(ContextId, const Point3 &, atmosphere_callback) {}

void finalize_async_atmosphere_update(ContextId) {}

bool is_building(ContextId) { return false; }

void ChannelParser::enum_shader_channel(int, int, int, int, int, ChannelModifier, int) {}

} // namespace bvh