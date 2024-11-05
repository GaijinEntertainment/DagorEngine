// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if DAGOR_DBGLEVEL > 0
#include <dag/dag_vector.h>

namespace drv3d_vulkan
{

template <size_t MaxSize, typename T>
struct UiPlotScrollBuffer
{
  size_t offset = 0;

  dag::Vector<T> xData;
  dag::Vector<T> yData;

  UiPlotScrollBuffer()
  {
    xData.reserve(MaxSize);
    yData.reserve(MaxSize);
  }

  void addPoint(T x, T y)
  {
    if (xData.size() < MaxSize)
    {
      xData.push_back(x);
      yData.push_back(y);
    }
    else
    {
      xData[offset] = x;
      yData[offset] = y;
      offset = (offset + 1) % MaxSize;
    }
  }
};

void debug_ui_memory();
void debug_ui_frame();
void debug_ui_timeline();
void debug_ui_resources();
void debug_ui_pipelines();
void debug_ui_renderpasses();
void debug_ui_misc();

void debug_ui_update_pipelines_data();

} // namespace drv3d_vulkan

#else // DAGOR_DBGLEVEL > 0

namespace drv3d_vulkan
{

inline void debug_ui_update_pipelines_data() {}

} // namespace drv3d_vulkan

#endif // DAGOR_DBGLEVEL > 0