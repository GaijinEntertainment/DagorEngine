// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ecsEntityCreateData.h"

#include <daECS/scene/scene.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <EditorCore/ec_rendEdObject.h>
#include <libTools/util/undo.h>

#include <EASTL/unique_ptr.h>

class DataBlock;

namespace ecs
{
class Array;
class Object;
} // namespace ecs

static const int CID_ECSEntityObject = 0xE9AC9702u; // ECSEntityObject

// Entity object placed on landscape
class ECSEntityObject : public RenderableEditableObject
{
  using Base = RenderableEditableObject;

public:
  EO_IMPLEMENT_RTTI_EX(CID_ECSEntityObject, RenderableEditableObject)

  struct Props
  {
    String notes;
    bool placeOnCollision;
    bool useCollisionNorm;
  };

  explicit ECSEntityObject(ecs::EntityId _id);

  ECSEntityObject *cloneObject();
  ECSEntityObject *cloneObject(const char *template_name);
  void recreateObject(ObjectEditor *objEditor);

  void update(float) override;
  void beforeRender() override {}
  void render() override;
  void renderTrans() override {}

  bool isValid() const { return !eid || g_entity_mgr->doesEntityExist(eid); } // invalid entity is used for newObj management
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;

  bool getWorldBox(BBox3 &box) const override
  {
    if (!hasTransform())
      return false;
    box = matrix * BSphere3(Point3(0, 0, 0), 0.5);
    return true;
  }

  bool mayRename() override { return false; }
  bool mayDelete() override { return !isLocked(); }

  void setWtm(const TMatrix &wtm) override
  {
    if (eid == ecs::INVALID_ENTITY_ID)
      RenderableEditableObject::setWtm(wtm);
    if (!hasTransform())
      return;
    RenderableEditableObject::setWtm(wtm);
    updateEntityPosition();
  }

  void onRemove(ObjectEditor *objEditor) override;
  void onAdd(ObjectEditor *objEditor) override;

  bool setPos(const Point3 &p) override
  {
    if (eid == ecs::INVALID_ENTITY_ID)
      return RenderableEditableObject::setPos(p);
    if (!hasTransform())
      return false;
    if (!RenderableEditableObject::setPos(p))
      return false;
    updateEntityPosition();
    return true;
  }

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }

  void updateEntityPosition();

  void replaceEid(ecs::EntityId id)
  {
    resetEid();
    setupEid(id);
  }

  const Props &getProps() const { return props; }
  void setProps(const Props &p) { props = p; }

  ecs::EntityId getEid() const { return eid; }
  void resetEid();
  void updateLbb();

  static void save(DataBlock &blk, const ecs::Scene::EntityRecord &erec);
  bool hasTransform() const;
  bool canTransformFreely() const;
  ECSEntityObject *getParentObject();

  void hideObject(bool hide = true) override;
  void lockObject(bool lock = true) override;

private:
  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<ECSEntityObject> obj;
    ECSEntityObject::Props oldProps, redoProps;

  public:
    UndoPropsChange(ECSEntityObject *o) : obj(o) { oldProps = redoProps = obj->props; }

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

  void setPosOnCollision(Point3 pos);
  void rePlaceAllEntities();
  void setupEid(ecs::EntityId eid);
  void updateGeneratedName();

  void fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;

  void updateObjectFlags() const;
  void resetObjectFlags();

  Props props;
  ecs::EntityId eid;
  BBox3 lbb;

  eastl::unique_ptr<ECSEntityCreateData> removedEntityData;
  eastl::unique_ptr<ecs::ComponentsList> removedEntityComponents;

  mutable ecs::template_t objectLastTemplateId = ecs::INVALID_TEMPLATE_INDEX;
  mutable int objectFlags = 0;
};
