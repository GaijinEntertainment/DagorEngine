//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector.h>

class String;

struct E3DCOLOR;

namespace scene
{
class TiledScene;
}
struct RiGenVisibility;
class DebugBoxRenderer
{
  eastl::vector<const char *> groupNames;
  eastl::vector<const scene::TiledScene *> groupScenes;
  eastl::vector<E3DCOLOR> groupColors;
  eastl::vector<bool> groupStates;

public:
  DebugBoxRenderer(eastl::vector<const char *> &&names, eastl::vector<const scene::TiledScene *> &&scenes,
    eastl::vector<E3DCOLOR> &&colors);

  String processCommand(const char *argv[], int argc);
  float verifyRIDistance;
  bool needLogText;
  void render(const RiGenVisibility *visibility);
};
