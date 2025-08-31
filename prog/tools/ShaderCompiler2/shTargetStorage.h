// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shaderTab.h"
#include <drv/3d/dag_renderStates.h>
#include <memory/dag_mem.h>
#include <shaders/shader_layout.h>
#include <EASTL/unique_ptr.h>

class ShaderClass;

struct ShaderTargetStorage
{
  Tab<ShaderClass *> shaderClass{};
  Tab<TabStcode> shadersStcode{};
  Tab<shaders::RenderState> renderStates{};

  Tab<dag::ConstSpan<unsigned>> shadersFsh{};
  Tab<dag::ConstSpan<unsigned>> shadersVpr{};
  Tab<TabFsh> ldShFsh{};
  Tab<TabVpr> ldShVpr{};
  Tab<SmallTab<unsigned, TmpmemAlloc> *> shadersCompProg{};

  Tab<shader_layout::StcodeConstValidationMask *> stcodeConstValidationMasks{};

  ~ShaderTargetStorage();
};