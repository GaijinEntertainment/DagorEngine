// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectPoint : public IFPObject
{
public:
  FPObjectPoint(FpdObject *obj, FastPhysEditor &editor);

  void refillPanel(PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void render() override;

  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool getWorldBox(BBox3 &box) const override;

  E3DCOLOR getColor() const;

  bool setPos(const Point3 &p) override;

  void putRotateUndo() override {}
  void putScaleUndo() override {}

  static E3DCOLOR normalColor, selectedColor;
  static real pointRadius;
};


//------------------------------------------------------------------
