// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectClipper : public IFPObject
{
public:
  FPObjectClipper(FpdObject *obj, FastPhysEditor &editor);

  void refillPanel(PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void render() override;

  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool getWorldBox(BBox3 &box) const override;

  E3DCOLOR getColor() const;
  int calcLineSegs(real len);

  void setMatrix(const Matrix3 &tm) override;
  bool setPos(const Point3 &p) override;

  void putScaleUndo() override {}

  static E3DCOLOR normalColor, selectedColor, pointColor, selPointColor;
};

//------------------------------------------------------------------