// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spinEditStandalone.h"
#include "../tooltipHelper.h"
#include <propPanel/c_window_event_handler.h>
#include <propPanel/imguiHelper.h>
#include <imgui/imgui_internal.h>
#include <stdio.h> // snprintf

namespace PropPanel
{

SpinEditControlStandalone::SpinEditControlStandalone(float in_min, float in_max, float in_step, int in_precision) :
  min(in_min), max(in_max), step(in_step), precision(in_precision > 0 ? (in_precision - 1) : 0)
{
  setValueInternal(0);
  setMinMaxStepValue(in_min, in_max, in_step);
  externalValue = getValue();
}

void SpinEditControlStandalone::setValue(float value)
{
  if (textInputFocused) // don't allow to overwrite our value from external source while it's being edited
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
  event_handler.onWcChange(nullptr);
}

void SpinEditControlStandalone::onTextChanged()
{
  float value = atof(textValue);
  value = correctBounds(value);
  internalValue = value;
}

bool SpinEditControlStandalone::spinButton(SpinnerButtonId button, float &step_multiplier, const String *tooltip,
  const void *tooltip_owner)
{
  G_ASSERT(button == SpinnerButtonId::Up || button == SpinnerButtonId::Down);

  const char *buttonId = button == SpinnerButtonId::Up ? "u" : "d";
  const ImGuiDir dir = button == SpinnerButtonId::Up ? ImGuiDir_Up : ImGuiDir_Down;
  const ImVec2 buttonSize = getSpinButtonsSize();
  const float arrowSize = ImMax(1.0f, buttonSize.y - 4.0f); // Two pixel padding on both sides. Style.FramePadding.y is too much.
  const bool spinButtonPressed = ImguiHelper::arrowButtonExWithSize(buttonId, dir, buttonSize, ImVec2(arrowSize, arrowSize));

  if (ImGui::IsItemActive())
  {
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
  const bool textChanged = ImguiHelper::inputTextWithEnterWorkaround(inputLabel, &textValue, textInputWasFocused);
  textInputFocused = ImGui::IsItemFocused();

  if (tooltip && !tooltip->empty())
    tooltip_helper.setPreviousImguiControlTooltip(tooltip_owner, tooltip->begin(), tooltip->end());

  ImGui::SameLine(0.0f, spaceBeforeSpinButtons);

  float stepMultiplier;
  if (spinButtons(stepMultiplier, tooltip, tooltip_owner))
  {
    setValueInternal(getValue() + (step * stepMultiplier));
    sendWcChangeIfVarChanged(event_handler);
  }
  else if (textInputFocused && (ImGui::IsKeyPressed(ImGuiKey_Enter, ImGuiInputFlags_None, ImGuiKeyOwner_Any) ||
                                 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, ImGuiKeyOwner_Any)))
  {
    onTextChanged();
    sendWcChangeIfVarChanged(event_handler);
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
      inputState->ReloadUserBufAndMoveToEnd();

    sendWcChangeIfVarChanged(event_handler);
  }
  else
  {
    if (textChanged)
      onTextChanged();

    if (textInputWasFocused && !textInputFocused)
      sendWcChangeIfVarChanged(event_handler);
  }

  ImGui::PopID();
}

} // namespace PropPanel