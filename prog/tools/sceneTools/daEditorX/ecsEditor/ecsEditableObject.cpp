// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsEditableObject.h"
#include "ecsObjectEditor.h"
#include <ecsPropPanel/ecsEditableObjectPropertyPanel.h>

#include <daECS/scene/scene.h>
#include <de3_interface.h>
#include <debug/dag_debug3d.h>
#include <ecs/anim/anim.h>
#include <ecs/core/utility/ecsBlkUtils.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_rect.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_math2d.h>
#include <math/dag_rayIntersectBox.h>
#include <memory/dag_framemem.h>
#include <shaders/dag_dynSceneRes.h>

using editorcore_extapi::dagRender;

void ecs_editor_update_visual_entity_tm(ecs::EntityId eid, const TMatrix &tm);

ECS_DECLARE_GET_FAST(TMatrix, transform);
ECS_REGISTER_EVENT(EditorToDeleteEntitiesEvent)
ECS_REGISTER_EVENT(EditorOnInitClonedEntity)

enum EditableObjectFlags
{
  EDITABLE_OBJECT_FLAG_HAS_TRANSFORM = 0x0001,
  EDITABLE_OBJECT_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM = 0x0002,
  EDITABLE_OBJECT_FLAG_HAS_SPHERE_ZONE__RADIUS = 0x0004,
  EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE = 0x0008,
  EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMIN = 0x0010,
  EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMAX = 0x0020,
  EDITABLE_OBJECT_FLAG_IGNORED_EDITABLE = 0x0040,
  EDITABLE_OBJECT_FLAG_DEPTH_AWARE_EDIT_BOX = 0x0080,
};

static bool is_box_selected_by_point_click(const BBox3 &box, const TMatrix &wtm, IGenViewportWnd *vp, int x, int y)
{
  Point3 dir, p0;
  vp->clientToWorld(Point2(x, y), p0, dir);

  float out_t;
  return ray_intersect_box(p0, dir, box, wtm, out_t);
}

static bool is_box_selected_by_rectangle(const BBox3 &box, const TMatrix &wtm, IGenViewportWnd *vp, const EcRect &rect)
{
  real z;
  Point2 cp[8];
  BBox2 box2;
  bool in_frustum = false;

#define TEST_POINT(i, P)                                                                         \
  in_frustum |= vp->worldToClient(wtm * P, cp[i], &z) && z > 0;                                  \
  if (z > 0 && rect.l <= cp[i].x && rect.t <= cp[i].y && cp[i].x <= rect.r && cp[i].y <= rect.b) \
    return true;                                                                                 \
  else                                                                                           \
    box2 += cp[i];

#define TEST_SEGMENT(i, j)                          \
  if (::isect_line_segment_box(cp[i], cp[j], rbox)) \
  return true

  for (int i = 0; i < 8; i++)
  {
    TEST_POINT(i, box.point(i));
  }

  if (!in_frustum)
    return false;
  BBox2 rbox(Point2(rect.l, rect.t), Point2(rect.r, rect.b));
  if (!(box2 & rbox))
    return false;

  TEST_SEGMENT(0, 4);
  TEST_SEGMENT(4, 5);
  TEST_SEGMENT(5, 1);
  TEST_SEGMENT(1, 0);
  TEST_SEGMENT(2, 6);
  TEST_SEGMENT(6, 7);
  TEST_SEGMENT(7, 3);
  TEST_SEGMENT(3, 2);
  TEST_SEGMENT(0, 2);
  TEST_SEGMENT(1, 3);
  TEST_SEGMENT(4, 6);
  TEST_SEGMENT(5, 7);

#undef TEST_POINT
#undef TEST_SEGMENT

  return is_box_selected_by_point_click(box, wtm, vp, rect.l, rect.t);
}

static void make_safe_entity_components_restored_from_undo(ECSEntityCreateData *removedData)
{
  // always disable cameras to avoid conflicts with free_cam
  const ecs::component_type_t typeBool = ECS_HASH("bool").hash;
  const ecs::component_t nameCamActive = ECS_HASH("camera__active").hash;
  if (g_entity_mgr->getTemplateDB().getComponentType(nameCamActive) == typeBool)
  {
    if (removedData)
      for (auto &attr : removedData->attrs)
        if (attr.name == nameCamActive)
          attr.second = false;
  }
}

