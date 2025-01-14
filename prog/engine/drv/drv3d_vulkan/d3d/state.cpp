// Copyright (C) Gaijin Games KFT.  All rights reserved.

// pipeline related state implementation
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include "globals.h"
#include "global_lock.h"
#include "frontend.h"
#include "resource_manager.h"
#include "pipeline_state.h"
#include "buffer.h"
#include "texture.h"
#include <perfMon/dag_graphStat.h>

using namespace drv3d_vulkan;

namespace
{
struct LocalAccessor
{
  PipelineState &pipeState;

  LocalAccessor() : pipeState(Frontend::State::pipe) { VERIFY_GLOBAL_LOCK_ACQUIRED(); }
};
} // namespace

bool d3d::set_render_state(shaders::DriverRenderStateId state_id)
{
  LocalAccessor la;

  la.pipeState.set<StateFieldGraphicsRenderState, shaders::DriverRenderStateId, FrontGraphicsState>(state_id);
  la.pipeState.set<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(STENCIL_REF_OVERRIDE_DISABLE);
  return true;
}

bool d3d::set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words)
{
  LocalAccessor la;

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)stage);
  if (resBinds.set<StateFieldImmediateConst, StateFieldImmediateConst::SrcData>({(uint8_t)num_words, data}))
    la.pipeState.markResourceBindDirty((ShaderStage)stage);
  return true;
}

bool d3d::set_depth_bounds(float zmin, float zmax)
{
  LocalAccessor la;
  using FieldName = StateFieldGraphicsDepthBounds;

  zmin = clamp(zmin, 0.f, 1.f);
  zmax = clamp(zmax, zmin, 1.f);
  la.pipeState.set<FieldName, FieldName::DataType, FrontGraphicsState>({zmin, zmax});
  return true;
}

bool d3d::setwire(bool wire)
{
  if (DAGOR_UNLIKELY(Globals::VK::phy.features.fillModeNonSolid == VK_FALSE))
    return false;
  LocalAccessor la;

  la.pipeState.set<StateFieldGraphicsPolygonLine, bool, FrontGraphicsState>(wire);
  return true;
}

bool d3d::setvdecl(VDECL vdecl)
{
  LocalAccessor la;

  if (vdecl == BAD_VDECL)
    la.pipeState.makeFieldDirty<StateFieldGraphicsProgram, FrontGraphicsState>();
  la.pipeState.set<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(InputLayoutID(vdecl));
  return true;
}

bool d3d::set_const_buffer(uint32_t stage, uint32_t unit, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size)
{
  G_ASSERT_RETURN(!consts_offset && !consts_size, false); // not implemented
  G_ASSERT((nullptr == buffer) || (buffer->getFlags() & SBCF_BIND_CONSTANT));
  LocalAccessor la;

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)stage);
  BufferRef bReg{};
  if (buffer)
  {
    GenericBufferInterface *gb = (GenericBufferInterface *)buffer;
    bReg = gb->getBufferRef();
    if (!bReg)
      bReg = gb->fillFrameMemWithDummyData();
  }

  if (resBinds.set<StateFieldBRegisterSet, StateFieldBRegister::Indexed>({unit, bReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)stage);
  return true;
}

bool d3d::setvsrc_ex(int stream, Vbuffer *vb, int ofs, int stride_bytes)
{
  G_ASSERT(stream < MAX_VERTEX_INPUT_STREAMS);
  LocalAccessor la;

  using Bind = StateFieldGraphicsVertexBufferBind;
  using StrideBind = StateFieldGraphicsVertexBufferStride;

  BufferRef bRef = {};
  if (vb)
  {
    bRef = ((GenericBufferInterface *)vb)->getBufferRef();
    G_ASSERTF(((GenericBufferInterface *)vb)->getFlags() & SBCF_BIND_VERTEX,
      "vulkan: using non vertex buffer %p:%s in vertex bind slot %u", bRef.buffer, bRef.buffer->getDebugName(), stream);
  }
  Bind bind{bRef, (uint32_t)ofs};
  la.pipeState.set<StateFieldGraphicsVertexBuffers, Bind::Indexed, FrontGraphicsState>({(uint32_t)stream, bind});
  la.pipeState.set<StateFieldGraphicsVertexBufferStrides, StrideBind::Indexed, FrontGraphicsState>(
    {(uint32_t)stream, (uint32_t)stride_bytes});
  return true;
}

bool d3d::setind(Ibuffer *ib)
{
  LocalAccessor la;

  GenericBufferInterface *gb = ((GenericBufferInterface *)ib); //-V522
  BufferRef bRef = {};
  if (gb)
    bRef = gb->getBufferRef();

  la.pipeState.set<StateFieldGraphicsIndexBuffer, BufferRef, FrontGraphicsState>(bRef); //-V522
  if (gb)
    la.pipeState.set<StateFieldGraphicsIndexBuffer, VkIndexType, FrontGraphicsState>(gb->getIndexType()); //-V522
  return true;
}

