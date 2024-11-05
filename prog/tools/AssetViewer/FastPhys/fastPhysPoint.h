// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectPoint : public IFPObject
{
public:
  FPObjectPoint(FpdObject *obj, FastPhysEditor &editor);

  virtual void refillPanel(PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  virtual void render();

  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool getWorldBox(BBox3 &box) const;

  E3DCOLOR getColor() const;

  virtual bool setPos(const Point3 &p);

  virtual void putRotateUndo() {}
  virtual void putScaleUndo() {}

  static E3DCOLOR normalColor, selectedColor;
  static real pointRadius;
};


//------------------------------------------------------------------
