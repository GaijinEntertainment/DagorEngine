// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_hotkeyEditorContextMenu.h"

#include "ec_editorCommand.h"
#include "ec_singleHotkeyEditor.h"

#include <EditorCore/ec_interface.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>

#include <EASTL/unique_ptr.h>

namespace
{

enum
{
  CONTEXT_MENU_ADD_HOTKEY = 1,
  CONTEXT_MENU_RESET_HOTKEY,
  CONTEXT_MENU_REMOVE_HOTKEY,
  CONTEXT_MENU_CHANGE_HOTKEY_START, // this must be the last one, it can have multiple entries
};

class ToolbarContextMenuEventHandler : public PropPanel::IMenuEventHandler
{
public:
  explicit ToolbarContextMenuEventHandler(const char *editor_command_id) : editorCommandId(editor_command_id) {}

  int onMenuItemClick(unsigned id) override
  {
    if (id == CONTEXT_MENU_ADD_HOTKEY)
    {
      ec_create_single_hotkey_editor(editorCommandId, -1);
      return 1;
    }
    else if (id == CONTEXT_MENU_REMOVE_HOTKEY)
    {
      ec_editor_commands.removeAllHotkeys(editorCommandId);
      return 1;
    }
    else if (id == CONTEXT_MENU_RESET_HOTKEY)
    {
      ec_editor_commands.resetToDefaultHotkeys(editorCommandId);
      return 1;
    }
    else if (id >= CONTEXT_MENU_CHANGE_HOTKEY_START)
    {
      ec_create_single_hotkey_editor(editorCommandId, id - CONTEXT_MENU_CHANGE_HOTKEY_START);
      return 1;
    }

    return 0;
  }

private:
  const SimpleString editorCommandId;
};

eastl::unique_ptr<PropPanel::IMenu> toolbar_context_menu;
eastl::unique_ptr<ToolbarContextMenuEventHandler> toolbar_context_menu_event_handler;

} // namespace

void *HotkeyEditorContextMenu::contextMenuMenuItem = nullptr;

void HotkeyEditorContextMenu::createContextMenu(const char *editor_command_id, void *menu_item)
{
  const EditorCommand *command = ec_editor_commands.getCommand(editor_command_id);
  if (!command)
  {
    logerr("Editor command \"%s\" cannot be found.", editor_command_id);
    return;
  }

  toolbar_context_menu.reset(PropPanel::create_context_menu());
  toolbar_context_menu_event_handler.reset(new ToolbarContextMenuEventHandler(editor_command_id));
  toolbar_context_menu->setEventHandler(toolbar_context_menu_event_handler.get());
  contextMenuMenuItem = menu_item;

  if (command->getHotkeyCount() == 1)
  {
    toolbar_context_menu->addItem(PropPanel::ROOT_MENU_ITEM, CONTEXT_MENU_CHANGE_HOTKEY_START, "Change shortcut...");
  }
  else
  {
    for (int i = 0; i < command->getHotkeyCount(); ++i)
    {
      const String title(0, "Change shortcut %d...", i + 1);
      toolbar_context_menu->addItem(PropPanel::ROOT_MENU_ITEM, CONTEXT_MENU_CHANGE_HOTKEY_START + i, title);
    }
  }

  toolbar_context_menu->addItem(PropPanel::ROOT_MENU_ITEM, CONTEXT_MENU_ADD_HOTKEY, "Add shortcut...");

  if (command->getHotkeyCount() > 0)
    toolbar_context_menu->addItem(PropPanel::ROOT_MENU_ITEM, CONTEXT_MENU_REMOVE_HOTKEY, "Remove shortcut(s)");

  if (!command->isUsingDefaultHotkeys())
    toolbar_context_menu->addItem(PropPanel::ROOT_MENU_ITEM, CONTEXT_MENU_RESET_HOTKEY, "Reset shortcut(s)");
}

void HotkeyEditorContextMenu::updateImguiContextMenu()
{
  if (toolbar_context_menu && !PropPanel::render_context_menu(*toolbar_context_menu))
  {
    toolbar_context_menu.reset();
    contextMenuMenuItem = nullptr;
  }
}
