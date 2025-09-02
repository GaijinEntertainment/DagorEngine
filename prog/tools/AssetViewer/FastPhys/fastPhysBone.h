// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectBone : public IFPObject
{
public:
  FPObjectBone(FpdObject *obj, FastPhysEditor &editor);

  void refillPanel(PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void render() override;
  void renderTrans() override;

  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool getWorldBox(BBox3 &box) const override;

  void putMoveUndo() override {}
  void putRotateUndo() override {}
  void putScaleUndo() override {}

  static E3DCOLOR normalColor, badColor, selectedColor, boneAxisColor, boneConeColor;
  static real unlinkedRadius, unlinkedLength;
};

//------------------------------------------------------------------
