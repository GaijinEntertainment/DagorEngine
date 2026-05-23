// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "histogram_panel.h"
#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <EditorCore/ec_interface.h>
#include <imgui/imgui.h>
#include <math/dag_color.h>
#include <math.h>

#include <EASTL/vector.h>


HistogramPanel::HistogramPanel(GraphEditorPlg &plg, IGraphTexGenService &ctx) : plugin(plg), texGenService(ctx), panelWindow(nullptr)
{
  panelWindow = IEditorCoreEngine::get()->createPropPanel(this, "Histogram");
}

HistogramPanel::~HistogramPanel() { IEditorCoreEngine::get()->deleteCustomPanel(panelWindow); }

void HistogramPanel::drawHistogramRect(const char *label, const eastl::vector<Color4> &hist, const int *channelIndices,
  int numChannels, const ImU32 *colors, float histW, float histH)
{
  if (hist.empty())
  {
    return;
  }

  const int binCount = (int)hist.size();
  // Border sits at row `borderInset` from the outer edge (crisp 1 px stroke thanks to
  // the +0.5 centerline offset). Bars must stay strictly inside it, so they are inset
  // by 1 more pixel.
  const float borderInset = 1.5f;
  const float barInset = borderInset + 0.5f;
  const float barAreaW = ImMax(histW - 2.0f * barInset, 1.0f);
  const float barAreaH = ImMax(histH - 2.0f * barInset, 1.0f);
  const float binW = barAreaW / (float)binCount;

  ImGui::Text("%s", label);

  const ImVec2 rawPos = ImGui::GetCursorScreenPos();
  const ImVec2 pos(floorf(rawPos.x), floorf(rawPos.y));
  const ImVec2 maxPos(pos.x + floorf(histW), pos.y + floorf(histH));
  ImDrawList *dl = ImGui::GetWindowDrawList();

  const float barBaseX = pos.x + barInset;
  const float barBaseY = floorf(pos.y + barInset + barAreaH);

  dl->AddRectFilled(pos, maxPos, IM_COL32(0, 0, 0, 255));
  dl->AddRect(ImVec2(pos.x + borderInset, pos.y + borderInset), ImVec2(maxPos.x - borderInset, maxPos.y - borderInset),
    IM_COL32(200, 200, 200, 255), 0.0f, 0, 1.0f);

  for (int ch = 0; ch < numChannels; ++ch)
  {
    for (int i = 0; i < binCount; ++i)
    {
      const float val = (&hist[i].r)[channelIndices[ch]];
      if (val <= 0.0f)
        continue;

      const float barH = ImMax(val * barAreaH, 1.0f);
      const float x0 = floorf(barBaseX + i * binW);
      const float x1 = ImMax(floorf(barBaseX + (i + 1) * binW), x0 + 1.0f);
      const float y0 = floorf(barBaseY - barH);
      dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, barBaseY), colors[ch]);
    }
  }

  ImGui::Dummy(ImVec2(histW, histH));
}

