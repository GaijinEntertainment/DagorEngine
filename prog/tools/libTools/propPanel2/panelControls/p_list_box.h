#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_boxes.h"
#include <propPanel2/c_panel_base.h>


class CListBox : public BasicPropertyControl
{
public:
  CListBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char caption[],
    const Tab<String> &vals, int index);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION | CONTROL_DATA_STRINGS; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_TYPE_STRING; }

  int getIntValue() const;
  int getTextValue(char *buffer, int buflen) const { return mListBox.getSelectedText(buffer, buflen); };
  void setIntValue(int index);
  void setStringsValue(const Tab<String> &vals);
  void setCaptionValue(const char value[]);
  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void setHeight(unsigned h);
  void moveTo(int x, int y);

private:
  WStaticText mCaption;
  WListBox mListBox;
};
