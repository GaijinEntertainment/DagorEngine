// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>
struct FastPhysTag
{}; // just a tag. We can't use ecs::Tag because we have constructor. But data is stored within Animchar. Unfortunately


// we can of course use ECS_DECLARE_TYPE, but than it would be type of size 1.
//  Empty structure (size 0) isn't common type component (I think it is the only one!), but because we pass ownership to AnimChar, it
//  is already there, so... and we have to have CTM (for component)

namespace ecs
{
template <>
struct ComponentTypeInfo<FastPhysTag>
{
  static constexpr const char *type_name = "FastPhysTag";
  static constexpr component_type_t type = ECS_HASH(type_name).hash;
  static constexpr bool is_pod_class = false;
  static constexpr bool is_boxed = false;
  static constexpr bool is_non_trivial_move = false;
  static constexpr bool is_legacy = false;
  static constexpr bool is_create_on_templ_inst = false;
  static constexpr size_t size = 0;
};
} // namespace ecs
