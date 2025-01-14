// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm.h>

namespace rtsm
{

void initialize(int, int, RenderMode, bool) {}
void teardown() {}

void render(bvh::ContextId, const Point3 &, const TMatrix4 &, bool, bool, Texture *, d3d::SamplerHandle) {}

void render_dynamic_light_shadows(bvh::ContextId, const Point3 &, bool) {}

void turn_off() {}

TextureIDPair get_output_texture() { return TextureIDPair(); }

} // namespace rtsm