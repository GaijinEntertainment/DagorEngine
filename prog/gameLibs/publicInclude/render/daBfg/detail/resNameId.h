//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/type_traits.h>
#include <util/dag_stdint.h>


// TODO: change to dabfg::detail
namespace dabfg
{

enum class ResNameId : uint16_t
{
  Invalid = static_cast<eastl::underlying_type_t<ResNameId>>(-1)
};

} // namespace dabfg
