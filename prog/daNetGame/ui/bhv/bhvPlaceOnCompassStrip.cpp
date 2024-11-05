// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvPlaceOnCompassStrip.h"
#include "ui/uiShared.h"
#include <ecs/scripts/sqEntity.h>
#include <math/dag_mathUtils.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <3d/dag_render.h>
#include <ecs/core/entityManager.h>
#include <math/dag_mathAng.h>


using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvPlaceOnCompassStrip, bhv_place_on_compass_strip, cstr);


BhvPlaceOnCompassStrip::BhvPlaceOnCompassStrip() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvPlaceOnCompassStrip::onAttach(Element *elem)
{
  fov = 200.f;
  fadeOutZone = 10.f;

  Sqrat::Table data = elem->props.scriptDesc.RawGetSlot(elem->csk->data);
  if (data.IsNull())
  {
    logerr("BhvPlaceOnCompassStrip::onAttach: 'data' not found");
    return;
  }

  fov = data.RawGetSlotValue("fov", fov);
  fadeOutZone = data.RawGetSlotValue("fadeOutZone", fadeOutZone);
}


int BhvPlaceOnCompassStrip::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_place_on_compass_strip_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Point3 lookDir = uishared::view_itm.getcol(2);
  float lookAngle = safe_atan2(lookDir.x, lookDir.z) * RAD_TO_DEG;

  const Sqrat::Object &dataKey = elem->csk->data;
  const Sqrat::Object &angleKey = elem->csk->angle;

  for (Element *child : elem->children)
  {
    Sqrat::Object data = child->props.scriptDesc.RawGetSlot(dataKey);

    float d = 0.f;

    float angle = data.RawGetSlotValue(angleKey, VERY_BIG_NUMBER);
    if (angle != VERY_BIG_NUMBER)
    {
      d = angle - lookAngle;
    }
    else
    {
      float relativeAngle = data.RawGetSlotValue("relativeAngle", VERY_BIG_NUMBER);
      if (relativeAngle != VERY_BIG_NUMBER)
      {
        d = relativeAngle;
        continue;
      }
      else
      {
        Sqrat::Object worldPosObj;
        if ((worldPosObj = data.RawGetSlot(strings->worldPos)).IsNull() == false)
        {
          Point3 dir = worldPosObj.Cast<Point3 &>() - uishared::view_itm.getcol(3);
          float angleToEntity = safe_atan2(dir.x, dir.z) * RAD_TO_DEG;
          d = angleToEntity - lookAngle;
        }
        else
        {
          ecs::EntityId eid = data.RawGetSlotValue(strings->eid, ecs::INVALID_ENTITY_ID);
          if (eid == ecs::INVALID_ENTITY_ID)
            continue;

          if (auto transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform")))
          {
            Point3 pos = transform->getcol(3);
            Point3 dir = pos - uishared::view_itm.getcol(3);
            float angleToEntity = safe_atan2(dir.x, dir.z) * RAD_TO_DEG;
            d = angleToEntity - lookAngle;
          }
          else
          {
            continue;
          }
        }
      }
    }

    G_ASSERT(fabs(d) <= 3600.f);
    if (fabs(d) > 3600.f)
      continue;

    d = norm_s_ang_deg(d);

    float x = cvt(d, -fov * 0.5f, fov * 0.5f, -0.5f, 0.5f);
    const Point2 &size = elem->screenCoord.size;

    if (child->transform)
      child->transform->translate = Point2(size.x * x, 0);

    bool clampToBorder = data.RawGetSlotValue(strings->clampToBorder, false);

    float opacity =
      clampToBorder ? 1.f : cvt(d, -fov * 0.5f, -fov * 0.5f + fadeOutZone, 0, 1) * cvt(d, fov * 0.5f, fov * 0.5f - fadeOutZone, 0, 1);

    child->props.setCurrentOpacity(opacity);
  }

  return 0;
}
