//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
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

namespace ptgi
{

// The RT system is working with the pointer values, not the string contents!
// Persistent textures are used during multiple operations in a frame, many need to persist their contents
// between mupltiple frames.
// Transient textures are only needed during the render function.

void get_required_persistent_texture_descriptors(denoiser::TexInfoMap &persistent_textures);
void get_required_transient_texture_descriptors(denoiser::TexInfoMap &transient_textures);

enum class Quality
{
  Low,
  Medium,
  High,
};

void initialize(bool half_res);
void teardown();

void turn_off();

void render(bvh::ContextId context_id, const TMatrix4 &proj_tm, TEXTUREID depth, bool in_cockpit, const denoiser::TexMap &textures,
  Quality quality, bool checkerboard = true);

} // namespace ptgi