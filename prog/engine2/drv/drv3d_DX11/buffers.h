#pragma once

#include <3d/dag_drv3dCmd.h>

#include "genericBuffer.h"

namespace drv3d_dx11
{
struct VertexStream
{
  GenericBuffer *source = nullptr;
  uint32_t offset = 0;
  uint32_t stride = 0;
};

struct VertexInput
{
  VertexStream vertexStream[MAX_VERTEX_STREAMS];
  GenericBuffer *indexBuffer = nullptr;
};

void clear_buffers_pool_garbage();
void advance_buffers();
} // namespace drv3d_dx11
