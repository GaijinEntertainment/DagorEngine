//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/detail/nodeNameId.h>


namespace dabfg::detail
{

// Index stamped with a generational counter.
// Used to implement RAII node handles
// NOTE: there's an additional validness flag to make this type
// trivially relocatable from das's point of view
// NOTE: nodeId should always be valid within this structure
struct NodeUid
{
  NodeNameId nodeId;
  uint32_t generation : 31;
  uint32_t valid : 1;
};

static_assert(sizeof(NodeUid) == sizeof(uint32_t) * 2);

} // namespace dabfg::detail
