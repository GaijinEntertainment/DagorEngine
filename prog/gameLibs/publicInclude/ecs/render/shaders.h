//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityManager.h>
#include <shaders/dag_shaders.h>

struct ShadersECS
{
  Ptr<ShaderMaterial> shmat;
  Ptr<ShaderElement> shElem;
  explicit operator bool() const;
};

// supports init component *name*_name:t="name"
ECS_DECLARE_RELOCATABLE_TYPE(ShadersECS);
