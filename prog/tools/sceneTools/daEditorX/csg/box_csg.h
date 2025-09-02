// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>

//
// BoxCSG object
//
class BoxCSG : public RenderableEditableObject
{
public:
  static constexpr unsigned HUID = 0x5177B0EEu; // occplugin::Occluder

public:
  BoxCSG();
  BoxCSG(const BoxCSG &);


  void update(float) override;

  void beforeRender() override {}
  void render() override;
  void renderTrans() override {}

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool getWorldBox(BBox3 &box) const override;

  void fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  bool mayRename() override { return true; }
  bool mayDelete() override { return true; }

  void onAdd(ObjectEditor *objEditor) override;

  EO_IMPLEMENT_RTTI(HUID)


  bool isBox() const { return box; }
  bool getQuad(Point3 v[4]);
  void renderCommonGeom(DynRenderBuffer &dynBuf, const TMatrix4 &gtm);

protected:
  ~BoxCSG() override {}

  bool box;

public:
  static E3DCOLOR norm_col, sel_col;
  static const BBox3 normBox;

  static void initStatics();
  static void renderBox(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c);
  static void renderQuad(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c);
  static void renderQuad(DynRenderBuffer &db, const Point3 v[4], E3DCOLOR c, bool ring = true);
};