void HistogramPanel::updateImgui()
{
  const SelectedTextureState &state = texGenService.getSelectedTextureState();

  if (state.histogram01.empty())
  {
    ImGui::TextDisabled("No histogram data");
    return;
  }

  const ImVec2 avail = ImGui::GetContentRegionAvail();
  // Use a fixed-size child with no scrollbars so the layout of the squares below cannot
  // cause scrollbars to toggle (which would shrink avail and feed back into side -> jitter).
  if (!ImGui::BeginChild("hist_content", avail, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
  {
    ImGui::EndChild();
    return;
  }

  const float textLineH = ImGui::GetTextLineHeightWithSpacing();
  const float padding = 4.0f;
  const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
  const bool hasAlpha = state.channels >= 4;

  const ImU32 rgbColors[] = {IM_COL32(200, 25, 20, 255), IM_COL32(25, 200, 20, 255), IM_COL32(20, 25, 200, 255)};
  const ImU32 alphaColor = IM_COL32(80, 80, 80, 255);
  const int rgbChannels[] = {0, 1, 2};
  const int alphaChannel[] = {3};
  const int drawChannels = hasAlpha ? 3 : state.channels;
  const int totalRects = hasAlpha ? 4 : 2;

  const char *range01Label = "0-1 Range [0 .. 255]";
  char fullRangeLabel[128];
  _snprintf(fullRangeLabel, sizeof(fullRangeLabel), "Full Range [%.3f .. %.3f]", state.minHistValue, state.maxHistValue);

  // Minimum side = widest visible label, so the rect is never narrower than its caption.
  float minSide = ImGui::CalcTextSize(range01Label).x;
  minSide = ImMax(minSide, ImGui::CalcTextSize(fullRangeLabel).x);
  if (hasAlpha)
  {
    minSide = ImMax(minSide, ImGui::CalcTextSize("0-1 Alpha").x);
    minSide = ImMax(minSide, ImGui::CalcTextSize("Full Alpha").x);
  }
  minSide = ImMax(16.0f, minSide);

  // Achievable square side for each candidate layout. The grid case only makes sense for 4 rects
  // (2x2); for 2 rects a 2-row grid collapses to the column case so we skip it.
  const bool gridAllowed = (totalRects == 4);
  const float sideRow = ImMin((avail.x - (totalRects - 1) * itemSpacing) / (float)totalRects, avail.y - textLineH);
  const float sideCol = ImMin(avail.x, (avail.y - totalRects * textLineH - (totalRects - 1) * padding) / (float)totalRects);
  const float sideGrid = gridAllowed ? ImMin((avail.x - itemSpacing) * 0.5f, (avail.y - 2.0f * textLineH - padding) * 0.5f) : 0.0f;

  enum Layout
  {
    LayoutRow,
    LayoutGrid,
    LayoutColumn
  };
  Layout layout = LayoutRow;
  float bestSide = sideRow;
  if (gridAllowed && sideGrid > bestSide)
  {
    layout = LayoutGrid;
    bestSide = sideGrid;
  }
  if (sideCol > bestSide)
  {
    layout = LayoutColumn;
    bestSide = sideCol;
  }
  const float side = ImMax(minSide, bestSide);

  if (layout == LayoutRow)
  {
    // All rects in a single row: [0-1 RGB] [0-1 Alpha] [Full RGB] [Full Alpha]
    ImGui::BeginGroup();
    drawHistogramRect(range01Label, state.histogram01, rgbChannels, drawChannels, rgbColors, side, side);
    ImGui::EndGroup();

    if (hasAlpha)
    {
      ImGui::SameLine();
      ImGui::BeginGroup();
      drawHistogramRect("0-1 Alpha", state.histogram01, alphaChannel, 1, &alphaColor, side, side);
      ImGui::EndGroup();
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    drawHistogramRect(fullRangeLabel, state.histogramFull, rgbChannels, drawChannels, rgbColors, side, side);
    ImGui::EndGroup();

    if (hasAlpha)
    {
      ImGui::SameLine();
      ImGui::BeginGroup();
      drawHistogramRect("Full Alpha", state.histogramFull, alphaChannel, 1, &alphaColor, side, side);
      ImGui::EndGroup();
    }
  }
  else if (layout == LayoutGrid)
  {
    // 2x2 grid (alpha case only): row 1 = [0-1 RGB] [0-1 Alpha], row 2 = [Full RGB] [Full Alpha].
    ImGui::BeginGroup();
    drawHistogramRect(range01Label, state.histogram01, rgbChannels, drawChannels, rgbColors, side, side);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    drawHistogramRect("0-1 Alpha", state.histogram01, alphaChannel, 1, &alphaColor, side, side);
    ImGui::EndGroup();

    ImGui::Dummy(ImVec2(0, padding));

    ImGui::BeginGroup();
    drawHistogramRect(fullRangeLabel, state.histogramFull, rgbChannels, drawChannels, rgbColors, side, side);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginGroup();
    drawHistogramRect("Full Alpha", state.histogramFull, alphaChannel, 1, &alphaColor, side, side);
    ImGui::EndGroup();
  }
  else
  {
    // All rects in a single column: [0-1 RGB] [0-1 Alpha] [Full RGB] [Full Alpha]
    drawHistogramRect(range01Label, state.histogram01, rgbChannels, drawChannels, rgbColors, side, side);

    if (hasAlpha)
    {
      ImGui::Dummy(ImVec2(0, padding));
      drawHistogramRect("0-1 Alpha", state.histogram01, alphaChannel, 1, &alphaColor, side, side);
    }

    ImGui::Dummy(ImVec2(0, padding));

    drawHistogramRect(fullRangeLabel, state.histogramFull, rgbChannels, drawChannels, rgbColors, side, side);

    if (hasAlpha)
    {
      ImGui::Dummy(ImVec2(0, padding));
      drawHistogramRect("Full Alpha", state.histogramFull, alphaChannel, 1, &alphaColor, side, side);
    }
  }

  ImGui::EndChild();
}