static void proj_point(const TMatrix &wtm, const Point3 &in_from, const Point3 &in_to, Tab<Point3> &points)
{
  Point3 from = wtm * in_from;
  Point3 to = wtm * in_to;
  Point3 dir = to - from;
  float dist = length(dir);
  dir *= safeinv(dist);
  IEditorCoreEngine::get()->traceRay(from, dir, dist);
  points.push_back(from + dir * dist);
}

void ECSEditableObject::resetObjectFlags()
{
  objectLastTemplateId = ecs::INVALID_TEMPLATE_INDEX;
  objectFlags = 0;
}

void ECSEditableObject::updateObjectFlags() const
{
  ecs::template_t curtid = g_entity_mgr->getEntityTemplateId(eid);
  if (objectLastTemplateId == curtid)
    return;

  objectLastTemplateId = curtid;
  objectFlags = 0;

  if (g_entity_mgr->has(eid, ECS_HASH("transform")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_TRANSFORM;
  if (nullptr != g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("effect_animation__transform")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM;
  if (nullptr != g_entity_mgr->getNullable<float>(eid, ECS_HASH("sphere_zone__radius")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_SPHERE_ZONE__RADIUS;
  if (g_entity_mgr->has(eid, ECS_HASH("box_zone")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE;
  if (nullptr != g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMin")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMIN;
  if (nullptr != g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMax")))
    objectFlags |= EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMAX;
  if (nullptr != g_entity_mgr->getNullable<ecs::Tag>(eid, ECS_HASH("ignoredEditable")))
    objectFlags |= EDITABLE_OBJECT_FLAG_IGNORED_EDITABLE;
  if (nullptr != g_entity_mgr->getNullable<ecs::Tag>(eid, ECS_HASH("depthAwareEditBox")))
    objectFlags |= EDITABLE_OBJECT_FLAG_DEPTH_AWARE_EDIT_BOX;
}

bool ECSEditableObject::hasTransform() const { return (objectFlags & EDITABLE_OBJECT_FLAG_HAS_TRANSFORM) != 0; }

ECSEditableObject::ECSEditableObject(ecs::EntityId _id)
{
  props.placeOnCollision = true;
  props.useCollisionNorm = false;
  setupEid(_id);
}

void ECSEditableObject::updateGeneratedName()
{
  setName(String(0, "%s (%u)", g_entity_mgr->getOr(eid, ECS_HASH("animchar__res"), g_entity_mgr->getEntityTemplateName(eid)),
    (ecs::entity_id_t)getEid()));
}

void ECSEditableObject::setupEid(ecs::EntityId _id)
{
  eid = _id;

  lbb = BBox3(Point3(-0.1, -0.1, -0.1), Point3(0.1, 0.1, 0.1));
  updateLbb();

  resetObjectFlags();
  updateObjectFlags();
  if (hasTransform())
    setWtm(g_entity_mgr->getOr(eid, ECS_HASH("transform"), TMatrix::IDENT));

  updateGeneratedName();
}

void ECSEditableObject::resetEid()
{
  removedEntityData.reset();
  eid = ecs::INVALID_ENTITY_ID;
  resetObjectFlags();
}

void ECSEditableObject::updateLbb()
{
  AnimV20::AnimcharRendComponent *animChar =
    g_entity_mgr->getNullableRW<AnimV20::AnimcharRendComponent>(eid, ECS_HASH("animchar_render"));
  if (animChar)
  {
    lbb = animChar->getSceneInstance()->getLocalBoundingBox();
    if (g_entity_mgr->getOr(eid, ECS_HASH("animchar__turnDir"), false))
    {
      Point3 lim0 = lbb.lim[0];
      Point3 lim1 = lbb.lim[1];

      lbb.lim[0] = Point3(-lim1.z, lim0.y, lim0.x);
      lbb.lim[1] = Point3(-lim0.z, lim1.y, lim1.x);
    }
  }
}

void ECSEditableObject::updateEntityPosition()
{
  if (hasTransform())
  {
    if (isSelected() || (objEditor && static_cast<ECSObjectEditor *>(objEditor)->isSampleObject(*this)))
    {
      const TMatrix *tm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!tm || *tm != getWtm())
      {
        g_entity_mgr->set(eid, ECS_HASH("transform"), getWtm());
        ECSObjectEditor::saveComponent(eid, "transform");
        ecs_editor_update_visual_entity_tm(eid, getWtm());
      }
    }
    else
    {
      const TMatrix *tm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!tm || *tm != getWtm())
      {
        ECSObjectEditor::saveComponent(eid, "transform");
        ecs_editor_update_visual_entity_tm(eid, getWtm());
      }
    }
  }
}

void ECSEditableObject::update(float)
{
  if (hasTransform())
  {
    const TMatrix ecsTm = ECS_GET_OR(eid, transform, TMatrix::IDENT);
    if (ecsTm != getWtm())
    {
      RenderableEditableObject::setWtm(ecsTm);
      ecs_editor_update_visual_entity_tm(eid, ecsTm);
    }
  }
}

bool ECSEditableObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  if (objectFlags & EDITABLE_OBJECT_FLAG_IGNORED_EDITABLE)
    return false;
  return hasTransform() && is_box_selected_by_rectangle(lbb, matrix, vp, rect);
}

bool ECSEditableObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  if (objectFlags & EDITABLE_OBJECT_FLAG_IGNORED_EDITABLE)
    return false;
  return hasTransform() && is_box_selected_by_point_click(lbb, matrix, vp, x, y);
}

void ECSEditableObject::render()
{
  updateObjectFlags();

  if (objectFlags & EDITABLE_OBJECT_FLAG_IGNORED_EDITABLE)
    return;

  if (objectFlags & EDITABLE_OBJECT_FLAG_HAS_TRANSFORM)
  {
    E3DCOLOR curColor = E3DCOLOR_MAKE(255, !isSelected() ? 255 : 0, !isSelected() ? 255 : 0, 100);

    if (objectFlags & EDITABLE_OBJECT_FLAG_HAS_SPHERE_ZONE__RADIUS)
    {
      if (auto zone_rad = g_entity_mgr->getNullable<float>(eid, ECS_HASH("sphere_zone__radius")))
      {
        ::set_cached_debug_lines_wtm(getWtm());
        draw_cached_debug_sphere(ZERO<Point3>(), *zone_rad, curColor);
      }
    }

    if (objectFlags & EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE)
    {
      if (g_entity_mgr->has(eid, ECS_HASH("box_zone")))
      {
        const Point3 scale(1, 1, 1);
        TMatrix wtm = getWtm();
        ::set_cached_debug_lines_wtm(wtm);
        ::draw_cached_debug_box(BBox3(-scale * 0.5f, scale * 0.5f), curColor);
        if (isSelected())
        {
          // project borders on collision
          Tab<Point3> projPts(framemem_ptr());
          const int num_iter = 20;
          ::set_cached_debug_lines_wtm(TMatrix::IDENT);
          for (int i = 0; i < num_iter; ++i)
            proj_point(wtm, Point3(cvt(i, 0, num_iter - 1, -1.f, 1.f), +scale.y, -1.f) * 0.5f,
              Point3(cvt(i, 0, num_iter - 1, -1.f, 1.f), -scale.y, -1.f) * 0.5f, projPts);
          for (int i = 0; i < num_iter; ++i)
            proj_point(wtm, Point3(1.f, +scale.y, cvt(i, 0, num_iter - 1, -1.f, 1.f)) * 0.5f,
              Point3(1.f, -scale.y, cvt(i, 0, num_iter - 1, -1.f, 1.f)) * 0.5f, projPts);
          for (int i = 0; i < num_iter; ++i)
            proj_point(wtm, Point3(cvt(i, 0, num_iter - 1, 1.f, -1.f), +scale.y, 1.f) * 0.5f,
              Point3(cvt(i, 0, num_iter - 1, 1.f, -1.f), -scale.y, 1.f) * 0.5f, projPts);
          for (int i = 0; i < num_iter; ++i)
            proj_point(wtm, Point3(-1.f, +scale.y, cvt(i, 0, num_iter - 1, 1.f, -1.f)) * 0.5f,
              Point3(-1.f, -scale.y, cvt(i, 0, num_iter - 1, 1.f, -1.f)) * 0.5f, projPts);
          draw_cached_debug_line(projPts.data(), projPts.size(), E3DCOLOR_MAKE(255, 0, 0, 255));
        }
      }
    }

    if (objectFlags & (EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMIN | EDITABLE_OBJECT_FLAG_HAS_BOX_ZONE__BOXMAX))
    {
      const Point3 *box_zone__boxMin = g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMin"));
      const Point3 *box_zone__boxMax = g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMax"));
      if (box_zone__boxMin && box_zone__boxMax)
      {
        TMatrix wtm = getWtm();
        ::set_cached_debug_lines_wtm(wtm);
        ::draw_cached_debug_box(BBox3(*box_zone__boxMin, *box_zone__boxMax), curColor);
      }
    }

    if (objectFlags & EDITABLE_OBJECT_FLAG_DEPTH_AWARE_EDIT_BOX)
    {
      const real dx = lbb[1].x - lbb[0].x;
      const real dy = lbb[1].y - lbb[0].y;
      const real dz = lbb[1].z - lbb[0].z;

      E3DCOLOR c = isSelected() ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);
      E3DCOLOR c2 = isSelected() ? E3DCOLOR(0xff, 0, 0, 64) : E3DCOLOR(0xff, 0xff, 0xff, 64);
      if (static_cast<ECSObjectEditor *>(getObjEditor())->getParentSelectionHoveredObject() == this)
      {
        c = E3DCOLOR(0xff, 0, 0xff);
        c2 = E3DCOLOR(0xff, 0, 0xff, 64);
      }

      ::set_cached_debug_lines_wtm(getWtm());

      ::draw_cached_debug_line_twocolored(lbb[0], lbb[0] + Point3(dx, 0, 0), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(0, dy, 0), lbb[0] + Point3(dx, dy, 0), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(0, 0, dz), lbb[0] + Point3(dx, 0, dz), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(0, dy, dz), lbb[0] + Point3(dx, dy, dz), c, c2);

      ::draw_cached_debug_line_twocolored(lbb[0], lbb[0] + Point3(0, 0, dz), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(dx, 0, 0), lbb[0] + Point3(dx, 0, dz), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(0, dy, 0), lbb[0] + Point3(0, dy, dz), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(dx, dy, 0), lbb[0] + Point3(dx, dy, dz), c, c2);

      ::draw_cached_debug_line_twocolored(lbb[0], lbb[0] + Point3(0, dy, 0), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(dx, 0, 0), lbb[0] + Point3(dx, dy, 0), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(0, 0, dz), lbb[0] + Point3(0, dy, dz), c, c2);
      ::draw_cached_debug_line_twocolored(lbb[0] + Point3(dx, 0, dz), lbb[0] + Point3(dx, dy, dz), c, c2);
    }
    else
    {
      E3DCOLOR color = isSelected() ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);
      if (static_cast<ECSObjectEditor *>(getObjEditor())->getParentSelectionHoveredObject() == this)
        color = E3DCOLOR(0xff, 0, 0xff);

      dagRender->setLinesTm(getWtm());
      dagRender->renderBox(lbb, color);
    }

    if (objectFlags & EDITABLE_OBJECT_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM)
    {
      const TMatrix *animWtm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("effect_animation__transform"));
      if (animWtm)
      {
        E3DCOLOR animColor = E3DCOLOR_MAKE(0, 255, 0, 100);
        const Point3 scale(1, 1, 1);
        ::set_cached_debug_lines_wtm(*animWtm);
        draw_cached_debug_box(BBox3(-scale * 0.5f, scale * 0.5f), animColor);
      }
    }

    if (isSelected())
    {
      ::set_cached_debug_lines_wtm(getWtm());
      ::draw_cached_debug_line(Point3(0, 0, 0), Point3(1, 0, 0), E3DCOLOR_MAKE(255, 0, 0, 255));
      ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, 1, 0), E3DCOLOR_MAKE(0, 255, 0, 255));
      ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, 0, 1), E3DCOLOR_MAKE(0, 0, 255, 255));
    }
  }
  else
  {
    const E3DCOLOR c = E3DCOLOR(0xff, 0xff, 0xff);
    ::set_cached_debug_lines_wtm(getWtm());
    ::draw_cached_debug_line(Point3(-1, 0, 0), Point3(1, 0, 0), c);
    ::draw_cached_debug_line(Point3(0, 0, -1), Point3(0, 0, 1), c);
    ::draw_cached_debug_line(Point3(0, 0, 0), Point3(0, 1, 0), c);
  }
}

