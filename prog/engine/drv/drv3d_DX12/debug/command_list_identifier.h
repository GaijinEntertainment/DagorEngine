// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace drv3d_dx12::debug
{
struct CommandListIdentifierOpaqueType;
using CommandListIdentifier = const CommandListIdentifierOpaqueType *;
inline CommandListIdentifier make_command_list_identfier(uint32_t frame_index, uint32_t command_list_index)
{
  // always set lowest bit so that zero can represent nullptr
  return reinterpret_cast<CommandListIdentifier>(static_cast<uintptr_t>(1u | (frame_index << 1) | (command_list_index << 4)));
}
} // namespace drv3d_dx12::debug