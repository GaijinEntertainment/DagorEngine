// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr/rtr_api.h>

namespace rtr
{

void initialize(bool, bool, bool) {}
void teardown() {}
void set_resolution(const IPoint2 &) {}
void set_dynamic_resolution(const IPoint2 &) {}

void set_performance_mode(bool) {}

void set_water_params() {}
void set_rtr_hit_distance_params() {}
void set_ray_limit_params(float, float, float, bool) {}
void set_use_anti_firefly(bool) {}
void set_blur_radius(float) {}

denoiser::ParamsHandle<FrameParams> create_frame_params(dafg::Registry registry)
{
  return registry.createBlob<FrameParams>("rtr_frame_params").handle();
}

void register_nodes(const NodeParams &, dag::Vector<dafg::NodeHandle> &) {}

bool is_validation_layer_enabled() { return false; }
void render_validation_layer() {}
void render_probes() {}

} // namespace rtr
