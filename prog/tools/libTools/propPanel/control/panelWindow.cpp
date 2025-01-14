// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/panelWindow.h>
#include <propPanel/focusHelper.h>
#include "../contextMenuInternal.h"
#include <ioSys/dag_dataBlock.h>
#include <sepGui/wndMenuInterface.h>
#include <imgui/imgui_internal.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace PropPanel
{

class PanelWindowContextMenuEventHandler : public IMenuEventHandler
{
public:
  PanelWindowContextMenuEventHandler(PanelWindowPropertyControl &panel_window) : panelWindow(panel_window) {}

  virtual int onMenuItemClick(unsigned id) override
  {
    auto it = menuIdToContainerMap.find(id);
    if (it == menuIdToContainerMap.end())
      return 0;

    for (ContainerPropertyControl *container = it->second; container && container != &panelWindow; container = container->getParent())
      container->setBoolValue(false); // Open the group.

    focus_helper.requestFocus(it->second);

    return 0;
  }

  ska::flat_hash_map<int, ContainerPropertyControl *> menuIdToContainerMap;

private:
  PanelWindowPropertyControl &panelWindow;
};

static void fill_jump_to_group_context_menu(IMenu &context_menu, ContainerPropertyControl &container, int menu_id, int &next_unique_id,
  ska::flat_hash_map<int, ContainerPropertyControl *> &menu_id_to_container_map)
{
  if (!container.isRealContainer())
    return;

  for (int childIndex = 0; childIndex < container.getChildCount(); ++childIndex)
  {
    ContainerPropertyControl *childContainer = container.getByIndex(childIndex)->getContainer();
    if (!childContainer)
      continue;

    bool hasGrandChildContainer = false;
    for (int grandChildIndex = 0; grandChildIndex < childContainer->getChildCount(); ++grandChildIndex)
    {
      ContainerPropertyControl *graindChild = childContainer->getByIndex(grandChildIndex)->getContainer();
      if (graindChild && graindChild->isRealContainer())
      {
        hasGrandChildContainer = true;
        break;
      }
    }

    const SimpleString caption = childContainer->getCaption();

    if (hasGrandChildContainer)
    {
      const int subMenuId = next_unique_id++;
      const int menuItemId = next_unique_id++;
      menu_id_to_container_map.insert({menuItemId, childContainer});

      context_menu.addSubMenu(menu_id, subMenuId, caption);
      context_menu.addItem(subMenuId, menuItemId, caption);
      context_menu.addSeparator(subMenuId);
      fill_jump_to_group_context_menu(context_menu, *childContainer, subMenuId, next_unique_id, menu_id_to_container_map);
    }
    else if (childContainer->isRealContainer() && !caption.empty())
    {
      const int menuItemId = next_unique_id++;
      menu_id_to_container_map.insert({menuItemId, childContainer});

      context_menu.addItem(menu_id, menuItemId, caption);
    }
  }
}

PanelWindowPropertyControl *PanelWindowPropertyControl::middleMouseDragWindow = nullptr;
ImVec2 PanelWindowPropertyControl::middleMouseDragStartPos;
float PanelWindowPropertyControl::middleMouseDragStartScrollY;

PanelWindowPropertyControl::PanelWindowPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent,
  int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]) :
  ContainerPropertyControl(id, event_handler, parent, x, y, w, h), controlCaption(caption)
{}

PanelWindowPropertyControl::~PanelWindowPropertyControl() {}

int PanelWindowPropertyControl::getScrollPos()
{
  ImGuiWindow *window = ImGui::FindWindowByName(controlCaption);
  return window ? (int)floorf(window->Scroll.y) : 0;
}

void PanelWindowPropertyControl::setScrollPos(int pos)
{
  if (pos >= 0)
  {
    scrollingRequestedPositionY = pos;

    // At the first frame the size of the panel is not yet known, ImGui would throw away our requested scroll position.
    // So set it for two frames.
    scrollingRequestedForFrames = 2;
  }
}

