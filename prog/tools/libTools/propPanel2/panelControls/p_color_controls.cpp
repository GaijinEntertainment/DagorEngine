// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_color_controls.h"
#include "../c_constants.h"

// -------------- CTwoColorIndicator -----------------

CTwoColorIndicator::CTwoColorIndicator(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y,
  hdpi::Px w, hdpi::Px h) :
  BasicPropertyControl(id, event_handler, parent, x, y, w, h + _pxScaled(DEFAULT_CONTROL_HEIGHT)),
  mTwoColorArea(this, parent->getWindow(), x, y, _px(w), _px(h))
{
  initTooltip(&mTwoColorArea);
}

PropertyContainerControlBase *CTwoColorIndicator::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createTwoColorIndicator(id, caption, _pxScaled(DEFAULT_CONTROL_HEIGHT), true, new_line);
  return NULL;
}


void CTwoColorIndicator::setColorValue(E3DCOLOR value) { mTwoColorArea.setColorValue(value); }

void CTwoColorIndicator::setPoint3Value(Point3 value) { mTwoColorArea.setColorFValue(value); }


void CTwoColorIndicator::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mTwoColorArea.resizeWindow(_px(w), mTwoColorArea.getHeight());
}

void CTwoColorIndicator::reset()
{
  mTwoColorArea.reset();
  mTwoColorArea.refresh(false);
}

void CTwoColorIndicator::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mTwoColorArea.moveWindow(x, y);
}


// -------------- CPaletteCell -----------------

CPaletteCell::CPaletteCell(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y) :
  PropertyControlBase(id, event_handler, parent, x, y, _pxScaled(DEFAULT_PALETTE_CELL_SIZE), _pxScaled(DEFAULT_PALETTE_CELL_SIZE + 1)),
  mPaletteCell(this, parent->getWindow(), x, y, _pxS(DEFAULT_PALETTE_CELL_SIZE), _pxS(DEFAULT_PALETTE_CELL_SIZE))
{
  reset();
}

PropertyContainerControlBase *CPaletteCell::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createPaletteCell(id, caption, true, new_line);
  return NULL;
}


void CPaletteCell::setColorValue(E3DCOLOR value) { mPaletteCell.setColorValue(value); }


void CPaletteCell::setTextValue(const char value[]) { mPaletteCell.setTextValue(value); }


void CPaletteCell::setBoolValue(bool value)
{
  mPaletteCell.setSelection(value);
  mPaletteCell.refresh(false);
}

void CPaletteCell::setWidth(hdpi::Px w) { return; }

void CPaletteCell::reset()
{
  mPaletteCell.reset();
  mPaletteCell.refresh(false);
}

void CPaletteCell::setEnabled(bool enabled) { mPaletteCell.setEnabled(enabled); }

void CPaletteCell::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mPaletteCell.moveWindow(x, y);
}