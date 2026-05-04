// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTiledMap.h"
#include "../tiledMapContext.h"
#include "ui/uiBindings.h"
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/scripts/sqEntity.h>

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTiledMap, bhv_tiled_map, cstr);


BhvTiledMap::BhvTiledMap() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvTiledMap::onAttach(Element *elem)
{
  G_UNUSED(elem);
  G_ASSERT(TiledMapContext::get_from_element(elem));
}


void BhvTiledMap::onDetach(Element *elem, DetachMode) { G_UNUSED(elem); }


int BhvTiledMap::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  (void)dt;
  TIME_PROFILE(bhv_tiled_map_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  TiledMapContext *tiledMapCtx = TiledMapContext::get_from_element(elem);
  if (!tiledMapCtx)
  {
    LOGERR_ONCE("%s: TiledMapContext = %p", __FUNCTION__, tiledMapCtx);
    return 0;
  }

  TMatrix tm;
  tiledMapCtx->calcTmFromView(tm);
  Point3 eyePosition = tiledMapCtx->getCurViewItm().getcol(3);

  int resultFlags = 0;

  const Sqrat::Object &dataKey = elem->csk->data;

  for (int iChild = 0, nChildren = elem->children.size(); iChild < nChildren; ++iChild)
  {
    Element *child = elem->children[iChild];
    if (!child->transform)
      continue;

    Sqrat::Object childElemData = child->props.scriptDesc.RawGetSlot(dataKey);
    if (childElemData.IsNull())
      continue;

    Point3 worldPos(0, 0, 0), lookDir(0, 0, 0);
    bool worldPosValid = false, lookDirValid = false;
    Sqrat::Object worldPosObj;
    ecs::EntityId eid = ecs::INVALID_ENTITY_ID;

    bool useAttrTransform = false;
    if ((eid = childElemData.RawGetSlotValue(strings->zoneEid, ecs::INVALID_ENTITY_ID)) != ecs::INVALID_ENTITY_ID)
      useAttrTransform = true;
    else if ((eid = childElemData.RawGetSlotValue(strings->eid, ecs::INVALID_ENTITY_ID)) != ecs::INVALID_ENTITY_ID)
      useAttrTransform = true;

    if (useAttrTransform)
    {
      const TMatrix *transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform_lastFrame"));
      if (!transform)
        transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
      if (!transform)
      {
        if (const ecs::EntityId *attachedTo = g_entity_mgr->getNullable<ecs::EntityId>(eid, ECS_HASH("game_effect__attachedTo")))
        {
          transform = g_entity_mgr->getNullable<TMatrix>(*attachedTo, ECS_HASH("transform_lastFrame"));
          if (!transform)
            transform = g_entity_mgr->getNullable<TMatrix>(*attachedTo, ECS_HASH("transform"));
        }
      }
      if (transform)
      {
        worldPos = transform->getcol(3);
        lookDir = transform->getcol(0);
        worldPosValid = lookDirValid = true;
      }
    }
    else if (!(worldPosObj = childElemData.RawGetSlot(strings->worldPos)).IsNull())
    {
      worldPos = worldPosObj.Cast<Point3 &>();
      worldPosValid = true;
    }

    bool wasHidden = child->isHidden();
    child->setHidden(!worldPosValid);

    if (worldPosValid && tiledMapCtx->isViewInitialized())
    {
      // zero of this transform should be the center of the map element
      Point2 screenPos = tiledMapCtx->worldToMap(worldPos);

      BBox2 screenBbox;
      screenBbox += elem->screenCoord.size / 2.f;
      screenBbox += -elem->screenCoord.size / 2.f;

      float maxDistance = childElemData.RawGetSlotValue(strings->maxDistance, -1.0f);
      if (maxDistance > 0 && lengthSq(worldPos - eyePosition) > sqr(maxDistance))
        child->setHidden(true);
      else if (childElemData.RawGetSlotValue(strings->clampToBorder, false))
      {
        bool isClipped = !(screenBbox & screenPos);
        if (isClipped)
          child->setGroupStateFlags(ui::S_CLIPPED);
        else
          child->clearGroupStateFlags(ui::S_CLIPPED);

        Point2 childOffset = child->screenCoord.size / 2.f;
        screenPos.x = eastl::clamp(screenPos.x, screenBbox.left() + childOffset.x, screenBbox.right() - childOffset.x);
        screenPos.y = eastl::clamp(screenPos.y, screenBbox.top() + childOffset.y, screenBbox.bottom() - childOffset.y);
      }
      else if (childElemData.RawGetSlotValue(strings->hideOutside, false))
        child->setHidden(!(screenBbox & screenPos));

      bool dirRotate = lookDirValid && childElemData.RawGetSlotValue(strings->dirRotate, false);
      if (dirRotate)
      {
        Point3 d = (lookDir % tm);         // inversed order because of the left-handed coordinate system
        float lookAng = atan2f(d.x, -d.z); // inversed z because of the left-handed coordinate system
        child->transform->rotate = lookAng;
      }

      child->transform->translate = round(screenPos);
      G_ASSERT(isfinite(child->transform->translate.x) && isfinite(child->transform->translate.y));
    }

    if (child->isHidden() != wasHidden)
      resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  return resultFlags;
}
