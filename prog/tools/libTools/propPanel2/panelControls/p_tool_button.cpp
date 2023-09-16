// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_tool_button.h"

CToolButton::CToolButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id) :

  PropertyControlBase(id, event_handler, parent, 0, 0, 0, 0), mTParent(static_cast<CToolbar *>(parent))
{
  mEventHandler = NULL;
}


void CToolButton::setCaptionValue(const char value[]) { mTParent->setButtonCaption(this->getID(), value); }


void CToolButton::setEnabled(bool enabled) { mTParent->setButtonEnabled(this->getID(), enabled); }


bool CToolButton::getBoolValue() const { return mTParent->getButtonValue(this->getID()); }


void CToolButton::setBoolValue(bool value) { mTParent->setButtonValue(this->getID(), value); }


void CToolButton::setButtonPictureValues(const char *fname) { mTParent->setButtonPictures(this->getID(), fname); }
