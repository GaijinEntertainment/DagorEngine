// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtsm.h>

namespace rtsm
{

void initialize(RenderMode, bool) {}
void teardown() {}

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

eastl::optional<RTSMContext> prepare(bvh::ContextId, const Point3 &, const denoiser::TexMap &, bool, Texture *, d3d::SamplerHandle,
  Texture *, d3d::SamplerHandle, int)
{
  return eastl::nullopt;
}
void do_trace(const RTSMContext &, const TMatrix4 &, const Point3 &) {}
void denoise(const RTSMContext &) {}

void render(bvh::ContextId, const Point3 &, const Point3 &, const TMatrix4 &, const denoiser::TexMap &, bool, Texture *,
  d3d::SamplerHandle, Texture *, d3d::SamplerHandle, int)
{}

void render_dynamic_light_shadows(bvh::ContextId, const Point3 &, Texture *, float, bool, bool) {}

void turn_off() {}

} // namespace rtsm