void ECSEditableObject::save(DataBlock &blk, const ecs::Scene::EntityRecord &erec) const
{
  G_ASSERT(!erec.templateName.empty());
  blk.setStr("_template", erec.templateName.c_str());
  ecs::components_to_blk(erec.clist.begin(), erec.clist.end(), blk, nullptr, false);
  blk.removeParam("editableObj");
}

void ECSEditableObject::onRemove(ObjectEditor *objEd)
{
  if (!g_entity_mgr->doesEntityExist(eid))
    return;
  if (eid != ecs::INVALID_ENTITY_ID)
    removedEntityData.reset(new ECSEntityCreateData(eid));
  if (const ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    removedEntityComponents.reset(new ecs::ComponentsList(eastl::move(pRec->clist)));
  static_cast<ECSObjectEditor *>(objEd)->destroyEntityDirect(eid);
  eid = ecs::INVALID_ENTITY_ID;
  resetObjectFlags();
}

void ECSEditableObject::onAdd(ObjectEditor *objEd)
{
  if (eid == ecs::INVALID_ENTITY_ID && removedEntityData) // recreate entity
  {
    make_safe_entity_components_restored_from_undo(removedEntityData.get());

    setupEid(static_cast<ECSObjectEditor *>(objEd)->createEntityDirect(removedEntityData.get()));
    if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(eid))
      if (removedEntityComponents)
        pRec->clist = eastl::move(*removedEntityComponents);
  }
  removedEntityData.reset();
  removedEntityComponents.reset();
}

