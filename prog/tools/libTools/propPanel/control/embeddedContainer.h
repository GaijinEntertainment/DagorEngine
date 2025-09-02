// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include <imgui/imgui.h>

namespace PropPanel
{

// Compared to ContainerPropertyControl this container works when used with new_line = false.
// NOTE: ImGui porting: this was called CContainer.
class EmbeddedContainerPropertyControl : public ContainerPropertyControl
{
public:
  EmbeddedContainerPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y,
    hdpi::Px w, hdpi::Px h, hdpi::Px vertical_space_between_controls) :
    ContainerPropertyControl(id, event_handler, parent, x, y, w, h)
  {
    setVerticalSpaceBetweenControls(vertical_space_between_controls);
  }

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void updateImgui() override
  {
    if (mControlArray.empty())
      return;

    // "c" stands for child. It could be anything.
    const ImVec2 childSize(mW, mH);
    if (
      ImGui::BeginChild("c", childSize, ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground))
      ContainerPropertyControl::updateImgui();
    ImGui::EndChild();
  }
};

} // namespace PropPanel