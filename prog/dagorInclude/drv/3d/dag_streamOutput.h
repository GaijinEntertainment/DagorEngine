//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>

struct StreamOutputBufferSetup;

namespace d3d
{
/**
 * \brief Sets the stream output buffer for the next draw calls.
 * \param slot The stream output slot index (slot is limited by MAX_STREAM_OUTPUT_SLOTS).
 * \param buffer The stream output slot description.
 * \details Note that you have to reset the stream output buffer to null after the needed draw calls.
 * \note It is invalid to call this when DeviceDriverCapabilities::hasStreamOutput feature is not supported.
 * \warning Stream output requires to ship DXIL on Xbox which will increase bindump size and pipelines creation time.
 *          Minimize amount of such pipelines since it is very expensive on Xbox!
 */
void set_stream_output_buffer(int slot, const StreamOutputBufferSetup &buffer);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline void set_stream_output_buffer(int slot, const StreamOutputBufferSetup &buffer) { d3di.set_stream_output_buffer(slot, buffer); }
} // namespace d3d
#endif