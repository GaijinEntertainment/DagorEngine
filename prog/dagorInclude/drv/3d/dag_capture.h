//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace d3d
{

/**
 * @brief Starts the capture process. All previous outstanding commands will be flushed.
 *
 * @param name The name of the capture. This might show up in the capture tool or be used in the filename depending on
 * platform and the capture tool used. nullptr is not allowed.
 * @param savepath The path where the captured file will be saved. Not all capture tools respect this. nullptr means default path is
 * used.
 * @return true on success, false otherwise.
 */
bool start_capture(const char *name, const char *savepath);

/**
 * @brief Stops the ongoing capture process.
 *
 * This function halts any active capture operation that is currently in progress.
 * Commands made since the last call to start_capture() will be flushed.
 */
void stop_capture();
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool start_capture(const char *name, const char *savepath) { return d3di.start_capture(name, savepath); }
inline void stop_capture() { d3di.stop_capture(); }
} // namespace d3d
#endif