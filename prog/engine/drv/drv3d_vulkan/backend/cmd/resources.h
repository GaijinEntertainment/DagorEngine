// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <drv/3d/dag_renderStates.h>
#include "shader_module.h"
#include "driver.h"
#include "render_pass_resource.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "memory_heap_resource.h"
#include "raytrace_as_resource.h"

namespace drv3d_vulkan
{

struct CmdAddShaderModule
{
  ShaderModuleBlob *sci;
  uint32_t id;
};

struct CmdRemoveShaderModule
{
  uint32_t id;
};

struct CmdAddComputeProgram
{
  ProgramID program;
  ShaderModuleBlob *sci;
  ShaderModuleHeader header;
};

struct CmdAddGraphicsProgram
{
  ProgramID program;
  uint32_t vs;
  uint32_t fs;
  uint32_t gs;
  uint32_t tc;
  uint32_t te;
};

struct CmdAddPipelineCache
{
  VulkanPipelineCacheHandle cache;
};

struct CmdAddRenderState
{
  shaders::DriverRenderStateId id;
  shaders::RenderState data;
};

struct CmdRemoveProgram
{
  ProgramID program;
};

struct CmdDestroyImage
{
  Image *image;
};

struct CmdDestroyRenderPassResource
{
  RenderPassResource *rp;
};

struct CmdDestroyBuffer
{
  Buffer *buffer;
};

struct CmdDestroyBufferDelayed
{
  Buffer *buffer;
};

struct CmdDestroyHeap
{
  MemoryHeapResource *heap;
};

struct CmdDestroyTLAS
{
  RaytraceAccelerationStructure *tlas;
};

struct CmdDestroyBLAS
{
  RaytraceAccelerationStructure *blas;
};

} // namespace drv3d_vulkan
