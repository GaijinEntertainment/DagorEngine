// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../tooltipHelper.h"
#include <propPanel/control/spinEditStandalone.h>
#include <propPanel/mathExprEval.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/colors.h>
#include <propPanel/toastManager.h>
#include <imgui/imgui_internal.h>
#include <stdio.h> // snprintf

namespace PropPanel
{

namespace
{
// How long the green highlight flashed after a successful expression commit takes to fade out.
constexpr float SUCCESS_HIGHLIGHT_FADE_SECONDS = 1.5f;
} // namespace

SpinEditControlStandalone::SpinEditControlStandalone(float in_min, float in_max, float in_step, int in_precision) :
  min(in_min), max(in_max), step(in_step), precision(in_precision > 0 ? (in_precision - 1) : 0)
{
  setValueInternal(0);
  setMinMaxStepValue(in_min, in_max, in_step);
  externalValue = getValue();
}

void SpinEditControlStandalone::setValue(float value)
{
  if (textInputFocused && textInputActive) // don't allow to overwrite our value from external source while it's being edited
    return;

  setValueInternal(value);
  externalValue = value;
}

void SpinEditControlStandalone::setMinMaxStepValue(float in_min, float in_max, float in_step)
{
  min = in_min;
  max = in_max;
  step = in_step;

  if (min == max)
    step = pow(0.1, precision);
  else
  {
    int p = ceil(log10(1 / step));
    precision = p > 0 ? p : 0;
  }

  setValueInternal(getValue());
}

void SpinEditControlStandalone::setPrecValue(int in_precision)
{
  precision = in_precision - 1;
  step = pow(0.1, precision);
  setValueInternal(getValue());
}

void SpinEditControlStandalone::setValueInternal(float value)
{
  value = correctBounds(value);
  if (integerValues)
  {
    value = floorf(value);
  }

  char buf[64];
  float minValue = precision > 0 ? powf(10.0f, -precision) : 1.0f;
  int printPrecision = precision;
  if (fabs(value) > 1e-16)
    while (printPrecision < 10 && fabs(value) < minValue * 100.0f)
    {
      ++printPrecision;
      minValue *= 0.1f;
    }

  snprintf(buf, sizeof(buf), "%.*f", printPrecision, value);

  char *p = buf + strlen(buf) - 1;
  if (strchr(buf, '.'))
  {
    while (p > buf && *p == '0')
    {
      *p = '\0';
      p--;
    }
    if (p > buf && *p == '.')
      *p = '\0';
  }

  textValue = buf;
  internalValue = value;
  mathExprError = false;
  successHighlightTimeS = -1.0; // a fresh value (spinner, arrow, external set) cancels the success flash
}

float SpinEditControlStandalone::correctBounds(float value)
{
  if (min != max)
  {
    value = value < min ? min : value;
    value = value > max ? max : value;
  }

  return value;
}

void SpinEditControlStandalone::sendWcChangeIfVarChanged(WindowControlEventHandler &event_handler)
{
  if (externalValue == getValue())
    return;

  externalValue = getValue();
  changePendingFinish = true;
  event_handler.onWcChange(nullptr);
}

void SpinEditControlStandalone::sendWcChangeFinishedIfPending(WindowControlEventHandler &event_handler)
{
  if (!changePendingFinish)
    return;

  changePendingFinish = false;
  event_handler.onWcChangeFinished(nullptr);
}

void SpinEditControlStandalone::commitText(bool revert_display_on_error)
{
  const MathExprResult r = eval_math_expr(textValue.str());
  if (r.ok)
  {
    const String rawInput = textValue;                // what the user typed, before canonicalization
    setValueInternal(r.value);                        // textValue becomes the evaluated, clamped result
    if (strcmp(rawInput.str(), textValue.str()) != 0) // commit rewrote the text (expression evaluated or literal reformatted)
      successHighlightTimeS = ImGui::GetTime();
    return;
  }

  // Invalid expression: alert the user with the reason, on Enter or focus loss alike. Skip if the
  // previous commit already failed (mathExprError still set) so holding Enter does not stack toasts.
  // set_toast_message is called directly because this control is inside propPanel; DAEDITOR3.showToast
  // is only the plugin-DLL route to the same toast.
  if (!mathExprError)
  {
    ToastMessage toast;
    toast.type = ToastType::Alert;
    toast.text = r.error.str();
    toast.setHideOnMouseMove(true);
    toast.setToMousePos(Point2(0.0f, 0.0f)); // top-left corner at the cursor
    set_toast_message(toast);
  }

  if (revert_display_on_error)
    setValueInternal(internalValue); // focus loss: restore the last valid value (clears error)
  else
  {
    mathExprError = true; // Enter: keep the raw text plus the red frame and tooltip
    mathExprErrorMsg = r.error;
    successHighlightTimeS = -1.0; // a fresh error cancels any in-progress success flash
  }
}

bool SpinEditControlStandalone::spinButton(SpinnerButtonId button, float &step_multiplier, const String *tooltip,
  const void *tooltip_owner)
{
  G_ASSERT(button == SpinnerButtonId::Up || button == SpinnerButtonId::Down);

  const char *buttonId = button == SpinnerButtonId::Up ? "u" : "d";
  const ImGuiDir dir = button == SpinnerButtonId::Up ? ImGuiDir_Up : ImGuiDir_Down;
  const ImVec2 buttonSize = getSpinButtonsSize();
  const float arrowSize = ImMax(1.0f, buttonSize.y - 4.0f); // Two pixel padding on both sides. Style.FramePadding.y is too much.
  ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
  const bool spinButtonPressed = ImguiHelper::arrowButtonExWithSize(buttonId, dir, buttonSize, ImVec2(arrowSize, arrowSize));
  ImGui::PopItemFlag();

  if (ImGui::IsItemActive())
  {
    spinnerButtonActive = true;

    if (draggedSpinnerButton == button)
    {
      const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
      if (fabs(delta.y) < 1.0f)
        return false;

      step_multiplier = -delta.y;
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
      return true;
    }
    else if (ImGui::IsItemHovered())
    {
      draggedSpinnerButton = SpinnerButtonId::Nothing;
    }
    else
    {
      draggedSpinnerButton = button;
      ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
      return false;
    }
  }
  else if (draggedSpinnerButton == button)
  {
    draggedSpinnerButton = SpinnerButtonId::Nothing;
  }

  if (tooltip && !tooltip->empty())
    tooltip_helper.setPreviousImguiControlTooltip(tooltip_owner, tooltip->begin(), tooltip->end());

  step_multiplier = button == SpinnerButtonId::Up ? 1.0f : -1.0f;
  return spinButtonPressed;
}

ImVec2 SpinEditControlStandalone::getSpinButtonsSize()
{
  const float frameHeight = ImGui::GetFrameHeight();
  return ImVec2(floorf(frameHeight), floorf(frameHeight * 0.5f));
}

bool SpinEditControlStandalone::spinButtons(float &step_multiplier, const String *tooltip, const void *tooltip_owner)
{
  spinnerButtonActive = false;

  ImGui::BeginGroup();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
  ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);

