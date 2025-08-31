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

  void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override;

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

  ~SnowSourceObject() override;

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<SnowSourceObject> obj;
    SnowSourceObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(SnowSourceObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }

    void redo() override { obj->setProps(redoProps); }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "UndoSnowPropsChange"; }
  };
};

struct NearSnowSource
{
  SnowSourceObject *source;
  float distance;

  NearSnowSource() : source(NULL), distance(0) {}
  NearSnowSource(SnowSourceObject *s, float d) : source(s), distance(d) {}
};
