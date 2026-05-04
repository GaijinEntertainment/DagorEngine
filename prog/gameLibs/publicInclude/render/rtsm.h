//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_textureIDHolder.h>
#include <render/denoiser.h>

#include <EASTL/optional.h>

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
extern const char *precomputed_dynamic_lights;
}

void initialize(RenderMode render_mode, bool dynamic_light_shadows);
void teardown();

// The RT system is working with the pointer values, not the string contents!
// Persistent textures are used during multiple operations in a frame, many need to persist their contents
// between mupltiple frames.
// Transient textures are only needed during the render function.

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures);

struct RTSMContext
{
  denoiser::ShadowDenoiser denoiserParams;
  IPoint2 resolution;
  bvh::ContextId ctxId;
  Texture *rtsmValueTex;
  Texture *rtsmTranslucencyTex;
  Texture *rtsmShadowsDenoisedTex;
  int qualityMode;
  bool hasNuke;
};
eastl::optional<RTSMContext> prepare(bvh::ContextId context_id, const Point3 &light_dir, const denoiser::TexMap &textures,
  bool has_nuke, Texture *csm_texture, d3d::SamplerHandle csm_sampler, Texture *vsm_texture, d3d::SamplerHandle vsm_sampler,
  int quality_mode);
void do_trace(const RTSMContext &ctx, const TMatrix4 &inv_proj_tm, const Point3 &view_pos);
void denoise(const RTSMContext &ctx);

void render(bvh::ContextId context_id, const Point3 &view_pos, const Point3 &light_dir, const TMatrix4 &proj_tm,
  const denoiser::TexMap &textures, bool has_nuke = false, Texture *csm_texture = nullptr,
  d3d::SamplerHandle csm_sampler = d3d::INVALID_SAMPLER_HANDLE, Texture *vsm_texture = nullptr,
  d3d::SamplerHandle vsm_sampler = d3d::INVALID_SAMPLER_HANDLE, int quality_mode = 0);

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, Texture *dynamic_lighting_texture,
  float light_radius, bool soft_shadow, bool has_nuke = false);

void turn_off();

} // namespace rtsm