//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BaseTexture;
typedef BaseTexture Texture;
class D3DRESID;
using TEXTUREID = D3DRESID;
namespace resptr_detail // TODO: move to engine header (something like dag_resPtrDecl.h)
{
template <typename ResType>
class ManagedRes;
template <typename ManagedResType>
class ManagedResView;
}; // namespace resptr_detail
using ManagedTex = resptr_detail::ManagedRes<BaseTexture>;
using ManagedTexView = resptr_detail::ManagedResView<ManagedTex>;
class IPoint2;

namespace objectmotionblur
{
static const int TILE_SIZE = 16;
struct MotionBlurSettings
{
  int maxBlurPx = 32;  // Only pixels reaching this velocity this will use all samples
  int maxSamples = 16; // Maximum samples used for one pixel.
  float strength = 0.0f;
  float vignetteStrength = 0.0f;
  float rampStrength = 1.0f; // how much to increase or decrease the velocity
  float rampCutoff = 1.0f;   // velocities greater than this will be increased, smaller will be decreased
  bool cancelCameraMotion = false;
  bool externalTextures = false;
};

bool is_enabled();
void on_settings_changed(const MotionBlurSettings &settings);

bool needs_transparent_gbuffer();

bool is_frozen_in_pause();

void teardown();

void on_render_resolution_changed(const IPoint2 &rendering_resolution, int format);

void apply(Texture *source_tex, TEXTUREID source_id, float current_frame_rate);
void apply(Texture *source_tex, TEXTUREID source_id, ManagedTexView resultTex, float current_frame_rate);

} // namespace objectmotionblur