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

  // Round committed values down to whole numbers so an integer control's displayed text matches its
  // floored getIntValue() (e.g. "10/3" shows 3, not 3.33). Set by the int spin edit and track bar.
  void setIntegerValues(bool yes) { integerValues = yes; }

  bool isTextInputFocused() const { return textInputFocused; }
  void sendWcChangeIfVarChanged(WindowControlEventHandler &event_handler);
  // Fires onWcChangeFinished if a value change has been sent (via onWcChange) since the last finish.
  // Used to commit once when an interaction ends -- e.g. on spinner release rather than every frame
  // the spin button is held.
  void sendWcChangeFinishedIfPending(WindowControlEventHandler &event_handler);
  // Convenience for onImmediateFocusLoss: evaluate the in-progress text, then send the resulting
  // value change (if any) and finish it, so an immediate focus loss (tree select / panel rebuild)
  // commits like Enter / focus loss. Without the commitText() the typed text is never parsed on this
  // path and the stale value would be committed; invalid text reverts to the last valid value.
  void sendWcChangeAndFinishIfVarChanged(WindowControlEventHandler &event_handler)
  {
    commitText(/*revert_display_on_error*/ true);
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
  // Evaluate the typed text as a math expression (see mathExprEval.h), store the result in
  // internalValue, and canonicalize the display to it. On an invalid expression the raw text is
  // kept (Enter) or the last valid value is restored (revert_display_on_error, focus loss).
  void commitText(bool revert_display_on_error);

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
  // Integer-domain control (int spin edit / track bar): committed values are floored so the displayed
  // text matches the floored getIntValue().
  bool integerValues = false;
  // Committed text is evaluated as a math expression (see mathExprEval.h). mathExprError marks the
  // last commit invalid, which shows the wrong-value background and the message as a tooltip.
  bool mathExprError = false;
  String mathExprErrorMsg;
  // ImGui time (s) of the last successful expression commit; drives the green fade-out flash.
  // Negative when inactive.
  double successHighlightTimeS = -1.0;
};

} // namespace PropPanel
