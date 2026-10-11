//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
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
enum InitDeviceType
{
  None = 0,
  Keyboard = 1,
  Pointing = 2, // Aka mouse or touchscreen
  Joystick = 4,
  All = 7,
#if _TARGET_PC
  Default = All,
#else
  Default = All & ~Keyboard
#endif
};

//! settings input/useSonyPads:b=true adds the raw HID Sony pad driver (Windows, Linux); on Windows and Linux the
//! composite's gamepad layout group is on, see CompositeJoystickClassDriver::setGamepadLayoutGroup
void init_hid_drivers(int poll_thread_interval_msec, int init_dev_type = InitDeviceType::Default);
void term_hid_drivers();

void init_input(const char *controls_config_fname);
void init_input(const DataBlock &controls_config_blk);
void term_input();
} // namespace ecs
