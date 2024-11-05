// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMinimap.h"
#include "../minimapContext.h"
#include "../minimapState.h"
#include "ui/uiBindings.h"
#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <gui/dag_stdGuiRender.h>
#include <math/dag_mathUtils.h>
#include <perfMon/dag_cpuFreq.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvMinimap, bhv_minimap, cstr);


BhvMinimap::BhvMinimap() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvMinimap::onAttach(Element *elem)
{
  G_UNUSED(elem);
  G_ASSERT(MinimapState::get_from_element(elem));
}


void BhvMinimap::onDetach(Element *elem, DetachMode) { G_UNUSED(elem); }


int BhvMinimap::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  TIME_PROFILE(bhv_minimap_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Point2 elemHalfSize = elem->screenCoord.size * 0.5f;

  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (!mmState || !mmState->ctx)
  {
    LOGERR_ONCE("%s: MinimapState = %p, ctx == %p", __FUNCTION__, mmState, mmState ? mmState->ctx : nullptr);
    return 0;
  }

  float targetVisibleRadius = mmState->getTargetVisibleRadius();
  if (targetVisibleRadius > 0.f && dt > 0.f)
    mmState->setVisibleRadius(approach(mmState->getVisibleRadius(), targetVisibleRadius, dt, 0.2f));

  float visibleRadius = mmState->getVisibleRadius();

  TMatrix tm;
  mmState->calcTmFromView(tm, visibleRadius);
  Point3 eyePosition = mmState->ctx->getCurViewItm().getcol(3);

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

    if (worldPosValid)
    {
      Point3 tp = tm * worldPos;

      float scale = 1.0f / visibleRadius;
      Point2 sp(tp.x * scale, -tp.z * scale);

      float maxDistance = childElemData.RawGetSlotValue(strings->maxDistance, -1.0f);
      if (maxDistance > 0 && lengthSq(worldPos - eyePosition) > sqr(maxDistance))
        child->setHidden(true);
      else if (childElemData.RawGetSlotValue(strings->clampToBorder, false))
      {
        const float roundClipThreshold = 0.95;
        const float squareClipThreshold = childElemData.RawGetSlotValue(strings->squareClipThreshold, 1.0);
        bool isClipped = false;
        if (mmState->isSquare)
          isClipped = fabsf(sp.x) > squareClipThreshold || fabsf(sp.y) > squareClipThreshold;
        else
          isClipped = lengthSq(sp) > roundClipThreshold;

        if (isClipped)
          child->setGroupStateFlags(ui::S_CLIPPED);
        else
          child->clearGroupStateFlags(ui::S_CLIPPED);

        if (mmState->isSquare)
          MinimapContext::clamp_square(sp.x, sp.y, squareClipThreshold);
        else
        {
          const float maxR = roundClipThreshold;
          float lenSq = lengthSq(sp);
          if (lenSq > maxR)
            sp *= maxR / sqrtf(lenSq);
        }
      }
      else if (childElemData.RawGetSlotValue(strings->hideOutside, false))
      {
        // TODO: take radius into account
        if (mmState->isSquare)
          child->setHidden(fabsf(sp.x) > 1.0f || fabsf(sp.y) > 1.0f);
        else
          child->setHidden(lengthSq(sp) > 1.0f);
      }

      bool dirRotate = lookDirValid && childElemData.RawGetSlotValue(strings->dirRotate, false);
      if (dirRotate)
      {
        Point3 d = (tm % lookDir);
        float lookAng = atan2f(d.x, d.z);
        child->transform->rotate = lookAng;
      }

      child->transform->translate = Point2(sp.x * elemHalfSize.x, sp.y * elemHalfSize.y);
    }


    if (child->isHidden() != wasHidden)
      resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  return resultFlags;
}
