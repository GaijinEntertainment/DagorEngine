// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtr.h>

namespace rtr
{

void initialize(bool, bool) {}
void teardown() {}

void set_performance_mode(bool) {}

void set_water_params() {}
void set_rtr_hit_distance_params() {}
void set_classify_threshold(float, float, float) {}
void set_reflection_method(denoiser::ReflectionMethod) {}
void render(bvh::ContextId, const TMatrix4 &, bool, bool, bool, TEXTUREID) {}

void render_validation_layer() {}

void turn_off() {}

} // namespace rtr