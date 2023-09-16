#pragma once

#include "p_base.h"
#include "../windowControls/w_gradient.h"
#include <propPanel2/c_panel_base.h>


class CGradientBox : public BasicPropertyControl
{
public:
  CGradientBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_GRADIENT | CONTROL_CAPTION | CONTROL_DATA_MIN_MAX_STEP; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_GRADIENT; }

  void setGradientValue(PGradient value);
  void getGradientValue(PGradient destGradient) const;
  void setCaptionValue(const char value[]);
  void setFloatValue(float value); // current X value line
  void setIntValue(int value);     // 2 byte - cycle
  void setMinMaxStepValue(float min, float max, float step) { mGradient.setMinMax(min, max); }

  void reset();

  void setWidth(unsigned w);
  void moveTo(int x, int y);

  long onWcClipboardCopy(WindowBase *source, DataBlock &blk);
  long onWcClipboardPaste(WindowBase *source, const DataBlock &blk);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WStaticText mText;
  WGradient mGradient;
};
