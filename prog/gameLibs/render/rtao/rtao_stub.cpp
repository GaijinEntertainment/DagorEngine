// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao.h>

namespace rtao
{

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

void initialize(bool) {}
void teardown() {}

void render_noisy(bvh::ContextId, const TMatrix4 &, Texture *, Texture *, bool) {}
void denoise(bool, const denoiser::TexMap &, bool) {}
bool do_trace(bvh::ContextId, const TMatrix4 &, Texture *, const denoiser::TexMap &, bool) { return false; }
void render(bvh::ContextId, const TMatrix4 &, bool, Texture *, const denoiser::TexMap &, bool) {}

} // namespace rtao