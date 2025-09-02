// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/ptgi.h>

namespace ptgi
{

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &) {}
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &) {}

void initialize(bool) {}
void teardown() {}

void turn_off() {}

void render(bvh::ContextId, const TMatrix4 &, TEXTUREID, bool, const denoiser::TexMap &, Quality, bool) {}

} // namespace ptgi