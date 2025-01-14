// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/assetMgr.h>
#include <generic/dag_span.h>
#include <propPanel/imguiHelper.h>
#include <EASTL/optional.h>
#include <imgui/imgui.h>

class AssetTypeFilterButtonGroupControl
{
public:
  // Returns with true if shown_types has changed.
  static bool updateImgui(const DagorAssetMgr &asset_mgr, dag::ConstSpan<bool> allowed_types, dag::Span<bool> shown_types,
    bool &request_picker)
  {
    bool changed = false;

    ImGui::PushID("filterButtonGroup");

    if (mem_eq(allowed_types, shown_types.data()))
    {
      if (filterButton("All"))
        request_picker = true;
    }
    else
    {
      int shownTypeCount = 0;
      int clickedTypeIndex = -1;

      for (int typeIndex = 0; typeIndex < allowed_types.size(); ++typeIndex)
      {
        if (!allowed_types[typeIndex] || !shown_types[typeIndex])
          continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));
        const char *typeName = asset_mgr.getAssetTypeName(typeIndex);
        const ImVec2 size = PropPanel::ImguiHelper::getButtonSize(typeName);
        ImGui::PopStyleVar();

        if (shownTypeCount != 0)
          ImGui::SameLine();

        if (ImGui::GetContentRegionAvail().x < size.x)
          ImGui::NewLine();

        if (filterButton(typeName))
          clickedTypeIndex = typeIndex;

        ++shownTypeCount;
      }

      if (shownTypeCount == 0)
      {
        if (filterButton("None"))
          request_picker = true;
      }
      else if (clickedTypeIndex >= 0)
      {
        changed = true;

        if (shownTypeCount == 1)
        {
          for (int typeIndex = 0; typeIndex < allowed_types.size(); ++typeIndex)
            shown_types[typeIndex] = allowed_types[typeIndex];
        }
        else
        {
          for (int typeIndex = 0; typeIndex < allowed_types.size(); ++typeIndex)
            shown_types[typeIndex] = typeIndex == clickedTypeIndex;
        }
      }
    }

    ImGui::PopID();

    return changed;
  }

private:
  static bool filterButton(const char *label)
  {
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 137, 255, 255));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(209, 209, 209, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(framePadding, framePadding));
    const bool result = ImGui::Button(label);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    return result;
  }

  static constexpr float framePadding = 2.0f;
};
