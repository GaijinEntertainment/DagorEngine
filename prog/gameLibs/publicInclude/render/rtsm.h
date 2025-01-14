//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_textureIDHolder.h>

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

void initialize(int width, int height, RenderMode render_mode, bool dynamic_light_shadows);
void teardown();

void render(bvh::ContextId context_id, const Point3 &view_pos, const TMatrix4 &proj_tm, bool has_nuke = false,
  bool has_dynamic_lights = false, Texture *csm_texture = nullptr, d3d::SamplerHandle csm_sampler = d3d::INVALID_SAMPLER_HANDLE);

void render_dynamic_light_shadows(bvh::ContextId context_id, const Point3 &view_pos, bool has_nuke = false);

void turn_off();

TextureIDPair get_output_texture();

} // namespace rtsm