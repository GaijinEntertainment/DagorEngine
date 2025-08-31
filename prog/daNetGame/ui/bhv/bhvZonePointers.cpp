// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvZonePointers.h"
#include "ui/uiShared.h"
#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <gui/dag_stdGuiRender.h>
#include <drv/3d/dag_decl.h>
#include <3d/dag_render.h>
#include <math/dag_mathUtils.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

#include <sqrat.h>

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvZonePointers, bhv_zone_pointers, cstr);


BhvZonePointers::BhvZonePointers() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvZonePointers::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Driver3dPerspective persp;
  if (!uishared::get_world_3d_persp(persp))
    return 0;

  int resultFlags = 0;

  Point2 screenHalfSize = StdGuiRender::screen_size() * 0.5f;
  Point2 screenCenter = screenHalfSize;
  Point2 elemLt = elem->screenCoord.screenPos;
  Point2 elemRb = elemLt + elem->screenCoord.size;
  Point2 elemCenter = (elemLt + elemRb) * 0.5f;
  Point2 elemHalfSize = elem->screenCoord.size * 0.5f;

  const Sqrat::Object &dataKey = elem->csk->data;

  for (int iChild = 0, nChildren = elem->children.size(); iChild < nChildren; ++iChild)
  {
    Element *child = elem->children[iChild];
    while (!child->transform && child->children.size() > 0)
      child = child->children[0];

    bool wasHidden = child->isHidden();

    Sqrat::Object data = child->props.scriptDesc.RawGetSlot(dataKey);
    ecs::EntityId zoneEid = data.RawGetSlotValue(strings->zoneEid, ecs::INVALID_ENTITY_ID);
    float yOffs = data.RawGetSlotValue(strings->yOffs, 0.0f);
    Point3 iconOffsP3 = data.RawGetSlotValue(strings->iconOffsP3, Point3(0., 0., 0.));
    Point3 zonePos{}, deltaPos{}; // init it to fix crash on NaN's

    if (!child->transform)
    {
      LOGERR_ONCE("%s: elem->children[%d]->transform == NULL", __FUNCTION__, iChild);
      child->setHidden(true);
    }
    else if (data.IsNull() || zoneEid == ecs::INVALID_ENTITY_ID)
    {
      child->setHidden(true);
    }
    else
    {
      if (auto transform = g_entity_mgr->getNullable<TMatrix>(zoneEid, ECS_HASH("transform")))
      {
        zonePos = transform->getcol(3);
        zonePos += iconOffsP3;
        zonePos.y += yOffs;
        deltaPos = zonePos - uishared::view_itm.getcol(3);
        child->setHidden(lengthSq(deltaPos) < sqr(0.1f));
      }
      else
        child->setHidden(true);
    }

    if (child->isHidden() != wasHidden)
      resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;

    if (child->isHidden())
      continue;

    Point3 sp = uishared::view_tm * zonePos;
    Point2 vp(0, 0);

    bool inElem = false;
    if (sp.z > 0.1f)
    {
      vp = Point2(sp.x / sp.z * persp.wk * screenHalfSize.x, -sp.y / sp.z * persp.hk * screenHalfSize.y) + screenCenter - elemCenter;
      if (fabsf(vp.x) <= elemHalfSize.x && fabsf(vp.y) <= elemHalfSize.y)
        inElem = true;
    }

    float arrowRotation = 0;

    if (!inElem)
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

      // clamp to element box
      Point2 d = Point2(c * diagonalPx, s * diagonalPx);
      if (d.x + screenHalfSize.x < elemLt.x)
        d *= (screenHalfSize.x - elemLt.x) / fabsf(d.x);
      else if (d.x + screenHalfSize.x > elemRb.x)
        d *= (elemRb.x - screenHalfSize.x) / fabsf(d.x);

      if (d.y + screenHalfSize.y < elemLt.y)
        d *= (screenHalfSize.y - elemLt.y) / fabsf(d.y);
      else if (d.y + screenHalfSize.y > elemRb.y)
        d *= (elemRb.y - screenHalfSize.y) / fabsf(d.y);

      vp = d + screenCenter - elemCenter;

      arrowRotation = mixedAngle;
    }


    child->transform->translate = vp;

    // assume that first transformable child is an arrow
    if (child->children.size())
    {
      Element *arrowElem = child->children[0];
      while (!arrowElem->transform && arrowElem->children.size() > 0)
        arrowElem = arrowElem->children[0];

      if (arrowElem->transform)
      {
        if (arrowElem->isHidden() != inElem)
          resultFlags |= R_REBUILD_RENDER_AND_INPUT_LISTS;
        arrowElem->setHidden(inElem);
        arrowElem->transform->rotate = arrowRotation + HALFPI;
      }
    }
  }

  return resultFlags;
}
