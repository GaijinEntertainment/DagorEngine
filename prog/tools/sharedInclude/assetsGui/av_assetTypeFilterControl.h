// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/assetMgr.h>
#include <generic/dag_span.h>
#include <propPanel/imguiHelper.h>
#include <util/dag_string.h>
#include <imgui/imgui.h>

class AssetTypeFilterControl
{
public:
  bool updateImgui(const DagorAssetMgr &asset_mgr, dag::Span<bool> allowed_types, dag::Span<bool> shown_types)
  {
    // Size the two buttons to the same size to fill the available space.
    // If there is not enough space then make some by using the larger button's size for both buttons.
    const char *resetButtonLabel = "Reset";
    const char *invertButtonLabel = "Invert";
    const ImVec2 resetButtonSize = PropPanel::ImguiHelper::getButtonSize(resetButtonLabel);
    const ImVec2 invertButtonSize = PropPanel::ImguiHelper::getButtonSize(invertButtonLabel);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float availableButtonWidth = (availableWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    const float buttonWidth = max(availableButtonWidth, max(resetButtonSize.x, invertButtonSize.x));
    bool filterChanged = false;

    if (ImGui::Button(resetButtonLabel, ImVec2(buttonWidth, 0.0f)))
    {
      for (int i = 0; i < asset_mgr.getAssetTypesCount(); ++i)
        shown_types[i] = allowed_types[i];

      filterChanged = true;
    }

    ImGui::SameLine();
    if (ImGui::Button(invertButtonLabel, ImVec2(buttonWidth, 0.0f)))
    {
      for (int i = 0; i < asset_mgr.getAssetTypesCount(); ++i)
        if (allowed_types[i])
          shown_types[i] = !shown_types[i];

      filterChanged = true;
    }

    if (showShownTypeFilters(asset_mgr, allowed_types, shown_types))
      filterChanged = true;

    return filterChanged;
  }

private:
  bool showShownTypeFilters(const DagorAssetMgr &asset_mgr, dag::Span<bool> allowed_types, dag::Span<bool> shown_types)
  {
    bool filterChanged = false;

    for (int i = 0; i < asset_mgr.getAssetTypesCount(); ++i)
    {
      if (!allowed_types[i])
        continue;

      typeLabelBuffer.printf(64, "%s##%d", asset_mgr.getAssetTypeName(i), i);
      if (PropPanel::ImguiHelper::checkboxWithDragSelection(typeLabelBuffer, &shown_types[i]))
        filterChanged |= true;
    }

    return filterChanged;
  }

  String typeLabelBuffer;
};
