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

  virtual void update(float) {}
  virtual void beforeRender() {}
  virtual void render();
  virtual void renderTrans();

  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const;

  virtual void fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects);

  virtual void save(DataBlock &blk);
  virtual void load(const DataBlock &blk);

  virtual bool mayRename() { return true; }
  virtual bool mayDelete() { return true; }
  virtual void setWtm(const TMatrix &wtm);

  virtual void onRemove(ObjectEditor *);
  virtual void onAdd(ObjectEditor *objEditor);

  virtual bool setPos(const Point3 &p);

  virtual void putMoveUndo();

  virtual void onObjectNameChange(RenderableEditableObject *obj, const char *old_name, const char *new_name) {}

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

  ~SphereLightObject();

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<SphereLightObject> obj;
    SphereLightObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(SphereLightObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }

    virtual void redo() { obj->setProps(redoProps); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoLightPropsChange"; }
  };
};
