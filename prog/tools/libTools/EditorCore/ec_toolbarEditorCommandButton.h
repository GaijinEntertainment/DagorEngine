// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ec_editorCommand.h"
#include "ec_hotkeyEditorContextMenu.h"

#include <EditorCore/ec_editorCommandSystem.h>
#include <propPanel/control/toolbarButton.h>

class ToolbarEditorCommandButton : public PropPanel::ToolbarButtonPropertyControl, public IEditorCommandKeyChordChangeEventHandler
{
public:
  ToolbarEditorCommandButton(int id, const char *editor_command_id, PropPanel::ControlEventHandler *event_handler,
    PropPanel::ContainerPropertyControl *parent) :
    ToolbarButtonPropertyControl(id, event_handler, parent, 0, 0, hdpi::Px(0), hdpi::Px(0), ""), editorCommandId(editor_command_id)
  {}

  void *queryInterfacePtr([[maybe_unused]] unsigned huid) override
  {
    if (huid == IEditorCommandKeyChordChangeEventHandler::HUID)
      return static_cast<IEditorCommandKeyChordChangeEventHandler *>(this);
    return ToolbarButtonPropertyControl::queryInterfacePtr(huid);
  }

  void setTooltip(const char value[]) override
  {
    baseTooltip = value;
    onEditorCommandKeyChordChanged();
  }

  void setCaptionValue(const char value[]) override { setTooltip(value); }

  void onEditorCommandKeyChordChanged() override { formatToolbarButtonTooltip(controlTooltip, baseTooltip, editorCommandId); }

  void onWcRightClick(WindowBase *source) override { HotkeyEditorContextMenu::createToolbarContextMenu(editorCommandId); }

  static void formatToolbarButtonTooltip(String &tooltip, const char *description, const char *editor_command_id)
  {
    const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);

    // Unfortunately in Asset Viewer the command registration runs later, so this is needed to avoid false errors.
    extern bool ec_log_missing_editor_commands;
    if (!command && ec_log_missing_editor_commands)
      logerr("Editor command \"%s\" cannot be found. Cannot update the toolbar button's tooltip.", editor_command_id);

    if (command && command->getHotkeyCount() > 0)
      tooltip.printf(0, "%s\n\nShortcut: %s", description, command->getKeyChordsAsText());
    else
      tooltip = description;
  }

private:
  const SimpleString editorCommandId;
  SimpleString baseTooltip;
};