ECSEditableObject *ECSEditableObject::cloneObject() { return cloneObject(nullptr); }

ECSEditableObject *ECSEditableObject::cloneObject(const char *template_name)
{
  ECSEditableObject *o = nullptr;
  if (eid == ecs::INVALID_ENTITY_ID)
    o = new ECSEditableObject(ecs::INVALID_ENTITY_ID);
  else
  {
    ECSEntityCreateData cd(eid, template_name);
    ecs::EntityId clone_eid = static_cast<ECSObjectEditor *>(getObjEditor())->createEntityDirect(&cd);
    o = new ECSEditableObject(clone_eid);
    if (clone_eid != ecs::INVALID_ENTITY_ID)
    {
      ecs::g_scenes->getActiveScene().cloneEntityRecord(eid, o->eid, template_name);
      g_entity_mgr->sendEventImmediate(clone_eid, EditorOnInitClonedEntity());
    }
  }
  o->props = props;
  return o;
}

void ECSEditableObject::recreateObject(ObjectEditor *objEd)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return;
  if (!g_entity_mgr->doesEntityExist(eid))
    return;

  eastl::unique_ptr<ECSEntityCreateData> newData(new ECSEntityCreateData(eid));
  const ecs::EntityId newEid = static_cast<ECSObjectEditor *>(objEd)->createEntityDirect(newData.get());
  if (newEid == ecs::INVALID_ENTITY_ID)
    return;

  selectObject(false);

  removedEntityData.swap(newData);
  if (const ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    removedEntityComponents.reset(new ecs::ComponentsList(eastl::move(pRec->clist)));
  static_cast<ECSObjectEditor *>(objEd)->destroyEntityDirect(eid);

  setupEid(newEid);
  if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(newEid))
    if (removedEntityComponents)
      pRec->clist = eastl::move(*removedEntityComponents);

  removedEntityData.reset();
  removedEntityComponents.reset();
}

