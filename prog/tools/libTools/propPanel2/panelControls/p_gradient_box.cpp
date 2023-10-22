// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include "p_gradient_box.h"
#include "../c_constants.h"

CGradientBox::CGradientBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[]) :
  BasicPropertyControl(id, event_handler, parent, x, y, w,
    _pxScaled(GRADIENT_HEIGHT) + _pxScaled(TRACK_GRADIENT_BUTTON_HEIGHT) + _pxScaled(DEFAULT_CONTROL_HEIGHT)),
  mGradient(this, parent->getWindow(), x + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2, y + _pxS(DEFAULT_CONTROL_HEIGHT),
    _px(w) - _pxS(TRACK_GRADIENT_BUTTON_WIDTH)),
  mText(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT) + _pxS(TRACK_GRADIENT_BUTTON_HEIGHT))
{
  mText.setTextValue(caption);
  initTooltip(&mGradient);
}

PropertyContainerControlBase *CGradientBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createGradientBox(id, caption, true, new_line);
  return NULL;
}

void CGradientBox::setGradientValue(PGradient value)
{
  mGradient.setValue(value);
  mGradient.refresh(false);
}

void CGradientBox::getGradientValue(PGradient destGradient) const { mGradient.getValue(destGradient); }

void CGradientBox::setCaptionValue(const char value[]) { mText.setTextValue(value); }


void CGradientBox::setIntValue(int value) { mGradient.setCycled(value & CONTROL_CYCLED_VALUES); }


void CGradientBox::setFloatValue(float value) { mGradient.setCurValue(value); }

void CGradientBox::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mGradient.resizeWindow(_px(w) - _pxS(TRACK_GRADIENT_BUTTON_WIDTH), mGradient.getHeight());
  mText.resizeWindow(_px(w), mText.getHeight());
}

void CGradientBox::reset()
{
  mGradient.reset();
  mGradient.refresh(false);
}

void CGradientBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mText.moveWindow(x, y);
  mGradient.moveWindow(x + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2, y + _pxS(DEFAULT_CONTROL_HEIGHT));
}

void CGradientBox::onWcChange(WindowBase *source)
{
  mGradient.refresh(false);
  PropertyControlBase::onWcChange(source);
}


long CGradientBox::onWcClipboardCopy(WindowBase *source, DataBlock &blk)
{
  blk.reset();
  blk.setStr("DataType", "Gradient");

  Gradient value(tmpmem);
  getGradientValue(&value);
  for (int i = 0; i < value.size(); ++i)
  {
    DataBlock *key_blk = blk.addNewBlock("key");
    if (key_blk)
    {
      key_blk->setReal("pos", value[i].position);
      key_blk->setE3dcolor("color", value[i].color);
    }
  }
  return 1;
}

long CGradientBox::onWcClipboardPaste(WindowBase *source, const DataBlock &blk)
{
  if (strcmp(blk.getStr("DataType", ""), "Gradient") != 0)
    return 0;

  int result = 0;
  mGradient.setSelected(true);
  char caption[128];
  mText.getTextValue(caption, 127);

  if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Replace gradient keys for \"%s\"?", caption) ==
      wingw::MB_ID_YES)
  {
    Gradient value(tmpmem);

    int i = -1;
    while (const DataBlock *key_blk = blk.getBlockByName("key", i))
    {
      GradientKey key;
      key.position = key_blk->getReal("pos");
      key.color = key_blk->getE3dcolor("color");
      value.push_back(key);
      ++i;
    }

    setGradientValue(&value);
    onWcChange(&mGradient);
    result = 1;
  }
  mGradient.setSelected(false);
  return result;
}
