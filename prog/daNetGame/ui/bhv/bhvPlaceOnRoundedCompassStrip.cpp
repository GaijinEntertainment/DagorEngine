// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvPlaceOnRoundedCompassStrip.h"
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


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvPlaceRoundCompassOnCompassStrip, bhv_place_on_round_compass_strip, cstr);


BhvPlaceRoundCompassOnCompassStrip::BhvPlaceRoundCompassOnCompassStrip() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}

int BhvPlaceRoundCompassOnCompassStrip::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_place_on_compass_strip_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Point3 lookDir = uishared::view_itm.getcol(2);
  float lookAngle = safe_atan2(lookDir.x, lookDir.z) * RAD_TO_DEG;

  const Sqrat::Object &dataKey = elem->csk->data;
  const Sqrat::Object &angleKey = elem->csk->angle;

  const Point2 &size = elem->screenCoord.size;
  float radius = max(size.x, size.y) / 2.0f;

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

    if (fabs(d) > 3600.f)
    {
      LOGERR_ONCE("Rund compass strip angle degree is %f", d);
      continue;
    }

    if (!child->transform)
    {
      continue;
    }

    d = norm_s_ang_deg(d);
    float rad = (d - 90.0f) * DEG_TO_RAD;

    Point2 childSize = child->screenCoord.size;
    Point2 offset = {childSize.x / 2, childSize.y};

    float c = 0.0;
    float s = 0.0;
    sincos(rad, s, c);

    // adjust bottom edge of element to circle
    //   ****
    //   ****
    //   ****  <<<<bottom edge
    //  /^^^^\ <<<<radius
    //
    // and fix pivot point by size / 2
    float fixedPosX = c * childSize.y / 2 - childSize.x / 2;
    float fixedPosY = s * childSize.y / 2 - childSize.y / 2;

    float x = (radius * c) + radius + fixedPosX;
    float y = (radius * s) + radius + fixedPosY;

    child->transform->translate = Point2(x, y);
    child->transform->rotate = rad + (90 * DEG_TO_RAD);
  }

  return 0;
}
