//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_rendInstRes.h>

class IEntityGetDynSceneLodsRes
{
public:
  static constexpr unsigned HUID = 0x96E6D403u; // IEntityGetDynSceneLodsRes

  virtual DynamicRenderableSceneLodsResource *getSceneLodsRes() = 0;
  virtual const DynamicRenderableSceneLodsResource *getSceneLodsRes() const = 0;
  virtual DynamicRenderableSceneInstance *getSceneInstance() = 0;
};

class IEntityGetRISceneLodsRes
{
public:
  static constexpr unsigned HUID = 0x96E6D404u; // IEntityGetRISceneLodsRes

  virtual RenderableInstanceLodsResource *getSceneLodsRes() = 0;
  virtual const RenderableInstanceLodsResource *getSceneLodsRes() const = 0;
};
