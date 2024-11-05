// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>

void init_webui();
void stop_webui();
void update_webui();

void select_entity_webui(const ecs::EntityId &eid);
