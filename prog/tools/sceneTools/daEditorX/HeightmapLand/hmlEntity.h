// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_rendEdObject.h>
#include <de3_composit.h>
#include <util/dag_simpleString.h>
#include "hmlLayers.h"

class IObjEntity;
class IDagorEdCustomCollider;
class HmapLandObjectEditor;

static constexpr unsigned CID_LandscapeEntityObject = 0xE79DC1EDu; // LandscapeEntityObject

//
// Entity object placed on landscape
//
class LandscapeEntityObject : public RenderableEditableObject
{
public:
  LandscapeEntityObject(const char *ent_name, int rnd_seed = 0);

  void update(float) override {}
  void beforeRender() override {}
  void render() override {}
  void renderTrans() override {}

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool getWorldBox(BBox3 &box) const override;

  void fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  bool mayRename() override { return true; }
  bool mayDelete() override { return true; }
  bool setName(const char *nm) override;
  void setWtm(const TMatrix &wtm) override;

  void onRemove(ObjectEditor *) override;
  void onAdd(ObjectEditor *objEditor) override;

  bool setPos(const Point3 &p) override;

  void putMoveUndo() override;

  void onObjectNameChange(RenderableEditableObject *obj, const char *old_name, const char *new_name) override { objectPropsChanged(); }

  EO_IMPLEMENT_RTTI(CID_LandscapeEntityObject)

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }
  void renderBox();
  LandscapeEntityObject *clone();

  void setRndSeed(int seed);
  int getRndSeed() { return rndSeed; }
  void setPerInstSeed(int seed);
  int getPerInstSeed() { return perInstSeed; }
  void setPlaceOnCollision(bool on_collison);

  void updateEntityPosition(bool apply_collision = false, bool setup_ri_collision = true);

  void setEditLayerIdx(int idx);
  int getEditLayerIdx() const { return editLayerIdx; }
  int lpIndex() const { return EditLayerProps::ENT; };

  struct Props
  {
    enum
    {
      PT_none = ICompositObj::Props::PT_none,
      PT_coll = ICompositObj::Props::PT_coll,
      PT_collNorm = ICompositObj::Props::PT_collNorm,
      PT_3pod = ICompositObj::Props::PT_3pod,
      PT_fnd = ICompositObj::Props::PT_fnd,
      PT_flt = ICompositObj::Props::PT_flt,
      PT_riColl = ICompositObj::Props::PT_riColl,

      PT_count
    };
    SimpleString entityName;
    int placeType = PT_coll;
    bool overridePlaceTypeForComposit = false;

    String notes;
  };

  const Props &getProps() const { return props; }
  void setProps(const Props &p)
  {
    props = p;
    propsChanged();
  }

  void propsChanged(bool prevent_gen = false);
  void objectPropsChanged();
  IObjEntity *getEntity() { return entity; }

  bool usesRendinstPlacement() const override { return props.placeType == props.PT_riColl; }
  void setCollisionIgnored() override
  {
    oldPerInstSeed = perInstSeed;
    setPerInstSeed(0xBAD);
  }
  void resetCollisionIgnored() override { setPerInstSeed(oldPerInstSeed); }

  struct FxProps
  {
    real maxRadius;
    bool updateWhenInvisible;

    FxProps() : maxRadius(10.f), updateWhenInvisible(false) {}
  };

  struct PhysObjProps
  {
    String scriptClass;
    bool active;

    PhysObjProps() : active(false) {}
  };

  FxProps fxProps;
  PhysObjProps physObjProps;

public:
  static void saveColliders(DataBlock &blk);
  static void loadColliders(const DataBlock &blk);
  static void rePlaceAllEntitiesOnCollision(HmapLandObjectEditor &objEd, bool loft_changed, bool polygeom_changed, bool roads_chanded,
    BBox3 changed_region);
  static void rePlaceAllEntitiesOverRI(HmapLandObjectEditor &objEd);
  static void changeAssset(ObjectEditor &object_editor, dag::ConstSpan<RenderableEditableObject *> objects,
    const char *initially_selected_asset_name);

  void setGizmoTranformMode(bool enable);

protected:
  struct CollidersData
  {
    CollidersData(const CollidersData &cl_data) : col(midmem) { copyData(cl_data); }
    CollidersData() : col(midmem)
    {
      filter = 0x7FFFFFFF;
      useFilters = false;
      tracertUpOffset = 1.0;
    }

    ~CollidersData() { col.clear(); }

    inline void operator=(const CollidersData &cl_data) { copyData(cl_data); }

    inline unsigned getFilter() { return useFilters ? filter : 0x7FFFFFFF; }

  public:
    Tab<IDagorEdCustomCollider *> col;
    real tracertUpOffset;
    unsigned filter;
    bool useFilters;

  protected:
    inline void copyData(const CollidersData &cl_data)
    {
      col.clear();
      col = cl_data.col;
      filter = cl_data.filter;
      useFilters = cl_data.useFilters;
      tracertUpOffset = cl_data.tracertUpOffset;
    }
  };

  Props props;
  IObjEntity *entity;
  int rndSeed;
  int perInstSeed = 0;
  int oldPerInstSeed = 0;
  bool isCollidable;
  int editLayerIdx = 0;
  bool gizmoEnabled = false;

  static CollidersData colliders;

  struct DecalMaterialIndex
  {
    int node_idx;
    int material_idx;
  };
  Tab<DecalMaterialIndex> decalMaterialIndices;

  ~LandscapeEntityObject() override;

  static bool isColliderEnabled(const IDagorEdCustomCollider *collider);
  void setPosOnCollision(Point3 pos, bool setup_ri_collision = true);
  void rePlaceAllEntities();
  void fillMaterialProps(PropPanel::ContainerPropertyControl &panel);

  class UndoStaticPropsChange : public UndoRedoObject
  {
    LandscapeEntityObject::CollidersData oldData, redoData;

  public:
    UndoStaticPropsChange() : redoData(LandscapeEntityObject::colliders), oldData(LandscapeEntityObject::colliders) {}

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoData = LandscapeEntityObject::colliders;

      LandscapeEntityObject::colliders = oldData;
    }

    void redo() override { LandscapeEntityObject::colliders = redoData; }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "UndoEntityStaticPropsChange"; }
  };

  class UndoPropsChange : public UndoRedoObject
  {
  private:
    Ptr<LandscapeEntityObject> obj;
    LandscapeEntityObject::Props oldProps, redoProps;

  protected:
    LandscapeEntityObject *getObj() { return obj; }

  public:
    UndoPropsChange(LandscapeEntityObject *o) : obj(o) { oldProps = redoProps = obj->props; }

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }

    void redo() override { obj->setProps(redoProps); }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "UndoEntityPropsChange"; }
  };
};
