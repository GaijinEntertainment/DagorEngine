//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>

class DataBlock;

namespace hdrrender
{
void init_globals(const DataBlock &videoCfg);
void init(int width, int height, int fp_format = TEXFMT_R11G11B10F, bool uav_usage = false);
void init(int width, int height, bool linear_input, bool uav_usage = false);
void shutdown();

bool int10_hdr_buffer();
bool is_hdr_enabled();
bool is_hdr_available();
bool is_active();
HdrOutputMode get_hdr_output_mode();

Texture *get_render_target_tex();
TEXTUREID get_render_target_tex_id();
const ManagedTex &get_render_target();

void set_render_target();

void set_resolution(int width, int height, bool uav_usage = false);

bool encode(int x_offset = 0, int y_offset = 0);

void update_globals();
void update_paper_white_nits(float value);
void update_hdr_brightness(float value);
void update_hdr_shadows(float value);

float get_default_paper_white_nits();
float get_default_hdr_brightness();
float get_default_hdr_shadows();

float get_paper_white_nits();
float get_hdr_brightness();
float get_hdr_shadows();
}; // namespace hdrrender
