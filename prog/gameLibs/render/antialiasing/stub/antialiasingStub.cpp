// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/antialiasing.h>

namespace render::antialiasing
{
void migrate(const char *) {}

void init(AppGlue *) {}
void close() {}

void recreate(const IPoint2 &, const IPoint2 &postfx_resolution, IPoint2 &rendering_resolution)
{
  rendering_resolution = postfx_resolution;
}

AntialiasingMethod get_method() { return AntialiasingMethod::None; }
AntialiasingMethod get_method_from_settings() { return AntialiasingMethod::None; }
AntialiasingMethod get_method_from_name(const char *) { return AntialiasingMethod::None; }
const char *get_method_name(AntialiasingMethod) { return "off"; }

bool is_valid_method_name(const char *) { return false; }
bool is_valid_upscaling_name(const char *) { return false; }

bool is_ssaa_compatible() { return false; }

bool is_temporal() { return false; }
bool need_motion_vectors() { return false; }
bool support_dynamic_resolution() { return false; }

int pull_settings_change() { return 0; }

bool need_reactive_tex(bool) { return false; }
bool need_reactive_tex_from_settings(bool) { return false; }

bool prepare_for_device_reset() { return false; }

TemporalAA *get_taa() { return nullptr; }

float get_mip_bias() { return 0; }
void adjust_mip_bias(const IPoint2 &, const IPoint2 &) {}

bool is_metalfx_upscale_supported() { return false; }

void before_render_frame() {}

void before_render_view(int) {}

Point2 get_jitter(const RenderView &, bool) { return Point2::ZERO; }

RTarget::CPtr apply(const ApplyContext &, bool) { return nullptr; }
void apply_fxaa(AntialiasingMethod, TEXTUREID, TEXTUREID, const Point4 &) {}
void apply_mobile_aa(TextureIDPair, TextureIDPair) {}

const char *get_available_methods(bool, bool) { return "off"; }
const char *get_available_upscaling_options(const char *) { return "native"; }

int process_console_cmd(const char *[], int) { return 0; }

bool need_gui_in_texture() { return false; }

bool allows_noise() { return false; }

void suppress_frame_generation(bool) {}
void enable_frame_generation(bool) {}
void schedule_generated_frames(const FrameGenContext &) {}
int get_supported_generated_frames(const char *, bool) { return 0; }
const char *get_frame_generation_unsupported_reason(const char *, bool) { return nullptr; }
int get_presented_frame_count() { return 1; }
bool is_frame_generation_enabled() { return false; }
bool is_frame_generation_enabled_in_config() { return false; }
} // namespace render::antialiasing
