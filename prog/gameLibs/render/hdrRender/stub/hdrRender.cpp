// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/hdrRender.h>

namespace hdrrender
{

void init_globals(const DataBlock &) {}
void init(int, int, int, bool) {}
void init(int, int, bool, bool) {}

void shutdown() {}

bool int10_hdr_buffer() { return false; }
bool is_hdr_enabled() { return false; }
bool is_hdr_available() { return false; }
bool is_active() { return false; }
HdrOutputMode get_hdr_output_mode() { return HdrOutputMode::SDR_ONLY; }

Texture *get_render_target_tex() { return nullptr; }
TEXTUREID get_render_target_tex_id() { return BAD_TEXTUREID; }
const ManagedTex &get_render_target()
{
  static UniqueTex tex;
  return tex;
}

void set_render_target() {}

void set_resolution(int, int, bool) {}

bool encode(int, int) { return false; }

void update_globals() {}
void update_paper_white_nits(float) {}
void update_hdr_brightness(float) {}
void update_hdr_shadows(float) {}

float get_default_paper_white_nits() { return 1; }
float get_default_hdr_brightness() { return 1; }
float get_default_hdr_shadows() { return 1; }

} // namespace hdrrender