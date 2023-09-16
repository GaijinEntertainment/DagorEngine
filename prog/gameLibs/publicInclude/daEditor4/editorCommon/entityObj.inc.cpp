// required externals:
//   ECS_EVENT(CmdTeleportEntity, TMatrix /* newtm*/, bool /*hard teleport*/)

#include "entityObj.h"
#include "entityEditor.h"

#include <shaders/dag_dynSceneRes.h>
#include <daECS/core/entityManager.h>
#include <daECS/scene/scene.h>
#include <ecs/core/utility/ecsBlkUtils.h>
#include <ecs/anim/anim.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug3d.h>
#include <gamePhys/collision/collisionLib.h>
#include <memory/dag_framemem.h>
#include <math/dag_mathUtils.h>
#include <ecs/scripts/sqEntity.h>
#include <quirrel/sqEventBus/sqEventBus.h>


ECS_DECLARE_GET_FAST(TMatrix, transform);
ECS_REGISTER_EVENT(EditorToDeleteEntitiesEvent)
ECS_REGISTER_EVENT(EditorOnInitClonedEntity)

enum EntityObjectFlags
{
  ENTOBJ_FLAG_HAS_TRANSFORM = 0x0001,
  ENTOBJ_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM = 0x0002,
  ENTOBJ_FLAG_HAS_SPHERE_ZONE__RADIUS = 0x0004,
  ENTOBJ_FLAG_HAS_BOX_ZONE = 0x0008,
  ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMIN = 0x0010,
  ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMAX = 0x0020,
  ENTOBJ_FLAG_IGNORED_EDITABLE = 0x0040,
  ENTOBJ_FLAG_DEPTH_AWARE_EDIT_BOX = 0x0080,
};
void EntityObj::resetObjectFlags()
{
  objectLastTemplateId = ecs::INVALID_TEMPLATE_INDEX;
  objectFlags = 0;
}
void EntityObj::updateObjectFlags() const
{
  ecs::template_t curtid = g_entity_mgr->getEntityTemplateId(eid);
  if (objectLastTemplateId == curtid)
    return;

  objectLastTemplateId = curtid;
  objectFlags = 0;

  if (g_entity_mgr->has(eid, ECS_HASH("transform")))
    objectFlags |= ENTOBJ_FLAG_HAS_TRANSFORM;
  if (nullptr != g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("effect_animation__transform")))
    objectFlags |= ENTOBJ_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM;
  if (nullptr != g_entity_mgr->getNullable<float>(eid, ECS_HASH("sphere_zone__radius")))
    objectFlags |= ENTOBJ_FLAG_HAS_SPHERE_ZONE__RADIUS;
  if (g_entity_mgr->has(eid, ECS_HASH("box_zone")))
    objectFlags |= ENTOBJ_FLAG_HAS_BOX_ZONE;
  if (nullptr != g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMin")))
    objectFlags |= ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMIN;
  if (nullptr != g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("box_zone__boxMax")))
    objectFlags |= ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMAX;
  if (nullptr != g_entity_mgr->getNullable<ecs::Tag>(eid, ECS_HASH("ignoredEditable")))
    objectFlags |= ENTOBJ_FLAG_IGNORED_EDITABLE;
  if (nullptr != g_entity_mgr->getNullable<ecs::Tag>(eid, ECS_HASH("depthAwareEditBox")))
    objectFlags |= ENTOBJ_FLAG_DEPTH_AWARE_EDIT_BOX;
}

bool EntityObj::hasTransform() const { return (objectFlags & ENTOBJ_FLAG_HAS_TRANSFORM) != 0; }

EntityObj::EntityObj(const char *ent_name, ecs::EntityId _id)
{
  props.entityName = ent_name;
  props.placeOnCollision = true;
  props.useCollisionNorm = false;
  setupEid(_id);
}

void EntityObj::setupEid(ecs::EntityId _id)
{
  eid = _id;

  lbb = BBox3(Point3(-0.1, -0.1, -0.1), Point3(0.1, 0.1, 0.1));
  updateLbb();

  resetObjectFlags();
  updateObjectFlags();
  if (hasTransform())
    setWtm(g_entity_mgr->getOr(eid, ECS_HASH("transform"), TMatrix::IDENT));
}

void EntityObj::resetEid()
{
  removedEntData.reset();
  eid = ecs::INVALID_ENTITY_ID;
  resetObjectFlags();
}

