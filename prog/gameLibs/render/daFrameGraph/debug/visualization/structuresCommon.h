// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>

#include <imgui.h>


[[maybe_unused]] constexpr auto IMGUI_WINDOW_GROUP_FG2 = "FrameGraph WIP";
[[maybe_unused]] constexpr auto IMGUI_USG_WIN_NAME = "User Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_IRG_WIN_NAME = "IR Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_RES_WIN_NAME = "Resourse Visualizer";
[[maybe_unused]] constexpr auto IMGUI_TEX_WIN_NAME = "Texture Visualizer";


namespace dafg::visualization
{

struct CanvasCamera
{
  const ImVec2 initCanvasOffset = ImVec2(0.f, 0.f);
  const int initCanvasScaleId = 0;

  ImVec2 canvasOffset = initCanvasOffset;
  int curCanvasScaleId = initCanvasScaleId;

  const dag::RelocatableFixedVector<float, 16> canvasScales = {1.0f};

public:
  float getZoom() const { return canvasScales[curCanvasScaleId]; }

  void zoomIn() { curCanvasScaleId = min<int>(curCanvasScaleId + 1, canvasScales.size() - 1); }
  void zoomOut() { curCanvasScaleId = max<int>(curCanvasScaleId - 1, 0); }

  void reset()
  {
    canvasOffset = initCanvasOffset;
    curCanvasScaleId = initCanvasScaleId;
  }
};

} // namespace dafg::visualization