// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/tool_window.h>

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_window.h"
#include "../panelControls/p_combo_box.h"
#include "../c_constants.h"

#include <windows.h>

#include <debug/dag_debug.h>


//---------------------------------------


CToolWindow::CToolWindow(ControlEventHandler *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char caption[]) :

  PropertyContainerHorz(0, event_handler, NULL, x, y, w, h),

  mPanelWindow(NULL),
  mPanel(NULL)
{
  mPanelWindow = new WWindow(this, phandle, x, y, _px(w), _px(h), caption);
  mPanel = new WContainer(this, mPanelWindow, 0, 0, _px(w), _px(h));

  G_ASSERT(mPanelWindow && "CToolWindow::mPanelWindow = NULL");
  G_ASSERT(mPanel && "CToolWindow::mPanel = NULL");

  if (phandle)
    mParentHandle = phandle;

  int cw, ch;
  mPanelWindow->getClientSize(cw, ch);
  mPanel->resizeWindow(cw, ch);

  mPanelWindow->show();
}


CToolWindow::~CToolWindow()
{
  clear();
  del_it(mPanel);
  del_it(mPanelWindow);
}


void CToolWindow::setCaptionValue(const char value[]) { mPanelWindow->setTextValue(value); }


WindowBase *CToolWindow::getWindow() { return mPanel; }


void *CToolWindow::getWindowHandle() { return mPanelWindow->getHandle(); }


void *CToolWindow::getParentWindowHandle() { return mPanelWindow->getParentHandle(); }


int CToolWindow::getPanelWidth() const { return mPanel->getWidth(); }


void CToolWindow::onWcResize(WindowBase *source)
{
  int oldmH = mH;
  int oldmW = mW;

  mH = mPanelWindow->getHeight();
  mW = mPanelWindow->getWidth();

  if ((oldmH != mH) || (oldmW != mW))
  {
    this->scrollCheck();
  }

  if ((source == mPanelWindow) && ((oldmW != mW)))
  {
    this->setWidth(_pxActual(mW));
  }

  PropertyContainerHorz::onWcResize(source);
}


void CToolWindow::scrollCheck()
{
  int cw, ch;
  mPanelWindow->getClientSize(cw, ch);

  // horizontal scroll

  if (this->getNextControlX(false) > cw)
  {
    if (!mPanelWindow->hasHScroll())
    {
      mPanelWindow->addHScroll();
    }

    mPanelWindow->hScrollUpdate(this->getNextControlX(false), cw);
  }
  else
  {
    if (mPanelWindow->hasHScroll())
    {
      mPanelWindow->removeHScroll();
    }
  }
}


// ----


void CToolWindow::onChildResize(int id)
{
  PropertyContainerHorz::onChildResize(id);

  this->scrollCheck();
}


void CToolWindow::resizeControl(unsigned w, unsigned h) { mPanel->resizeWindow(w, h); }


void CToolWindow::setWidth(hdpi::Px w) { mPanelWindow->resizeWindow(_px(w), mPanelWindow->getHeight()); }


void CToolWindow::setHeight(hdpi::Px h) { mPanelWindow->resizeWindow(mPanelWindow->getWidth(), _px(h)); }


void CToolWindow::moveTo(int x, int y) { mPanelWindow->moveWindow(x, y); }


void CToolWindow::showPanel(bool visible) { (visible) ? mPanel->show() : mPanel->hide(); }


void CToolWindow::onControlAdd(PropertyControlBase *control)
{
  PropertyContainerHorz::onControlAdd(control);

  if (control)
  {
    this->scrollCheck();
  }
}


void CToolWindow::onWmEmbeddedResize(int width, int height) { setWidth(_pxActual(width)); }


const int TOOLBAR_OVER_SIZE = 10;
bool CToolWindow::onWmEmbeddedMakingMovable(int &width, int &height)
{
  int movWidth = mPanel->getWidth() + hdpi::_pxS(TOOLBAR_OVER_SIZE);

  width = (movWidth < width) ? movWidth : width;
  height = mPanel->getHeight();
  return true;
}
