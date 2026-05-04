// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_mainMenu.h"

#include "ec_editorCommand.h"
#include "ec_hotkeyEditorContextMenu.h"
#include <EditorCore/ec_menu.h>

#include <propPanel/imguiHelper.h>

EditorCoreMenuCommon::EditorCommandMenuItem::EditorCommandMenuItem(unsigned item_id, MenuItemType item_type,
  const char *editor_command_id) :
  MenuItem(item_id, item_type), editorCommandId(editor_command_id)
{}

void *EditorCoreMenuCommon::EditorCommandMenuItem::queryInterfacePtr([[maybe_unused]] unsigned huid)
{
  if (huid == IEditorCommandKeyChordChangeEventHandler::HUID)
    return static_cast<IEditorCommandKeyChordChangeEventHandler *>(this);
  return MenuItem::queryInterfacePtr(huid);
}

void EditorCoreMenuCommon::EditorCommandMenuItem::onEditorCommandKeyChordChanged()
{
  const EditorCommand *command = ec_editor_commands.getCommand(editorCommandId);
  G_ASSERT(command);
  shortcut = command->getKeyChordsAsText();
}

bool EditorCoreMenuCommon::EditorCommandMenuItem::updateImguiButton(bool is_checked, bool is_bullet)
{
  const bool hotkeyEditorContextMenuOpen = HotkeyEditorContextMenu::isMenuContextMenuOpen(this);
  const bool result = PropPanel::ImguiHelper::menuItemExWithLeftSideCheckmark(getTitle(), /*icon = */ nullptr, shortcut, is_checked,
    enabled, is_bullet, hotkeyEditorContextMenuOpen);

  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    HotkeyEditorContextMenu::createMenuContextMenu(editorCommandId, this);

  if (hotkeyEditorContextMenuOpen)
    HotkeyEditorContextMenu::updateImguiMainMenuContextMenu(this);

  return result;
}

EditorCoreMenuCommon::EditorCommandMenuItem *EditorCoreMenuCommon::createEditorCommandItem(unsigned item_id,
  const char *editor_command_id, const char *title)
{
  EditorCommandMenuItem *newItem = new EditorCommandMenuItem(item_id, MenuItemType::Button, editor_command_id);
  newItem->setTitle(title);

  // Unfortunately in Asset Viewer the command registration runs later, so this is needed to avoid false errors.
  extern bool ec_log_missing_editor_commands;
  const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);
  if (!command && ec_log_missing_editor_commands)
    logerr("Editor command \"%s\" cannot be found. Cannot set the menu item's shortcut.", editor_command_id);

  if (command && command->getHotkeyCount() > 0)
    newItem->shortcut = command->getKeyChordsAsText();

  return newItem;
}

void EditorCoreMenuCommon::updateEditorCommandShortcuts(MenuItem &item)
{
  for (MenuItem *child : item.children)
    updateEditorCommandShortcuts(*child);

  IEditorCommandKeyChordChangeEventHandler *editorCommandMenuItem = item.queryInterface<IEditorCommandKeyChordChangeEventHandler>();
  if (editorCommandMenuItem)
    editorCommandMenuItem->onEditorCommandKeyChordChanged();
}

void EditorCoreMainMenu::addEditorCommandItem(unsigned menu_id, unsigned item_id, const char *editor_command_id, const char *title)
{
  addItem(menu_id, *EditorCoreMenuCommon::createEditorCommandItem(item_id, editor_command_id, title));
}

void EditorCoreMainMenu::updateEditorCommandShortcuts() { EditorCoreMenuCommon::updateEditorCommandShortcuts(rootMenu); }

void *EditorCoreMainMenu::queryInterfacePtr(unsigned huid)
{
  if (huid == IEditorCoreMenu::HUID)
    return static_cast<IEditorCoreMenu *>(this);
  return Menu::queryInterfacePtr(huid);
}

void EditorCoreContextMenu::addEditorCommandItem(unsigned menu_id, unsigned item_id, const char *editor_command_id, const char *title)
{
  addItem(menu_id, *EditorCoreMenuCommon::createEditorCommandItem(item_id, editor_command_id, title));
}

void EditorCoreContextMenu::updateEditorCommandShortcuts() { EditorCoreMenuCommon::updateEditorCommandShortcuts(rootMenu); }

void *EditorCoreContextMenu::queryInterfacePtr(unsigned huid)
{
  if (huid == IEditorCoreMenu::HUID)
    return static_cast<IEditorCoreMenu *>(this);
  return Menu::queryInterfacePtr(huid);
}

PropPanel::IMenu *ec_create_menu() { return new EditorCoreMainMenu(); }

PropPanel::IMenu *ec_create_context_menu() { return new EditorCoreContextMenu(); }
