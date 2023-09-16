// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "p_base.h"

BasicPropertyControl::BasicPropertyControl(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x,
  int y, unsigned w, unsigned h) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h)
{}

BasicPropertyControl::~BasicPropertyControl()
{
  for (int i = 0; i < tooltips.size(); ++i)
  {
    del_it(tooltips[i]);
  }
}

void BasicPropertyControl::initTooltip(WindowBase *window)
{
  if (!window)
    return;
  WTooltip *tooltip = new WTooltip(this, window);
  tooltips.push_back(tooltip);
}

void BasicPropertyControl::setTooltip(const char text[])
{
  for (int i = 0; i < tooltips.size(); ++i)
  {
    tooltips[i]->setText(text);
  }
}