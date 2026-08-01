// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/visualization/userGraphVisualizer.h>
#include <debug/visualization/gpuCaptureWindow.h>
#include <debug/gpuRangeCapture.h>

#include <debug/backendDebug.h>
#include <debug/textureVisualization.h>

#include <runtime/runtime.h>
#include <frontend/nodeTracker.h>
#include <frontend/nameResolver.h>
#include <frontend/multiplexingInternal.h>
#include <backend/simdBitVector.h>
#include <backend/simdBitMatrix.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <imgui-node-editor/imgui_bezier_math.h>

#include <generic/dag_reverseView.h>
#include <generic/dag_bitset.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <render/debugTexOverlay.h>
#include <perfMon/dag_statDrv.h>

namespace dafg::visualization::usergraph
{

void Visualizer::drawGpuCaptureWindow()
{
  if (gpuCapture.focusGpuCaptureWindow)
  {
    ImGui::SetWindowFocus();
    ImGui::SetWindowCollapsed(false);
    gpuCapture.focusGpuCaptureWindow = false;
  }
  if (ImGui::IsWindowCollapsed())
    return;

  if (ImGuiDagor::ComboWithFilter("Frame start##captureStart", nsNodeNames, gpuCapture.captureStart.comboIndex,
        gpuCapture.captureStart.searchInput, false, true, "frame start (search node...)") &&
      gpuCapture.captureStart.comboIndex != UNKNOWN_INDEX)
    gpuCapture.captureStart.nameId = nsNodeNameIds[gpuCapture.captureStart.comboIndex];
  if (gpuCapture.captureStart.nameId != NodeNameId::Invalid)
  {
    ImGui::SameLine();
    if (ImGui::SmallButton("x##captureStartReset"))
      gpuCapture.captureStart.clear();
  }

  if (ImGuiDagor::ComboWithFilter("Frame end##captureEnd", nsNodeNames, gpuCapture.captureEnd.comboIndex,
        gpuCapture.captureEnd.searchInput, false, true, "frame end (search node...)") &&
      gpuCapture.captureEnd.comboIndex != UNKNOWN_INDEX)
    gpuCapture.captureEnd.nameId = nsNodeNameIds[gpuCapture.captureEnd.comboIndex];
  if (gpuCapture.captureEnd.nameId != NodeNameId::Invalid)
  {
    ImGui::SameLine();
    if (ImGui::SmallButton("x##captureEndReset"))
      gpuCapture.captureEnd.clear();
  }

  if (gpuCapture.distinctExtents.size() > 1)
  {
    const multiplexing::Extents extents = dafg::Runtime::get().getMultiplexingExtents();

    const auto formatExtentLabel = [](eastl::string &out, intermediate::MultiplexingIndex idx, multiplexing::Extents ext) {
      out.clear();
      if (idx == intermediate::MultiplexingIndex::Invalid)
      {
        out.append_sprintf("First extent (execution order)");
        return;
      }
      append_multiplexing_index_label(out, idx, ext, " ");
      if (out.empty())
        out.append_sprintf("0");
    };

    // The preview label only depends on the filter and extents; reformat on change.
    if (gpuCapture.captureExtentFilter != gpuCapture.lastLabeledExtentFilter || extents != gpuCapture.lastLabeledExtents)
    {
      formatExtentLabel(gpuCapture.captureExtentLabel, gpuCapture.captureExtentFilter, extents);
      gpuCapture.lastLabeledExtentFilter = gpuCapture.captureExtentFilter;
      gpuCapture.lastLabeledExtents = extents;
    }
    if (ImGui::BeginCombo("Multiplex extent##captureExtent", gpuCapture.captureExtentLabel.c_str()))
    {
      if (ImGui::Selectable("First extent (execution order)",
            gpuCapture.captureExtentFilter == intermediate::MultiplexingIndex::Invalid))
        gpuCapture.captureExtentFilter = intermediate::MultiplexingIndex::Invalid;
      eastl::string itemLabel;
      for (const auto midx : gpuCapture.distinctExtents)
      {
        formatExtentLabel(itemLabel, midx, extents);
        const bool selected = gpuCapture.captureExtentFilter == midx;
        if (ImGui::Selectable(itemLabel.c_str(), selected))
          gpuCapture.captureExtentFilter = midx;
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }
  else
  {
    ImGui::TextDisabled("single multiplexing extent");
  }

  if (ImGui::Button("Make GPU capture"))
    dafg::gpu_capture_range_request(gpuCapture.captureStart.nameId, gpuCapture.captureEnd.nameId, gpuCapture.captureExtentFilter,
      gpuCapture.captureCount, registry);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80);
  ImGui::InputInt("captures##captureCount", &gpuCapture.captureCount);
  gpuCapture.captureCount = gpuCapture.captureCount < 1 ? 1 : gpuCapture.captureCount;

  const auto captureState = dafg::gpu_capture_range_get_state();

  if (captureState.capturedLog && captureState.capturedLog[0])
    ImGui::TextUnformatted(captureState.capturedLog);
}

void Visualizer::setCaptureBoundary(CaptureBoundary &boundary, NodeNameId id)
{
  const auto it = eastl::find(nsNodeNameIds.begin(), nsNodeNameIds.end(), id);
  if (it == nsNodeNameIds.end())
    return;
  boundary.nameId = id;
  boundary.comboIndex = static_cast<int>(it - nsNodeNameIds.begin());
  boundary.searchInput = getName(id);
}

} // namespace dafg::visualization::usergraph
