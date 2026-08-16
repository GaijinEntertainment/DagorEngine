// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/ecsQuery.h>
#include <daECS/core/componentType.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <sqrat.h>

// Component lists written in script, shared by the ecs quirrel bindings and by
// libraries that build queries from the same syntax.

namespace ecs
{
namespace sq
{

// FROM_REGISTRY (a class or null in the type slot) is not the same as OMITTED:
// an omitted slot stays auto.
enum class CompTypeSlot
{
  OMITTED,
  EXPLICIT,
  FROM_REGISTRY,
  SCRIPT_COMP
};

struct ParsedComp
{
  Sqrat::Object key;
  Sqrat::Object defVal;
  bool hasDefVal = false; // a default was given, which may itself be null
  component_type_t type = ComponentTypeInfo<auto_type>::type;
  CompTypeSlot typeSlot = CompTypeSlot::OMITTED;

  const char *getName() const { return sq_objtostring(&const_cast<Sqrat::Object &>(key).GetObject()); }
};

// One entry of a component list: "name", ["name"], ["name", ecs.TYPE_*] or
// ["name", ecs.TYPE_*, default]. list_label and entry_idx only appear in err_msg.
bool parse_comp_entry(const Sqrat::Object &entry, int entry_idx, const char *list_label, ParsedComp &out, String &err_msg);

// auto_type while nothing has registered the component yet
component_type_t registered_comp_type(component_t name_hash);

struct CompTypeInfo
{
  component_type_t type = 0;
  type_index_t typeId = INVALID_COMPONENT_TYPE_INDEX;
  uint16_t size = 0;
};

class QueryCompTypes
{
public:
  QueryCompTypes(IMemAlloc *m = tmpmem_ptr()) : info(m) {}

  // Fills entry i from query column start + i. False while any column type is
  // still unknown: an auto component gets one only once something registers it.
  bool update(const QueryView &qv, uint32_t start, uint32_t count);

  const CompTypeInfo &operator[](uint32_t i) const { return info[i]; }
  dag::ConstSpan<CompTypeInfo> all() const { return dag::ConstSpan<CompTypeInfo>(info.data(), info.size()); }

private:
  Tab<CompTypeInfo> info;
  uint32_t lastRegisteredCount = 0;
  bool resolved = false;
};

// null when the component is missing from this chunk; an unknown type means an
// unresolved query column, which has no data either
inline uint8_t *comp_row_data(const QueryView &qv, uint32_t comp_id, uint32_t row, const CompTypeInfo &ti)
{
  if (ti.typeId == INVALID_COMPONENT_TYPE_INDEX)
    return nullptr;
  uint8_t *base = (uint8_t *)qv.getComponentUntypedData(comp_id);
  return base ? base + row * ti.size : nullptr;
}

// True when the script value equals what push_comp_val_copy would push for
// this component data, without building anything. False negatives are allowed
// (the caller then rebuilds the value); false positives are not. Uses the VM
// stack transiently, net stack effect is zero.
bool comp_val_equal(HSQUIRRELVM vm, const HSQOBJECT &val, const void *comp_data, component_type_t type, type_index_t type_id);

} // namespace sq
} // namespace ecs
