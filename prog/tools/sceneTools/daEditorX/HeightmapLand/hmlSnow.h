// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <generic/dag_ptrTab.h>

class StaticGeometryContainer;
class StaticGeometryMaterial;
struct NearSnowSource;

static constexpr unsigned CID_SnowSourceObject = 0xD04E90DAu;

//
// Entity object placed on landscape
//
class SnowSourceObject : public RenderableEditableObject
{
public:
  SnowSourceObject();

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

  EO_IMPLEMENT_RTTI(CID_SnowSourceObject)

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }
  SnowSourceObject *clone();

  struct Props
  {
    real value;
    real border;

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

  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);

  float getRadius() { return getSize().x / 2.0; }

  static int showSubtypeId;

public:
  static const int NEAR_SRC_COUNT = 16;

  static void updateSnowSources(dag::ConstSpan<NearSnowSource> nearSources);

  static void calcSnow(dag::ConstSpan<SnowSourceObject *> src, float avg, StaticGeometryContainer &cont);
  static void clearMats();
  static StaticGeometryMaterial *makeSnowMat(StaticGeometryMaterial *source_mat);

protected:
  Props props;

  ~SnowSourceObject();

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<SnowSourceObject> obj;
    SnowSourceObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(SnowSourceObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }

    virtual void redo() { obj->setProps(redoProps); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoSnowPropsChange"; }
  };
};

struct NearSnowSource
{
  SnowSourceObject *source;
  float distance;

  NearSnowSource() : source(NULL), distance(0) {}
  NearSnowSource(SnowSourceObject *s, float d) : source(s), distance(d) {}
};
