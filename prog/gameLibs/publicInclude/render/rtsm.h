//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_textureIDHolder.h>
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

namespace rtsm
{

enum class RenderMode
{
  Hard,
  Denoised,
  DenoisedTranslucent
};

namespace TextureNames
{
extern const char *rtsm_dynamic_lights;
}

void initialize(RenderMode render_mode, bool dynamic_light_shadows);
void teardown();

// The RT system is working with the pointer values, not the string contents!
// Persistent textures are used during multiple operations in a frame, many need to persist their contents
// between mupltiple frames.
// Transient textures are only needed during the render function.

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures);

void render(bvh::ContextId context_id, const Point3 &view_pos, const Point3 &light_dir, const TMatrix4 &proj_tm,
  const denoiser::TexMap &textures, float tree_max_distance, bool has_nuke = false, bool has_dynamic_lights = false,
  Texture *csm_texture = nullptr, d3d::SamplerHandle csm_sampler = d3d::INVALID_SAMPLER_HANDLE, Texture *vsm_texture = nullptr,
  d3d::SamplerHandle vsm_sampler = d3d::INVALID_SAMPLER_HANDLE, int quality_mode = 0);

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, TextureIDPair dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke = false);

void turn_off();

} // namespace rtsm