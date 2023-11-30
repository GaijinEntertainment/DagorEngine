#pragma once

#include <anim/dag_animDecl.h>

#include <EditorCore/ec_rendEdObject.h>

class FastPhysEditor;

//------------------------------------------------------------------

class FastPhysCharRoot : public RenderableEditableObject
{
public:
  static constexpr unsigned HUID = 0x449E3939u; // FastPhysCharRoot

public:
  AnimV20::IAnimCharacter2 *character;

  FastPhysCharRoot(AnimV20::IAnimCharacter2 *ch, FastPhysEditor &editor);

  void updateNodeTm();

  virtual void render();
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void onPPChange(int pid, bool edit_finished, PropertyContainerControlBase &panel,
    dag::ConstSpan<RenderableEditableObject *> objects){};

  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) {}

  virtual void putScaleUndo() {}


  virtual bool setPos(const Point3 &p);
  virtual void setMatrix(const Matrix3 &tm);
  virtual void setWtm(const TMatrix &wtm);

  EO_IMPLEMENT_RTTI(HUID, RenderableEditableObject)

  // default implementation for some inherited methods
  virtual bool mayDelete() { return false; }

  virtual void update(real dt);
  virtual void beforeRender() {}
  virtual void renderTrans() {}

  void renderGeometry(int stage);

  bool getCharVisible() { return isCharVisible; }
  void setCharVisible(bool value) { isCharVisible = value; }

  static real sphereRadius;
  static E3DCOLOR normalColor, selectedColor;

protected:
  FastPhysEditor &mFPEditor;
  bool isCharVisible;
};

//------------------------------------------------------------------
