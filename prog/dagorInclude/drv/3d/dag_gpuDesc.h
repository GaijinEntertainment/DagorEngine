//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_consts.h"
#include "dag_info.h"


/**
 * \brief To describe a GPU category and make query with it.
 * Example usage:
 * if (auto &desc = d3d::get_driver_desc(); desc.info >= GpuDesc{.vendor = GpuVendor::INTEL, .family = GpuDesc::ALCHEMIST})
 *   ...
 */
struct GpuDesc
{
  using enum DeviceAttributes::AmdFamily;
  using enum DeviceAttributes::IntelFamily;
  using enum DeviceAttributes::NvidiaFamily;

  /**
   * \brief Dagor specific value about the vendor of the currently use GPU.
   */
  decltype(DeviceAttributes::vendor) vendor = GpuVendor::UNKNOWN;
  /**
   * \brief Unified Memory Architecture, true if the GPU is integrated.
   */
  decltype(DeviceAttributes::isUMA) isUMA = false;
  /**
   * \brief ID of the vendor.
   */
  decltype(DeviceAttributes::vendorId) vendorId = DeviceAttributes::UNKNOWN;
  /**
   * \brief Internal representation of the currently used GPU's microarchitecture, aka family.
   */
  decltype(DeviceAttributes::family) family = DeviceAttributes::UNKNOWN;
};

constexpr bool operator==(const DeviceAttributes &l, const GpuDesc &r)
{
  if (l.vendor == GpuVendor::UNKNOWN || r.vendor == GpuVendor::UNKNOWN || l.vendor != r.vendor)
  {
    return false;
  }

  return l.family == r.family;
}

constexpr bool operator!=(const DeviceAttributes &l, const GpuDesc &r) { return !(l == r); }

constexpr bool operator>=(const DeviceAttributes &l, const GpuDesc &r)
{
  if (l.vendor == GpuVendor::UNKNOWN || r.vendor == GpuVendor::UNKNOWN || l.vendor != r.vendor)
  {
    return false;
  }

  if (l.vendor == GpuVendor::NVIDIA)
  {
    constexpr auto tegraBasedMask = 0xE0000000;
    if ((l.family & tegraBasedMask) != (r.family & tegraBasedMask))
    {
      return false;
    }
  }

  return l.family >= r.family;
}