int PanelWindowPropertyControl::saveState(DataBlock &datablk)
{
  DataBlock *_block = datablk.addNewBlock(String(64, "panel_%d", getID()).str());
  _block->addInt("vscroll", getScrollPos());

  ContainerPropertyControl::saveState(datablk);
  return 0;
}

int PanelWindowPropertyControl::loadState(DataBlock &datablk)
{
  ContainerPropertyControl::loadState(datablk);

  DataBlock *_block = datablk.getBlockByName(String(64, "panel_%d", getID()).str());
  if (_block)
    setScrollPos(_block->getInt("vscroll", 0));

  return 0;
}

void PanelWindowPropertyControl::onWcRightClick(WindowBase *source)
{
  ImGui::FocusWindow(ImGui::GetCurrentWindow());

  PanelWindowContextMenuEventHandler *panelWindowContextMenuEventHandler = new PanelWindowContextMenuEventHandler(*this);
  ska::flat_hash_map<int, ContainerPropertyControl *> &menuIdToContainerMap = panelWindowContextMenuEventHandler->menuIdToContainerMap;

  contextMenuEventHandler.reset(panelWindowContextMenuEventHandler);
  contextMenu.reset(new ContextMenu());
  contextMenu->setEventHandler(contextMenuEventHandler.get());

  int nextIdToUse = 1;
  fill_jump_to_group_context_menu(*contextMenu, *this, ROOT_MENU_ITEM, nextIdToUse, menuIdToContainerMap);

  if (contextMenu->getItemCount(ROOT_MENU_ITEM) == 0)
  {
    contextMenu.reset();
    contextMenuEventHandler.reset();
  }
}

void PanelWindowPropertyControl::beforeImguiBegin()
{
  if (scrollingRequestedPositionY >= 0)
  {
    ImGui::SetNextWindowScroll(ImVec2(-1.0f, scrollingRequestedPositionY));

    --scrollingRequestedForFrames;
    if (scrollingRequestedForFrames <= 0)
      scrollingRequestedPositionY = -1;
  }
}

void PanelWindowPropertyControl::updateImgui()
{
  ContainerPropertyControl::updateImgui();

  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
  {
    if (ImGui::Shortcut(ImGuiKey_MouseMiddle))
    {
      middleMouseDragWindow = this;
      middleMouseDragStartPos = ImGui::GetMousePos();
      middleMouseDragStartScrollY = ImGui::GetScrollY();

      // The window's ID is not really an item ID, but it will work, we could use any ID (beside 0) here.
      ImGui::SetActiveID(ImGui::GetCurrentWindow()->ID, ImGui::GetCurrentWindow());
    }
    else if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
      onWcRightClick(nullptr);
    }
  }

  if (contextMenu)
  {
    const bool open = static_cast<ContextMenu *>(contextMenu.get())->updateImgui();
    if (!open)
    {
      contextMenu.reset();
      contextMenuEventHandler.reset();
    }
  }
  else if (middleMouseDragWindow == this)
  {
    const ImGuiWindow *window = ImGui::GetCurrentWindowRead();
    const float sizeVisible = window->InnerRect.GetHeight();
    const float sizeContent = window->ContentSize.y + (window->WindowPadding.y * 2.0f); // From ImGui::Scrollbar().
    if (sizeContent > sizeVisible && sizeVisible > 0.0f)
    {
      const float scrollRatio = sizeContent / sizeVisible;
      const float scrollAmount = (ImGui::GetMousePos().y - middleMouseDragStartPos.y) * scrollRatio;
      const float newScrollY = clamp(middleMouseDragStartScrollY - scrollAmount, 0.0f, ImGui::GetScrollMaxY());
      if (newScrollY != ImGui::GetScrollY())
        ImGui::SetScrollY(newScrollY);
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
    {
      // Because the ID is not a real item ID (not added with ItemAdd), we have to keep it alive.
      ImGui::KeepAliveID(ImGui::GetCurrentWindow()->ID);

      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    else
    {
      middleMouseDragWindow = nullptr;
    }
  }
}

} // namespace PropPanel