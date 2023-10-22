// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include "p_text_gradient.h"
#include "../c_constants.h"

CTextGradient::CTextGradient(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y,
  hdpi::Px w, const char caption[]) :
  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) + _pxScaled(TEXT_GRADIENT_HEIGHT)),
  mText(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mGradient(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT), _px(w))
{
  mText.setTextValue(caption);
  initTooltip(&mGradient);
}

PropertyContainerControlBase *CTextGradient::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createTextGradient(id, caption, true, new_line);
  return NULL;
}


void CTextGradient::getTextGradientValue(TextGradient &destGradient) const { mGradient.getValues(destGradient); }


void CTextGradient::setTextGradientValue(const TextGradient &source)
{
  mGradient.setValues(source);
  mGradient.refresh(false);
}


void CTextGradient::setTextValue(const char value[]) { mGradient.setCurrentKeyText(value); }


int CTextGradient::getTextValue(char *buffer, int buflen) const
{
  const char *_value = mGradient.getCurrentKeyText();
  int _len = i_strlen(_value);
  if (_len < buflen)
    strcpy(buffer, _value);
  else
  {
    buffer[0] = '\0';
    return 0;
  }
  return _len;
}


void CTextGradient::setMinMaxStepValue(float min, float max, float step) { mGradient.setMinMax(min, max); }


void CTextGradient::setFloatValue(float value) { mGradient.setCurValue(value); }


void CTextGradient::setCaptionValue(const char value[]) { mText.setTextValue(value); }


void CTextGradient::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mText.resizeWindow(_px(w), mText.getHeight());
  mGradient.resizeWindow(_px(w), mGradient.getHeight());
}

void CTextGradient::reset()
{
  mGradient.reset();
  mGradient.refresh(false);
}

void CTextGradient::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mText.moveWindow(x, y);
  mGradient.moveWindow(x, y + _pxS(DEFAULT_CONTROL_HEIGHT));
}

void CTextGradient::onWcChange(WindowBase *source)
{
  mGradient.refresh(false);
  PropertyControlBase::onWcChange(source);
}


long CTextGradient::onWcClipboardCopy(WindowBase *source, DataBlock &blk)
{
  blk.reset();
  blk.setStr("DataType", "Gradient");

  TextGradient value(tmpmem);
  getTextGradientValue(value);
  for (int i = 0; i < value.size(); ++i)
  {
    DataBlock *key_blk = blk.addNewBlock("key");
    if (key_blk)
    {
      key_blk->setReal("pos", value[i].position);
      key_blk->setStr("text", value[i].text);
    }
  }
  return 1;
}

long CTextGradient::onWcClipboardPaste(WindowBase *source, const DataBlock &blk)
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
    TextGradient value(tmpmem);

    int i = -1;
    while (const DataBlock *key_blk = blk.getBlockByName("key", i))
    {
      TextGradientKey key;
      key.position = key_blk->getReal("pos");
      key.text = key_blk->getStr("text", "");
      value.push_back(key);
      ++i;
    }

    setTextGradientValue(value);
    onWcChange(&mGradient);
    result = 1;
  }
  mGradient.setSelected(false);
  return result;
}
