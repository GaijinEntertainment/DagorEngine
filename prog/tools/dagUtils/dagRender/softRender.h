// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <math/dag_Point3.h>
#include <EASTL/string.h>

class Mesh;

// Aims at the bbox center; yaw: +Z toward +X
struct RenderView
{
  float yawDeg = 40, pitchDeg = 25;
  float dist = 0; // 0: fit the bbox; ortho ignores it
  bool ortho = false;
  float orthoWidth = 0; // meters across; 0: fit the bbox
  bool centered = false;
  float cx = 0, cy = 0; // ortho center: p * right, p * up
};

struct RenderDot
{
  Point3 pos, color; // color: linear
};

struct RenderOverlay
{
  const char *ref = nullptr;     // image under the render
  const char *refDiff = nullptr; // the silhouette diff against ref, a png
  const char *env = nullptr;     // equirectangular sky; null: procedural
  float refAlpha = 0.5f;
  int supersample = 2; // per axis
  bool grid = false;   // 0.1 m lines on y = 0
  dag::ConstSpan<RenderDot> dots;
  Point3 background = Point3(0.85f, 0.85f, 0.85f); // sRGB
  float envYaw = 0;                                // degrees, +Z toward +X
};

constexpr float RENDER_GRAY = 0.8f; // linear, a face without a color

enum
{
  SHADE_PAINT,
  SHADE_CHROME,
  SHADE_MATTE,
  SHADE_GLASS,
  SHADE_METALLIC,
  SHADE_COUNT,
};

// face mat: a SHADE_ value; diffuse: per face, scales the base color (empty: 1); needs vertnorm and facengr;
// width 0: the ref image's size, else a size of its aspect to a pixel
bool render_png(const Mesh &m, dag::ConstSpan<Point3> diffuse, const RenderView &view, const RenderOverlay &overlay, int &width,
  int &height, const char *path, eastl::string &err);
