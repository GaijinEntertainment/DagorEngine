//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <drv/3d/dag_tex3d.h>
#include <render/denoiser.h>

class Point2;
class Point3;
class TMatrix;
class TMatrix4;
class BaseTexture;
typedef BaseTexture Texture;

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace rtr
{

void initialize(denoiser::ReflectionMethod method, bool half_res, bool checkerboard);
void teardown();

void change_method(denoiser::ReflectionMethod method);

// The RT system is working with the pointer values, not the string contents!
// Persistent textures are used during multiple operations in a frame, many need to persist their contents
// between mupltiple frames.
// Transient textures are only needed during the render function.

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures);

void set_performance_mode(bool performance_mode);

void set_water_params();
void set_rtr_hit_distance_params();
void set_ray_limit_params(float ray_limit_coeff, float ray_limit_power, bool use_rtr_probes);
void set_use_anti_firefly(bool use_anti_firefly_);
void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, bool rt_shadow, bool csm_shadow, const denoiser::TexMap &textures,
  bool checkerboard);

void render_validation_layer();
void render_probes();
bool is_validation_layer_enabled();

void turn_off();

} // namespace rtr