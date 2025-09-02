//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

static constexpr unsigned ROOT_MENU_ITEM = (unsigned)-1;

class IMenuEventHandler
{
public:
  /// Should return 1 if menu was processed
  virtual int onMenuItemClick(unsigned id) = 0;
};

// Use ROOT_MENU_ITEM for menu_id to add to the root menu.
class IMenu
{
public:
  virtual ~IMenu() {}

  virtual void addItem(unsigned menu_id, unsigned item_id, const char *title) = 0;
  virtual void addSeparator(unsigned menu_id, unsigned item_id = 0) = 0;
  virtual void addSubMenu(unsigned menu_id, unsigned submenu_id, const char *title) = 0;

  virtual int getItemCount(unsigned menu_id) = 0;

  virtual void clearMenu(unsigned menu_id) = 0;

  virtual void setEnabledById(unsigned item_id, bool enabled = true) = 0;
  virtual void setCheckById(unsigned item_id, bool checked = true) = 0;
  // Both group_first_item_id and group_last_item_id are inclusive.
  virtual void setRadioById(unsigned item_id, unsigned group_first_item_id = 0, unsigned group_last_item_id = 0) = 0;
  virtual void setCaptionById(unsigned item_id, const char caption[]) = 0;

  virtual void setEventHandler(IMenuEventHandler *event_handler) = 0;
  virtual void click(unsigned item_id) = 0;

  virtual bool isContextMenu() const = 0;
};

IMenu *create_menu();
void render_menu(IMenu &menu);

IMenu *create_context_menu();

// Returns with true while the context menu is open OR is no longer open but a menu item has been selected and the
// event is being delivered (as a delayed callback).
// Returns with false if the context menu has been closed (either by selecting something or just closing it), and
// it can be destoyed.
//
// So it should be used something like this:
// if (popupMenu && !PropPanel::render_context_menu(*popupMenu))
//  del_it(popupMenu);
bool render_context_menu(IMenu &menu);

} // namespace PropPanel