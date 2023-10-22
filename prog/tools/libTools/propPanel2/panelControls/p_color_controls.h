#pragma once

#include "p_base.h"
#include "../windowControls/w_color_button.h"
#include <propPanel2/c_panel_base.h>

class CTwoColorIndicator : public BasicPropertyControl
{
public:
  CTwoColorIndicator(ControlEventHandler *event_h, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_POINT3 | CONTROL_DATA_TYPE_COLOR; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_POINT3 | CONTROL_DATA_TYPE_COLOR; }

  virtual void setColorValue(E3DCOLOR value);
  virtual void setPoint3Value(Point3 value);

  void reset();

  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

private:
  WTwoColorArea mTwoColorArea;
};


class CPaletteCell : public PropertyControlBase
{
public:
  CPaletteCell(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_COLOR | CONTROL_DATA_TYPE_STRING | CONTROL_DATA_TYPE_BOOL; }
  unsigned getTypeMaskForGet() const { return 0; }

  virtual void setColorValue(E3DCOLOR value);
  virtual void setTextValue(const char value[]);
  virtual void setBoolValue(bool value);

  void reset();
  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

private:
  WPaletteCell mPaletteCell;
};