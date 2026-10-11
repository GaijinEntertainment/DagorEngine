// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

namespace PropPanel
{

class ControlSizeHelper
{
public:
  // The width of the edit part of the controls that have their label on the left, like the spin edit. It is scaled by the font size.
  static float getDefaultRightSideEditWidth();

  // Preferred width of a text input that fills the available width. It follows the font size.
  static float getPreferredTextInputWidth();

  // The size of PropertyControlBase::separateLineLabelWithTooltip(), including the space below it. Zero for an empty
  // label.
  static Point2 getSeparateLineLabelPreferredSize(const char *label, const char *label_end);

  // The size of a separate line label above a row of spin_edit_count spin edits, like in the Point2 control.
  static Point2 getSpinEditRowPreferredSize(const char *label, const char *label_end, int spin_edit_count, float max_width_px);

  // The size of a separate line label above a slider and a spin edit, like in the TrackBarInt control.
  static Point2 getTrackBarPreferredSize(const char *label, const char *label_end, float max_width_px);

  // The size of a spin edit with its label on the left. With width_includes_label a set width is the fixed width,
  // without it a set width is the width of the edit part.
  static Point2 getSpinEditPreferredSize(bool width_includes_label, unsigned width, const char *label, const char *label_end,
    float max_width_px);

  // The text width of the longest item. Zero for no items.
  static float getLongestTextWidth(dag::ConstSpan<String> items);

  // The width of a list box that shows its longest item, with room for a vertical scrollbar.
  static float getListBoxPreferredWidth(dag::ConstSpan<String> items);

  // The height of a list box with control_height set as the control's height. Zero for Constants::LISTBOX_FULL_HEIGHT,
  // because then it fills the remaining height.
  static float getListBoxPreferredHeight(unsigned control_height);
};

} // namespace PropPanel
