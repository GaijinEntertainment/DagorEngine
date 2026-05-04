// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

namespace ecs
{

// ecs20 compatibility
void EntityManager::set(EntityId eid, const HashedConstString name, const char *v)
{
  ecs::string *str = getNullableRW<ecs::string>(eid, name);
  if (str)
    *str = v;
  else
    G_ASSERTF(0, "component %s|0x%X is not a string or missing", name.str, name.hash);
}
const char *EntityManager::getOr(ecs::EntityId eid, ecs::HashedConstString name, const char *def) const
{
  const ecs::string *str = getNullable<ecs::string>(eid, name);
  return str ? str->c_str() : def;
}

} // namespace ecs
