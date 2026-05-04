// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "no_tool.h"


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{
struct Configuration;
namespace gpu_capture
{
struct Issues;
namespace intel
{
class GraphicsPerformanceAnalyzers : public NoTool
{
public:
  constexpr void configure() {}

  template <typename T>
  static constexpr bool connect(const Configuration &, Direct3D12Enviroment &, Issues &, T &)
  {
    return false;
  }
};
} // namespace intel
} // namespace gpu_capture
} // namespace debug
} // namespace drv3d_dx12
