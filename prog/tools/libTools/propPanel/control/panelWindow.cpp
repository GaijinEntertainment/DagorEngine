// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/panelWindow.h>
#include <propPanel/focusHelper.h>
#include "../contextMenuInternal.h"
#include <ioSys/dag_dataBlock.h>
#include <sepGui/wndMenuInterface.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class PanelWindowContextMenuEventHandler : public IMenuEventHandler
{
public:
  PanelWindowContextMenuEventHandler(PanelWindowPropertyControl &panel_window) : panelWindow(panel_window) {}

  virtual int onMenuItemClick(unsigned id) override
  {
    PropertyControlBase *control = panelWindow.getById(id, false);
    if (control)
    {
      ContainerPropertyControl *container = control->getContainer();
      while (container && container != &panelWindow)
      {
        container->setBoolValue(false); // Open the group.
        container = container->getParent();
      }

      focus_helper.requestFocus(control);
    }

    return 0;
  }

private:
  PanelWindowPropertyControl &panelWindow;
};

PanelWindowPropertyControl::PanelWindowPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent,
  int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]) :
  ContainerPropertyControl(id, event_handler, parent, x, y, w, h), controlCaption(caption)
{}

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

void PanelWindowPropertyControl::fillJumpToGroupContextMenu(ContainerPropertyControl &container, int menu_id)
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
    const int menuId = childContainer->getID();
    const int subMenuIdOffset = 100000;
    G_ASSERT(menuId < subMenuIdOffset);

    if (hasGrandChildContainer)
    {
      const int subMenuId = menuId + subMenuIdOffset;
      contextMenu->addSubMenu(menu_id, subMenuId, caption);
      contextMenu->addItem(subMenuId, menuId, caption);
      contextMenu->addSeparator(subMenuId);
      fillJumpToGroupContextMenu(*childContainer, subMenuId);
    }
    else if (childContainer->isRealContainer() && !caption.empty())
    {
      contextMenu->addItem(menu_id, menuId, caption);
    }
  }
}

void PanelWindowPropertyControl::onWcRightClick(WindowBase *source)
{
  ImGui::FocusWindow(ImGui::GetCurrentWindow());

  contextMenuEventHandler.reset(new PanelWindowContextMenuEventHandler(*this));
  contextMenu.reset(new ContextMenu());
  contextMenu->setEventHandler(contextMenuEventHandler.get());
  fillJumpToGroupContextMenu(*this, ROOT_MENU_ITEM);
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

  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered() &&
      ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    onWcRightClick(nullptr);

  if (contextMenu)
  {
    const bool open = static_cast<ContextMenu *>(contextMenu.get())->updateImgui();
    if (!open)
    {
      contextMenu.reset();
      contextMenuEventHandler.reset();
    }
  }
}

} // namespace PropPanel