// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <grdk.h>
#include <GameInput.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>


namespace gameinput
{

constexpr size_t MAX_DEVICES_PER_TYPE = 8;
using DevicesList = eastl::fixed_vector<IGameInputDevice *, MAX_DEVICES_PER_TYPE, false>;

struct ReadingDeleter
{
  void operator()(IGameInputReading *reading);
};
using Reading = eastl::unique_ptr<IGameInputReading, ReadingDeleter>;

void init();
void shutdown();

unsigned get_devices_config_generation(GameInputKind kind);

Reading get_current_reading(GameInputKind kind, IGameInputDevice *device);

void get_devices(GameInputKind kind, DevicesList &devices);
bool has_input_device_of_kind(GameInputKind kind);

} // namespace gameinput
