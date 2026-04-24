// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "platform.h"

namespace drv3d_dx12::issues::external
{
/// Issues of the core runtime
namespace core
{
#if _TARGET_PC_WIN
/// With Agility SDK 618 the D3D Core runtime will crash when trying to get or store pipeline cache for pipelines that use stream
/// output and have no geometry shader stage.
inline bool broken_pipeline_cache_with_so_without_gs() { return 618 == Direct3D12Enviroment::getD3D12SdkVersion(); }
#else
inline constexpr bool broken_pipeline_cache_with_so_without_gs() { return false; }
#endif
/// Issues with Nvidia drivers
namespace nvidia
{}
/// Issues with AMD drivers
namespace amd
{}
/// Issues with Intel drivers
namespace intel
{}
} // namespace core
} // namespace drv3d_dx12::issues::external