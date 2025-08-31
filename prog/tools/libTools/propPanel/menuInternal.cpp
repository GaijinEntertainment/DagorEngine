// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "menuInternal.h"
#include "messageQueueInternal.h"
#include "scopedImguiBeginDisabled.h"
#include <propPanel/imguiHelper.h>
#include <imgui/imgui.h>

namespace PropPanel
{

Menu::MenuItem *Menu::MenuItem::getMenuItemById(unsigned item_id)
{
  // The ROOT_MENU_ITEM is handled by Menu::getMenuItemById, item_id cannot be that at this point.
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, nullptr);

  for (MenuItem *item : children)
  {
    if (item->id == item_id)
      return item;

    if (item->itemType == MenuItemType::SubMenu)
    {
      MenuItem *searchResult = item->getMenuItemById(item_id);
      if (searchResult)
        return searchResult;
    }
  }

  return nullptr;
}

void Menu::MenuItem::updateImgui(MenuItem *&clicked_item)
{
  for (MenuItem *item : children)
  {
    if (item->itemType == MenuItemType::Button)
    {
      ScopedImguiBeginDisabled scopedDisabled(!item->enabled);

      if (ImguiHelper::menuItemExWithLeftSideCheckmark(item->title, nullptr, item->shortcut))
        clicked_item = item;
    }
    else if (item->itemType == MenuItemType::CheckButton)
    {
      ScopedImguiBeginDisabled scopedDisabled(!item->enabled);

      if (ImguiHelper::menuItemExWithLeftSideCheckmark(item->title, nullptr, item->shortcut, item->checked))
      {
        item->checked = !item->checked;
        clicked_item = item;
      }
    }
    else if (item->itemType == MenuItemType::RadioButton)
    {
      ScopedImguiBeginDisabled scopedDisabled(!item->enabled);

      if (ImguiHelper::menuItemExWithLeftSideCheckmark(item->title, nullptr, item->shortcut, item->checked,
            /*enabled = */ true, /*bullet = */ true))
      {
        item->checked = !item->checked;
        clicked_item = item;
      }
    }
    else if (item->itemType == MenuItemType::Separator)
    {
      ImGui::Separator();
    }
    else if (item->itemType == MenuItemType::SubMenu)
    {
      if (ImguiHelper::beginMenuExWithLeftSideCheckmark(item->title, nullptr, item->enabled))
      {
        item->updateImgui(clicked_item);
        ImGui::EndMenu();
      }
    }
    else
    {
      G_ASSERT(false);
    }
  }
}

Menu::Menu() : rootMenu(ROOT_MENU_ITEM, MenuItemType::SubMenu) {}

Menu::~Menu() { message_queue.removeDelayedCallback(*this); }

void Menu::addItem(unsigned menu_id, unsigned item_id, const char *title)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *menu = getMenuItemById(menu_id);
  G_ASSERT_RETURN(menu, );
  G_ASSERT_RETURN(menu->itemType == MenuItemType::SubMenu, );
  G_ASSERT_RETURN(!getMenuItemById(item_id), ); // The item_id must be globally unique.

  MenuItem *newItem = new MenuItem(item_id);
  splitTitleAndShortcut(title, newItem->title, newItem->shortcut);
  menu->children.push_back(newItem);
}

void Menu::addSeparator(unsigned menu_id, unsigned item_id)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *menu = getMenuItemById(menu_id);
  G_ASSERT_RETURN(menu, );
  G_ASSERT_RETURN(menu->itemType == MenuItemType::SubMenu, );

  MenuItem *newItem = new MenuItem(item_id, MenuItemType::Separator);
  menu->children.push_back(newItem);
}

