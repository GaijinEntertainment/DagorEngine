// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <math/dag_easingFunctions.h>
#include <math/dag_mathUtils.h>
#include <game/player.h>
#include "render/renderEvent.h"
#include "net/time.h"


ECS_TAG(render, ui)
ECS_NO_ORDER
static inline void offender_mark_es(const RenderEventUI &evt, ecs::EntityId possessedByPlr, ecs::Array &offender_marks)
{
  if (possessedByPlr != game::get_local_player_eid())
    return;

  const TMatrix &viewItm = evt.get<1>();

  StdGuiRender::start_raw_layer();
  StdGuiRender::reset_textures();
  Point2 screenCenter(StdGuiRender::screen_size() * 0.5f);
  const float markOffset = StdGuiRender::screen_height() * 0.2f;
  const float markSize = 10.f;
  float lookAngle = safe_atan2(viewItm.getcol(2).z, viewItm.getcol(2).x);
  for (auto &mark : offender_marks)
  {
    const ecs::Object &markObj = mark.get<ecs::Object>();
    const Point3 lastPos = markObj.getMemberOr<Point3>(ECS_HASH("offenderPos"), Point3());
    Point3 dir = lastPos - viewItm.getcol(3);
    float angle = safe_atan2(dir.z, dir.x);
    float relAngle = angle - lookAngle;
    float sinAng, cosAng;
    sincos(relAngle, sinAng, cosAng);
    const float showTimeEnd = markObj.getMemberOr<float>(ECS_HASH("showTimeEnd"), 0.f);
    float curMarkSize = lerp(2.f, 1.f, outQuart(saturate(showTimeEnd - get_sync_time()))) * markSize;
    Point2 pointerPos = screenCenter - Point2(sinAng, cosAng) * markOffset;
    Point2 tip = pointerPos - Point2(sinAng, cosAng) * curMarkSize;
    Point2 leftBot = pointerPos - Point2(sinf(relAngle + HALFPI), cosf(relAngle + HALFPI)) * curMarkSize;
    Point2 rightBot = pointerPos - Point2(sinf(relAngle - HALFPI), cosf(relAngle - HALFPI)) * curMarkSize;
    E3DCOLOR color = E3DCOLOR_MAKE(255, 0, 0, int(saturate(showTimeEnd - get_sync_time()) * 255.f));
    StdGuiRender::set_color(color);
    StdGuiRender::draw_line(pointerPos + (leftBot - pointerPos) * 1.2f, pointerPos + (rightBot - pointerPos) * 1.2f, 1);
    StdGuiRender::draw_fill_tri(tip, leftBot, rightBot, color);
  }
  StdGuiRender::end_raw_layer();
}
