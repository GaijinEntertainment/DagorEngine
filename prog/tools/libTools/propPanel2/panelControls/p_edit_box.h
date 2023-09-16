#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CEditBox : public BasicPropertyControl
{
public:
  CEditBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, const char caption[],
    bool multiline);
  ~CEditBox();

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_STRING; }

  void setTextValue(const char value[]);
  void setCaptionValue(const char value[]);
  void setBoolValue(bool value);
  int getTextValue(char *buffer, int buflen) const;

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void setHeight(unsigned h); // make more than DEFAULT_CONTROL_HEIGHT * 2 for multiline
  void setFocus();
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WStaticText mCaption;
  WEdit *mEdit;
  bool manualChange;
};
