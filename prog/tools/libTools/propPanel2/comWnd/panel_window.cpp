// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/panel_window.h>

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_window.h"
#include "../c_constants.h"

#include <windows.h>
#include <ioSys/dag_dataBlock.h>
#include <sepGui/wndMenuInterface.h>

class PanelWindowContextMenuEventHandler : public IMenuEventHandler
{
public:
  PanelWindowContextMenuEventHandler(CPanelWindow *pw) : panelWindow(pw) {}

  virtual int onMenuItemClick(unsigned id)
  {
    PropertyControlBase *control = panelWindow->getById(id, false);
    if (control)
    {
      PropertyContainerControlBase *curContainer = control->getContainer();
      int yPos = 0;
      while (curContainer != panelWindow)
      {
        curContainer->setBoolValue(false);
        curContainer = curContainer->getParent();
        yPos += curContainer->getY(); //-V1026
      }
      panelWindow->setScrollPos(-control->getY() - yPos);
    }
    return 0;
  }

private:
  CPanelWindow *panelWindow;
};

CPanelWindow::CPanelWindow(ControlEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
  const char caption[]) :
  PropertyContainerVert(0, event_handler, NULL, x, y, w, h), contextMenuEventHandler(new PanelWindowContextMenuEventHandler(this))
{
  mPanelWindow = new WWindow(this, phandle, x, y, w, h, caption, true);
  mPanel = new WContainer(this, mPanelWindow, 0, 0, w, h);

  if (phandle)
    mParentHandle = phandle;

  int cw, ch;
  mPanelWindow->getClientSize(cw, ch);
  mPanel->resizeWindow(cw, mPanel->getHeight());

  contextMenuEventHandler = new PanelWindowContextMenuEventHandler(this);
  contextMenu = IMenu::createPopupMenu(mPanel->getHandle(), contextMenuEventHandler);

  mPanelWindow->show();
}


CPanelWindow::~CPanelWindow()
{
  del_it(contextMenuEventHandler);
  del_it(contextMenu);
  del_it(mPanel);
  del_it(mPanelWindow);
}


unsigned CPanelWindow::getTypeMaskForSet() const { return CONTROL_CAPTION; }


unsigned CPanelWindow::getTypeMaskForGet() const { return 0; }


void CPanelWindow::setCaptionValue(const char value[]) { mPanelWindow->setTextValue(value); }


WindowBase *CPanelWindow::getWindow() { return mPanel; }


void *CPanelWindow::getWindowHandle() { return mPanelWindow->getHandle(); }


void *CPanelWindow::getParentWindowHandle()
{
  return mPanelWindow->getParentHandle(); // for child window
}


void CPanelWindow::onChildResize(int id)
{
  PropertyContainerVert::onChildResize(id);
  this->scrollCheck();
}


void CPanelWindow::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    int cw, ch;
    mPanelWindow->getClientSize(cw, ch);
    mPanel->resizeWindow(cw, this->getNextControlY());
    this->scrollCheck();
  }
}


void CPanelWindow::onWcResize(WindowBase *source)
{
  int oldmH = mH;
  int oldmW = mW;

  mH = mPanelWindow->getHeight();
  mW = mPanelWindow->getWidth();

  if (oldmH != mH)
  {
    this->scrollCheck();
  }

  if ((source == mPanelWindow) && ((oldmW != mW)))
  {
    this->setWidth(mW);
  }

  __super::onWcResize(source);
}

