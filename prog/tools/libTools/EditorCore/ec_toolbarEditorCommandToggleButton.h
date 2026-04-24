// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ec_toolbarEditorCommandButton.h"

#include <propPanel/control/toolbarToggleButton.h>

class ToolbarEditorCommandToggleButton : public PropPanel::ToolbarToggleButtonPropertyControl,
                                         public IEditorCommandKeyChordChangeEventHandler
{
public:
  ToolbarEditorCommandToggleButton(int id, const char *editor_command_id, PropPanel::ControlEventHandler *event_handler,
    PropPanel::ContainerPropertyControl *parent) :
    ToolbarToggleButtonPropertyControl(id, event_handler, parent, 0, 0, hdpi::Px(0), hdpi::Px(0), ""),
    editorCommandId(editor_command_id)
  {}

  void *queryInterfacePtr([[maybe_unused]] unsigned huid) override
  {
    if (huid == IEditorCommandKeyChordChangeEventHandler::HUID)
      return static_cast<IEditorCommandKeyChordChangeEventHandler *>(this);
    return ToolbarToggleButtonPropertyControl::queryInterfacePtr(huid);
  }

  void setTooltip(const char value[]) override
  {
    baseTooltip = value;
    onEditorCommandKeyChordChanged();
  }

  void setCaptionValue(const char value[]) override { setTooltip(value); }

  void onEditorCommandKeyChordChanged() override
  {
    ToolbarEditorCommandButton::formatToolbarButtonTooltip(controlTooltip, baseTooltip, editorCommandId);
  }

  void onWcRightClick(WindowBase *source) override { HotkeyEditorContextMenu::createToolbarContextMenu(editorCommandId); }

private:
  const SimpleString editorCommandId;
  SimpleString baseTooltip;
};
