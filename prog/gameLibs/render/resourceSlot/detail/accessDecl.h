#pragma once

#include <detail/ids.h>
#include <detail/expectedLimits.h>

#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/variant.h>

namespace resource_slot::detail
{

struct CreateDecl
{
  SlotId slot;
  ResourceId resource;
};

struct UpdateDecl
{
  SlotId slot;
  ResourceId resource;
  int priority;
};

struct ReadDecl
{
  SlotId slot;
  int priority;
};

typedef eastl::variant<eastl::monostate, CreateDecl, UpdateDecl, ReadDecl> AccessDecl;
typedef dag::RelocatableFixedVector<AccessDecl, EXPECTED_MAX_DECLARATIONS> AccessDeclList;

} // namespace resource_slot::detail