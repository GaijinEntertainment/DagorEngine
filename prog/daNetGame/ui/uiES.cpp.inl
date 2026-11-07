// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "overlay.h"
#include "game/scriptsLoader.h"


ECS_NO_ORDER
static void ui_rebind_das_events_es(const EventGameModeLoaded &) { overlay_ui::rebind_das(); }