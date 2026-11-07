//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

class Point2;
class Point3;
class Point4;
class TMatrix4;
class BaseTexture;
typedef BaseTexture Texture;

namespace water_trail
{
struct Settings
{
  int tessFactor;

  float fadeOutTime;
  char texName[64];
  bool useTrail;

  bool enableGenMuls;
  float underwaterFoamWidth;
  float underwaterFoamAlphaMult;
};

void init();
void on_unload_scene();
bool available();
bool hasUnderwaterTrail();
void close();
void enable_render(bool v);
void update_render_settings(const Settings &s);

int create_emitter(float width, float life_time, int node_count, float alpha_mul, float foam_mul, float inscat_mul,
  float turbulence_mul, float wave_height);
void destroy_emitter(int em);
void detach_emitter(int em);
void emit_point(int em, const Point2 &pt, const Point2 &dir, float alpha, bool is_underwater = true, float velocity = 10.0f);
void emit_point_ex(int em, const Point2 &pt, const Point2 &dir, const Point2 &left, const Point2 &right, float alpha, float velocity);
void finalize_trail(int em);

void before_render(float dt, const Point3 &origin, const Frustum &frustum, float water_level);
bool render();

const Settings &get_settings();
} // namespace water_trail