  float downStepMultiplier;
  const bool upSpinnerPressed = spinButton(SpinnerButtonId::Up, step_multiplier, tooltip, tooltip_owner);
  const bool downSpinnerPressed = spinButton(SpinnerButtonId::Down, downStepMultiplier, tooltip, tooltip_owner);

  ImGui::PopItemFlag();
  ImGui::PopStyleVar();
  ImGui::EndGroup();

  if (downSpinnerPressed)
    step_multiplier = downStepMultiplier;

  return upSpinnerPressed || downSpinnerPressed;
}

void SpinEditControlStandalone::updateImgui(WindowControlEventHandler &event_handler, const String *tooltip, const void *tooltip_owner)
{
  ImGui::PushID(this);

  const float spaceBeforeSpinButtons = getSpaceBeforeSpinButtons();
  const float editBoxWidth = ImGui::CalcItemWidth() - spaceBeforeSpinButtons - getSpinButtonsSize().x;
  ImGui::SetNextItemWidth(editBoxWidth);

  const char *inputLabel = "";
  const bool textInputWasFocused = textInputFocused;
  const bool textInputWasActive = textInputActive;

  // Frame background, inner to any color the owner control set. An invalid commit shows the
  // wrong-value background until the next edit; otherwise a successful expression commit flashes the
  // evaluated-value background and fades it out. Error takes priority over the flash.
  float successAlpha = 0.0f;
  if (successHighlightTimeS >= 0.0)
  {
    const float elapsed = ImGui::GetTime() - successHighlightTimeS;
    if (elapsed >= 0.0 && elapsed < SUCCESS_HIGHLIGHT_FADE_SECONDS)
      successAlpha = 1.0f - static_cast<float>(elapsed / SUCCESS_HIGHLIGHT_FADE_SECONDS);
    else
      successHighlightTimeS = -1.0;
  }

  const bool pushFrameBg = mathExprError || successAlpha > 0.0f;
  if (mathExprError)
    ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(ColorOverride::EDIT_BOX_WRONG_VALUE_BACKGROUND));
  else if (successAlpha > 0.0f)
  {
    ImVec4 successColor = getOverriddenColor(ColorOverride::EDIT_BOX_EVALUATED_VALUE_BACKGROUND);
    successColor.w *= successAlpha;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, successColor);
  }

  const bool textChanged = ImguiHelper::inputTextWithEnterWorkaround(inputLabel, nullptr, &textValue, textInputWasFocused);
  textInputFocused = ImGui::IsItemFocused();
  textInputActive = ImGui::IsItemActive();

  if (pushFrameBg)
    ImGui::PopStyleColor();

  static const String mathExprHelp("Math expression supported.\n"
                                   "Operators: + - * / ^ ** ( )    Constant: pi\n"
                                   "Functions: sqrt abs sin cos tan atan pow   (trig in degrees)\n"
                                   "Example: (100/2)^2  ->  2500");
  const String *effectiveTooltip = mathExprError ? &mathExprErrorMsg : (tooltip && !tooltip->empty()) ? tooltip : &mathExprHelp;
  if (!effectiveTooltip->empty())
    tooltip_helper.setPreviousImguiControlTooltip(tooltip_owner, effectiveTooltip->begin(), effectiveTooltip->end());

  ImGui::SameLine(0.0f, spaceBeforeSpinButtons);

  const bool spinnerWasActive = spinnerButtonActive;
  float stepMultiplier;
  if (spinButtons(stepMultiplier, tooltip, tooltip_owner))
  {
    setValueInternal(getValue() + (step * stepMultiplier));
    sendWcChangeIfVarChanged(event_handler);
  }
  else if (textInputFocused && (ImGui::IsKeyPressed(ImGuiKey_Enter, ImGuiInputFlags_None, ImGuiKeyOwner_Any) ||
                                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, ImGuiKeyOwner_Any)))
  {
    commitText(/*revert_display_on_error*/ false);

    // On a valid expression the text was rewritten to the evaluated value; refresh the
    // still-focused input so the box shows the result rather than the typed expression.
    if (!mathExprError)
    {
      if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(inputLabel)))
      {
        inputState->ReloadUserBufAndMoveToEnd();
        // The reload updates the buffer and cursor but not the horizontal scroll, which a long typed
        // expression has pushed past the short result; reset it so the result stays in view.
        inputState->Scroll.x = 0.0f;
      }
    }

    sendWcChangeIfVarChanged(event_handler);
    sendWcChangeFinishedIfPending(event_handler);
  }
  else if (textInputFocused && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)))
  {
    // Prevent navigation by the up and down keyboard buttons, we need those keys.
    ImGui::GetCurrentContext()->ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
    ImGui::NavMoveRequestCancel();

    const float stepToAdd = ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? step : -step;
    setValueInternal(getValue() + stepToAdd);

    // Workaround needed to be able to set text of a focused text input.
    // See: https://github.com/ocornut/imgui/issues/7482
    if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(inputLabel)))
    {
      inputState->ReloadUserBufAndMoveToEnd();
      // The reload puts the cursor at the end but does not scroll to it. A spin step changes the
      // trailing digits, so let the scroll follow the cursor to keep the last digit in view.
      inputState->CursorFollow = true;
    }

    sendWcChangeIfVarChanged(event_handler);
    sendWcChangeFinishedIfPending(event_handler);
  }
  else
  {
    const bool lostFocus = (textInputWasFocused && !textInputFocused) || (textInputWasActive && !textInputActive);

    // Evaluate on commit only. While typing, keep the raw text and just drop any stale error
    // highlight; on focus loss evaluate and revert the display if the expression is invalid.
    if (lostFocus)
      commitText(/*revert_display_on_error*/ true);
    else if (textChanged)
      mathExprError = false;

    if (lostFocus)
    {
      sendWcChangeIfVarChanged(event_handler);
      sendWcChangeFinishedIfPending(event_handler);
    }
  }

  // A spin button that was held/dragged and is now released commits the accumulated change once.
  if (spinnerWasActive && !spinnerButtonActive)
    sendWcChangeFinishedIfPending(event_handler);

  ImGui::PopID();
}

} // namespace PropPanel