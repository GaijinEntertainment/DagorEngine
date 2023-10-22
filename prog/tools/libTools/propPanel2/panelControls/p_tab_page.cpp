// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_tab_page.h"
#include "p_tab_panel.h"

CTabPage::CTabPage(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h, const char caption[]) :

  CGroupBase(event_handler, parent, new WContainer(this, parent->getWindow(), x, y, _px(w), _px(h)), id, x, y, w, h, caption)
{}


PropertyContainerControlBase *CTabPage::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createTabPage(id, caption);
}


void CTabPage::setCaptionValue(const char value[])
{
  CTabPanel *tabPanel = static_cast<CTabPanel *>(mParent);
  tabPanel->setPageCaption(mId, value);
}


int CTabPage::getNextControlY(bool new_line) { return PropertyContainerVert::getNextControlY(new_line); }
