// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/denoiser.h>

namespace denoiser
{

void initialize(int, int) {}
void teardown() {}

void make_shadow_maps(UniqueTex &, UniqueTex &) {}
void make_shadow_maps(UniqueTex &, UniqueTex &, UniqueTex &) {}
void make_ao_maps(UniqueTex &, UniqueTex &, bool) {}
void make_reflection_maps(UniqueTex &, UniqueTex &, ReflectionMethod, bool) {}

void prepare(const FrameParams &) {}

void denoise_shadow(const ShadowDenoiser &) {}
void denoise_ao(const AODenoiser &) {}
void denoise_reflection(const ReflectionDenoiser &) {}

int get_frame_number() { return 0; }

void get_memory_stats(int &, int &, int &, int &) {}

TEXTUREID get_nr_texId(int) { return BAD_D3DRESID; }

TEXTUREID get_viewz_texId(int) { return BAD_D3DRESID; }

} // namespace denoiser