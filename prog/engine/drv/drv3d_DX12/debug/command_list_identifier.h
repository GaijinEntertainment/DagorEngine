// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bit.h>

namespace drv3d_dx12::debug
{
struct CommandListIdentifierOpaqueType;
using CommandListIdentifier = const CommandListIdentifierOpaqueType *;

struct CommandListIdentifierFields
{
  uint32_t frameSlot;
  uint32_t historySet;
  uint32_t commandListIndex;
};

inline CommandListIdentifier make_command_list_identifier(const CommandListIdentifierFields &fields)
{
  // always set lowest bit so that zero can represent nullptr
  return eastl::bit_cast<CommandListIdentifier>(1u | (static_cast<uintptr_t>(fields.historySet & 1u) << 1) |
                                                (static_cast<uintptr_t>(fields.frameSlot & 3u) << 2) |
                                                (static_cast<uintptr_t>(fields.commandListIndex) << 4));
}

inline CommandListIdentifierFields decode_command_list_identifier(CommandListIdentifier id)
{
  const auto bits = eastl::bit_cast<uintptr_t>(id);
  return {
    .frameSlot = static_cast<uint32_t>(bits >> 2) & 3u,
    .historySet = static_cast<uint32_t>(bits >> 1) & 1u,
    .commandListIndex = static_cast<uint32_t>(bits >> 4),
  };
}
} // namespace drv3d_dx12::debug
