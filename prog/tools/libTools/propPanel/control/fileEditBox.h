// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/constants.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"
#include <gui/dag_imguiUtil.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class FileEditBoxPropertyControl : public PropertyControlBase
{
public:
  FileEditBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), masks("All|*.*||")
  {}

  virtual unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION | CONTROL_DATA_TYPE_USER | CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT;
  }

  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  virtual void setTextValue(const char value[]) override
  {
    const bool clearActiveId = textInputActive && strcmp(value, controlValue) != 0;

    controlValue = value;

    // ImGui does not like when the contents of an active (edited) edit box is changed from code.
    // See https://github.com/ocornut/imgui/issues/3878 for the issue.
    //
    // So this code is here to prevent an infinite change loop.
    //
    // Original reproduction steps:
    // - the user selects a file name with extension
    // - clicks in the edit box and types in something
    // - in onChange the application removes the extension and sets the new file name
    // - ImGui stores the original file name for the active edit box and thus sends onWcChange again
    //
    // TODO: ImGui porting: try better (?) workaround: https://github.com/ocornut/imgui/issues/7482
    if (clearActiveId)
    {
      textInputActive = false;
      ImGui::ClearActiveID();
    }
  }

  virtual void setBoolValue(bool value) override { setIntValue(value ? FS_DIALOG_DIRECTORY : FS_DIALOG_OPEN_FILE); }

  virtual void setIntValue(int value) override
  {
    if (value == FS_DIALOG_NONE || value == FS_DIALOG_OPEN_FILE || value == FS_DIALOG_SAVE_FILE || value == FS_DIALOG_DIRECTORY)
      dialogMode = value;
    else
      G_ASSERT_LOG(false, "Invalid dialog mode: %d.", value);
  }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setStringsValue(const Tab<String> &vals) override
  {
    masks.clear();

    for (int i = 0; i < vals.size(); ++i)
    {
      masks += vals[i];
      masks += "|";
    }

    masks += "|";
  }

  virtual void setUserDataValue(const void *value) override
  {
    if (value)
      basePath = (const char *)value;
  }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  virtual void reset() override
  {
    controlValue.clear();

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    const float buttonWidth = ImGui::GetFrameHeight(); // Simply use a square button.
    const float spaceBetweenControls = hdpi::_pxS(Constants::SPACE_BETWEEN_EDIT_BOX_AND_BUTTON_IN_COMBINED_CONTROL);
    const float editBoxWidth = ImGui::GetContentRegionAvail().x - buttonWidth - spaceBetweenControls;
    ImGui::SetNextItemWidth(editBoxWidth);

    const bool textChanged = ImGuiDagor::InputText("##it", &controlValue);
    textInputActive = ImGui::IsItemActive();
    setPreviousImguiControlTooltip();

    if (textChanged)
      onWcChange(nullptr);

    ImGui::SameLine(0.0f, spaceBetweenControls);
    const bool clickedOnPickButton = ImGui::Button("...", ImVec2(buttonWidth, 0.0f));
    setPreviousImguiControlTooltip();

    if (clickedOnPickButton)
      onWcClick(nullptr);
  }

private:
  virtual void onWcClick(WindowBase *source) override
  {
    if (dialogMode == FS_DIALOG_NONE)
    {
      PropertyControlBase::onWcClick(source);
      onWcChange(nullptr);
      return;
    }

    onWcChanging(source); // for change paths if it needs

    char fn[512] = "\0";
    ImguiHelper::getTextValueForString(controlValue, fn, sizeof(fn));

    SimpleString fname, full_fn, result;
    char floc[260];

    if (!basePath.empty())
    {
      full_fn = ::make_full_path(basePath.str(), fn);
      fname = dd_get_fname(full_fn.str());
      dd_get_fname_location(floc, full_fn.str());
    }
    else
    {
      full_fn = fn;
      fname = dd_get_fname(fn);
      dd_get_fname_location(floc, fn);
    }

    switch (dialogMode)
    {
      case FS_DIALOG_OPEN_FILE:
        result = wingw::file_open_dlg(getRootParent()->getWindowHandle(), "Open file...", masks, "", floc, fname);
        break;

      case FS_DIALOG_SAVE_FILE:
        result = wingw::file_save_dlg(getRootParent()->getWindowHandle(), "Save file as...", masks, "", floc, fname);
        break;

      case FS_DIALOG_DIRECTORY:
        result = wingw::dir_select_dlg(getRootParent()->getWindowHandle(), "Open folder...", full_fn.str());
        break;
    }

    if (!result.empty())
    {
      if (!basePath.empty())
      {
        const String rel_fn = ::make_path_relative(result.str(), basePath.str());
        result = rel_fn;
      }

      controlValue = result;
      onWcChange(nullptr);
    }
  }

  String controlCaption;
  String controlValue;
  bool controlEnabled = true;
  int dialogMode = FS_DIALOG_OPEN_FILE;
  String basePath;
  String masks;
  bool textInputActive = false;
};

} // namespace PropPanel