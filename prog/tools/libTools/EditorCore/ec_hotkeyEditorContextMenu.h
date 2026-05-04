// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>

class HotkeyEditorContextMenu
{
public:
  static void createMenuContextMenu(const char *editor_command_id, void *menu_item)
  {
    createContextMenu(editor_command_id, menu_item);
  }

  static void createToolbarContextMenu(const char *editor_command_id) { createContextMenu(editor_command_id, nullptr); }

  static bool isMenuContextMenuOpen(void *menu_item) { return menu_item == contextMenuMenuItem; }

  static void updateImguiMainMenuContextMenu(void *menu_item)
  {
    if (isMenuContextMenuOpen(menu_item))
      updateImguiContextMenu();
  }

  static void updateImguiToolbarContextMenu()
  {
    if (!contextMenuMenuItem)
      updateImguiContextMenu();
  }

private:
  static void createContextMenu(const char *editor_command_id, void *menu_item);
  static void updateImguiContextMenu();

  static void *contextMenuMenuItem;
};
