//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class HudPrimitives;

namespace trackir
{
struct TrackData
{
  // angles (-1..1)
  float relYaw = 0.f;
  float relPitch = 0.f;
  float relRoll = 0.f;

  // angles (radians -PI..PI)
  float yaw = 0.f;
  float pitch = 0.f;
  float roll = 0.f;

  // head relative position (-1..1)
  float relX = 0.f;
  float relY = 0.f;
  float relZ = 0.f;

  // raw head position (0..1)
  float rawX = 0.5f;
  float rawY = 0.5f;
  float rawZ = 0.5f;
  float smoothX = 0.5f;
  float smoothY = 0.5f;
  float smoothZ = 0.5f;

  // head moving delta (-1..1)
  float deltaX = 0.f;
  float deltaY = 0.f;
  float deltaZ = 0.f;
};

void init(int smoothing = 0);
void shutdown();

bool is_installed();
bool is_enabled();
bool is_active();

void update();

void start();
void stop();

TrackData &get_data();

void toggle_debug_draw();
void draw_debug(HudPrimitives *hud_prim);
} // namespace trackir
