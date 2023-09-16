#pragma once

#include "../c_window_controls.h"
#include <debug/dag_debug.h>

// ------------------- Controls ---------------------------

// ----------------- Containers ------------------

class WContainer : public WindowControlBase
{
public:
  WContainer(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  ~WContainer() {}
  virtual void onPostEvent(unsigned id);
  virtual intptr_t onRButtonUp(long x, long y)
  {
    if (mEventHandler)
      mEventHandler->onWcRightClick(this);
    return 0;
  }
  intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
};


class WGroup : public WindowControlBase
{
public:
  WGroup(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  virtual void onPaint();
  virtual void onPostEvent(unsigned id);
  virtual intptr_t onRButtonUp(long x, long y);
};


//-------------------------------------------------


class WStaticText : public WindowControlBase
{
public:
  WStaticText(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  virtual intptr_t onResize(unsigned new_width, unsigned new_height);
};


class WEdit : public WindowControlBase
{
public:
  WEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, bool multiline = false);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onKeyDown(unsigned v_key, int flags);

  virtual void setColorIndication(bool value);
  virtual intptr_t onDrawEdit(void *hdc);

private:
  bool mNeedColorIndicate;
};


class WButton : public WindowControlBase
{
public:
  WButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, bool show_sel = true);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
};


class WCheckBox : public WindowControlBase
{
public:
  WCheckBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);
  ~WCheckBox();
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);

  bool checked() const;
  void setState(bool value);

private:
  virtual intptr_t onMouseMove(int x, int y) override;
  virtual intptr_t onRButtonUp(long x, long y) override;
  virtual intptr_t onRButtonDown(long x, long y) override;
  virtual intptr_t onCaptureChanged(void *window_gaining_capture) override;

  void stopDragChecking();

  static bool dragChecking;
  static bool dragCheckingNewValue;
};


class WRadioButton : public WindowControlBase
{
public:
  WRadioButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, int index);

  virtual int getIndex() const { return mGroupIndex; }
  virtual void setCheck(bool value = true) const;
  virtual bool getCheck() const;

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onKeyDown(unsigned v_key, int flags);
  virtual intptr_t onLButtonDClick(long x, long y);

private:
  int mGroupIndex;
};


class WSeparator : public WindowControlBase
{
public:
  WSeparator(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);
};


class WTooltip
{
public:
  WTooltip(WindowControlEventHandler *event_handler, WindowBase *parent);
  ~WTooltip();
  void setText(const char text[]);

private:
  SimpleString tText;
  intptr_t cHandle = 0;

  static WindowBase *tooltipWnd;
};
