// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_editorCommandSystem.h>

#include "ec_editorCommand.h"
#include "ec_hotkeyEditorContextMenu.h"
#include "ec_mainMenu.h"
#include "ec_singleHotkeyEditor.h"
#include "ec_toolbarEditorCommandButton.h"
#include "ec_toolbarEditorCommandRadioButton.h"
#include "ec_toolbarEditorCommandToggleButton.h"

#include <propPanel/control/container.h>

bool ec_log_missing_editor_commands = false;

void EditorCommandSystem::addCommand(const char *id) { ec_editor_commands.addCommand(id); }

void EditorCommandSystem::addCommand(const char *id, ImGuiKeyChord key_chord) { ec_editor_commands.addCommand(id, key_chord); }

void EditorCommandSystem::addCommand(const char *id, ImGuiKeyChord key_chord1, ImGuiKeyChord key_chord2)
{
  ec_editor_commands.addCommand(id, key_chord1, key_chord2);
}

const char *EditorCommandSystem::getCommandKeyChordsAsText(const char *id)
{
  const EditorCommand *command = ec_editor_commands.getCommand(id);
  return command ? command->getKeyChordsAsText() : nullptr;
}

unsigned int EditorCommandSystem::getCommandCount() const { return ec_editor_commands.getCommandCount(); }

const char *EditorCommandSystem::getCommandId(int index) const { return ec_editor_commands.getCommandId(index); }

unsigned int EditorCommandSystem::getCommandCmdId(const char *id)
{
  const EditorCommand *command = ec_editor_commands.getCommand(id);
  return command != nullptr ? command->getCmdId() : 0;
}

void EditorCommandSystem::loadChangedHotkeys(const DataBlock &blk) { ec_editor_commands.loadChangedHotkeys(blk); }

void EditorCommandSystem::saveChangedHotkeys(DataBlock &blk) const { ec_editor_commands.saveChangedHotkeys(blk); }

void EditorCommandSystem::createToolbarButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
  const char *tooltip)
{
  ToolbarEditorCommandButton *button = new ToolbarEditorCommandButton(id, editor_command_id, parent.getEventHandler(), &parent);
  button->setTooltip(tooltip);
  parent.addControl(button);
}

void EditorCommandSystem::createToolbarRadioButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
  const char *tooltip)
{
  ToolbarEditorCommandRadioButton *button =
    new ToolbarEditorCommandRadioButton(id, editor_command_id, parent.getEventHandler(), &parent);
  button->setTooltip(tooltip);
  parent.addControl(button);
}

void EditorCommandSystem::createToolbarToggleButton(PropPanel::ContainerPropertyControl &parent, int id, const char *editor_command_id,
  const char *tooltip)
{
  ToolbarEditorCommandToggleButton *button =
    new ToolbarEditorCommandToggleButton(id, editor_command_id, parent.getEventHandler(), &parent);
  button->setTooltip(tooltip);
  parent.addControl(button);
}

void EditorCommandSystem::updateToolbarButtons(PropPanel::ContainerPropertyControl &container)
{
  const int childCount = container.getChildCount();
  for (int i = 0; i < childCount; ++i)
  {
    PropPanel::PropertyControlBase *control = container.getByIndex(i);
    if (!control)
      continue;

    if (PropPanel::ContainerPropertyControl *childContainer = control->getContainer())
      updateToolbarButtons(*childContainer);
    else if (IEditorCommandKeyChordChangeEventHandler *button = control->queryInterface<IEditorCommandKeyChordChangeEventHandler>())
      button->onEditorCommandKeyChordChanged();
  }
}

void EditorCommandSystem::addMenuItem(PropPanel::IMenu &menu, unsigned menu_id, unsigned item_id, const char *editor_command_id,
  const char *title)
{
  IEditorCoreMenu *ecMenu = menu.queryInterface<IEditorCoreMenu>();
  G_ASSERT(ecMenu);
  ecMenu->addEditorCommandItem(menu_id, item_id, editor_command_id, title);
}

void EditorCommandSystem::updateMenuItemShortcuts(PropPanel::IMenu &menu)
{
  IEditorCoreMenu *ecMenu = menu.queryInterface<IEditorCoreMenu>();
  G_ASSERT(ecMenu);
  ecMenu->updateEditorCommandShortcuts();
}

void EditorCommandSystem::enableMissingCommandLogging() { ec_log_missing_editor_commands = true; }

void EditorCommandSystem::updateImgui()
{
  HotkeyEditorContextMenu::updateImguiToolbarContextMenu();
  ec_update_imgui_single_hotkey_editor();
}