void Menu::addSubMenu(unsigned menu_id, unsigned submenu_id, const char *title)
{
  G_ASSERT_RETURN(submenu_id != ROOT_MENU_ITEM, );

  MenuItem *menu = getMenuItemById(menu_id);
  G_ASSERT_RETURN(menu, );
  G_ASSERT_RETURN(menu->itemType == MenuItemType::SubMenu, );
  G_ASSERT_RETURN(!getMenuItemById(submenu_id), ); // The submenu_id must be globally unique.

  MenuItem *newItem = new MenuItem(submenu_id, MenuItemType::SubMenu);
  newItem->title = title;
  menu->children.push_back(newItem);
}

int Menu::getItemCount(unsigned menu_id)
{
  MenuItem *menu = getMenuItemById(menu_id);
  G_ASSERT_RETURN(menu, 0);
  G_ASSERT_RETURN(menu->itemType == MenuItemType::SubMenu, 0);

  return menu->children.size();
}

void Menu::clearMenu(unsigned menu_id)
{
  MenuItem *menu = getMenuItemById(menu_id);
  G_ASSERT_RETURN(menu, );
  G_ASSERT_RETURN(menu->itemType == MenuItemType::SubMenu, );

  clear_all_ptr_items(menu->children);
}

void Menu::setEnabledById(unsigned item_id, bool enabled)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *item = getMenuItemById(item_id);
  if (!item)
    return;

  item->enabled = enabled;
}

void Menu::setCheckById(unsigned item_id, bool checked)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *item = getMenuItemById(item_id);
  if (!item)
    return;

  item->itemType = MenuItemType::CheckButton;
  item->checked = checked;
}

void Menu::setRadioById(unsigned item_id, unsigned group_first_item_id, unsigned group_last_item_id)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );
  G_ASSERT(item_id >= group_first_item_id);
  G_ASSERT(item_id <= group_last_item_id);

  for (unsigned id = group_first_item_id;; ++id)
  {
    MenuItem *item = getMenuItemById(id);
    if (item)
    {
      item->itemType = MenuItemType::RadioButton;
      item->checked = id == item_id;
    }

    if (id == group_last_item_id)
      return;
  }
}

void Menu::setCaptionById(unsigned item_id, const char caption[])
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *item = getMenuItemById(item_id);
  G_ASSERT_RETURN(item, );

  splitTitleAndShortcut(caption, item->title, item->shortcut);
}

void Menu::setEventHandler(IMenuEventHandler *event_handler) { eventHandler = event_handler; }

void Menu::click(unsigned item_id)
{
  G_ASSERT_RETURN(item_id != ROOT_MENU_ITEM, );

  MenuItem *item = getMenuItemById(item_id);
  G_ASSERT(item);

  if (item && eventHandler)
    eventHandler->onMenuItemClick(item->id);
}

void Menu::onImguiDelayedCallback(void *user_data)
{
  if (eventHandler)
    eventHandler->onMenuItemClick((unsigned)(uintptr_t)user_data);
}

Menu::MenuItem *Menu::getMenuItemById(unsigned id) { return id == ROOT_MENU_ITEM ? &rootMenu : rootMenu.getMenuItemById(id); }

void Menu::splitTitleAndShortcut(const char *title_with_shortcut, String &title, String &shortcut)
{
  const char *separator = strchr(title_with_shortcut, '\t');
  if (separator)
  {
    title.setSubStr(title_with_shortcut, separator);
    shortcut = separator + 1;
  }
  else
  {
    title = title_with_shortcut;
    shortcut.clear();
  }
}

void Menu::updateImgui()
{
  ImGui::PushID(this);

  if (ImGui::BeginMainMenuBar())
  {
    MenuItem *clickedItem = nullptr;
    rootMenu.updateImgui(clickedItem);

    if (clickedItem && eventHandler)
      message_queue.requestDelayedCallback(*this, (void *)((uintptr_t)clickedItem->id));

    ImGui::EndMainMenuBar();
  }

  ImGui::PopID();
}

IMenu *create_menu() { return new Menu(); }

void render_menu(IMenu &menu)
{
  G_ASSERT(!menu.isContextMenu());
  static_cast<Menu &>(menu).updateImgui();
}

} // namespace PropPanel