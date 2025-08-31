// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/constants.h>
#include <propPanel/imguiHelper.h>

namespace PropPanel
{

class WindowControlEventHandler;

class SpinEditControlStandalone
{
public:
  SpinEditControlStandalone(float in_min, float in_max, float in_step, int in_precision);

  float getValue() const { return internalValue; }
  float getMinValue() const { return min; }
  float getMaxValue() const { return max; }
  float getStepValue() const { return step; }
  void setValue(float value);
  void setMinMaxStepValue(float in_min, float in_max, float in_step);
  void setPrecValue(int in_precision);

  bool isTextInputFocused() const { return textInputFocused; }
  void sendWcChangeIfVarChanged(WindowControlEventHandler &event_handler);

  // The size of the two spin buttons not including the space before it.
  static ImVec2 getSpinButtonsSize();

  // The gap between the edit box and the spin buttons.
  static int getSpaceBeforeSpinButtons() { return hdpi::_pxS(Constants::SPACE_BETWEEN_EDIT_BOX_AND_BUTTON_IN_COMBINED_CONTROL); }

  void updateImgui(WindowControlEventHandler &event_handler, const String *tooltip = nullptr, const void *tooltip_owner = nullptr);

private:
  enum class SpinnerButtonId
  {
    Nothing,
    Up,
    Down,
  };

  bool spinButton(SpinnerButtonId button, float &step_multiplier, const String *tooltip, const void *tooltip_owner);
  bool spinButtons(float &step_multiplier, const String *tooltip, const void *tooltip_owner);
  void setValueInternal(float value);
  float correctBounds(float value);
  void onTextChanged();

  String textValue;
  float internalValue, min, max, step;
  float externalValue; // The value, which is known externally (either sent through onWcChange() or set externally).
  int precision;
  bool textInputFocused = false;
  SpinnerButtonId draggedSpinnerButton = SpinnerButtonId::Nothing;
};

} // namespace PropPanel