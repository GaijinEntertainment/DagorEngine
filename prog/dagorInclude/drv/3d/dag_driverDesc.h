//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_driverDesc.inl"

namespace d3d
{

/**
 * @brief Gets the driver description.
 * @return The driver description
 */
const DriverDesc &get_driver_desc();

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{

inline const DriverDesc &get_driver_desc() { return d3di.drvDesc; }

} // namespace d3d
#endif