bool ECSEditableObject::canTransformFreely() const
{
  const ecs::EntityId *parentEid = g_entity_mgr->getNullable<ecs::EntityId>(getEid(), ECS_HASH("hierarchy_parent"));
  return !parentEid || !*parentEid || g_entity_mgr->has(getEid(), ECS_HASH("hierarchy_parent_last_transform"));
}

ECSEditableObject *ECSEditableObject::getParentObject()
{
  const ecs::EntityId *parentEid = g_entity_mgr->getNullable<ecs::EntityId>(getEid(), ECS_HASH("hierarchy_parent"));
  if (!parentEid || !*parentEid)
    return nullptr;

  EditableObject *parentObject = static_cast<ECSObjectEditor *>(getObjEditor())->getObjectByEid(*parentEid);
  return RTTI_cast<ECSEditableObject>(parentObject);
}

void ECSEditableObject::fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  ECSEditableObjectPropertyPanel editableObjectPropertyPanel(panel);

  if (objects.size() == 1 && objects[0] == this)
    editableObjectPropertyPanel.fillProps(getEid(), static_cast<ECSObjectEditor *>(getObjEditor()));
  else
    editableObjectPropertyPanel.fillProps(ecs::INVALID_ENTITY_ID, nullptr);
}

void ECSEditableObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (objects.size() != 1 || objects[0] != this)
    return;

  ECSEditableObjectPropertyPanel editableObjectPropertyPanel(panel);
  editableObjectPropertyPanel.onPPChange(pid, edit_finished, getEid(), static_cast<ECSObjectEditor *>(getObjEditor()));
}

void ECSEditableObject::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (objects.size() != 1 || objects[0] != this)
    return;

  ECSEditableObjectPropertyPanel editableObjectPropertyPanel(panel);
  editableObjectPropertyPanel.onPPBtnPressed(pid, getEid(), static_cast<ECSObjectEditor *>(getObjEditor()));
}
