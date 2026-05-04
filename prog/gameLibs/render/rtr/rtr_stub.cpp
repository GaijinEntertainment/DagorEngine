// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr.h>

namespace rtr
{

void initialize(denoiser::ReflectionMethod, bool, bool) {}
void teardown() {}

void change_method(denoiser::ReflectionMethod) {}

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

void set_performance_mode(bool) {}

void set_water_params() {}
void set_rtr_hit_distance_params() {}
void set_ray_limit_params(float, float, float, bool) {}
void set_use_anti_firefly(bool) {}

bool prepare(bvh::ContextId, bool, bool, const denoiser::TexMap &, bool) { return false; }
void bind_params() {}
void unbind_params() {}
void do_update_probes() {}
void do_trace(const TMatrix4 &) {}
void denoise(const denoiser::TexMap &) {}
void render(bvh::ContextId, const TMatrix4 &, bool, bool, const denoiser::TexMap &, bool) {}

void render_validation_layer() {}
void render_probes() {}

bool is_validation_layer_enabled() { return false; }

void turn_off() {}

void denoise_noop(const denoiser::TexMap &) {}

} // namespace rtr