void EntityObj::updateLbb()
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

void EntityObj::updateEntityPosition(bool)
{
  if (hasTransform())
  {
    if (isSelected())
    {
      const TMatrix *tm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!tm || *tm != getWtm())
      {
        g_entity_mgr->set(eid, ECS_HASH("transform"), getWtm());
        g_entity_mgr->sendEvent(eid, CmdTeleportEntity(getWtm(), true));
        EntityObjEditor::saveComponent(eid, "transform");

        if (HSQUIRRELVM vm = sqeventbus::get_vm())
          sqeventbus::send_event("entity_editor.onEntityMoved", Sqrat::Object(eid, vm));
      }
    }
    else
    {
      const TMatrix *tm = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!tm || *tm != getWtm())
        EntityObjEditor::saveComponent(eid, "transform");
    }
  }
}

void EntityObj::update(float)
{
  if (hasTransform())
    EditableObject::setWtm(ECS_GET_OR(eid, transform, TMatrix::IDENT));
}

bool EntityObj::isSelectedByRectangle(const IBBox2 &rect) const
{
  if (objectFlags & ENTOBJ_FLAG_IGNORED_EDITABLE)
    return false;
  return hasTransform() ? isBoxSelectedByRectangle(lbb, rect) : false;
}
bool EntityObj::isSelectedByPointClick(int x, int y) const
{
  if (objectFlags & ENTOBJ_FLAG_IGNORED_EDITABLE)
    return false;
  return hasTransform() ? isBoxSelectedByPointClick(lbb, x, y) : false;
}

static void proj_point(const TMatrix &wtm, const Point3 &in_from, const Point3 &in_to, Tab<Point3> &points)
{
  Point3 from = wtm * in_from;
  Point3 to = wtm * in_to;
  Point3 dir = to - from;
  float dist = length(dir);
  dir *= safeinv(dist);
  dacoll::traceray_normalized(from, dir, dist, nullptr, nullptr);
  points.push_back(from + dir * dist);
}

