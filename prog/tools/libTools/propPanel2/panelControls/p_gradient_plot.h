#pragma once

#include "p_base.h"
#include "../windowControls/w_gradient_plot.h"
#include <propPanel2/c_panel_base.h>

class CGradientPlot : public BasicPropertyControl
{
public:
  CGradientPlot(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_POINT3 | CONTROL_DATA_TYPE_COLOR | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_POINT3 | CONTROL_DATA_TYPE_COLOR | CONTROL_DATA_TYPE_INT; }

  virtual void setColorValue(E3DCOLOR value);
  virtual void setPoint3Value(Point3 value);
  virtual void setIntValue(int value); // mode

  virtual E3DCOLOR getColorValue() const;
  virtual Point3 getPoint3Value() const;
  virtual int getIntValue() const; // mode

  void reset();

  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WStaticText mCaption;
  WGradientPlot mPlot;
};
