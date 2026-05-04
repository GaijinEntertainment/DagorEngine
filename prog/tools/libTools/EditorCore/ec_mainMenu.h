// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_editorCommandSystem.h>
#include <propPanel/control/contextMenu.h>
#include <propPanel/control/menuImplementation.h>
#include <util/dag_simpleString.h>

class IEditorCoreMenu
{
public:
  virtual void addEditorCommandItem(unsigned menu_id, unsigned item_id, const char *editor_command_id, const char *title) = 0;
  virtual void updateEditorCommandShortcuts() = 0;

  static constexpr unsigned HUID = 0x69E89C28u; // IEditorCoreMenu
};

class EditorCoreMenuCommon : public PropPanel::Menu
{
public:
  class EditorCommandMenuItem : public MenuItem, public IEditorCommandKeyChordChangeEventHandler
  {
  public:
    EditorCommandMenuItem(unsigned item_id, MenuItemType item_type, const char *editor_command_id);

  private:
    void onEditorCommandKeyChordChanged() override;
    void *queryInterfacePtr(unsigned huid) override;
    bool updateImguiButton(bool is_checked, bool is_bullet) override;

    const SimpleString editorCommandId;
  };

  static EditorCommandMenuItem *createEditorCommandItem(unsigned item_id, const char *editor_command_id, const char *title);
  static void updateEditorCommandShortcuts(MenuItem &item);
};

class EditorCoreMainMenu : public PropPanel::Menu, public IEditorCoreMenu
{
public:
  void addEditorCommandItem(unsigned menu_id, unsigned item_id, const char *editor_command_id, const char *title) override;
  void updateEditorCommandShortcuts() override;
  void *queryInterfacePtr(unsigned huid) override;
};

class EditorCoreContextMenu : public PropPanel::ContextMenu, public IEditorCoreMenu
{
public:
  void addEditorCommandItem(unsigned menu_id, unsigned item_id, const char *editor_command_id, const char *title) override;
  void updateEditorCommandShortcuts() override;
  void *queryInterfacePtr(unsigned huid) override;
};
