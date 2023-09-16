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


  virtual void update(float);

  virtual void beforeRender() {}
  virtual void render();
  virtual void renderTrans() {}

  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void fillProps(PropertyContainerControlBase &op, DClassID for_class_id, dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void onPPChange(int pid, bool edit_finished, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void onPPBtnPressed(int pid, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);
  // virtual bool onPPValidateParam(int pid, PropPanel2 &panel,
  //   dag::ConstSpan<RenderableEditableObject*> objects);

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);

  virtual bool mayRename() { return true; }
  virtual bool mayDelete() { return true; }

  virtual void onAdd(ObjectEditor *objEditor);

  EO_IMPLEMENT_RTTI(HUID)


  bool isBox() const { return box; }
  bool getQuad(Point3 v[4]);
  void renderCommonGeom(DynRenderBuffer &dynBuf, const TMatrix4 &gtm);

protected:
  virtual ~BoxCSG() {}

  bool box;

public:
  static E3DCOLOR norm_col, sel_col;
  static const BBox3 normBox;

  static void initStatics();
  static void renderBox(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c);
  static void renderQuad(DynRenderBuffer &db, const TMatrix &tm, E3DCOLOR c);
  static void renderQuad(DynRenderBuffer &db, const Point3 v[4], E3DCOLOR c, bool ring = true);
};
