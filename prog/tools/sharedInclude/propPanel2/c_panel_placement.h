//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_panel_base.h>

enum class HorzFlow
{
  Disabled,
  Enabled
};

// Logic for vertical control placement
class PropertyContainerVert : public PropertyContainerControlBase
{
  static const int PPANEL_WIDTH = 280;

public:
  PropertyContainerVert(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x, int y, unsigned w,
    unsigned h, HorzFlow horzFlow = HorzFlow::Disabled);

  virtual void clear();
  virtual void setWidth(unsigned w);
  virtual void onChildResize(int id);
  void setVerticalInterval(int value) { mVInterval = value; }

  // fast fill for large pannels
  virtual void disableFillAutoResize();
  virtual void restoreFillAutoResize();

  void enableResizeCallback() override;
  void disableResizeCallback() override;

protected:
  int mVInterval;
  bool m_BlockResizeCallback, m_BlockFillAutoResize;
  HorzFlow horzFlow;

  virtual void resizeControl(unsigned w, unsigned h){};
  virtual void addControl(PropertyControlBase *pcontrol, bool new_line = true);
  virtual void onControlAdd(PropertyControlBase *control){};

  virtual int getNextControlX(bool new_line = true);
  virtual int getNextControlY(bool new_line = true);
  virtual int getVerticalInterval(int line_number, bool new_line) { return mVInterval; }

  void setNewLine();
  const int defaultControlWidth = PPANEL_WIDTH;
};


// Logic for horizontal control placement
class PropertyContainerHorz : public PropertyContainerControlBase
{
public:
  PropertyContainerHorz(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x, int y, unsigned w,
    unsigned h) :
    PropertyContainerControlBase(id, event_handler, parent, x, y, w, h){};

  virtual void setWidth(unsigned w){};
  virtual void onChildResize(int id);

protected:
  virtual void resizeControl(unsigned w, unsigned h){};
  virtual void addControl(PropertyControlBase *pcontrol, bool new_line = true);
  virtual void onControlAdd(PropertyControlBase *control);

  virtual int getNextControlX(bool new_line = true);
  virtual int getNextControlY(bool new_line = true);
  unsigned getClientWidth();
};
