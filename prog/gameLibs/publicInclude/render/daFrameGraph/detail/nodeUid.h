//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/nodeNameId.h>


namespace dafg::detail
{

// Index stamped with a generational counter.
// Used to implement RAII node handles
// NOTE: there's an additional validness flag to make this type
// trivially relocatable from das's point of view
// NOTE: nodeId should always be valid within this structure
struct NodeUid
{
  NodeNameId nodeId;
  uint16_t generation : 15;
  uint16_t valid : 1;
};

static_assert(sizeof(NodeUid) == sizeof(uint16_t) * 2);

} // namespace dafg::detail
