#pragma once

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_group_button.h"
#include <propPanel2/c_panel_placement.h>

class CGroup : public PropertyContainerVert
{
public:
  CGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const char caption[], HorzFlow horzFlow = HorzFlow::Disabled);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_BOOL; }

  virtual bool getBoolValue() const { return mMinimized; }
  virtual void setBoolValue(bool value);
  void setCaptionValue(const char value[]);

  virtual void setEnabled(bool enabled);
  virtual void setUserDataValue(const void *value) override { mUserData = const_cast<void *>(value); }
  virtual void *getUserDataValue() const override { return mUserData; }
  virtual int getCaptionValue(char *buffer, int buflen) const;

  void clear();
  void minimize();
  void restore();
  void setWidth(unsigned w);
  void moveTo(int x, int y);

  virtual WindowBase *getWindow();

  // saving and loading state with datablock

  virtual int saveState(DataBlock &datablk);
  virtual int loadState(DataBlock &datablk);

protected:
  void resizeControl(unsigned w, unsigned h);
  virtual void onWcClick(WindowBase *source);
  virtual void onWcRefresh(WindowBase *source);
  virtual void onControlAdd(PropertyControlBase *control);
  virtual int getNextControlY(bool new_line = true);

  WGroup mRect;
  WMaxGroupButton mMaxButton;
  bool mMinimized;
  unsigned mMaximizedSize;
  void *mUserData = nullptr;
};