static int recursiveFillMenu(IMenu *contextMenu, PropertyControlBase *curControl, int menu_id, int depth)
{
  const int SUBMENU_PREFIX = 0x8000;
  PropertyContainerControlBase *curControlPanel = curControl->getContainer();
  int ret = 0;

  if (curControlPanel && curControlPanel->isRealContainer())
  {
    // contextMenu->addItem(ROOT_MENU_ITEM, curControl->getID(), curControlPanel->getCaption());
    for (int ind = 0; ind < curControlPanel->getChildCount(); ind++)
    {
      PropertyContainerControlBase *childContainer = curControlPanel->getByIndex(ind)->getContainer();
      if (childContainer)
      {
        String name = String(256, "%s", childContainer->getCaption().str());
        //
        bool haveChilden = false;
        for (int y = 0; y < childContainer->getChildCount(); y++)
        {
          PropertyContainerControlBase *testChildren = childContainer->getByIndex(y)->getContainer();
          if (testChildren && testChildren->isRealContainer())
          {
            haveChilden = true;
            break;
          }
        }

        if (haveChilden)
        {
          int newMenuId = childContainer->getID() + SUBMENU_PREFIX;
          contextMenu->addSubMenu(menu_id, newMenuId, name);
          contextMenu->addItem(newMenuId, childContainer->getID(), name);
          contextMenu->addSeparator(newMenuId);
          ret += recursiveFillMenu(contextMenu, curControlPanel->getByIndex(ind), newMenuId, depth + 1);
        }
        else if (name[0])
        {
          contextMenu->addItem(menu_id, childContainer->getID(), name);
          ret++;
        }
      }
    }
  }
  return ret;
}

void CPanelWindow::onWcRightClick(WindowBase *source)
{
  // if (source == mPanel)
  {
    contextMenu->clearMenu(ROOT_MENU_ITEM);
    if (recursiveFillMenu(contextMenu, this, ROOT_MENU_ITEM, 0))
      contextMenu->showPopupMenu();
  }
}

void CPanelWindow::onWcCommand(WindowBase *source, unsigned notify_code, unsigned elem_id)
{
  if (source == mPanel)
    contextMenu->click(elem_id);
}

void CPanelWindow::scrollCheck()
{
  int cw, ch;
  mPanelWindow->getClientSize(cw, ch);

  if (this->getNextControlY() > ch)
  {
    if (!mPanelWindow->hasVScroll())
    {
      mPanelWindow->addVScroll();
      this->setWidth(mW); // To resize controls after scrollbar show
    }

    mPanelWindow->vScrollUpdate(this->getNextControlY(), ch);
  }
  else
  {
    if (mPanelWindow->hasVScroll())
    {
      mPanelWindow->removeVScroll();
    }
  }
}


int CPanelWindow::getScrollPos() { return mPanelWindow->getVScrollPos(); }


void CPanelWindow::setScrollPos(int pos) { mPanelWindow->setVScrollPos(pos); }


void CPanelWindow::resizeControl(unsigned w, unsigned h)
{
  int cw, ch;
  mPanelWindow->getClientSize(cw, ch);
  mPanel->resizeWindow(cw, h);
}


void CPanelWindow::clear()
{
  PropertyContainerControlBase::clear(); // Calling super's super.
  scrollCheck();
  resizeControl(getWidth(), getNextControlY());
}


void CPanelWindow::setWidth(unsigned w)
{
  mPanelWindow->resizeWindow(w, mPanelWindow->getHeight());
  this->resizeControl(w, this->getNextControlY());

  PropertyContainerVert::setWidth(w);
}


void CPanelWindow::setHeight(unsigned h) { mPanelWindow->resizeWindow(mPanelWindow->getWidth(), h); }


void CPanelWindow::moveTo(int x, int y) { mPanelWindow->moveWindow(x, y); }


void CPanelWindow::onWmEmbeddedResize(int width, int height)
{
  setWidth(width);
  setHeight(height);
}

void CPanelWindow::showPanel(bool visible) { (visible) ? mPanel->show() : mPanel->hide(); }

// ----------------------------------------------------------------------------

int CPanelWindow::saveState(DataBlock &datablk)
{
  DataBlock *_block = datablk.addNewBlock(String(64, "panel_%d", this->getID()).str());
  _block->addInt("vscroll", this->getScrollPos());

  __super::saveState(datablk);
  return 0;
}


int CPanelWindow::loadState(DataBlock &datablk)
{
  __super::loadState(datablk);

  DataBlock *_block = datablk.getBlockByName(String(64, "panel_%d", this->getID()).str());
  if (_block)
    this->setScrollPos(_block->getInt("vscroll", 0));

  return 0;
}
