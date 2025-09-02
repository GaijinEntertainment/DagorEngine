//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/nameSpaceNameId.h>
#include <daScript/simulate/bind_enum.h>

DAS_BIND_ENUM_CAST(dafg::NameSpaceNameId)
DAS_BASE_BIND_ENUM_FACTORY(dafg::NameSpaceNameId, "NameSpaceNameId")

namespace das
{

template <>
struct typeName<dafg::NameSpaceNameId>
{
  constexpr static const char *name() { return "NameSpaceNameId"; }
};

}; // namespace das