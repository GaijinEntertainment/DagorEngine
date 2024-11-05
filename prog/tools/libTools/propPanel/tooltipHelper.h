// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{

class TooltipHelper
{
public:
  // This must be called after ImGui::NewFrame before calling any updateImgui functions.
  void afterNewFrame();

  // This must be called before ImGui::EndFrame.
  void beforeEndFrame();

  // Display tooltip for the previous ImGui control if the mouse is over it.
  // Similar to ImGui::SetItemTooltip but the tooltip behaves more like Windows tooltips.
  // control: any unique identifier that identifies the control. It can be (const void *)ImGui::GetItemID() too.
  void setPreviousImguiControlTooltip(const void *control, const char *text, const char *text_end = nullptr);

private:
  const void *lastHoveredControl = nullptr;
  const void *preventTooltipForControl = nullptr;
  bool preventTooltips = false;
};

extern TooltipHelper tooltip_helper;

} // namespace PropPanel