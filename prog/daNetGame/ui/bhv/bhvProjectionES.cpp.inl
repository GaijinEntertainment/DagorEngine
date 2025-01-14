// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvProjection.h"
#include "ui/uiShared.h"
#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <gui/dag_stdGuiRender.h>
#include <3d/dag_render.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>

#include <daECS/core/entityId.h>
#include <daECS/core/component.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>


#include <vr/vrGuiSurface.h>


using namespace darg;

static bool get_ui_entity_marker_pos(ecs::EntityId eid, Point3 &pos)
{
  if (const Point3 *node_pos = g_entity_mgr->getNullable<Point3>(eid, ECS_HASH("ui__node_pos")))
  {
    pos = *node_pos;
    return true;
  }

  if (const TMatrix *transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform")))
  {
    pos = transform->getcol(3);
    return true;
  }

  return false;
}


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvProjection, bhv_projection, cstr);


BhvProjection::BhvProjection() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


template <typename Callable>
static void visibility_ecs_query(ecs::EntityId eid, Callable c);


int BhvProjection::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_projection_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  bool selfWasHidden = elem->isHidden();

  Point2 screenHalfSize = StdGuiRender::screen_size() * 0.5f;
  Point2 screenCenter = screenHalfSize;
  Driver3dPerspective persp;
  if (!uishared::get_world_3d_persp(persp) || (screenHalfSize.x < 1 || screenHalfSize.y < 1))
  {
    elem->setHidden(true);
    return selfWasHidden ? 0 : R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  elem->setHidden(false);
  int resultFlags = selfWasHidden ? R_REBUILD_RENDER_AND_INPUT_LISTS : 0;

  Point2 viewPortLt = elem->screenCoord.screenPos;
  Point2 viewPortRb = viewPortLt + elem->screenCoord.size;

  const Sqrat::Object &dataKey = elem->csk->data;

  if (elem->children.size())
  {
    Element *child = elem->children[0];
    Sqrat::Object data = child->props.scriptDesc.RawGetSlot(dataKey);
    if (data.RawGetSlotValue(strings->isViewport, false))
    {
      viewPortLt = child->screenCoord.screenPos;
      viewPortRb = viewPortLt + child->screenCoord.size;
    }
  }

  Point2 viewPortCenter = (viewPortLt + viewPortRb) * 0.5f;
  Point2 viewPortHalfSize = (viewPortRb - viewPortLt) * 0.5f;

  for (int iChild = 0, nChildren = elem->children.size(); iChild < nChildren; ++iChild)
  {
    Element *child = elem->children[iChild];

    Sqrat::Object data = child->props.scriptDesc.RawGetSlot(dataKey);
    if (data.IsNull())
      continue;

    ecs::EntityId eid = data.RawGetSlotValue(strings->eid, ecs::INVALID_ENTITY_ID);
    float opacityForInvisibleAnimchar = data.RawGetSlotValue(strings->opacityForInvisibleAnimchar, 1.0f);
    float maxDistance = data.RawGetSlotValue(strings->maxDistance, -1.0f);
    float yOffs = data.RawGetSlotValue(strings->yOffs, 0.0f);
    bool alwaysVisible = data.RawGetSlotValue(strings->alwaysVisible, false);

    Point3 worldPos(0, 0, 0);
    bool worldPosValid = false;
    Sqrat::Object worldPosObj;

    if (!(worldPosObj = data.RawGetSlot(strings->worldPos)).IsNull())
    {
      worldPos = worldPosObj.Cast<Point3 &>();
      worldPosValid = true;
    }
    else if (eid != ecs::INVALID_ENTITY_ID && get_ui_entity_marker_pos(eid, worldPos))
      worldPosValid = true;

    bool wasHidden = child->isHidden();
    child->setHidden(true);
    float distanceToObject = length(worldPos - uishared::view_itm.getcol(3));
    if (!worldPosValid || (maxDistance > 0 && distanceToObject > maxDistance))
    {
      if (!wasHidden)
        resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
      continue;
    }

    worldPos.y += yOffs;

    if (!child->transform)
    {
      LOGERR_ONCE("%s: elem->children[%d]->transform == NULL", __FUNCTION__, iChild);
      continue;
    }

    Point2 vp(0, 0);
    float distScale = 1.0f;

    bool itemVisible = false;
    bool animcharVisible = false;
    visibility_ecs_query(eid, [&](const bool *item__visibleCached, const bool *item__visible, bool animchar__visible = true) {
      itemVisible = item__visibleCached ? *item__visibleCached : item__visible ? *item__visible : true;
      animcharVisible = animchar__visible;
    });

    if (eid == ecs::INVALID_ENTITY_ID || alwaysVisible || itemVisible)
    {
      Point3 sp = uishared::view_tm * worldPos;
      Point3 deltaPos = worldPos - uishared::view_itm.getcol(3);

      bool inElem = false;
      float minDistance = data.RawGetSlotValue(strings->minDistance, 0.1f);
      if (distanceToObject > minDistance && sp.z > 0)
      {
        if (vrgui::is_inited())
        {
          Point3 intersection;
          vrgui::intersect(Point3(0, 0, 0), uishared::view_tm % normalize(deltaPos), vp, intersection);
          vp -= screenCenter;
        }
        else
        {
          vp = Point2(sp.x / sp.z * persp.wk * screenHalfSize.x, -sp.y / sp.z * persp.hk * screenHalfSize.y);
        }

        if (fabsf(vp.x + screenCenter.x - viewPortCenter.x) <= viewPortHalfSize.x &&
            fabsf(vp.y + screenCenter.y - viewPortCenter.y) <= viewPortHalfSize.y)
        {
          inElem = true;
          child->setHidden(false);
        }
      }

      float opacity = animcharVisible ? 1.0f : opacityForInvisibleAnimchar;
      float opacityRangeViewDistance = data.RawGetSlotValue(strings->opacityRangeViewDistance, 0.0f);
      if (distanceToObject > opacityRangeViewDistance)
      {
        Point2 opacityRangeX(0, 0), opacityRangeY(0, 0);
        Sqrat::Object tmpSq = data.RawGetSlot(strings->opacityRangeX);
        const Point2 *tmp = tmpSq.Cast<const Point2 *>();
        if (tmp)
          opacityRangeX = *tmp;
        tmpSq = data.RawGetSlot(strings->opacityRangeY);
        tmp = tmpSq.Cast<const Point2 *>();
        if (tmp)
          opacityRangeY = *tmp;

        if (data.RawGetSlotValue(strings->opacityIgnoreFov, false))
        {
          opacityRangeX /= persp.wk;
          opacityRangeY /= persp.hk;
        }

        if (opacityRangeX[0] > 0 && opacityRangeX[1] > 0 && opacityRangeY[0] > 0 && opacityRangeY[1] > 0)
        {
          opacity *= cvt(fabsf(safediv(sp.x, sp.z)), opacityRangeX[0], opacityRangeX[1], 1.0f, 0.0f) *
                     cvt(fabsf(safediv(sp.y, sp.z)), opacityRangeY[0], opacityRangeY[1], 1.0f, 0.0f);
          if (opacity < 1e-6f)
          {
            child->setHidden(true);
            if (!wasHidden)
              resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
            continue;
          }
        }
      }
      if (opacity < 0.9999f)
        child->props.setCurrentOpacity(opacity);
      else
        child->props.resetCurrentOpacity();

      float arrowRotation = 0;

      Point2 dir = Point2(0, 0);
      if (!inElem && data.RawGetSlotValue(strings->clampToBorder, false))
      {
        // bend from PoV projection to view-from-top projection when point is far back
        float zAngle = safe_atan2(-sp.y, sp.x);
        float yAngle = safe_atan2(-sp.z, sp.x);

        float backCos = clamp(-normalize(deltaPos) * uishared::view_itm.getcol(2) - 0.25f, 0.0f, 0.75f) * (1.0 / 0.75f);
        float angDiff = yAngle - zAngle;
        if (angDiff > PI)
          angDiff = angDiff - TWOPI;
        else if (angDiff < -PI)
          angDiff = angDiff + TWOPI;

        float mixedAngle = zAngle + angDiff * backCos;
        float s, c;
        sincos(mixedAngle, s, c);

        float diagonalPx = length(screenHalfSize);

        // clamp to viewport box
        Point2 d = Point2(c * diagonalPx, s * diagonalPx);
        dir = normalize(d);
        if (d.x + screenHalfSize.x < viewPortLt.x)
          d *= (screenHalfSize.x - viewPortLt.x) / fabsf(d.x);
        else if (d.x + screenHalfSize.x > viewPortRb.x)
          d *= (viewPortRb.x - screenHalfSize.x) / fabsf(d.x);

        if (d.y + screenHalfSize.y < viewPortLt.y)
          d *= (screenHalfSize.y - viewPortLt.y) / fabsf(d.y);
        else if (d.y + screenHalfSize.y > viewPortRb.y)
          d *= (viewPortRb.y - screenHalfSize.y) / fabsf(d.y);

        vp = d;
        child->setHidden(false);

        arrowRotation = mixedAngle;
      }

      if (!child->isHidden())
      {
        // float distance = ::max(sp.z, minDistance);
        float distance = ::max(minDistance, deltaPos.length());
        float distScaleFactor = data.RawGetSlotValue(strings->distScaleFactor, 0.0f);
        float distK = ::clamp(safediv(2.0f * persp.hk, distance), 0.0f, 1.0f);
        distScale = 1.0f - (1.0f - distK) * distScaleFactor;

        // rotate (or hide) child arrows
        for (int iSubChild = 0, nSubChildren = child->children.size(); iSubChild < nSubChildren; ++iSubChild)
        {
          Element *sub = child->children[iSubChild];
          int markerFlags = sub->props.getInt(strings->markerFlags, 0);
          float clampBorderOffset = sub->props.getFloat(strings->clampBorderOffset, 0.0f);

          if (clampBorderOffset > 0.0f && sub->transform)
          {
            float offsetLen = clampBorderOffset;
            sub->transform->translate = dir * -offsetLen;
          }

          if ((markerFlags & MARKER_ARROW) && sub->transform)
          {
            if (sub->isHidden() != inElem)
              resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
            sub->setHidden(inElem);
            sub->transform->rotate = arrowRotation + HALFPI;
          }
          else if (markerFlags & MARKER_SHOW_ONLY_IN_VIEWPORT)
          {
            if (sub->isHidden() == inElem)
            {
              sub->setHidden(!inElem);
              resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
            }
          }
          else if (markerFlags & MARKER_SHOW_ONLY_WHEN_CLAMPED)
          {
            if (sub->isHidden() != inElem)
            {
              sub->setHidden(inElem);
              resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
            }
          }

          if ((markerFlags & MARKER_KEEP_SCALE) && sub->transform)
          {
            float scaleInv = 1.0f / max(0.1f, distScale);
            sub->transform->scale = Point2(scaleInv, scaleInv);
          }
        }
      }
    }

    child->transform->translate = vp;
    child->transform->scale = Point2(distScale, distScale);

    if (child->isHidden() != wasHidden)
      resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
  }

  return resultFlags;
}
