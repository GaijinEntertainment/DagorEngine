// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/component.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/utility/enumDescription.h>
#include <EASTL/vector_map.h>
#include <EASTL/string.h>

namespace ecs
{

using DescriptionMap = eastl::vector_map<eastl::string, EnumLoaderDesc>;

static DescriptionMap &get_descriptions()
{
  static DescriptionMap descriptionMap;
  return descriptionMap;
}

void ecs_enum_registration(const char *enum_name, enum_parse_t parse, enum_names_t get_names, update_enum_t update_value,
  find_enum_idx_t find_enum, component_type_t component)
{
  if (auto it = get_descriptions().find_as(enum_name, eastl::less<>()); it != get_descriptions().end())
  {
    logerr("you try to register more than one enum type %s", enum_name);
    return;
  }
  get_descriptions().emplace(enum_name, EnumLoaderDesc{parse, get_names, update_value, find_enum, component});
}

const EnumLoaderDesc *find_enum_description(eastl::string_view tp_name)
{
  auto it = get_descriptions().find_as(tp_name.data(), eastl::less<>());
  return it == get_descriptions().end() ? nullptr : &it->second;
}

bool is_type_ecs_enum(const char *type_name) { return find_enum_description(type_name) != nullptr; }

dag::ConstSpan<const char *> get_ecs_enum_values(const char *type_name)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    return enumDescr->getNames();
  }
  return {};
}

void update_enum_value(const char *type_name, ecs::EntityComponentRef &enum_component, int enum_idx)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    enumDescr->updateValue(enum_component, enum_idx);
  }
}

int find_enum_idx(const char *type_name, const ecs::EntityComponentRef &enum_component)
{
  if (auto enumDescr = find_enum_description(type_name))
  {
    return enumDescr->findEnum(enum_component);
  }
  return -1;
}
} // namespace ecs