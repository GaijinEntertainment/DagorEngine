// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_status_bar.h"

#include "pluginService/graph_tex_gen_service.h"

#include <de3_interface.h>
#include <propPanel/propPanelService.h>
#include <graphEditor/graph_data.h>
#include <libTools/util/hdpiUtil.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <EASTL/algorithm.h>

#include <math.h>

namespace
{
constexpr int STATUS_BAR_HEIGHT = 32;
constexpr int STATUS_BAR_PADDING = 6;    // bar's inner horizontal padding
constexpr int STATUS_BAR_BLOCK_GAP = 8;  // gap between status blocks
constexpr int STATUS_BAR_ICON_GAP = 4;   // gap between a block's icon and its label
constexpr int STATUS_DONUT_SIZE = 20;    // memory donut outer diameter
constexpr int STATUS_MSG_ICON_SIZE = 16; // status message icon (error / warning / info)
constexpr int STATUS_PROGRESS_HEIGHT = 20;
constexpr int STATUS_PROGRESS_PAD_X = 8; // pill's inner left padding for the counter label

constexpr int STATUS_SLOT_MIN_WIDTH = 200;
constexpr float STATUS_SLOT_MAX_FRACTION = 0.25f;

constexpr ImU32 STATUS_ERROR_COLOR = IM_COL32(0xF1, 0x47, 0x47, 0xFF);
constexpr ImU32 STATUS_WARNING_COLOR = IM_COL32(0xFF, 0xCF, 0x11, 0xFF);
constexpr ImU32 STATUS_INFO_COLOR = IM_COL32(0x42, 0x96, 0xFA, 0xFF);
// White at 0.15 alpha; shared by the donut's background ring and the progress pill's track.
constexpr ImU32 STATUS_TRACK_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 38);
constexpr ImU32 STATUS_DONUT_FILL_COLOR = IM_COL32(0xD9, 0xD9, 0xD9, 0xFF);
constexpr ImU32 STATUS_PROGRESS_FILL_COLOR = IM_COL32(0x24, 0x5D, 0xB3, 0xFF);
constexpr ImU32 STATUS_TRACK_HOVER_COLOR = IM_COL32(0xFF, 0xFF, 0xFF, 89);
constexpr ImU32 STATUS_DONUT_FILL_HOVER_COLOR = IM_COL32(0x42, 0x96, 0xFA, 0xFF);

// "Label: " in the regular face + value in bold, as one visual run. The design renders the
// numeric part of every status label with fontWeight 700.
void draw_status_label_value(const char *label, const char *value, ImFont *bold_font)
{
  ImGui::TextUnformatted(label);
  ImGui::SameLine(0.0f, 0.0f);
  ImGui::PushFont(bold_font, 0.0f);
  ImGui::TextUnformatted(value);
  ImGui::PopFont();
}

// Full-circle track + clockwise value arc starting at 12 o'clock. The design's donut uses
// innerRadius 0.5, i.e. the ring spans [0.5R .. R] -- a stroke of thickness R/2 centered on
// radius 0.75R.
void draw_status_donut(ImDrawList *dl, const ImVec2 &center, float outer_radius, float fraction, ImU32 track_color, ImU32 fill_color)
{
  const float thickness = outer_radius * 0.5f;
  const float radius = outer_radius - thickness * 0.5f;
  dl->AddCircle(center, radius, track_color, 0, thickness);

  const float f = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
  if (f >= 0.999f)
  {
    dl->AddCircle(center, radius, fill_color, 0, thickness);
  }
  else if (f > 0.001f)
  {
    const float startAngle = -IM_PI * 0.5f;
    const float endAngle = startAngle + f * 2.0f * IM_PI;
    dl->PathArcTo(center, radius, startAngle, endAngle);
    dl->PathStroke(fill_color, ImDrawFlags_None, thickness);
    // PathStroke ends with butt caps; the design's value arc is round-capped (clearly
    // visible on the rendered component) -- patch a half-disc onto each end.
    const ImVec2 capStart(center.x + cosf(startAngle) * radius, center.y + sinf(startAngle) * radius);
    const ImVec2 capEnd(center.x + cosf(endAngle) * radius, center.y + sinf(endAngle) * radius);
    dl->AddCircleFilled(capStart, thickness * 0.5f, fill_color);
    dl->AddCircleFilled(capEnd, thickness * 0.5f, fill_color);
  }
}

} // namespace

