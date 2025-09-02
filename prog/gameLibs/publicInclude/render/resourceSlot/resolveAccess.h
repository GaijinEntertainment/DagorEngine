//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace resource_slot
{
/** Perform all registered access and set NodeHandles.
 *
 * MUST be executed in every frame before dafg::run_nodes()
 *
 * \param storage_id RESERVED for future use
 */
void resolve_access();
} // namespace resource_slot