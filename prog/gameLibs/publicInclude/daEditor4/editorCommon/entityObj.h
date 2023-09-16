//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_function.h>
#include <daEditor4/de4_object.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <rendInst/rendInstGen.h>
#include <daECS/scene/scene.h>

#include <sqrat.h>


class DataBlock;
class EntityObjEditor;

static constexpr unsigned CID_EntityObj = 0x354A2D0Bu;   // EntityObj
static constexpr unsigned CID_RendInstObj = 0x889033FCu; // RendInstObj
struct EntCreateData
{
  eastl::string templName;
  ecs::ComponentsInitializer attrs;
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  EntCreateData(ecs::EntityId e, const char *template_name = NULL);
  EntCreateData(const char *template_name, const TMatrix *tm = NULL);
  EntCreateData(const char *template_name, const TMatrix *tm, const char *riex_name);

  static void createCb(EntCreateData *data);
};

//
// Entity object placed on landscape
//
class EntityObj : public EditableObject
{
public:
  EO_IMPLEMENT_RTTI(CID_EntityObj, EditableObject)

  EntityObj(const char *ent_name, ecs::EntityId _id);

  virtual EditableObject *cloneObject();
  EditableObject *cloneObject(const char *template_name);
  virtual void recreateObject(ObjectEditor *objEditor);

  virtual void update(float);
  virtual void beforeRender() {}
  virtual void render();
  virtual void renderTrans() {}

  virtual bool isValid() const { return !eid || g_entity_mgr->doesEntityExist(eid); } // invalid entity is used for newObj management
  virtual bool isSelectedByRectangle(const IBBox2 &rect) const;
  virtual bool isSelectedByPointClick(int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const
  {
    if (!hasTransform())
      return false;
    box = matrix * BSphere3(Point3(0, 0, 0), 0.5);
    return true;
  }

  /*
    virtual void fillProps(PropPanel2 &panel,
      DClassID for_class_id, dag::ConstSpan<EditableObject*> objects);

    virtual void onPPChange(int pid, bool edit_finished,
      PropPanel2 &panel,
      dag::ConstSpan<EditableObject*> objects);

    virtual void onPPBtnPressed(int pid, PropPanel2 &panel,
      dag::ConstSpan<EditableObject*> objects);
  */

  virtual bool mayRename() { return true; }
  virtual bool mayDelete() { return true; }
  virtual void setWtm(const TMatrix &wtm)
  {
    if (eid == ecs::INVALID_ENTITY_ID)
      EditableObject::setWtm(wtm);
    if (!hasTransform())
      return;
    EditableObject::setWtm(wtm);
    updateEntityPosition(true);
  }

  virtual void onRemove(ObjectEditor *objEditor);
  virtual void onAdd(ObjectEditor *objEditor);

  virtual bool setPos(const Point3 &p)
  {
    if (eid == ecs::INVALID_ENTITY_ID)
      return EditableObject::setPos(p);
    if (!hasTransform())
      return false;
    if (!EditableObject::setPos(p))
      return false;
    updateEntityPosition(true);
    return true;
  }
  virtual void onObjectNameChange(EditableObject *, const char *, const char *) { objectPropsChanged(); }

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }

  void setPlaceOnCollision(bool) {}
  void updateEntityPosition(bool = false);
  void replaceEid(ecs::EntityId id)
  {
    resetEid();
    setupEid(id);
  }

  struct Props
  {
    SimpleString entityName;
    bool placeOnCollision;
    bool useCollisionNorm;

    String notes;
  };

  const Props &getProps() const { return props; }
  void setProps(const Props &p)
  {
    props = p;
    propsChanged();
  }

  void propsChanged() {}
  void objectPropsChanged() {}

  ecs::EntityId getEid() const { return eid; }
  void resetEid();
  void updateLbb();

  void save(DataBlock &blk, const ecs::Scene::EntityRecord &erec) const;
  bool hasTransform() const;

  static void register_script_class(HSQUIRRELVM vm);
  BBox3 getLbb() const override { return lbb; }

protected:
  Props props;
  ecs::EntityId eid;
  BBox3 lbb;

  eastl::unique_ptr<EntCreateData> removedEntData;
  eastl::unique_ptr<ecs::ComponentsList> removedEntComps;

  void setPosOnCollision(Point3 pos);

  void rePlaceAllEntities();

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<EntityObj> obj;
    EntityObj::Props oldProps, redoProps;

  public:
    UndoPropsChange(EntityObj *o) : obj(o) { oldProps = redoProps = obj->props; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }
    virtual void redo() { obj->setProps(redoProps); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoEntityPropsChange"; }
  };

  void setupEid(ecs::EntityId eid);

private:
  mutable ecs::template_t objectLastTemplateId = ecs::INVALID_TEMPLATE_INDEX;
  mutable int objectFlags = 0;
  void updateObjectFlags() const;
  void resetObjectFlags();
};


class RendInstObj : public EditableObject
{
public:
  EO_IMPLEMENT_RTTI(CID_RendInstObj, EditableObject)

  RendInstObj(const rendinst::RendInstDesc &d);

  virtual void update(float);
  virtual void beforeRender() {}
  virtual void render() { renderBox(lbb); }
  virtual void renderTrans() {}

  virtual bool isValid() const { return true; }
  virtual bool isSelectedByRectangle(const IBBox2 &rect) const { return isBoxSelectedByRectangle(lbb, rect); }
  virtual bool isSelectedByPointClick(int x, int y) const { return isBoxSelectedByPointClick(lbb, x, y); }
  virtual bool getWorldBox(BBox3 &box) const
  {
    box = matrix * BSphere3(Point3(0, 0, 0), 0.5);
    return true;
  }

  virtual bool mayRename() { return false; }
  virtual bool mayDelete() { return false; }
  virtual void setWtm(const TMatrix &wtm)
  {
    EditableObject::setWtm(wtm);
    updateEntityPosition(true);
  }

  virtual bool setPos(const Point3 &p)
  {
    if (!EditableObject::setPos(p))
      return false;
    updateEntityPosition(true);
    return true;
  }

  void updateEntityPosition(bool = false);
  rendinst::riex_handle_t getRiEx() const { return riDesc.getRiExtraHandle(); }

  BBox3 getLbb() const override { return lbb; }

protected:
  rendinst::RendInstDesc riDesc;
  BBox3 lbb;
  int lastSelTimeMsec;
};

void entity_obj_editor_for_each_entity(EntityObjEditor &editor, eastl::fixed_function<sizeof(void *), void(EntityObj *)> cb);
