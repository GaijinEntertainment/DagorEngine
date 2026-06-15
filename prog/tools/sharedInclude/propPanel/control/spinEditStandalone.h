//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
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
  // Fires onWcChangeFinished if a value change has been sent (via onWcChange) since the last finish.
  // Used to commit once when an interaction ends -- e.g. on spinner release rather than every frame
  // the spin button is held.
  void sendWcChangeFinishedIfPending(WindowControlEventHandler &event_handler);
  // Convenience for onImmediateFocusLoss: send the pending value change (if any) and immediately
  // finish it, so an immediate focus loss (tree select / panel rebuild) commits like Enter / focus loss.
  void sendWcChangeAndFinishIfVarChanged(WindowControlEventHandler &event_handler)
  {
    sendWcChangeIfVarChanged(event_handler);
    sendWcChangeFinishedIfPending(event_handler);
  }

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
  bool textInputActive = false;
  // True while a spin button is held (click-repeat or drag); used to fire onWcChangeFinished on release.
  bool spinnerButtonActive = false;
  // True once onWcChange has been sent and not yet finished; gates sendWcChangeFinishedIfPending.
  bool changePendingFinish = false;
  SpinnerButtonId draggedSpinnerButton = SpinnerButtonId::Nothing;
};

} // namespace PropPanel
