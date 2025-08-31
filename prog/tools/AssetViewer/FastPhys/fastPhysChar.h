// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

  void render() override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool getWorldBox(BBox3 &box) const override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override {};

  void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override {}

  void putScaleUndo() override {}


  bool setPos(const Point3 &p) override;
  void setMatrix(const Matrix3 &tm) override;
  void setWtm(const TMatrix &wtm) override;

  EO_IMPLEMENT_RTTI(HUID)

  // default implementation for some inherited methods
  bool mayDelete() override { return false; }

  void update(real dt) override;
  void beforeRender() override {}
  void renderTrans() override {}

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
