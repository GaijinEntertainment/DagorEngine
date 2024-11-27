// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "connectivity.h"

#include <ecs/core/entityManager.h>
#include <ecs/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>

#include "net/net.h"
#include "net/time.h"


using namespace ui;


ECS_TAG(ui, render, netClient)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void connectivity_state_es(const UpdateStageInfoBeforeRender & /*brinfo*/, int &ui_state__connectivity)
{
  ui_state__connectivity = CONNECTIVITY_OK;

  g_entity_mgr->broadcastEventImmediate(CheckConnectivityState(&ui_state__connectivity));

  if (get_no_packets_time_ms() > (TIME_SYNC_INTERVAL_MS + 500)) // should be safe enough
    ui_state__connectivity = CONNECTIVITY_NO_PACKETS;
}
