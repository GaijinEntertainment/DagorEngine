//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>

class String;
class Point3;

namespace scene
{
class TiledScene;
}

class IndoorProbeScenes;
class IndoorProbeManager;
struct RiGenVisibility;

class DebugLightProbeShapeRenderer
{
public:
  DebugLightProbeShapeRenderer(const IndoorProbeScenes *scenes);

  const IndoorProbeScenes *scenes;
  void render(const RiGenVisibility *visibility, const Point3 &view_pos);
};