// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_static.h"

CStatic::CStatic(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], hdpi::Px h) :

  PropertyControlBase(id, event_handler, parent, x, y, w, h), mCaption(this, parent->getWindow(), x, y, _px(w), _px(h))
{
  mCaption.setTextValue(caption);
}


PropertyContainerControlBase *CStatic::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createStatic(id, caption, new_line);
  return NULL;
}


void CStatic::setEnabled(bool enabled) { mCaption.setEnabled(enabled); }


void CStatic::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CStatic::setTextValue(const char value[]) { mCaption.setTextValue(value); }


int CStatic::getTextValue(char *buffer, int buflen) const { return mCaption.getTextValue(buffer, buflen); }


void CStatic::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);
  mCaption.resizeWindow(_px(w), mCaption.getHeight());
}


void CStatic::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mCaption.moveWindow(x, y);
}


void CStatic::setBoolValue(bool value) { mCaption.setFont(value ? mCaption.getBoldFont() : mCaption.getNormalFont()); }
