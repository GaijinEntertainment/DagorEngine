// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>

class HmapLandObjectEditor;
class GeomObject;
class MaterialData;
class StaticGeometryContainer;
class StaticGeometryMaterial;
struct Color4;

static constexpr unsigned CID_SphereLightObject = 0xD04E90D9u; // SphereLightObject

//
// Entity object placed on landscape
//
class SphereLightObject : public RenderableEditableObject
{
public:
  SphereLightObject();

  void update(float) override {}
  void beforeRender() override {}
  void render() override;
  void renderTrans() override;

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool getWorldBox(BBox3 &box) const override;

  void fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  bool mayRename() override { return true; }
  bool mayDelete() override { return true; }
  void setWtm(const TMatrix &wtm) override;

  void onRemove(ObjectEditor *) override;
  void onAdd(ObjectEditor *objEditor) override;

  bool setPos(const Point3 &p) override;

  void putMoveUndo() override;

  void onObjectNameChange(RenderableEditableObject *obj, const char *old_name, const char *new_name) override {}

  EO_IMPLEMENT_RTTI(CID_SphereLightObject)

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }
  SphereLightObject *clone();

  struct Props
  {
    E3DCOLOR color;
    real brightness;
    E3DCOLOR ambColor;
    real ambBrightness;
    real ang;
    bool directional;

    int segments;

    void defaults();
  };

  const Props &getProps() const { return props; }
  void setProps(const Props &p)
  {
    props = p;
    propsChanged();
  }

  void propsChanged();
  void renderObject();

  void buildGeom();
  void gatherGeom(StaticGeometryContainer &container) const;


  static int lightGeomSubtype;

  static void clearMats();
  static MaterialData *makeMat(const char *class_name, const Color4 &emis, const Color4 &amb_emis, real power);
  static StaticGeometryMaterial *makeMat2(const char *class_name, const Color4 &emis, const Color4 &amb_emis, real power);

protected:
  Props props;
  GeomObject *ltGeom;

  ~SphereLightObject() override;

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<SphereLightObject> obj;
    SphereLightObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(SphereLightObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }

    void redo() override { obj->setProps(redoProps); }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "UndoLightPropsChange"; }
  };
};
