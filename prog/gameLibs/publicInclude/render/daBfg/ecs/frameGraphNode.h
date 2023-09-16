//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityManager.h>
#include <render/daBfg/bfg.h>


template <>
struct ecs::TypeCopyConstructible<dag::Vector<dabfg::NodeHandle>>
{
  static constexpr bool value = false;
};

template <>
struct ecs::TypeCopyAssignable<dag::Vector<dabfg::NodeHandle>>
{
  static constexpr bool value = false;
};

template <>
struct ecs::TypeComparable<dag::Vector<dabfg::NodeHandle>>
{
  static constexpr bool value = false;
};

ECS_DECLARE_RELOCATABLE_TYPE(dabfg::NodeHandle);
ECS_DECLARE_RELOCATABLE_TYPE(dag::Vector<dabfg::NodeHandle>);
