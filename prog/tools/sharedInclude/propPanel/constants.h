//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class Constants
{
public:
  // The default vertical space between controls in the container.
  static constexpr int DEFAULT_CONTROLS_INTERVAL = 7;

  // If the track bar's slider width would be smaller than this then only the spin edit control is shown.
  static constexpr int MINIMUM_TRACK_BAR_SLIDER_WIDTH_TO_SHOW = 20;

  // The space between edit box and the "..." button in the FileEditBoxPropertyControl.
  static constexpr int SPACE_BETWEEN_EDIT_BOX_AND_BUTTON_IN_COMBINED_CONTROL = 0.0f;

  // The space between the picker button and the clear button in the FileButtonPropertyControl.
  static constexpr int SPACE_BETWEEN_BUTTONS_IN_COMBINED_CONTROL = 2.0f;

  // Vertical space between a label and a control when they are on a separate line.
  static constexpr int SPACE_BETWEEN_SEPARATE_LINE_LABEL_AND_CONTROL = 0.0f;

  // The default height can fit ~7 items.
  static constexpr hdpi::Px LISTBOX_DEFAULT_HEIGHT = (hdpi::Px)0;

  // Use all the available height.
  static constexpr hdpi::Px LISTBOX_FULL_HEIGHT = (hdpi::Px)1;

  // The inner window padding of toolbars.
  static constexpr ImVec2 TOOLBAR_WINDOW_PADDING = ImVec2(0.0f, 5.0f);
};

} // namespace PropPanel