bool d3d::setscissor(int x, int y, int w, int h)
{
  LocalAccessor la;

  VkRect2D scissor = {{x, y}, {(uint32_t)w, (uint32_t)h}};
  la.pipeState.set<StateFieldGraphicsScissorRect, VkRect2D, FrontGraphicsState>(scissor);
  return true;
}

bool d3d::setstencil(uint32_t ref)
{
  LocalAccessor la;

  // this will override stencil ref from render state id
  // until new render state is setted up
  la.pipeState.set<StateFieldGraphicsStencilRefOverride, uint16_t, FrontGraphicsState>(uint16_t(ref));
  return true;
}

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz)
{
  LocalAccessor la;
  const ViewportState &viewport = la.pipeState.getRO<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>();

  x = viewport.rect2D.offset.x;
  y = viewport.rect2D.offset.y;
  w = viewport.rect2D.extent.width;
  h = viewport.rect2D.extent.height;
  minz = viewport.minZ;
  maxz = viewport.maxZ;
  return true;
}

bool d3d::setview(int x, int y, int w, int h, float minz, float maxz)
{
  LocalAccessor la;
  // it is ok for other APIs, but we will crash GPU due to RP render area dependency
  // clamp to 0 with wh fixup for now, as it does not make sense to draw with negative offset
  if (x < 0)
  {
    w += x;
    x = 0;
  }
  if (y < 0)
  {
    h += y;
    y = 0;
  }

  ViewportState viewport = {{{x, y}, {(uint32_t)w, (uint32_t)h}}, minz, maxz};
  la.pipeState.set<StateFieldGraphicsViewport, ViewportState, FrontGraphicsState>(viewport);
  return true;
}

bool d3d::set_program(PROGRAM prog_id)
{
  LocalAccessor la;

  ProgramID prog{prog_id};
  if (ProgramID::Null() != prog)
  {
    switch (get_program_type(prog))
    {
      case program_type_graphics:
      {
        bool layoutOvChanged =
          la.pipeState.set<StateFieldGraphicsInputLayoutOverride, InputLayoutID, FrontGraphicsState>(InputLayoutID::Null());
        bool progChanged = la.pipeState.set<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>(prog);
        if (layoutOvChanged && !progChanged)
          la.pipeState.makeFieldDirty<StateFieldGraphicsProgram, FrontGraphicsState>();
        Stat3D::updateProgram();
      }
      break;
      case program_type_compute: la.pipeState.set<StateFieldComputeProgram, ProgramID, FrontComputeState>(prog); break;
      default: G_ASSERTF(false, "Broken program type"); return false;
    }
  }
  return true;
}


bool d3d::set_blend_factor(E3DCOLOR color)
{
  LocalAccessor la;

  la.pipeState.set<StateFieldGraphicsBlendConstantFactor, E3DCOLOR, FrontGraphicsState>(color);
  return true;
}

bool d3d::set_tex(unsigned shader_stage, unsigned unit, BaseTexture *tex, bool use_sampler)
{
  LocalAccessor la;

  BaseTex *texture = cast_to_texture_base(tex);
  if (texture && texture->isStub())
    texture = texture->getStubTex();

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);

  bool anyDiff = false;
  if (use_sampler && texture)
  {
    SRegister sReg(texture->samplerState);
    anyDiff |= resBinds.set<StateFieldSRegisterSet, StateFieldSRegister::Indexed>({unit, sReg});
  }
  TRegister tReg(texture);
  anyDiff |= resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({unit, tReg});

  if (anyDiff)
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  return true;
}

bool d3d::set_rwtex(unsigned shader_stage, unsigned unit, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint)
{
  LocalAccessor la;

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  URegister uReg(tex, face, mip_level, as_uint);
  if (resBinds.set<StateFieldURegisterSet, StateFieldURegister::Indexed>({unit, uReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  return true;
}

bool d3d::set_buffer(unsigned shader_stage, unsigned unit, Sbuffer *buffer)
{
  G_ASSERT((nullptr == buffer) || buffer->getFlags() & SBCF_BIND_SHADER_RES || buffer->getFlags() & SBCF_BIND_UNORDERED);
  LocalAccessor la;

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  TRegister tReg(buffer);
  if (resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({unit, tReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  return true;
}

bool d3d::set_rwbuffer(unsigned shader_stage, unsigned unit, Sbuffer *buffer)
{
  LocalAccessor la;

  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);
  URegister uReg(buffer);
  if (resBinds.set<StateFieldURegisterSet, StateFieldURegister::Indexed>({unit, uReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
  return true;
}

void d3d::set_sampler(unsigned shader_stage, unsigned unit, d3d::SamplerHandle sampler)
{
  LocalAccessor la;

  SamplerResource *samplerResource = reinterpret_cast<SamplerResource *>(sampler);
  auto &resBinds = la.pipeState.getStageResourceBinds((ShaderStage)shader_stage);

  SRegister sReg(samplerResource);
  if (resBinds.set<StateFieldSRegisterSet, StateFieldSRegister::Indexed>({unit, sReg}))
    la.pipeState.markResourceBindDirty((ShaderStage)shader_stage);
}
