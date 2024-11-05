// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/menu.h>
#include "messageQueueInternal.h"
#include <dag/dag_vector.h>
#include <util/dag_string.h>

namespace PropPanel
{

class Menu : public IMenu, public IDelayedCallbackHandler
{
public:
  Menu();
  ~Menu();

  virtual void addItem(unsigned menu_id, unsigned item_id, const char *title) override;
  virtual void addSeparator(unsigned menu_id, unsigned item_id) override;
  virtual void addSubMenu(unsigned menu_id, unsigned submenu_id, const char *title) override;

  virtual int getItemCount(unsigned menu_id) override;

  virtual void clearMenu(unsigned menu_id) override;

  virtual void setEnabledById(unsigned item_id, bool enabled) override;
  virtual void setCheckById(unsigned item_id, bool checked) override;
  virtual void setRadioById(unsigned item_id, unsigned group_first_item_id, unsigned group_last_item_id) override;
  virtual void setCaptionById(unsigned item_id, const char caption[]) override;

  virtual void setEventHandler(IMenuEventHandler *event_handler) override;
  virtual void click(unsigned item_id) override;

  virtual bool isContextMenu() const override { return false; }

  void updateImgui();

protected:
  enum class MenuItemType
  {
    Button,
    CheckButton,
    RadioButton,
    Separator,
    SubMenu,
  };

  struct MenuItem
  {
    explicit MenuItem(unsigned item_id, MenuItemType item_type = MenuItemType::Button) : id(item_id), itemType(item_type) {}

    ~MenuItem() { clear_all_ptr_items(children); }

    MenuItem *getMenuItemById(unsigned item_id);
    void updateImgui(MenuItem *&clicked_item);

    String title;
    String shortcut;
    const unsigned id;
    bool enabled = true;
    bool checked = false;
    MenuItemType itemType;

    dag::Vector<MenuItem *> children;
  };

  virtual void onImguiDelayedCallback(void *user_data) override;

  MenuItem *getMenuItemById(unsigned id);

  static void splitTitleAndShortcut(const char *title_with_shortcut, String &title, String &shortcut);

  MenuItem rootMenu;
  IMenuEventHandler *eventHandler = nullptr;
};

} // namespace PropPanel