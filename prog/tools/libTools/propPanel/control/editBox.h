// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <propPanel/imguiHelper.h>
#include "../c_constants.h"
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
    hdpi::Px h, const char caption[], bool multiline = false, bool auto_height = false) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h),
    controlCaption(caption),
    controlMultiline(multiline),
    autoHeight(auto_height)
  {
    if (autoHeight && controlMultiline)
      recalcAutoHeight();
  }

  ~EditBoxPropertyControl() override
  {
    // Prevent ImGui from re-applying changes to another control.
    // Yes, that actually could happen in Mission Editor when editing a variable then switching to another one. In this
    // case EditBoxPropertyControl gets re-created with the exact same address, so it gets the same ImGuiID, and it gets
    // the value of the previous control because of the re-apply on focus loss code in ImGui. So prevent that.
    preventReapplyingEditAtFocusLoss();
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  void setTextValue(const char value[]) override
  {
    if (strcmp(controlValue, value) == 0)
      return;

    controlValue = value;

    if (autoHeight && controlMultiline)
      recalcAutoHeight();

    if (textInputFocused)
      focusedTextInputChangedByCode = true;

    preventReapplyingEditAtFocusLoss();
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setBoolValue(bool value) override { needColorIndicate = value; }

  void setDragSourceHandlerValue(IDragSourceHandler *handler) override { controlDragHandler = handler; }

  void setDropTargetHandler(IDropTargetHandler *handler) override { controlDropTargetHandler = handler; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void setShowTooltipAlways(bool show) override { showTooltipAlways = show; }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  void reset() override { setTextValue(""); }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void updateImgui() override
  {
    const float controlScreenTopY = ImGui::GetCursorScreenPos().y;
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());

    // Drag source is anchored to the caption label (rendered just above), not to the text input.
    // This avoids conflict with text selection inside the input field.
    if (controlDragHandler && !controlCaption.empty())
    {
      if (ImGui::BeginDragDropSource(controlDragHandler->getDragDropFlags() | ImGuiDragDropFlags_SourceAllowNullID))
      {
        controlDragHandler->onBeginDrag(mId);
        ImGui::EndDragDropSource();
      }
    }

    setFocusToNextImGuiControlIfRequested();

    const bool useValueHighlight = valueHighlightColor != ColorOverride::NONE;
    const bool useIndicatorColor = !useValueHighlight && needColorIndicate;

    // Override the background color of the edit box.
    if (useValueHighlight)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));
    else if (useIndicatorColor)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(ColorOverride::EDIT_BOX_INDICATOR_BACKGROUND));

    // Workaround needed to be able to set text of a focused text input.
    // See: https://github.com/ocornut/imgui/issues/7482
    const char *inputLabel = "##it";
    if (focusedTextInputChangedByCode)
    {
      focusedTextInputChangedByCode = false;
      if (ImGuiInputTextState *inputState = ImGui::GetInputTextState(ImGui::GetID(inputLabel)))
        inputState->ReloadUserBufAndMoveToEnd();
    }

    // NOTE: if you add an option to this that will require the changes to be confirmed (for example with the Enter key)
    // and the changes also will be saved on focus loss then you will have to use IImmediateFocusLossHandler.
    bool textChanged;
    if (controlMultiline)
    {
      if (autoHeight && textInputFocused)
      {
        // ImGui appends text to controlValue inside InputTextMultiline, so recalcAutoHeight() after the call
        // will always be one frame late. Pre-expand height before the call to avoid a one-frame scrollbar flash.
        if (ImGui::IsKeyPressed(ImGuiKey_Enter))
          recalcAutoHeight(/*extra_lines*/ 1);
        else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_V))
        {
          // For paste, read the clipboard to know how many lines will be added.
          // recalcAutoHeight() after the call will correct the exact value (e.g. if pasted text replaced a selection).
          int pastedLines = 0;
          if (const char *clipboard = ImGui::GetClipboardText())
            for (const char *p = clipboard; *p; ++p)
              if (*p == '\n')
                ++pastedLines;
          if (pastedLines > 0)
            recalcAutoHeight(/*extra_lines*/ pastedLines);
        }
      }

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

    if (useValueHighlight || useIndicatorColor)
      ImGui::PopStyleColor();

    if (showTooltipAlways)
    {
      const char *tooltip = getTooltip();
      if (tooltip && *tooltip)
      {
        // Anchor the hint above the input field
        // Use a regular window, not tooltip layer for avoid render on top of other windows
        const ImVec2 fieldTopLeft = ImGui::GetItemRectMin();
        const float displayWidth = ImGui::GetIO().DisplaySize.x;
        const float hintX = ImClamp(fieldTopLeft.x, 0.0f, ImMax(0.0f, displayWidth - hintWindowSize.x));
        ImGui::SetNextWindowPos(ImVec2(hintX, fieldTopLeft.y), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(ImGui::GetStyle().Colors[ImGuiCol_PopupBg].w);
        const ImGuiWindowFlags hintFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing;
        const String windowId(0, "##editbox_hint_%d", mId);
        if (ImGui::Begin(windowId.c_str(), nullptr, hintFlags))
        {
          ImGui::TextUnformatted(tooltip);
          hintWindowSize = ImGui::GetWindowSize();
        }
        ImGui::End();
      }
    }
    else
      setPreviousImguiControlTooltip();

    if (textChanged)
    {
      if (autoHeight && controlMultiline)
        recalcAutoHeight();
      onWcChange(nullptr);
    }

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

    if (controlDropTargetHandler)
    {
      const ImGuiID dropTargetId = ImGui::GetItemID();
      const float halfSpacing =
        (mParent ? (float)mParent->getVerticalSpaceBetweenControls() : hdpi::_pxS(Constants::DEFAULT_CONTROLS_INTERVAL)) * 0.5f;
      const ImVec2 rectMin(ImGui::GetItemRectMin().x, controlScreenTopY - halfSpacing);
      const ImVec2 rectMax(ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y + halfSpacing);
      if (dropTargetId != 0 && ImGui::BeginDragDropTargetCustom(ImRect(rectMin, rectMax), dropTargetId))
      {
        const DragAndDropResult result = controlDropTargetHandler->handleDropTarget(mId, mParent);
        handleDragAndDropNotAllowed(result);
        ImGui::EndDragDropTarget();
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
      // ImGui::IsItemDeactivatedAfterEdit() checks this variable.
      context->DeactivatedItemData.HasBeenEditedBefore = false;
    }
  }

  void recalcAutoHeight(int extra_lines = 0)
  {
    int lineCount = 1;
    for (char c : controlValue)
      if (c == '\n')
        ++lineCount;
    int height = (lineCount + extra_lines) * ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
    setHeight(hdpi::_pxActual(height));
  }

  IDragSourceHandler *controlDragHandler = nullptr;
  IDropTargetHandler *controlDropTargetHandler = nullptr;
  String controlCaption;
  String controlValue;
  bool controlEnabled = true;
  bool controlMultiline = false;
  bool needColorIndicate = false;
  bool textInputFocused = false;
  bool focusedTextInputChangedByCode = false;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
  bool showTooltipAlways = false;
  bool autoHeight = false;
  ImVec2 hintWindowSize = ImVec2(0.0f, 0.0f);
};

} // namespace PropPanel