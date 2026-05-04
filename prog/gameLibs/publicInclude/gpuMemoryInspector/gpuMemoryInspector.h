//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resourceTag.h>

class DataBlock;

using GpuMemoryInspectorCallbackToken = const void *;

GpuMemoryInspectorCallbackToken gpu_memory_inspect_register_external_callback(
  dag::FunctionRef<void(const ResourceVisitor &visitor) const> callback);
void gpu_memory_inspect_unregister_external_callback(GpuMemoryInspectorCallbackToken token);

/// Updates the settings of the GPU Memory Inspector from the config values
void gpu_memory_inspector_load_settings(const DataBlock *block);