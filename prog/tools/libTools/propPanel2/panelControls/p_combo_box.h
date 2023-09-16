#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_boxes.h"
#include <propPanel2/c_panel_base.h>


class CComboBox : public BasicPropertyControl
{
public:
  CComboBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[], const Tab<String> &vals, int index, bool sorted = false);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const
  {
    return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING | CONTROL_DATA_STRINGS;
  }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_TYPE_STRING; }

  int getIntValue() const;
  int getTextValue(char *buffer, int buflen) const { return mComboBox.getSelectedText(buffer, buflen); };
  void setTextValue(const char value[]) { mComboBox.setSelectedText(value); }
  void setIntValue(int index);
  void setStringsValue(const Tab<String> &vals);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

private:
  WStaticText mCaption;
  WComboBox mComboBox;
  const bool mSorted;
};
