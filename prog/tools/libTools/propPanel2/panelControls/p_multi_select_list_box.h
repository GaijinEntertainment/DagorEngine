#pragma once

#include "p_base.h"
#include "../windowControls/w_boxes.h"
#include <propPanel2/c_panel_base.h>


class CMultiSelectListBox : public BasicPropertyControl
{
public:
  CMultiSelectListBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const Tab<String> &vals);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_STRINGS | CONTROL_DATA_SELECTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_STRINGS | CONTROL_DATA_SELECTION | CONTROL_DATA_TYPE_INT; }

  void setTextValue(const char value[]);
  void setStringsValue(const Tab<String> &vals);
  void setSelectionValue(const Tab<int> &sels);
  void reset();

  int getIntValue() const;
  int getStringsValue(Tab<String> &vals);
  int getSelectionValue(Tab<int> &sels);

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

private:
  WMultiSelListBox mListBox;
};
