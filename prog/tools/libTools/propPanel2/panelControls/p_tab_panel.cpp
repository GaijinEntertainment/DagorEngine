// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_tab_panel.h"
#include "p_tab_page.h"
#include "../c_constants.h"

#include <debug/dag_debug.h>

CTabPanel::CTabPanel(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h, const char caption[]) :

  PropertyContainerControlBase(id, event_handler, parent, x, y, w, h), mTab(this, parent->getWindow(), x, y, _px(w), _px(h))
{}


PropertyContainerControlBase *CTabPanel::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  (void)new_line;
  return parent->createTabPanel(id, caption);
}


//-----------------------------------------------------------------------------


PropertyContainerControlBase *CTabPanel::createTabPage(int id, const char caption[])
{
  unsigned width = mTab.getWidth() - 2 * _pxS(DEFAULT_TAB_PAGE_OFFSET);

  CTabPage *newTabPage = new CTabPage(this->mEventHandler, this, id, _pxS(DEFAULT_TAB_PAGE_OFFSET), _pxS(DEFAULT_TAB_HEIGHT),
    _pxActual(width), _pxScaled(DEFAULT_GROUP_HEIGHT), "");

  mControlsNewLine.push_back(false);
  mControlArray.push_back(newTabPage);
  mTab.addPage(id, caption);

  if (mTab.getCurrentPageId() != id)
    newTabPage->getWindow()->hide();

  return newTabPage;
}


void CTabPanel::onControlAdd(PropertyControlBase *control) { G_ASSERT(false && "TabPanel can contain only TabPages!"); }


void CTabPanel::clear()
{
  __super::clear();
  mTab.deleteAllPages();
}

void CTabPanel::onPageActivated()
{
  int id = mTab.getCurrentPageId();

  for (int i = 0; i < mControlArray.size(); ++i)
  {
    PropertyContainerControlBase *page = mControlArray[i]->getContainer();

    G_ASSERT(page && "TabPanel can contain only containers (TabPages)!");

    if (page->getID() == id)
      page->getWindow()->show();
    else
      page->getWindow()->hide();
  }
}

void CTabPanel::onWcChange(WindowBase *source)
{
  if (source == &mTab)
    onPageActivated();

  __super::onWcChange(source);
}


int CTabPanel::getIntValue() const { return mTab.getCurrentPageId(); }


void CTabPanel::setIntValue(int value)
{
  mTab.setCurrentPage(value);
  onPageActivated();
}


void CTabPanel::setPageCaption(int id, const char caption[]) { mTab.setPageCaption(id, caption); }


void CTabPanel::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mTab.moveWindow(x, y);
}


void CTabPanel::setWidth(hdpi::Px w)
{
  for (int i = 0; i < mControlArray.size(); ++i)
  {
    mControlArray[i]->setWidth(w - _pxScaled(DEFAULT_TAB_PAGE_OFFSET) * 2);
  }

  this->resizeControl(_px(w), _pxS(DEFAULT_TAB_HEIGHT));

  PropertyContainerControlBase::setWidth(w);
}


void CTabPanel::resizeControl(unsigned w, unsigned h)
{
  unsigned maxW = w, maxH = h;

  for (int i = 0; i < mControlArray.size(); ++i)
  {
    PropertyContainerControlBase *page = mControlArray[i]->getContainer();
    G_ASSERT(page && "TabPanel can contain only containers (TabPages)!");

    unsigned cW = page->getWidth() + _pxS(DEFAULT_TAB_PAGE_OFFSET) * 2;
    unsigned cH = _pxS(DEFAULT_TAB_HEIGHT);

    if (page->getChildCount())
      cH = page->getHeight() + _pxS(DEFAULT_TAB_HEIGHT) + _pxS(DEFAULT_CONTROLS_INTERVAL);

    if (cW > maxW)
      maxW = cW;

    if (cH > maxH)
      maxH = cH;
  }

  mW = maxW;
  mH = maxH;

  mTab.resizeWindow(mW, mH);
}


void CTabPanel::onChildResize(int id) { resizeControl(mW, mH); }


WindowBase *CTabPanel::getWindow() { return &mTab; }
