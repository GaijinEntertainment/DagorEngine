#pragma once

#include "fastPhysObject.h"

//------------------------------------------------------------------

class FPObjectClipper : public IFPObject
{
public:
  FPObjectClipper(FpdObject *obj, FastPhysEditor &editor);

  virtual void refillPanel(PropPanel2 *panel);
  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  virtual void render();

  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool getWorldBox(BBox3 &box) const;

  E3DCOLOR getColor() const;
  int calcLineSegs(real len);

  virtual void setMatrix(const Matrix3 &tm);
  virtual bool setPos(const Point3 &p);

  virtual void putScaleUndo() {}

  static E3DCOLOR normalColor, selectedColor, pointColor, selPointColor;
};

//------------------------------------------------------------------