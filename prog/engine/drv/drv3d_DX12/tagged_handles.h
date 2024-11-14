// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "tagged_handle.h"


namespace drv3d_dx12
{

struct InputLayoutIDTag;
struct StaticRenderStateIDTag;
struct InternalInputLayoutIDTag;
struct FramebufferLayoutIDTag;
struct BindlessSetIdTag;

using InputLayoutID = TaggedHandle<int, -1, InputLayoutIDTag>;
using StaticRenderStateID = TaggedHandle<int, -1, StaticRenderStateIDTag>;
using InternalInputLayoutID = TaggedHandle<int, -1, InternalInputLayoutIDTag>;
using FramebufferLayoutID = TaggedHandle<int, -1, FramebufferLayoutIDTag>;
using BindlessSetId = TaggedHandle<uint32_t, 0, BindlessSetIdTag>;

}; // namespace drv3d_dx12
