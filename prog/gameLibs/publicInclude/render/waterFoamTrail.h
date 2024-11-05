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
  int texSize;
  int cascadeCount;
  float cascadeArea;
  float cascadeAreaMul;
  float cascadeAreaScale;

  float fadeInTime;
  float fadeOutTime;
  float forwardExpand;
  float quadHeight;
  float widthThreshold;
  float tailFadeInLength;

  int activeVertexCount;
  int finalizedVertexCount;
  int maxPointsPerSegment;

  char texName[64];
  bool useTrail;
  bool useObstacle;
  bool useTexArray;
  bool enableGenMuls;
  float underwaterFoamWidth;
  float underwaterFoamAlphaMult;
};

void init(const Settings &s);
void reset();
bool available();
bool hasUnderwaterTrail();
void release();
void enable_render(bool v);
void update_settings(const Settings &s);

int create_emitter(float width, float gen_mul, float alpha_mul, float foam_mul, float inscat_mul);
void destroy_emitter(int em);
void emit_point(int em, const Point2 &pt, const Point2 &dir, float alpha, bool is_underwater = true);
void emit_point_ex(int em, const Point2 &pt, const Point2 &dir, const Point2 &left, const Point2 &right, float alpha);
void finalize_trail(int em);

void before_render(float dt, const Point3 &origin, const Frustum &frustum);
bool render();

const Settings &get_settings();
} // namespace water_trail
