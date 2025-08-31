//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/resNameId.h>


namespace dafg::detail
{

// This is different from a regular res id in that it stores a
// history flag inside it. Note that more info might be packed here
// in the future, but hopefully not.
// Storing additional flags allows us to uniquely identify the resource
// e.g. in handles
// NOTE: resId should always be valid within this structure
struct ResUid
{
  ResNameId resId;
  uint16_t history : 1;
};

static_assert(sizeof(ResUid) == sizeof(uint16_t) * 2);

} // namespace dafg::detail