float graph_status_bar_height() { return static_cast<float>(hdpi::_pxS(STATUS_BAR_HEIGHT)); }

void draw_graph_status_bar(const GraphData &graph_data, IGraphTexGenService *tex_gen_service, int selected_object_count,
  float bar_height)
{
  TexGenPipelineStatus pipeline;
  if (tex_gen_service)
  {
    pipeline = tex_gen_service->getPipelineStatus();
  }

  const float padX = static_cast<float>(hdpi::_pxS(STATUS_BAR_PADDING));
  const float blockGap = static_cast<float>(hdpi::_pxS(STATUS_BAR_BLOCK_GAP));
  const float iconGap = static_cast<float>(hdpi::_pxS(STATUS_BAR_ICON_GAP));
  const float donutSize = static_cast<float>(hdpi::_pxS(STATUS_DONUT_SIZE));

  // Theme-driven background; labels inherit the theme text color (no forced white).
  const ImU32 barBg = ImGui::GetColorU32(ImGuiCol_PopupBg);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, barBg);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padX, 0.0f));
  const bool barVisible = ImGui::BeginChild("graph_status_bar", ImVec2(0.0f, bar_height), ImGuiChildFlags_AlwaysUseWindowPadding,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  if (barVisible)
  {
    ImFont *boldFont = DAEDITOR3.getPropPanelService()->getBoldFont();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float textY = (bar_height - ImGui::GetTextLineHeight()) * 0.5f;
    const float donutY = (bar_height - donutSize) * 0.5f;

    // Donut + label + hover tooltip carrying the exact megabyte figure (the donut alone
    // only shows the fraction of the budget).
    auto drawMemoryBlock = [&](const char *label, float fraction, uint64_t bytes) {
      ImGui::SetCursorPosY(donutY);
      ImGui::BeginGroup();
      const ImVec2 donutPos = ImGui::GetCursorScreenPos();
      ImGui::Dummy(ImVec2(donutSize, donutSize));
      ImGui::SameLine(0.0f, iconGap);
      ImGui::SetCursorPosY(textY);
      ImGui::TextUnformatted(label);
      ImGui::EndGroup();

      const bool hovered = ImGui::IsItemHovered();
      draw_status_donut(dl, ImVec2(donutPos.x + donutSize * 0.5f, donutPos.y + donutSize * 0.5f), donutSize * 0.5f, fraction,
        hovered ? STATUS_TRACK_HOVER_COLOR : STATUS_TRACK_COLOR, hovered ? STATUS_DONUT_FILL_HOVER_COLOR : STATUS_DONUT_FILL_COLOR);
      if (hovered)
      {
        if (pipeline.gpuMemTotal > 0)
        {
          ImGui::SetTooltip("%.3f / %.0f GB", static_cast<double>(bytes) / static_cast<double>(1 << 30),
            static_cast<double>(pipeline.gpuMemTotal) / static_cast<double>(1 << 30));
        }
        else
        {
          ImGui::SetTooltip("%.1f MB", static_cast<double>(bytes) / static_cast<double>(1 << 20));
        }
      }
    };

    String tempString;

    // ---- Left group: Items / Selected / Outputs. "Selected:" only appears while the user
    // has a live selection, mirroring the design's hidden-by-default block.
    ImGui::SetCursorPosY(textY);
    tempString.printf(0, "%d", static_cast<int>(graph_data.nodes.size()));
    draw_status_label_value("Items: ", tempString.str(), boldFont);

    if (selected_object_count > 0)
    {
      ImGui::SameLine(0.0f, blockGap);
      ImGui::SetCursorPosY(textY);
      tempString.printf(0, "%d", selected_object_count);
      draw_status_label_value("Selected: ", tempString.str(), boldFont);
    }

    ImGui::SameLine(0.0f, blockGap);
    ImGui::SetCursorPosY(textY);
    tempString.printf(0, "%d", pipeline.outputsCount);
    draw_status_label_value("Outputs: ", tempString.str(), boldFont);
    const float leftEndX = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;

    // ---- Right group, right-aligned: [status slot, <= 350px] [mem max] [mem current]. The status
    // slot holds EITHER the live progress pill (while generating) OR a severity-colored message
    // (after a pass) -- never both, they share one location capped at the Figma progress-indicator
    // width. Idle with nothing to report -> the slot is empty (zero width), so a freshly shown
    // status bar carries no message until a generation actually runs.
    const char *MEM_MAX_LABEL = "Memory used (max)";
    const char *MEM_CUR_LABEL = "Memory used (current)";
    const bool showProgress = pipeline.generating && pipeline.commandsTotal > 0;
    // Pill / message frame: at most 1/4 of the bar, but never below STATUS_SLOT_MIN_WIDTH so it
    // stays usable on a narrow panel.
    const float slotMaxW =
      eastl::max(static_cast<float>(hdpi::_pxS(STATUS_SLOT_MIN_WIDTH)), ImGui::GetWindowSize().x * STATUS_SLOT_MAX_FRACTION);
    const float progressHeight = static_cast<float>(hdpi::_pxS(STATUS_PROGRESS_HEIGHT));
    const float iconSize = static_cast<float>(hdpi::_pxS(STATUS_MSG_ICON_SIZE));

    // Message occupies the slot only when NOT generating (the pill owns it during a pass).
    // Priority: stage-1 graph-compile failure > stage-2 texgen error > texgen warning > clean info.
    String msgText;
    ImU32 msgColor = 0;
    const char *iconName = nullptr; // commonData/icons/<theme>/<name>.png; pre-colored, drawn untinted
    if (!showProgress)
    {
      if (pipeline.graphCompileFailed)
      {
        msgText = "ERROR: graph compile failed (unresolved node dependencies)";
        msgColor = STATUS_ERROR_COLOR;
        iconName = "error";
      }
      else if (pipeline.hasErrors)
      {
        msgText = String(0, "ERROR: %s", pipeline.lastError.c_str());
        msgColor = STATUS_ERROR_COLOR;
        iconName = "error";
      }
      else if (pipeline.hasWarnings)
      {
        msgText = String(0, "WARNING: %s", pipeline.lastWarning.c_str());
        msgColor = STATUS_WARNING_COLOR;
        iconName = "warning";
      }
      else if (pipeline.generationCompleted)
      {
        msgText = "Generation complete";
        msgColor = STATUS_INFO_COLOR;
        iconName = "info";
      }
    }

    const ImTextureID msgIcon =
      iconName ? DAEDITOR3.getPropPanelService()->getIconTextureId(iconName, hdpi::_pxS(STATUS_MSG_ICON_SIZE)) : ImTextureID{};
    const float msgIconW = msgIcon ? iconSize + iconGap : 0.0f;

    // Shared slot width: pill is fixed; message hugs its content up to the cap; empty otherwise.
    float slotWidth = 0.0f;
    if (showProgress)
    {
      slotWidth = slotMaxW;
    }
    else if (!msgText.empty())
    {
      slotWidth = eastl::min(slotMaxW, msgIconW + ImGui::CalcTextSize(msgText.str()).x);
    }

    const float slotPart = slotWidth > 0.0f ? slotWidth + blockGap : 0.0f;
    const float rightWidth = slotPart + donutSize + iconGap + ImGui::CalcTextSize(MEM_MAX_LABEL).x + blockGap + donutSize + iconGap +
                             ImGui::CalcTextSize(MEM_CUR_LABEL).x;
    const float rightStartX = eastl::max(ImGui::GetWindowSize().x - padX - rightWidth, leftEndX + blockGap);

    ImGui::SameLine();
    ImGui::SetCursorPosX(rightStartX);

    if (showProgress)
    {
      ImGui::SetCursorPosY((bar_height - progressHeight) * 0.5f);
      const ImVec2 pillMin = ImGui::GetCursorScreenPos();
      ImGui::Dummy(ImVec2(slotMaxW, progressHeight));
      const ImVec2 pillMax(pillMin.x + slotMaxW, pillMin.y + progressHeight);
      const float rounding = progressHeight * 0.5f; // design radius 64 == full pill
      dl->AddRectFilled(pillMin, pillMax, STATUS_TRACK_COLOR, rounding);

      // texGenStep advances by the per-batch command count, so the last batch can overshoot
      // the total; clamp for display.
      const int done = eastl::min(pipeline.commandsDone, pipeline.commandsTotal);
      const float fillWidth = slotMaxW * (static_cast<float>(done) / static_cast<float>(pipeline.commandsTotal));
      if (fillWidth >= 1.0f)
      {
        dl->AddRectFilled(pillMin, ImVec2(pillMin.x + fillWidth, pillMax.y), STATUS_PROGRESS_FILL_COLOR, rounding);
      }

      tempString.printf(0, "%d/%d", done, pipeline.commandsTotal);
      // Theme text color, not a fixed white: the label is left-aligned, so at low progress it sits
      // over the (theme-tinted) track rather than the blue fill -- forcing white made it vanish on
      // the light theme. ImGuiCol_Text reads dark-on-light / light-on-dark and stays legible over
      // both the track and the blue fill across themes.
      dl->AddText(ImVec2(pillMin.x + static_cast<float>(hdpi::_pxS(STATUS_PROGRESS_PAD_X)),
                    pillMin.y + (progressHeight - ImGui::GetTextLineHeight()) * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), tempString.str());
      ImGui::SameLine(0.0f, blockGap);
    }
    else if (slotWidth > 0.0f)
    {
      // Message in the same slot: real themed icon (untinted, pre-colored) + ellipsized
      // severity-colored text, capped at slotWidth. Full text on hover.
      if (msgIcon)
      {
        ImGui::SetCursorPosY((bar_height - iconSize) * 0.5f);
        ImGui::Image(msgIcon, ImVec2(iconSize, iconSize));
        ImGui::SameLine(0.0f, iconGap);
      }
      ImGui::SetCursorPosY(textY);
      const float msgTextWidth = slotWidth - msgIconW;
      const ImVec2 msgTextPos = ImGui::GetCursorScreenPos();
      ImGui::PushStyleColor(ImGuiCol_Text, msgColor);
      ImGui::RenderTextEllipsis(dl, msgTextPos, ImVec2(msgTextPos.x + msgTextWidth, msgTextPos.y + ImGui::GetTextLineHeight()),
        msgTextPos.x + msgTextWidth, msgText.str(), nullptr, nullptr);
      ImGui::PopStyleColor();
      // The ellipsized run is draw-list-only output; a matching Dummy turns it into an item so the
      // full message can be hovered for a tooltip.
      ImGui::Dummy(ImVec2(msgTextWidth, ImGui::GetTextLineHeight()));
      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("%s", msgText.str());
      }
      ImGui::SameLine(0.0f, blockGap);
    }

    // Donut fractions are relative to the dedicated GPU memory budget -- "how close is
    // texgen to exhausting VRAM". When the driver can't report one, fall back to the session
    // peak so current-vs-peak still reads proportionally.
    const float memBudget = static_cast<float>(pipeline.gpuMemTotal > 0 ? pipeline.gpuMemTotal : pipeline.memUsedMax);
    const float maxFraction = memBudget > 0.0f ? static_cast<float>(pipeline.memUsedMax) / memBudget : 0.0f;
    const float currentFraction = memBudget > 0.0f ? static_cast<float>(pipeline.memUsedCurrent) / memBudget : 0.0f;

    drawMemoryBlock(MEM_MAX_LABEL, maxFraction, pipeline.memUsedMax);
    ImGui::SameLine(0.0f, blockGap);
    drawMemoryBlock(MEM_CUR_LABEL, currentFraction, pipeline.memUsedCurrent);
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  ImGui::PopStyleColor(1);
}
