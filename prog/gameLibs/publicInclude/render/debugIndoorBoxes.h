//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>

class ShaderElement;
class ShaderMaterial;

class DebugIndoorBoxes
{
  eastl::unique_ptr<ShaderMaterial> mat;
  ShaderElement *elem = nullptr;

public:
  explicit DebugIndoorBoxes(const char *shaderName);
  void render() const;
};
