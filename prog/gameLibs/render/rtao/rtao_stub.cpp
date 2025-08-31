// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtao.h>

namespace rtao
{

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

void initialize(bool) {}
void teardown() {}

void render_noisy(bvh::ContextId, const TMatrix4 &, TEXTUREID, TextureIDPair, bool) {}
void render(bvh::ContextId, const TMatrix4 &, bool, TEXTUREID, const denoiser::TexMap &, bool) {}

} // namespace rtao