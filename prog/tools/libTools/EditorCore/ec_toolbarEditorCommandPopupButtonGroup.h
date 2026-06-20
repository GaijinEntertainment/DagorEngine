// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_editorCommandSystem.h>
#include <propPanel/control/container.h>

class ToolbarEditorCommandToggleButton;

class ToolbarEditorCommandPopupButtonGroup : public PropPanel::ContainerPropertyControl
{
public:
  ToolbarEditorCommandPopupButtonGroup(int id, PropPanel::ControlEventHandler *event_handler, ContainerPropertyControl *parent);

  bool isRealContainer() override { return false; }
  void addControl(PropPanel::PropertyControlBase *pcontrol, bool new_line) override;
  void updateImgui() override;

private:
  void updateImguiPopupContents(ToolbarEditorCommandToggleButton &active_button);
  void updateImguiPopupButtonGroup(ToolbarEditorCommandToggleButton &active_button, ImGuiID button_id, bool button_active);

  static const char *getCommandDisplayName(const char *command_id);

  int activeButtonId = 0;
  bool popupOpen = false;
};
