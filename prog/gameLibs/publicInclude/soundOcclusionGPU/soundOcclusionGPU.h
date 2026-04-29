//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <stdint.h>

namespace snd_occlusion_gpu
{

struct Config
{
  int maxSources = 128;
  int ringBufferFrames = 3;
};

enum OcclusionMode : uint8_t
{
  MODE_SIMPLE = 0,
  MODE_DIRECT_REVERB = 1,
};

void init(const Config &cfg);
void close();
bool is_inited();

void set_listener(const Point3 &pos);
int create_source(const Point3 &pos, float radius, OcclusionMode mode = MODE_SIMPLE);
void delete_source(int idx);
void set_source(int idx, const Point3 &pos, float radius, OcclusionMode mode = MODE_SIMPLE);
int get_num_active_sources();
int get_max_active_sources();
int get_active_pair_count();

void dispatch();

bool get_occlusion(int source_idx, float &out_occlusion);
bool get_occlusion_direct_reverb(int source_idx, float &out_direct, float &out_reverb);

void set_hardness_k(float k);
void set_cone_half_angle(float deg);
int get_readback_age();

void debug_render_3d();
void debug_render_imgui();
void debug_set_enabled(bool enabled);
bool debug_is_enabled();

void world_sdf_debug_render_shadow(const Point3 &listener);

} // namespace snd_occlusion_gpu
