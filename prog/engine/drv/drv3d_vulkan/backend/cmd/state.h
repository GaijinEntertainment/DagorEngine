// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_e3dColor.h>
#include "buffer_ref.h"
#include "vulkan_api.h"
#include "driver.h"
#include "graphics_state.h"

namespace drv3d_vulkan
{

struct CmdSetConstRegisterBuffer
{
  BufferRef value;
  ShaderStage stage;
};

struct CmdSetBlendConstantFactor
{
  E3DCOLOR value;
};

struct CmdSetDepthBoundsRange
{
  float from;
  float to;
};

struct CmdSetStencilRefOverride
{
  uint16_t value;
};

struct CmdSetPolygonLineEnable
{
  bool value;
};

struct CmdSetScissorRect
{
  VkRect2D value;
};

struct CmdSetInputLayout
{
  InputLayoutID value;
};

struct CmdSetIndexBuffer
{
  BufferRef buffer;
  VkIndexType type;
};

struct CmdSetVertexBuffer
{
  uint32_t stream;
  BufferRef buffer;
  uint32_t offset;
};

struct CmdSetVertexStride
{
  uint32_t stream;
  uint32_t stride;
};

struct CmdSetComputeProgram
{
  ProgramID value;
};

struct CmdSetGraphicsProgram
{
  ProgramID value;
};

struct CmdSetViewport
{
  ViewportState value;
};

struct CmdSetConditionalRender
{
  BufferRef buffer;
  VkDeviceSize offset;
};

struct CmdSetRenderState
{
  shaders::DriverRenderStateId value;
};

struct CmdTransitURegister
{
  ShaderStage stage;
  uint32_t index;
  URegister value;
};

struct CmdTransitTRegister
{
  ShaderStage stage;
  uint32_t index;
  TRegister value;
};

struct CmdTransitBRegister
{
  ShaderStage stage;
  uint32_t index;
  BufferRef value;
};

struct CmdTransitSRegister
{
  ShaderStage stage;
  uint32_t index;
  SRegister value;
};

struct CmdSetImmediateConsts
{
  ShaderStage stage;
  bool enable;
  uint32_t data[MAX_IMMEDIATE_CONST_WORDS];
};

struct CmdPipelineCompilationTimeBudget
{
  PipelineManager::AsyncMask mask;
};

struct CmdSetGraphicsQuery
{
  VulkanQueryPoolHandle pool;
  uint32_t index;
};

struct CmdTransitShadingRate
{
  uint32_t encodedState;
};

} // namespace drv3d_vulkan
