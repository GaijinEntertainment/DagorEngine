//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/detail/nameSpaceNameId.h>
#include <daScript/simulate/bind_enum.h>

DAS_BIND_ENUM_CAST(dabfg::NameSpaceNameId)
DAS_BASE_BIND_ENUM_FACTORY(dabfg::NameSpaceNameId, "NameSpaceNameId")

namespace das
{

template <>
struct typeName<dabfg::NameSpaceNameId>
{
  constexpr static const char *name() { return "NameSpaceNameId"; }
};

}; // namespace das