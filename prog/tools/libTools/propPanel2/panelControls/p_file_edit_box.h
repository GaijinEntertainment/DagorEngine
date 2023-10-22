#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>

class CFileEditBox : public BasicPropertyControl
{
public:
  CFileEditBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const
  {
    return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION | CONTROL_DATA_TYPE_USER | CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT |
           CONTROL_DATA_TYPE_STRING;
  }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_STRING; }

  void setTextValue(const char value[]);
  void setBoolValue(bool value); // remove this
  void setIntValue(int value);
  void setCaptionValue(const char value[]);
  void setStringsValue(const Tab<String> &vals);
  void setUserDataValue(const void *value); // for base path set
  int getTextValue(char *buffer, int buflen) const;

  void reset();

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);
  virtual void onWcClick(WindowBase *source);

private:
  WStaticText mCaption;
  WEdit mEdit;
  WButton mButton;
  int m_DialogMode;
  char mMasks[FILTER_STRING_SIZE];
  bool manualChange;
  SimpleString basePath;
};
