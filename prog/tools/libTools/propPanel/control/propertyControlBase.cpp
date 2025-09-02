// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/control/container.h>
#include "../tooltipHelper.h"

namespace PropPanel
{

PropertyControlBase::PropertyControlBase(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y,
  hdpi::Px w, hdpi::Px h) :
  mId(id),
  mEventHandler(event_handler),
  mParent(parent),
  mX(x),
  mY(y),
  mW(_px(w)),
  mH(_px(h)),
  hasCaption(true),
  mEnabledChanges(false)
{}

PropertyControlBase::~PropertyControlBase() { mEventHandler = nullptr; }

int PropertyControlBase::addStringValue(const char *value) { return 0; }

void *PropertyControlBase::getUserDataValue() const { return nullptr; }

int PropertyControlBase::getTextValue(char *buffer, int buflen) const { return 0; }

int PropertyControlBase::getCaptionValue(char *buffer, int buflen) const { return 0; }

float PropertyControlBase::getFloatValue() const { return 0; }

int PropertyControlBase::getIntValue() const { return 0; }

bool PropertyControlBase::getBoolValue() const { return false; }

E3DCOLOR PropertyControlBase::getColorValue() const { return 0; }

Point2 PropertyControlBase::getPoint2Value() const { return Point2(0, 0); }

Point3 PropertyControlBase::getPoint3Value() const { return Point3(0, 0, 0); }

Point4 PropertyControlBase::getPoint4Value() const { return Point4(0, 0, 0, 0); }

TMatrix PropertyControlBase::getMatrixValue() const { return TMatrix::IDENT; }

void PropertyControlBase::getGradientValue(PGradient destGradient) const
{
  if (destGradient)
    clear_and_shrink(*destGradient);
}

void PropertyControlBase::getTextGradientValue(TextGradient &destGradient) const { clear_and_shrink(destGradient); }

void PropertyControlBase::getCurveCoefsValue(Tab<Point2> &points) const { clear_and_shrink(points); }

bool PropertyControlBase::getCurveCubicCoefsValue(Tab<Point2> &xy_4c_per_seg) const
{
  clear_and_shrink(xy_4c_per_seg);
  return false;
}

const char *PropertyControlBase::getTooltip() const { return controlTooltip.c_str(); }

int PropertyControlBase::getStringsValue(Tab<String> &vals)
{
  clear_and_shrink(vals);
  return 0;
}

dag::ConstSpan<String> PropertyControlBase::getStringsValue() { return dag::ConstSpan<String>(); }

int PropertyControlBase::getSelectionValue(Tab<int> &sels)
{
  clear_and_shrink(sels);
  return 0;
}

long PropertyControlBase::onWcChanging(WindowBase *source)
{
  if (mEnabledChanges && mEventHandler)
    return mEventHandler->onChanging(mId, getRootParent());

  return 0;
}

void PropertyControlBase::onWcChange(WindowBase *source)
{
  if (mEnabledChanges && mEventHandler)
    mEventHandler->onChange(mId, getRootParent());
}

void PropertyControlBase::onWcClick(WindowBase *source)
{
  if (mEventHandler)
    mEventHandler->onClick(mId, getRootParent());
}

void PropertyControlBase::onWcDoubleClick(WindowBase *source)
{
  if (mEventHandler)
    mEventHandler->onDoubleClick(mId, getRootParent());
}

long PropertyControlBase::onWcKeyDown(WindowBase *source, unsigned v_key)
{
  if (mEventHandler)
    return mEventHandler->onKeyDown(mId, getRootParent(), v_key);

  return 0;
}

void PropertyControlBase::onWcResize(WindowBase *source)
{
  if (this->mParent)
    this->mParent->onChildResize(mId);

  if (mEventHandler)
    mEventHandler->onResize(mId, getRootParent());
}

void PropertyControlBase::onWcPostEvent(unsigned id)
{
  if (mEventHandler)
    mEventHandler->onPostEvent(id, getRootParent());
}

ContainerPropertyControl *PropertyControlBase::getRootParent()
{
  if ((!mParent) || (mParent->getEventHandler() != this->getEventHandler()))
  {
    ContainerPropertyControl *root = this->getContainer();
    G_ASSERT(root && "PropertyControlBase::getRootParent(): root isn't container!");

    return root;
  }

  return mParent->getRootParent();
}

ContainerPropertyControl *PropertyControlBase::getParent() { return mParent; }

void PropertyControlBase::setPreviousImguiControlTooltip()
{
  if (controlTooltip.empty())
    return;

  tooltip_helper.setPreviousImguiControlTooltip(this, controlTooltip.begin(), controlTooltip.end());
}

} // namespace PropPanel