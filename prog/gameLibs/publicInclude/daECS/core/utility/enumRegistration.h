//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "enumReflection.h"

namespace ecs
{
template <typename Enum>
ecs::ChildComponent parse_enum(const char *component_name, const char *enum_type_name, const char *enum_value)
{
  Enum enumValue{};
  if (!enum_value || *enum_value == 0)
  {
    logerr("enum component %s : %s hasn't value, fallback to %s(%d)", enum_type_name, component_name, enum_to_str(enumValue),
      enumValue);
    return ecs::ChildComponent(enumValue);
  }
  if (!str_to_enum(enum_value, enumValue))
  {
    logerr("enum component %s : %s value parse failed, unsupported %s value, fallback to %s(%d)", enum_type_name, component_name,
      enum_value, enum_to_str(enumValue), enumValue);
  }
  return ecs::ChildComponent(enumValue);
}

template <typename Enum>
void update_enum(ecs::EntityComponentRef &enum_component, int enum_idx)
{
  change_enum_values(enum_component.getRW<Enum>(), enum_idx);
}

template <typename Enum>
int find_enum_index(const ecs::EntityComponentRef &enum_component)
{
  return find_enum_index(enum_component.get<Enum>());
}

typedef ecs::ChildComponent (*enum_parse_t)(const char *component_name, const char *enum_type_name, const char *enum_value);
typedef dag::ConstSpan<const char *> (*enum_names_t)();
typedef void (*update_enum_t)(ecs::EntityComponentRef &enum_component, int enum_idx);
typedef int (*find_enum_idx_t)(const ecs::EntityComponentRef &enum_component);

void ecs_enum_registration(const char *enum_name, enum_parse_t parse, enum_names_t get_names, update_enum_t update_value,
  find_enum_idx_t find_enum_idx, component_type_t component);

template <typename Enum>
struct EcsEnumRegister
{
  EcsEnumRegister(const char *enum_name)
  {
    ecs_enum_registration(enum_name, &parse_enum<Enum>, &get_enum_names<Enum>, &update_enum<Enum>, &find_enum_index<Enum>,
      ComponentTypeInfo<Enum>::type);
  }
};
} // namespace ecs
#define ECS_REGISTER_ENUM(enum_type) static ecs::EcsEnumRegister<enum_type> MAKE_UNIQUE(enum_type_register, __COUNTER__)(#enum_type);
