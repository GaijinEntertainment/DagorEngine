// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include <drv/3d/rayTrace/dag_drvRayTrace.h>


namespace drv3d_dx12
{

// TODO rename some of the names to better resemble what they are intended for
enum class DeviceMemoryClass
{
  DEVICE_RESIDENT_IMAGE,
  DEVICE_RESIDENT_BUFFER,
  // linear cpu cached textures
  HOST_RESIDENT_HOST_READ_WRITE_IMAGE,
  // linear cpu non-cached textures
  HOST_RESIDENT_HOST_WRITE_ONLY_IMAGE,
  HOST_RESIDENT_HOST_READ_WRITE_BUFFER,

  HOST_RESIDENT_HOST_READ_ONLY_BUFFER,
  HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER,
  // special AMD memory type,
  // a portion of gpu mem is host
  // visible (256mb).
  DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER,
  // we handle memory for push ring buffer differently than any other
  PUSH_RING_BUFFER,
  TEMPORARY_UPLOAD_BUFFER,

  READ_BACK_BUFFER,
  BIDIRECTIONAL_BUFFER,

  RESERVED_RESOURCE,

#if DX12_USE_ESRAM
  ESRAM_RESOURCE,
#endif

#if D3D_HAS_RAY_TRACING
  DEVICE_RESIDENT_ACCELERATION_STRUCTURE,
#endif

  COUNT,
  INVALID = COUNT
};

} // namespace drv3d_dx12
