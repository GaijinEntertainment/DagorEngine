// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include "userUi.h"
#include "overlay.h"
#include "game/scriptsLoader.h"


ECS_NO_ORDER
static void ui_rebind_das_events_es(const EventGameModeLoaded &)
{
  user_ui::rebind_das();
  overlay_ui::rebind_das();
}