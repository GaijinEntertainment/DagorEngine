#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectBone : public IFPObject
{
public:
  FPObjectBone(FpdObject *obj, FastPhysEditor &editor);

  virtual void refillPanel(PropPanel2 *panel);
  virtual void onChange(int pcb_id, PropPanel2 *panel);

  virtual void render();

  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void putMoveUndo() {}
  virtual void putRotateUndo() {}
  virtual void putScaleUndo() {}

  static E3DCOLOR normalColor, badColor, selectedColor, boneAxisColor, boneConeColor;
  static real unlinkedRadius, unlinkedLength;
};

//------------------------------------------------------------------
