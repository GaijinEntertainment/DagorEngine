// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_simple_color.h"
#include "../c_constants.h"

CSimpleColor::CSimpleColor(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT),

  mButtonWidth(DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH),

  mCaption(this, parent->getWindow(), x, y, w - mButtonWidth, DEFAULT_CONTROL_HEIGHT),

  mButton(this, parent->getWindow(), x + mCaption.getWidth(), y, mButtonWidth, DEFAULT_CONTROL_HEIGHT)
{
  mCaption.setTextValue(caption);
  // mButton.setTextValue("...");
  this->setColorValue(E3DCOLOR(0, 0, 0, 255));
  initTooltip(&mButton);
}


PropertyContainerControlBase *CSimpleColor::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createSimpleColor(id, caption, E3DCOLOR(0, 0, 0, 255), true, new_line);
  return NULL;
}


void CSimpleColor::setColorValue(E3DCOLOR value)
{
  mColor = value;
  mButton.setColorValue(mColor);
}


void CSimpleColor::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CSimpleColor::reset()
{
  this->setColorValue(E3DCOLOR(0));
  PropertyControlBase::reset();
}


void CSimpleColor::onWcChange(WindowBase *source)
{
  if (source == &mButton)
  {
    mColor = mButton.getColorValue();
    PropertyControlBase::onWcChange(source);
  }
}


void CSimpleColor::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mButton.moveWindow(x + mCaption.getWidth(), y);
}


void CSimpleColor::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mButton.setEnabled(enabled);
}


void CSimpleColor::setWidth(unsigned w)
{
  int minw = mButtonWidth;
  w = (w < minw) ? minw : w;

  PropertyControlBase::setWidth(w);
  mCaption.resizeWindow(w - mButtonWidth, mCaption.getHeight());

  this->moveTo(this->getX(), this->getY());
}
