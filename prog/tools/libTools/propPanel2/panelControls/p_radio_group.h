#pragma once

#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_placement.h>


class CRadioGroup : public PropertyContainerVert
{
public:
  CRadioGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_INT; }

  void setCaptionValue(const char value[]);
  void setIntValue(int value);
  int getIntValue() const;

  void setBoolValue(bool value) { G_ASSERT(false && "Old radio button use, contact developers"); }
  bool getBoolValue() const
  {
    G_ASSERT(false && "Old radio button use, contact developers");
    return false;
  }

  void moveTo(int x, int y);

  virtual WindowBase *getWindow();

  virtual void setEnabled(bool enabled);
  virtual void setWidth(hdpi::Px w);
  virtual void reset();
  virtual void clear();

  virtual void onChildResize(int id);
  virtual bool isRealContainer() { return false; }

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onWcChange(WindowBase *source);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual int getVerticalInterval(int line_number, bool new_line);

private:
  WStaticText mRect;
  int mSelectedIndex;
  bool mWResize;
  bool hasCaption;
};
