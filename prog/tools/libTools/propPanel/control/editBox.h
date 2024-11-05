// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"
#include <gui/dag_imguiUtil.h>
#include <imgui/imgui_internal.h>
#include <winGuiWrapper/wgw_input.h>

namespace PropPanel
{

class EditBoxPropertyControl : public PropertyControlBase
{
public:
  EditBoxPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[], bool multiline = false) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h), controlCaption(caption), controlMultiline(multiline)
  {}

  ~EditBoxPropertyControl()
  {
    // Prevent ImGui from re-applying changes to another control.
    // Yes, that actually could happen in Mission Editor when editing a variable then switching to another one. In this
    // case EditBoxPropertyControl gets re-created with the exact same address, so it gets the same ImGuiID, and it gets
    // the value of the previous control because of the re-apply on focus loss code in ImGui. So prevent that.
    preventReapplyingEditAtFocusLoss();
  }

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  virtual void setTextValue(const char value[]) override
  {
    if (strcmp(controlValue, value) == 0)
      return;

    controlValue = value;

    if (textInputFocused)
      focusedTextInputChangedByCode = true;

    preventReapplyingEditAtFocusLoss();
  }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setBoolValue(bool value) override { needColorIndicate = value; }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  virtual void reset() override { setTextValue(""); }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    const bool useIndicatorColor = needColorIndicate;

    // Override the background color of the edit box.
    if (useIndicatorColor)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 255, 0, 255));

    // Workaround needed to be able to set text of a focused text input.
    // See: https://github.com/ocornut/imgui/issues/7482
    const char *inputLabel = "##it";
    if (focusedTextInputChangedByCode)
    {
      focusedTextInputChangedByCode = false;
      if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(inputLabel)))
        inputState->ReloadUserBufAndMoveToEnd();
    }

    bool textChanged;
    if (controlMultiline)
    {
      // Use full width by default.
      const ImVec2 size(mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN, mH > 0 ? mH : 0.0f);

      textChanged = ImGuiDagor::InputTextMultiline(inputLabel, &controlValue, size);
    }
    else
    {
      // Use full width by default.
      ImGui::SetNextItemWidth(mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN);

      textChanged = ImguiHelper::inputTextWithEnterWorkaround(inputLabel, &controlValue, textInputFocused);
    }

    textInputFocused = ImGui::IsItemFocused();

    if (useIndicatorColor)
      ImGui::PopStyleColor();

    setPreviousImguiControlTooltip();

    if (textChanged)
      onWcChange(nullptr);

    if (textInputFocused && !controlMultiline)
    {
      // TODO: ImGui porting: hardcoding these three keys that are used by the TreeListWindow is really hacky. But
      // there is no generic keyboard event available at this point.
      if (ImGui::IsKeyPressed(ImGuiKey_Enter))
      {
        // Prevent Enter reaching the dialog because it would close it.
        ImGui::SetKeyOwner(ImGuiKey_Enter, ImGuiKeyOwner_Any, ImGuiInputFlags_LockUntilRelease);

        onWcKeyDown(nullptr, wingw::V_ENTER);
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
      {
        // Prevent navigation by the up and down keyboard buttons, we need that key.
        ImGui::GetCurrentContext()->ActiveIdUsingNavDirMask |= 1 << ImGuiDir_Up;
        ImGui::NavMoveRequestCancel();

        onWcKeyDown(nullptr, wingw::V_UP);
      }
      else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
      {
        // Prevent navigation by the up and down keyboard buttons, we need that key.
        ImGui::GetCurrentContext()->ActiveIdUsingNavDirMask |= 1 << ImGuiDir_Down;
        ImGui::NavMoveRequestCancel();

        onWcKeyDown(nullptr, wingw::V_DOWN);
      }
    }
  }

private:
  // ImGui does not like when the contents of an active (edited) edit box is changed from code the same frame when
  // the InputText loses focus.
  // See https://github.com/ocornut/imgui/issues/3878 for the issue.
  //
  // In case of focus loss if the input was edited ImGui::InputTextEx overwrites the text newly set from code from the
  // text from InputTextDeactivatedState if IsItemDeactivatedAfterEdit returns with true, and there is no official way
  // to prevent that. So to avoid that we prevent IsItemDeactivatedAfterEdit from returning true in this case.
  void preventReapplyingEditAtFocusLoss()
  {
    if (!textInputFocused)
      return;

    ImGuiContext *context = ImGui::GetCurrentContext();
    if (context)
    {
      // ImGui::IsItemDeactivatedAfterEdit() checks these two variables.
      context->ActiveIdPreviousFrameHasBeenEditedBefore = false;
      context->ActiveIdHasBeenEditedBefore = false;
    }
  }

  String controlCaption;
  String controlValue;
  bool controlEnabled = true;
  bool controlMultiline = false;
  bool needColorIndicate = false;
  bool textInputFocused = false;
  bool focusedTextInputChangedByCode = false;
};

} // namespace PropPanel