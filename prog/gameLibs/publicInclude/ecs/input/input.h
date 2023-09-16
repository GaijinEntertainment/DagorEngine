//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ecs/core/entityManager.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

class DataBlock;

struct DaInputComponent
{
  DaInputComponent(const ecs::EntityManager &mgr, ecs::EntityId eid);
};
ECS_DECLARE_RELOCATABLE_TYPE(DaInputComponent);

namespace ecs
{
void init_hid_drivers(int poll_thread_interval_msec);
void term_hid_drivers();

void init_input(const char *controls_config_fname);
void init_input(const DataBlock &controls_config_blk);
void term_input();
} // namespace ecs
