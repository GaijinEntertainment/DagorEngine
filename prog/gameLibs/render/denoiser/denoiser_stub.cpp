// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/denoiser.h>
#include "denoiser_names.h"

namespace denoiser
{
bool is_available() { return false; }

void initialize(int, int, bool) {}
void teardown() {}

bool is_ray_reconstruction_enabled() { return false; }

void get_required_persistent_texture_descriptors(TexInfoMap &, bool) {}

void get_required_persistent_texture_descriptors_for_ao(TexInfoMap &) {}
void get_required_transient_texture_descriptors_for_ao(TexInfoMap &) {}

void get_required_persistent_texture_descriptors_for_rtr(TexInfoMap &, ReflectionMethod) {}
void get_required_transient_texture_descriptors_for_rtr(TexInfoMap &, ReflectionMethod) {}

void get_required_persistent_texture_descriptors_for_rtsm(TexInfoMap &, bool) {}
void get_required_transient_texture_descriptors_for_rtsm(TexInfoMap &, bool) {}

void get_required_persistent_texture_descriptors_for_gi(TexInfoMap &) {}
void get_required_transient_texture_descriptors_for_gi(TexInfoMap &) {}

void prepare(const FrameParams &) {}

void denoise_shadow(const ShadowDenoiser &) {}
void denoise_ao(const AODenoiser &) {}
void denoise_gi(const GIDenoiser &) {}
void denoise_reflection(const ReflectionDenoiser &) {}

int get_frame_number() { return 0; }

} // namespace denoiser