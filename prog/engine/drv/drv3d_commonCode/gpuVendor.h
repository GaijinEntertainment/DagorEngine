// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <osApiWrappers/dag_miscApi.h>


class DataBlock;
struct DeviceAttributesBase;

namespace gpu
{
void update_device_attributes(uint32_t vendor_id, uint32_t device_id, DeviceAttributesBase &device_attributes);

bool is_forced_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete);

bool is_preferred_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete);

bool is_blacklisted_device(const DataBlock &gpu_preferences, uint32_t vendor_id, uint32_t device_id,
  eastl::span<const uint32_t> other_discrete);

DagorDateTime get_driver_date(uint32_t vendor_id, uint32_t device_id);

} // namespace gpu