void EntityObj::render()
{
  updateObjectFlags();

  if (objectFlags & ENTOBJ_FLAG_IGNORED_EDITABLE)
    return;

  if (objectFlags & ENTOBJ_FLAG_HAS_TRANSFORM)
  {
    E3DCOLOR curColor = E3DCOLOR_MAKE(255, !isSelected() ? 255 : 0, !isSelected() ? 255 : 0, 100);

    if (objectFlags & ENTOBJ_FLAG_HAS_SPHERE_ZONE__RADIUS)
    {
      if (auto zone_rad = g_entity_mgr->getNullable<float>(eid, ECS_HASH("sphere_zone__radius")))
      {
        ::set_cached_debug_lines_wtm(getWtm());
        draw_cached_debug_sphere(ZERO<Point3>(), *zone_rad, curColor);
      }
    }

    if (objectFlags & ENTOBJ_FLAG_HAS_BOX_ZONE)
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

    if (objectFlags & (ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMIN | ENTOBJ_FLAG_HAS_BOX_ZONE__BOXMAX))
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

    if (objectFlags & ENTOBJ_FLAG_DEPTH_AWARE_EDIT_BOX)
    {
      const real dx = lbb[1].x - lbb[0].x;
      const real dy = lbb[1].y - lbb[0].y;
      const real dz = lbb[1].z - lbb[0].z;

      const E3DCOLOR c = isSelected() ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);
      const E3DCOLOR c2 = isSelected() ? E3DCOLOR(0xff, 0, 0, 64) : E3DCOLOR(0xff, 0xff, 0xff, 64);

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
      renderBox(lbb);
    }

    if (objectFlags & ENTOBJ_FLAG_HAS_EFFECT_ANIMATION__TRANSFORM)
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
void EntityObj::save(DataBlock &blk, const ecs::Scene::EntityRecord &erec) const
{
  G_ASSERT(!erec.templateName.empty());
  blk.setStr("_template", erec.templateName.c_str());
  ecs::components_to_blk(erec.clist.begin(), erec.clist.end(), blk, NULL, false);
  blk.removeParam("editableObj");
}

void EntityObj::onRemove(ObjectEditor *objEd)
{
  if (!g_entity_mgr->doesEntityExist(eid))
    return;
  if (eid != ecs::INVALID_ENTITY_ID)
    removedEntData.reset(new EntCreateData(eid));
  if (const ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    removedEntComps.reset(new ecs::ComponentsList(eastl::move(pRec->clist)));
  static_cast<EntityObjEditor *>(objEd)->destroyEntityDirect(eid);
  if (HSQUIRRELVM vm = sqeventbus::get_vm())
    sqeventbus::send_event("entity_editor.onEntityRemoved", Sqrat::Object(eid, vm));
  eid = ecs::INVALID_ENTITY_ID;
  resetObjectFlags();
}

void EntityObj::onAdd(ObjectEditor *objEd)
{
  propsChanged();

  if (name.empty())
  {
    // objEd->setUniqName(this, props.entityName);
    // eid + template name is enough unique name
    name = String(0, "%d: %s", (ecs::entity_id_t)getEid(), props.entityName.c_str());
  }
  if (eid == ecs::INVALID_ENTITY_ID && removedEntData) // recreate entity
  {
    makeSafeEntityComponentsRestoredFromUndo(removedEntData.get());

    setupEid(static_cast<EntityObjEditor *>(objEd)->createEntityDirect(removedEntData.get()));
    if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(eid))
      if (removedEntComps)
        pRec->clist = eastl::move(*removedEntComps);
  }
  removedEntData.reset();
  removedEntComps.reset();

  if (HSQUIRRELVM vm = sqeventbus::get_vm())
    sqeventbus::send_event("entity_editor.onEntityAdded", Sqrat::Object(eid, vm));
}

EditableObject *EntityObj::cloneObject() { return cloneObject(NULL); }

EditableObject *EntityObj::cloneObject(const char *template_name)
{
  EntityObj *o = NULL;
  if (eid == ecs::INVALID_ENTITY_ID)
    o = new EntityObj(name, ecs::INVALID_ENTITY_ID);
  else
  {
    EntCreateData cd(eid, template_name);
    ecs::EntityId clone_eid = static_cast<EntityObjEditor *>(getObjEditor())->createEntityDirect(&cd);
    o = new EntityObj(name, clone_eid);
    if (clone_eid != ecs::INVALID_ENTITY_ID)
    {
      ecs::g_scenes->getActiveScene().cloneEntityRecord(eid, o->eid, template_name);
      g_entity_mgr->sendEventImmediate(clone_eid, EditorOnInitClonedEntity());
    }
  }
  o->props = props;
  getObjEditor()->setUniqName(o, o->props.entityName);
  return o;
}

void EntityObj::recreateObject(ObjectEditor *objEd)
{
  if (eid == ecs::INVALID_ENTITY_ID)
    return;
  if (!g_entity_mgr->doesEntityExist(eid))
    return;

  eastl::unique_ptr<EntCreateData> newData(new EntCreateData(eid));
  const ecs::EntityId newEid = static_cast<EntityObjEditor *>(objEd)->createEntityDirect(newData.get());
  if (newEid == ecs::INVALID_ENTITY_ID)
    return;

  selectObject(false);

  removedEntData.swap(newData);
  if (const ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecord(eid))
    removedEntComps.reset(new ecs::ComponentsList(eastl::move(pRec->clist)));
  static_cast<EntityObjEditor *>(objEd)->destroyEntityDirect(eid);

  setupEid(newEid);
  if (ecs::Scene::EntityRecord *pRec = ecs::g_scenes->getActiveScene().findEntityRecordForModify(newEid))
    if (removedEntComps)
      pRec->clist = eastl::move(*removedEntComps);

  removedEntData.reset();
  removedEntComps.reset();
}

RendInstObj::RendInstObj(const rendinst::RendInstDesc &d) : riDesc(d)
{
  setWtm(rendinst::getRIGenMatrix(riDesc));
  lbb = rendinst::getRIGenBBox(riDesc);
  lastSelTimeMsec = get_time_msec();
}

void RendInstObj::updateEntityPosition(bool)
{
  mat44f m;
  v_mat44_make_from_43cu_unsafe(m, getWtm().m[0]);
  rendinst::moveRIGenExtra44(getRiEx(), m, true, false);
}

void RendInstObj::update(float)
{
  if (isSelected())
    lastSelTimeMsec = get_time_msec();
  else if (lastSelTimeMsec + 5000 < get_time_msec())
    if (objEditor)
      static_cast<EntityObjEditor *>(objEditor)->scheduleObjRemoval(this);
}
