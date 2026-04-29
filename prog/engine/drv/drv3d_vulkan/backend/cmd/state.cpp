// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "state.h"
#include "backend/context.h"
#include "util/fault_report.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdSetConstRegisterBuffer &cmd)
{
  auto &resBinds = Backend::State::pipe.getStageResourceBinds(cmd.stage);
  if (cmd.value)
    verifyResident(cmd.value.buffer);
  resBinds.set_raw<StateFieldGlobalConstBuffer, BufferRef>(cmd.value);
}

#define TRANSIT_WRITE(cmd_name, field, type) \
  TSPEC void BEContext::execCmd(const cmd_name &cmd) { Backend::State::pipe.set_raw<field, type, FrontGraphicsState>(cmd.value); }

TRANSIT_WRITE(CmdSetBlendConstantFactor, StateFieldGraphicsBlendConstantFactor, E3DCOLOR);
TRANSIT_WRITE(CmdSetStencilRefOverride, StateFieldGraphicsStencilRefOverride, uint16_t);
TRANSIT_WRITE(CmdSetPolygonLineEnable, StateFieldGraphicsPolygonLine, bool);
TRANSIT_WRITE(CmdSetScissorRect, StateFieldGraphicsScissorRect, VkRect2D);
TRANSIT_WRITE(CmdSetInputLayout, StateFieldGraphicsInputLayoutOverride, InputLayoutID);
TRANSIT_WRITE(CmdSetGraphicsProgram, StateFieldGraphicsProgram, ProgramID);
TRANSIT_WRITE(CmdSetViewport, StateFieldGraphicsViewport, ViewportState);
TRANSIT_WRITE(CmdSetRenderState, StateFieldGraphicsRenderState, shaders::DriverRenderStateId);

TSPEC void BEContext::execCmd(const CmdSetComputeProgram &cmd)
{
  Backend::State::pipe.set_raw<StateFieldComputeProgram, ProgramID, FrontComputeState>(cmd.value);
}

#undef TRANSIT_WRITE

TSPEC void BEContext::execCmd(const CmdSetDepthBoundsRange &cmd)
{
  using FieldName = StateFieldGraphicsDepthBounds;
  Backend::State::pipe.set_raw<FieldName, FieldName::DataType, FrontGraphicsState>({cmd.from, cmd.to});
}

TSPEC void BEContext::execCmd(const CmdSetIndexBuffer &cmd)
{
  Backend::State::pipe.set_raw<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(cmd.buffer);
  Backend::State::pipe.set_raw<StateFieldGraphicsIndexBuffer, VkIndexType, FrontGraphicsState>(cmd.type);
}

TSPEC void BEContext::execCmd(const CmdSetVertexBuffer &cmd)
{
  using Bind = StateFieldGraphicsVertexBufferBind;

  Bind bind{cmd.buffer, cmd.offset};
  Backend::State::pipe.set_raw<StateFieldGraphicsVertexBuffers, Bind::Indexed, FrontGraphicsState>({cmd.stream, bind});
}

TSPEC void BEContext::execCmd(const CmdSetVertexStride &cmd)
{
  using Bind = StateFieldGraphicsVertexBufferStride;
  Backend::State::pipe.set_raw<StateFieldGraphicsVertexBufferStrides, Bind::Indexed, FrontGraphicsState>({cmd.stream, cmd.stride});
}

TSPEC void BEContext::execCmd(const CmdSetConditionalRender &cmd)
{
  Backend::State::pipe.set_raw<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
    ConditionalRenderingState{cmd.buffer, cmd.offset});
}

#define TRANSIT_REGISTER(cmd_type, field_type)                                      \
  TSPEC void BEContext::execCmd(const cmd_type &cmd)                                \
  {                                                                                 \
    auto &resBinds = Backend::State::pipe.getStageResourceBinds(cmd.stage);         \
    resBinds.set_raw<field_type##Set, field_type::Indexed>({cmd.index, cmd.value}); \
  }

TRANSIT_REGISTER(CmdTransitURegister, StateFieldURegister)
TRANSIT_REGISTER(CmdTransitTRegister, StateFieldTRegister)
TRANSIT_REGISTER(CmdTransitBRegister, StateFieldBRegister)
TRANSIT_REGISTER(CmdTransitSRegister, StateFieldSRegister)

#undef TRANSIT_REGISTER

TSPEC void BEContext::execCmd(const CmdSetImmediateConsts &cmd)
{
  auto &resBinds = Backend::State::pipe.getStageResourceBinds(cmd.stage);
  resBinds.set_raw<StateFieldImmediateConst, StateFieldImmediateConst::SrcData>(
    {(uint8_t)(cmd.enable ? MAX_IMMEDIATE_CONST_WORDS : 0), cmd.data});
}

TSPEC void BEContext::execCmd(const CmdPipelineCompilationTimeBudget &cmd) { Globals::pipelines.setAsyncCompile(cmd.mask); }

TSPEC void BEContext::execCmd(const CmdSetGraphicsQuery &cmd)
{
  Backend::State::pipe.set_raw<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({cmd.pool, cmd.index});
}

TSPEC void BEContext::execCmd(const CmdTransitShadingRate &cmd)
{
  Backend::State::pipe.set_raw<StateFieldGraphicsShadingRate, uint32_t, FrontGraphicsState>({cmd.encodedState});
}

TSPEC void FaultReportDump::dumpCmd(const CmdSetComputeProgram &v)
{
  significants.programCS = currentCommand;
  VK_CMD_DUMP_PARAM(value);
}

TSPEC void FaultReportDump::dumpCmd(const CmdSetGraphicsProgram &v)
{
  significants.programGR = currentCommand;
  VK_CMD_DUMP_PARAM(value);
}
