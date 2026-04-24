// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <ecs/rendInst/riExtra.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <util/dag_convar.h>

#include <camera/sceneCam.h>
#include <render/renderEvent.h>


CONSOLE_BOOL_VAL("render", riextra_motion_vectors, true);

constexpr float MAX_DIST = 100.0f;
constexpr float MAX_DIST_SQR = MAX_DIST * MAX_DIST;

ECS_TAG(render)
static void riex_set_prev_tm_es(const EventOnRendinstMovement &event, ecs::EntityId eid)
{
  G_UNREFERENCED(eid);

  if (!riextra_motion_vectors)
    return;

  rendinst::riex_handle_t ri_handle = event.get<0>();
  const TMatrix &prevTm = event.get<1>();

  const Point3 &camPos = get_cam_itm().getcol(3);
  Point3 diff = camPos - prevTm.getcol(3);

  if (diff * diff > MAX_DIST_SQR)
    return;

  rendinst::RiExtraPerInstanceGpuItem data[4];

  data[0] = {bitwise_cast<uint32_t>(prevTm.m[0][0]), bitwise_cast<uint32_t>(prevTm.m[0][1]), bitwise_cast<uint32_t>(prevTm.m[0][2]),
    bitwise_cast<uint32_t>(0.0f)};
  data[1] = {bitwise_cast<uint32_t>(prevTm.m[1][0]), bitwise_cast<uint32_t>(prevTm.m[1][1]), bitwise_cast<uint32_t>(prevTm.m[1][2]),
    bitwise_cast<uint32_t>(0.0f)};
  data[2] = {bitwise_cast<uint32_t>(prevTm.m[2][0]), bitwise_cast<uint32_t>(prevTm.m[2][1]), bitwise_cast<uint32_t>(prevTm.m[2][2]),
    bitwise_cast<uint32_t>(0.0f)};
  data[3] = {bitwise_cast<uint32_t>(prevTm.m[3][0]), bitwise_cast<uint32_t>(prevTm.m[3][1]), bitwise_cast<uint32_t>(prevTm.m[3][2]),
    bitwise_cast<uint32_t>(1.0f)};

  rendinst::setRiExtraPerInstanceRenderAdditionalData(ri_handle, rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA,
    rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME, data, 4);
}
