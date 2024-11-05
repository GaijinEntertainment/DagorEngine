//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IEffectController
{
public:
  static constexpr unsigned HUID = 0xD06199A1u; // IEffectController

  virtual bool isActive() const = 0;
  virtual void setSpawnRate(float rate) = 0;
};
