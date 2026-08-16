// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compDesc.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

namespace ecs
{
namespace sq
{

component_type_t registered_comp_type(component_t name_hash)
{
  const component_index_t cidx = g_entity_mgr->getDataComponents().findComponentId(name_hash);
  if (cidx != INVALID_COMPONENT_INDEX)
    return g_entity_mgr->getDataComponents().getComponentById(cidx).componentTypeName;
  return ComponentTypeInfo<auto_type>::type;
}

bool parse_comp_entry(const Sqrat::Object &entry, int entry_idx, const char *list_label, ParsedComp &out, String &err_msg)
{
  if (entry.GetType() == OT_STRING)
  {
    out.key = entry;
    return true;
  }
  if (entry.GetType() != OT_ARRAY)
  {
    err_msg.printf(0, "%s entry #%d must be a string or an array, got %s", list_label, entry_idx, sq_objtypestr(entry.GetType()));
    return false;
  }

  Sqrat::Array desc(entry);
  const SQInteger len = desc.Length();
  if (len < 1 || len > 3)
  {
    err_msg.printf(0,
      "%s entry #%d has %d elements; a component is \"name\", [\"name\"], "
      "[\"name\", TYPE] or [\"name\", TYPE, default]",
      list_label, entry_idx, (int)len);
    return false;
  }

  out.key = desc.GetSlot(SQInteger(0));
  if (out.key.GetType() != OT_STRING)
  {
    err_msg.printf(0, "%s entry #%d name must be a string, got %s", list_label, entry_idx, sq_objtypestr(out.key.GetType()));
    return false;
  }

  if (len > 1)
  {
    // ecs.TYPE_* is an integer natively and a class in the script stubs
    Sqrat::Object typeObj = desc.GetSlot(SQInteger(1));
    switch (typeObj.GetType())
    {
      case OT_INTEGER:
        out.type = (component_type_t)typeObj.Cast<SQInteger>();
        out.typeSlot = CompTypeSlot::EXPLICIT;
        break;
      case OT_TABLE: out.typeSlot = CompTypeSlot::SCRIPT_COMP; break;
      case OT_CLASS:
      case OT_NULL: out.typeSlot = CompTypeSlot::FROM_REGISTRY; break;
      default:
        err_msg.printf(0, "%s entry #%d type must be an ecs.TYPE_* value, a table or null, got %s", list_label, entry_idx,
          sq_objtypestr(typeObj.GetType()));
        return false;
    }
  }
  if (len > 2)
  {
    out.defVal = desc.GetSlot(SQInteger(2));
    out.hasDefVal = true;
  }
  return true;
}

bool QueryCompTypes::update(const QueryView &qv, uint32_t start, uint32_t count)
{
  const DataComponents &dataComponents = qv.manager().getDataComponents();
  if (resolved || dataComponents.size() <= lastRegisteredCount)
    return resolved;
  lastRegisteredCount = dataComponents.size();
  resolved = true;
  info.resize(count);

  const component_index_t *cindices = qv.manager().queryComponents(qv.getQueryId()) + start;
  for (uint32_t i = 0; i < count; ++i, ++cindices)
  {
    const DataComponent dt = dataComponents.getComponentById(*cindices);
    info[i].type = dt.componentTypeName;
    info[i].typeId = dt.componentType;
    if (dt.componentType != INVALID_COMPONENT_TYPE_INDEX)
      info[i].size = qv.manager().getComponentTypes().getTypeInfo(dt.componentType).size;
    else
    {
      info[i].size = 0;
      resolved = false;
    }
  }
  return resolved;
}

} // namespace sq
} // namespace ecs
