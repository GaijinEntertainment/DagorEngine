//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <util/dag_stdint.h>


// TODO: change to dafg::detail
namespace dafg
{

enum class ResNameId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<ResNameId>>(-1)
};

} // namespace dafg
