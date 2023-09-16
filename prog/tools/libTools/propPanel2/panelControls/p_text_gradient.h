#pragma once

#include "p_base.h"
#include "../windowControls/w_text_gradient.h"
#include <propPanel2/c_panel_base.h>


class CTextGradient : public BasicPropertyControl
{
public:
  CTextGradient(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_TEXT_GRADIENT | CONTROL_CAPTION | CONTROL_DATA_MIN_MAX_STEP; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_TEXT_GRADIENT; }

  void getTextGradientValue(TextGradient &destGradient) const;
  int getTextValue(char *buffer, int buflen) const;
  void setTextGradientValue(const TextGradient &source);
  void setTextValue(const char value[]);
  void setCaptionValue(const char value[]);
  void setMinMaxStepValue(float min, float max, float step);
  void setFloatValue(float value); // current X value line

  void reset();

  void setWidth(unsigned w);
  void moveTo(int x, int y);

  long onWcClipboardCopy(WindowBase *source, DataBlock &blk);
  long onWcClipboardPaste(WindowBase *source, const DataBlock &blk);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WStaticText mText;
  WTextGradient mGradient;
};
