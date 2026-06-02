// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "backendDebug.h"
#include "textureVisualization.h"

#include <runtime/runtime.h>
#include <gui/dag_imgui.h>


namespace dafg
{
void reset_texture_visualization() { clear_visualization_node(); }

void debug_clear_resource_placements() { Runtime::get().getVisualizerPtr()->clearResourcePlacements(); }

void debug_clear_resource_barriers() { Runtime::get().getVisualizerPtr()->clearResourceBarriers(); }

void debug_rec_resource_placement(ResNameId id, int frame, int heap, int offset, int size, bool is_cpu)
{
  Runtime::get().getVisualizerPtr()->recResourcePlacement(id, frame, heap, offset, size, is_cpu);
}

void debug_rec_resource_barrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  Runtime::get().getVisualizerPtr()->recResourceBarrier(res_id, res_frame, exec_time, exec_frame, barrier);
}

} // namespace dafg