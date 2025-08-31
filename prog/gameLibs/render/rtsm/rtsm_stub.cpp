// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm.h>

namespace rtsm
{

namespace TextureNames
{
const char *rtsm_dynamic_lights = "rtsm_dynamic_lights";
}

void initialize(RenderMode, bool) {}
void teardown() {}

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

void render(bvh::ContextId, const Point3 &, const Point3 &, const TMatrix4 &, const denoiser::TexMap &, float, bool, bool, Texture *,
  d3d::SamplerHandle, Texture *, d3d::SamplerHandle, int)
{}

void render_dynamic_light_shadows(bvh::ContextId, const Point3 &, TextureIDPair, float, bool, bool) {}

void turn_off() {}

} // namespace rtsm