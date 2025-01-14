// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include <pthread.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_info.h>

#include "drv_log_defs.h"
#include "render.h"
#include "render_inline_shaders.h"
#include "acceleration_structure_desc.h"

#include <EASTL/algorithm.h>

extern bool get_aync_pso_compilation_supported();
extern uint32_t get_shader_cache_version();

static String getShaderName(const char *shader)
{
  String res;
  if (!shader)
    return res;
  const char *end = strchr(shader, '\n');
  if (!end)
    return res;
  res = String(shader, end - shader);
  return res;
}

namespace drv3d_metal
{
  namespace
  {
    MTLPrimitiveType convertPrimitiveType(int prim_type)
    {
      switch(prim_type)
      {
      case PRIM_POINTLIST:
        return MTLPrimitiveTypePoint;
      case PRIM_LINELIST:
        return MTLPrimitiveTypeLine;
      case PRIM_LINESTRIP:
        return MTLPrimitiveTypeLineStrip;
      case PRIM_TRILIST:
        return MTLPrimitiveTypeTriangle;
      case PRIM_TRISTRIP:
        return MTLPrimitiveTypeTriangleStrip;
      case PRIM_TRIFAN:
      case PRIM_4_CONTROL_POINTS:
      case PRIM_COUNT:
          G_ASSERT(0 && "Unsupported primitive type");
      }
      return MTLPrimitiveTypePoint;
    }

    MTLCullMode convertCullMode(int cull)
    {
      switch (cull)
      {
        case CULL_NONE:
          return MTLCullModeNone;
        case CULL_CCW:
          return MTLCullModeBack;
        case CULL_CW:
          return MTLCullModeFront;
      }
      G_ASSERTF(0, "Unknown cull mode %d", cull);
      return MTLCullModeNone;
    }

    MTLBlendOperation convertBlendOp(int op)
    {
      switch (op)
      {
        case BLENDOP_ADD:
          return MTLBlendOperationAdd;
        case BLENDOP_SUBTRACT:
          return MTLBlendOperationSubtract;
        case BLENDOP_REVSUBTRACT:
          return MTLBlendOperationReverseSubtract;
        case BLENDOP_MIN:
          return MTLBlendOperationMin;
        case BLENDOP_MAX:
          return MTLBlendOperationMax;
      }
      G_ASSERT(0 && "Unknown blend op");
      return MTLBlendOperationAdd;
    }

    MTLCompareFunction convertCompareFunction(int func)
    {
      switch (func)
      {
        case CMPF_NEVER:
          return MTLCompareFunctionNever;
        case CMPF_LESS:
          return MTLCompareFunctionLess;
        case CMPF_EQUAL:
          return MTLCompareFunctionEqual;
        case CMPF_LESSEQUAL:
          return MTLCompareFunctionLessEqual;
        case CMPF_GREATER:
          return MTLCompareFunctionGreater;
        case CMPF_NOTEQUAL:
          return MTLCompareFunctionNotEqual;
        case CMPF_GREATEREQUAL:
          return MTLCompareFunctionGreaterEqual;
        case CMPF_ALWAYS:
          return MTLCompareFunctionAlways;
      }
      G_ASSERT(0 && "Unknown compare function");
      return MTLCompareFunctionAlways;
    }
  }
  Render render;

  MTLDepthStencilDescriptor* Render::depthStateDesc = NULL;

  VDECL   copy_vdecl = BAD_VDECL;
  VSDTYPE clear_dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END };

  Sbuffer* clear_mesh_buffer = nullptr;

  int tmp_copy_buf_size = 131072;

  int clear_vshd = -1;
  int clear_pshdi = -1;
  int clear_pshdui = -1;
  int clear_pshdf = -1;
  int clear_vdecl = -1;
  int clear_progi = -1;
  int clear_progui = -1;
  int clear_progf = -1;

  int copy_vshd = -1;
  int copy_pshd = -1;
  int copy_prog = -1;

  int clear_vcnst_offset;
  id<MTLBuffer> clear_vcnst;

  int clear_pcnst_offset;
  id<MTLBuffer> clear_pcnst;

  const char g_clear_cs_src[] = ""
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct Const\n"
    "{\n"
    "uint4 val;\n"
    "uint len;\n"
    "};\n"
    "kernel void cs_main(uint global_index[[thread_position_in_grid]],\n"
    "    constant Const &consts[[buffer(0)]], device uint* buffer[[buffer(1)]])\n"
    "{\n"
    "   if (global_index < consts.len) buffer[global_index] = consts.val.x;\n"
    "}\n";

  const char g_copy_cs_src[] = ""
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct Const\n"
    "{\n"
    "uint size;\n"
    "uint src_offset;\n"
    "uint dst_offset;\n"
    "};\n"
    "kernel void cs_main(uint global_index[[thread_position_in_grid]],\n"
    "    constant Const &consts[[buffer(0)]], device uint* dst[[buffer(1)]], device uint* src[[buffer(2)]])\n"
    "{\n"
    "   if (global_index < consts.size)\n"
    "     dst[consts.dst_offset + global_index] = src[consts.src_offset + global_index];\n"
    "}\n";

  static MTLBlendFactor convBlendArg(int arg)
  {
    switch (arg)
    {
    case BLEND_ZERO:      return MTLBlendFactorZero;
    case BLEND_ONE:       return MTLBlendFactorOne;
    case BLEND_SRCCOLOR:    return MTLBlendFactorSourceColor;
    case BLEND_INVSRCCOLOR:   return MTLBlendFactorOneMinusSourceColor;
    case BLEND_SRCALPHA:    return MTLBlendFactorSourceAlpha;
    case BLEND_INVSRCALPHA:   return MTLBlendFactorOneMinusSourceAlpha;
    case BLEND_DESTALPHA:     return MTLBlendFactorDestinationAlpha;
    case BLEND_INVDESTALPHA:  return MTLBlendFactorOneMinusDestinationAlpha;
    case BLEND_DESTCOLOR:     return MTLBlendFactorDestinationColor;
    case BLEND_INVDESTCOLOR:  return MTLBlendFactorOneMinusDestinationColor;
    case BLEND_SRCALPHASAT:   return MTLBlendFactorSourceAlphaSaturated;
    case BLEND_BOTHINVSRCALPHA: return  MTLBlendFactorSourceAlphaSaturated;
    case BLEND_BLENDFACTOR:   return MTLBlendFactorBlendColor;
    case BLEND_INVBLENDFACTOR:  return MTLBlendFactorOneMinusBlendColor;
    default:
      G_ASSERT(0 && "Unknown blend mode");
    }

    return MTLBlendFactorOne;
  }

  static void setRenderTarget(Texture* tex, int level, int layer)
  {
    render.rt.colors[0].texture = tex;
    render.rt.colors[0].level = level;
    render.rt.colors[0].layer = layer;
    render.rt.colors[0].load_action = MTLLoadActionLoad;
    render.rt.colors[0].store_action = MTLStoreActionStore;

    for (int i = 1; i < Program::MAX_SIMRT; i++)
      render.rt.colors[i] = {};

    render.rt.depth = {};
    render.need_change_rt = 1;
    render.cached_viewport.originX = -FLT_MAX;
    render.checkNoRenderPass();

    render.dirty_state &= ~DirtyFlags::Viewport;
  }

  static void setConstants(Render::StageBinding& stage, float consts[4])
  {
    stage.cbuffer.applyCmd(consts, 0, 16);
  }

  static void setConstants(Render::StageBinding& stage, float consts)
  {
    stage.cbuffer.applyCmd(&consts, 0, 4);
  }

  struct StateSaver
  {
    Program::RenderState old_rstate;
    Render::DepthState old_depthstate;
    Render::RenderTargetConfig old_rt;

    Render::StageBinding::TextureSlot old_texture;

    Buffer *old_buffers_0 = nullptr;
    int old_buffers_offset_0 = 0;

    MTLCullMode old_cull;
    bool save_tex, save_rt;

    bool scissor_on = false;
    int sci_x = 0;
    int sci_y = 0;
    int sci_w = 0;
    int sci_h = 0;

    float vs_cbuffer[4];
    float ps_cbuffer[4];
    int vs_cbuffer_num_stored = 0;
    int ps_cbuffer_num_stored = 0;

    StateSaver(Render& render, bool save_tex, bool save_rt, bool color_write, bool depth_write)
      : save_tex(save_tex)
      , save_rt(save_rt)
    {
      old_rstate = render.cur_rstate;
      old_depthstate = render.cur_depthstate;
      old_rt = render.rt;
      old_cull = render.cur_cull;

      Render::StageBinding &old_vs_stage = render.stages[STAGE_VS];
      Render::StageBinding &old_ps_stage = render.stages[STAGE_PS];

      if (save_tex)
        old_texture = old_ps_stage.textures[0];
      old_buffers_0 = old_vs_stage.buffers[0];
      old_buffers_offset_0 = old_vs_stage.buffers_offset[0];

      memcpy(vs_cbuffer, old_vs_stage.cbuffer.cbuffer, 16);
      memcpy(ps_cbuffer, old_ps_stage.cbuffer.cbuffer, 16);
      vs_cbuffer_num_stored = old_vs_stage.cbuffer.num_stored;
      ps_cbuffer_num_stored = old_ps_stage.cbuffer.num_stored;

      render.cur_depthstate.zenable = 0;
      render.cur_depthstate.depth_write_on = depth_write ? 1 : 0;
      render.dirty_state |= DirtyFlags::DepthState;

      render.setCull(MTLCullModeNone);

      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
        render.cur_rstate.raster_state.blend[i].ablend = false;
      render.cur_rstate.raster_state.writeMask = color_write ? 0xffffffff : MTLColorWriteMaskNone;
      render.cur_rstate.raster_state.a2c = 0;
      render.cur_rstate.raster_state.pad[0] = render.cur_rstate.raster_state.pad[1] = 0;

      scissor_on = render.scissor_on;
      sci_x = render.sci_x;
      sci_y = render.sci_y;
      sci_w = render.sci_w;
      sci_h = render.sci_h;

      if (save_rt)
        render.scissor_on = false;
    }

    ~StateSaver()
    {
      render.cur_depthstate.zenable = old_depthstate.zenable;
      render.cur_depthstate.depth_write_on = old_depthstate.depth_write_on;
      render.dirty_state |= DirtyFlags::DepthState;

      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
        render.cur_rstate.raster_state.blend[i].ablend = old_rstate.raster_state.blend[i].ablend;
      render.cur_rstate.raster_state.writeMask = old_rstate.raster_state.writeMask;
      render.cur_rstate.raster_state.a2c = 0;
      render.cur_rstate.raster_state.pad[0] = render.cur_rstate.raster_state.pad[1] = 0 ;

      render.stages[STAGE_VS].setBuf(0, old_buffers_0, old_buffers_offset_0);
      render.setCull(old_cull);

      if (save_rt)
      {
        render.rt = old_rt;
        render.need_change_rt = 1;
        render.cached_viewport.originX = -FLT_MAX;
        render.checkNoRenderPass();
      }

      if (save_tex)
        render.stages[STAGE_PS].setTex(0, old_texture.texture, old_texture.read_stencil, old_texture.mip_level, old_texture.as_uint);

      memcpy(render.stages[STAGE_VS].cbuffer.cbuffer, vs_cbuffer, 16);
      memcpy(render.stages[STAGE_PS].cbuffer.cbuffer, ps_cbuffer, 16);
      render.stages[STAGE_VS].cbuffer.num_stored = vs_cbuffer_num_stored;
      render.stages[STAGE_PS].cbuffer.num_stored = ps_cbuffer_num_stored;

      render.scissor_on = scissor_on;
      render.sci_x = sci_x;
      render.sci_y = sci_y;
      render.sci_w = sci_w;
      render.sci_h = sci_h;
    }
  };

  static bool is_signed_int_format(Texture* tex)
  {
    if (tex == nullptr)
      return false;
    return tex->base_format == TEXFMT_R32SI;
  }

  static id<MTLCommandBuffer> createCommandBuffer(id<MTLCommandQueue> queue, const char* name)
  {
    id<MTLCommandBuffer> cmdBuf = nil;
#if DAGOR_DBGLEVEL > 0 && MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
    if (@available(macOS 11.0, iOS 14.0, *))
    {
      MTLCommandBufferDescriptor* desc = [[MTLCommandBufferDescriptor alloc] init];
      desc.retainedReferences = NO;
      desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
      cmdBuf = [queue commandBufferWithDescriptor:desc];
      [desc release];

      [cmdBuf addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
      {
        if (buffer.error)
        {
          logerr("Achtung! command buffer %s failed to execute\nerror summary: %s", [buffer.label UTF8String], [buffer.error.localizedDescription UTF8String]);
          id<MTLCommandBufferEncoderInfo> info = buffer.error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
          if (info)
            debug("Error code is %d, at encoder %s", info.errorState, [info.label UTF8String]);
        }
      }];
    }
    else
#endif
    {
      cmdBuf = [queue commandBufferWithUnretainedReferences];
    }
#if DAGOR_DBGLEVEL > 0
    cmdBuf.label = [NSString stringWithFormat:@"%s %llu", name, render.frame];
#endif
    return [cmdBuf retain];
  }

  static void setupCommadBufferErrorHandler(id<MTLCommandBuffer> commandBuffer)
  {
    if (render.max_number_of_frames_to_skip_after_error == 0)
      return;
    [commandBuffer addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
    {
      if (buffer.status == MTLCommandBufferStatusError)
      {
        render.hadError.store(buffer.error ? buffer.error.code : 100500);
      }
    }];
  }

  static uint64_t submitCommandBuffer(id<MTLCommandBuffer> commandBuffer, bool wait = false, bool advance_submits = true)
  {
    uint64_t submit_number = 0;
    if (advance_submits)
    {
      {
        std::unique_lock<std::mutex> lock(render.frame_mutex);
        submit_number = render.submits_scheduled++;
      }
      bool capture_gpu_time = render.capture_command_gpu_time;
      [commandBuffer addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
       {
        {
          std::unique_lock<std::mutex> lock(render.frame_mutex);
          G_ASSERTF(submit_number <= render.submits_scheduled, "%s expected %llu, got %llu", [buffer.label UTF8String], render.submits_scheduled, submit_number);
          G_ASSERTF(submit_number == render.submits_completed + 1, "%s expected %llu, got %llu", [buffer.label UTF8String], render.submits_completed + 1, submit_number);
          render.submits_completed = submit_number;
        }
        render.current_gpu_time += uint64_t(float(buffer.GPUEndTime - buffer.GPUStartTime)*1000000.f);
        if (capture_gpu_time)
          render.current_command_gpu_time = uint64_t(float(buffer.GPUEndTime - buffer.GPUStartTime)*1000000.f);
        render.frame_condition.notify_all();
      }];
    }

    setupCommadBufferErrorHandler(commandBuffer);
    [commandBuffer commit];

    if (wait)
    {
      TIME_PROFILE(wait_for_cmd_buffer);
      [commandBuffer waitUntilCompleted];
    }

    [commandBuffer release];

    return submit_number;
  }

#if DAGOR_DBGLEVEL > 0
  inline void checkRenderAcquired(bool acquired = true)
  {
    G_ASSERTF((render.acquire_depth > 0) == acquired, "depth %d", (int)render.acquire_depth);
    G_ASSERT(render.commandBuffer);
  }
#else
  inline void checkRenderAcquired(bool acquired = true)
  {
  }
#endif

  void Render::ConstBuffer::destroy()
  {
  }

  void Render::ConstBuffer::init()
  {
    device_buffer_offset = 0;
    is_binded = nil;
    is_binded_offset = -1;
    num_stored = 0;
  }

  void Render::ConstBuffer::applyCmd(void* data, int reg_base, int num_regs)
  {
    G_ASSERT(reg_base + num_regs <= MAX_CBUFFER_SIZE);

    memcpy(&cbuffer[reg_base], data, num_regs);
    num_stored = max(num_stored, reg_base + num_regs);
  }

  template <int stage>
  __forceinline static void bindMetalBuffer(id<MTLBuffer> buffer, id<MTLBuffer>& buffer_cache, int& cache_offset, int slot, unsigned offset)
  {
    if (buffer_cache == buffer && buffer && cache_offset == offset)
      return;
    if (buffer_cache == buffer && buffer)
    {
      if (stage == STAGE_VS)
      {
        [render.renderEncoder setVertexBufferOffset : offset atIndex : slot];
      }
      else
      if (stage == STAGE_PS)
      {
        [render.renderEncoder setFragmentBufferOffset : offset atIndex : slot];
      }
      else
      if (stage == STAGE_CS)
      {
        [render.computeEncoder setBufferOffset : offset atIndex : slot];
      }
      cache_offset = offset;
    }
    else if (buffer)
    {
      if (stage == STAGE_VS)
      {
        [render.renderEncoder setVertexBuffer : buffer offset : offset atIndex : slot];
      }
      else
      if (stage == STAGE_PS)
      {
        [render.renderEncoder setFragmentBuffer : buffer offset : offset atIndex : slot];
      }
      else
      if (stage == STAGE_CS)
      {
        [render.computeEncoder setBuffer : buffer offset : offset atIndex : slot];
      }

      buffer_cache = buffer;
      cache_offset = offset;
    }
  }

  template <int stage>
  void Render::ConstBuffer::applyBuffer(int num_reg)
  {
    if (num_reg == 0)
    {
      return;
    }

    if (num_stored > num_reg)
    {
      num_reg = num_stored;
    }

    if (num_last_bound >= num_reg && memcmp(cbuffer, cbuffer_cache, num_reg) == 0)
    {
      G_ASSERT(is_binded);
      return;
    }

    id<MTLBuffer> buf = render.AllocateConstants(num_reg, device_buffer_offset);
    uint8_t* ptr = (uint8_t*)buf.contents + device_buffer_offset;
    G_ASSERT(device_buffer_offset + num_reg <= RingBufferItem::max_constant_size);

    memcpy(ptr, cbuffer, num_reg);
    memcpy(cbuffer_cache, cbuffer, num_reg);
    num_last_bound = num_reg;

    bindMetalBuffer<stage>(buf, is_binded, is_binded_offset, BIND_POINT, device_buffer_offset);

    num_stored = 0;
  }

  void Render::StageBinding::bindVertexStream(uint32_t stream, uint32_t offset)
  {
    id<MTLBuffer> buffer = NULL;

    uint32_t dynamic_offset = 0;
    if (buffers[stream])
      buffer = buffers[stream]->getBuffer(), dynamic_offset = buffers[stream]->getDynamicOffset();

    G_ASSERT(stream < 2);
    bindMetalBuffer<STAGE_VS>(buffer, buffers_metal[stream], buffers_metal_offset[stream], stream,
                    buffers_offset[stream] + dynamic_offset + offset);
  }

  template <int shaderType>
  static __forceinline void apply_immediate_constants(Shader* shader, Render::StageBinding& stage)
  {
    G_ASSERTF(stage.immediate_dword_count > 0, "Shader name and variant: %s", getShaderName(shader->src == nil ? "" : [shader->src cStringUsingEncoding:[NSString defaultCStringEncoding]]).c_str());

    if (shader->immediate_slot == stage.immediate_slot_metal && stage.immediate_dword_count <= stage.immediate_dword_count_metal
        && memcmp(stage.immediate_dwords, stage.immediate_dwords_metal, stage.immediate_dword_count*sizeof(uint32_t)) == 0)
      return;

    int buf_imm_offset = 0;
    id<MTLBuffer> buf_imm = render.AllocateConstants(stage.immediate_dword_count*sizeof(uint32_t), buf_imm_offset);
    memcpy((uint8_t *)buf_imm.contents + buf_imm_offset, stage.immediate_dwords, stage.immediate_dword_count*sizeof(uint32_t));
    memcpy(stage.immediate_dwords_metal, stage.immediate_dwords, stage.immediate_dword_count*sizeof(uint32_t));

    stage.immediate_dword_count_metal = stage.immediate_dword_count;
    stage.immediate_slot_metal = shader->immediate_slot;

    int target = shader->immediate_slot;
    bindMetalBuffer<shaderType>(buf_imm, stage.buffers_metal[target], stage.buffers_metal_offset[target], target, buf_imm_offset);
  }

  template <int stage>
  void Render::StageBinding::apply_vertex_streams()
  {
    G_ASSERT(stage == STAGE_VS);
    for (int i = 0; i < GEOM_BUFFER_COUNT; ++i)
      bindVertexStream(i, 0);
  }

  template <int stage>
  void Render::StageBinding::apply_buffers(Shader* shader)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_buffers; ++i)
    {
      int index = shader->buffers[i].remapped_slot;

      id<MTLBuffer> buffer = NULL;
      unsigned offset = buffers_offset[index];

      uint32_t dynamic_offset = 0;
      if (buffers[index])
        buffer = buffers[index]->getBuffer(), dynamic_offset = buffers[index]->getDynamicOffset();

      if (buffer == nil && index >= CONST_BUFFER_POINT)
      {
        buffer = render.stub_buffer;
        offset = 0;
        dynamic_offset = 0;
      }

      int target = shader->buffers[i].slot;
      bindMetalBuffer<stage>(buffer, buffers_metal[target], buffers_metal_offset[target], target, offset + dynamic_offset);
    }
  }

  template <int stage>
  void Render::StageBinding::apply_textures(Shader* shader)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_tex; i++)
    {
      int slot = shader->tex_binding[i];
      int tslot = shader->tex_remap[i];
      MetalImageType type = shader->tex_type[i].type;

      id<MTLTexture> texture = nil;

      bool is_uav = slot >= MAX_SHADER_TEXTURES;

      const auto &cache = textures[slot];
      if (type == MetalImageType::TexBuffer)
      {
        auto buf_tex = buffers[slot] ? buffers[slot]->getTexture() : nullptr;
        if (buf_tex)
          buf_tex->apply(texture, false, false, false, false);
      }
      else if (cache.texture)
        cache.texture->apply(texture, cache.read_stencil, cache.mip_level, is_uav, cache.as_uint);

      if (texture == nil)
      {
        int index = int(type);
        G_ASSERT(index < int(MetalImageType::Count));
        if (is_uav)
          render.blank_tex_rw[index]->apply(texture, false, 0, false, false);
        else if (shader->tex_type[i].is_uint)
          render.blank_tex_uint[index]->apply(texture, false, 0, false, false);
        else
          render.blank_tex[index]->apply(texture, false, 0, false, false);
      }

      if (texture && textures_metal[tslot] != texture)
      {
        if (stage == STAGE_VS)
          [render.renderEncoder setVertexTexture : texture atIndex : tslot];
        else if (stage == STAGE_PS)
          [render.renderEncoder setFragmentTexture : texture atIndex : tslot];
        else if (stage == STAGE_CS)
          [render.computeEncoder setTexture : texture atIndex : tslot];
        textures_metal[tslot] = texture;
      }
    }
  }

  template <int stage>
  void Render::StageBinding::apply_samplers(Shader* shader)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_samplers; i++)
    {
      int slot = shader->sampler_binding[i];
      int tslot = shader->sampler_remap[i];

      id<MTLSamplerState> sampler = nil;

      G_ASSERT(slot < 16);
      if (samplers[slot])
        sampler = samplers[slot];
      else
        render.blank_tex[0]->applySampler(sampler);

      if (sampler && samplers_metal[tslot] != sampler)
      {
        if (stage == STAGE_VS)
          [render.renderEncoder setVertexSamplerState : sampler atIndex : tslot];
        else if (stage == STAGE_PS)
          [render.renderEncoder setFragmentSamplerState : sampler atIndex : tslot];
        else if (stage == STAGE_CS)
          [render.computeEncoder setSamplerState : sampler atIndex : tslot];
        samplers_metal[tslot] = sampler;
      }
    }
  }

  template <int stage>
  void Render::StageBinding::apply_acceleration_structs(Shader* shader)
  {
    G_ASSERT(shader);

    for (int i = 0; i < shader->accelerationStructureCount; i++)
    {
      const int inDriverSlot = shader->acceleration_structure_remap[i];
      const int inShaderSlot = shader->acceleration_structure_binding[inDriverSlot];

      if (@available(iOS 15.0, macos 11.0, *))
      {
        [render.computeEncoder setAccelerationStructure : (id<MTLAccelerationStructure>)acceleration_structures[inDriverSlot]
                                          atBufferIndex : inShaderSlot];
      }
    }
  }

  template <int stage>
  void Render::StageBinding::apply(Shader* shader)
  {
    G_ASSERT(shader);

    // early out if nothing was bound since last time
    if (stage == STAGE_VS && (buffer_dirty_mask & GEOM_BUFFER_MASK))
      apply_vertex_streams<stage>();
    if (buffer_dirty_mask & shader->buffer_mask)
      apply_buffers<stage>(shader);

    buffer_dirty_mask &= ~(shader->buffer_mask | GEOM_BUFFER_MASK);

    if (shader->immediate_slot >= 0)
      apply_immediate_constants<stage>(shader, *this);

    if (texture_dirty_mask & shader->texture_mask)
      apply_textures<stage>(shader);
    texture_dirty_mask &= ~shader->texture_mask;

    if (sampler_dirty_mask & shader->sampler_mask)
      apply_samplers<stage>(shader);
    sampler_dirty_mask &= ~shader->sampler_mask;

    if (stage == STAGE_CS)
      apply_acceleration_structs<stage>(shader);

    cbuffer.applyBuffer<stage>(shader->num_reg);
  }

  void Render::StageBinding::resetBuffer(int buf)
  {
    buffers_metal[buf] = nil;
    buffers_metal_offset[buf] = -1;
  }

  void Render::StageBinding::resetBuffers()
  {
      for (int i = 0; i < BUFFER_POINT_COUNT; i++)
        resetBuffer(i);
      cbuffer.is_binded = nil;
      cbuffer.num_last_bound = 0;
      immediate_slot_metal = -1;
      buffer_dirty_mask = ~0ull;
  }

  void Render::StageBinding::reset(bool full)
  {
    for (int i = 0; i < MAX_STAGE_TEXTURES; i++)
    {
      textures_metal[i] = nil;
      samplers_metal[i] = nil;
      if (full)
        textures[i] = {};
    }
    sampler_dirty_mask = ~0ull;
    texture_dirty_mask = ~0ull;

    for (int i = 0; i < BUFFER_POINT_COUNT; i++)
    {
      if (full)
      {
        if (buffers[i])
          buffers[i]->bound_slots = 0;
        buffers[i] = nil;
      }
    }

    resetBuffers();
  }

  void Render::StageBinding::removeBuf(Buffer* buf)
  {
    G_ASSERT(buf);
    buf->bound_slots = 0;
    for (int i = 0; i < BUFFER_POINT_COUNT; i++)
    {
      if (buffers[i] == buf)
      {
        buffers[i] = nullptr;
      }
    }
  }

  void Render::StageBinding::removeTex(Texture* tex)
  {
    for (int i = 0; i < MAX_STAGE_TEXTURES; i++)
    {
      if (textures[i].texture == tex)
      {
        textures[i] = {};
      }
    }
  }

  Render::Render()
    : shaders(midmem_ptr(), 128)
    , progs(midmem_ptr(), 128)
    , vdecls(midmem_ptr(), 128)
    , blases(midmem_ptr(), 128)
  {
    inited = false;

    scr_wd = 0;
    scr_ht = 0;

    cur_wd = 0;
    cur_ht = 0;

    scr_bpp = 0;

    cur_vsync = false;

    commandBuffer = nil;

    cur_depthstate.zenable = 1;
    cur_depthstate.zfunc = MTLCompareFunctionGreaterEqual;
    cur_depthstate.depth_write_on = 1;

    render.dirty_state |= DirtyFlags::DepthState;

    need_change_rt = 0;

    depth_clip = MTLDepthClipModeClip;

    main_thread = 0;

    device_name[0] = 0;

    drawable_acquired = false;

    cached_viewport.originX = -FLT_MAX;

    save_backBuffer = false;
  }

  static void PatchShader(int shader, int num_reg, int num_tex)
  {
    Shader* vshader = render.shaders.getObj(shader);
    vshader->num_reg = num_reg;
    vshader->num_tex = num_tex;
    vshader->tex_type[0].value = 0;
    vshader->tex_binding[0] = 0;
    vshader->tex_remap[0] = 0;
    vshader->texture_mask = num_tex ? 1 : 0;
    vshader->num_samplers = num_tex;
    vshader->sampler_mask = num_tex ? 1 : 0;
    vshader->sampler_binding[0] = 0;
    vshader->sampler_remap[0] = 0;
  }

  bool Render::init()
  {
    if (inited)
    {
      return false;
    }

#if USE_METALFX_UPSCALE
    if (@available(iOS 16, macos 13, *))
    {
      upscale.supported = [MTLFXSpatialScalerDescriptor supportsDevice: device];
    }
#endif

#if _TARGET_IOS
    has_image_blocks = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("has_image_blocks", true);
#else
    has_image_blocks = false;
#endif

    max_number_of_frames_to_skip_after_error = ::dgs_get_settings()->getBlockByNameEx("metal")->getInt("framesToSkip", 0);
    validate_framemem_bounds = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("validate_framem_bounds", false);
    hadError = false;

    #if _TARGET_TVOS
    caps.samplerHaveCmprFun = [device supportsFeatureSet:MTLFeatureSet_tvOS_GPUFamily2_v1];
    caps.drawIndxWithBaseVertes = [device supportsFeatureSet:MTLFeatureSet_tvOS_GPUFamily2_v1];
    #elif _TARGET_IOS
    caps.samplerHaveCmprFun = [device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
    caps.drawIndxWithBaseVertes = [device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
    #endif

    caps.readWriteTextureTier1 = device.readWriteTextureSupport == MTLReadWriteTextureTier1 || device.readWriteTextureSupport == MTLReadWriteTextureTier2;
    caps.readWriteTextureTier2 = device.readWriteTextureSupport == MTLReadWriteTextureTier2;

    shadersPreCache.init(get_shader_cache_version());

    commandQueue = [device newCommandQueue];

    query_buffer = createBuffer(QUERIES_MAX * sizeof(uint64_t), MTLResourceStorageModeShared, "query_buffer");
    stub_buffer = createBuffer(MAX_CBUFFER_SIZE, MTLResourceStorageModeShared, "stub_buffer");
    memset(stub_buffer.contents, 0, MAX_CBUFFER_SIZE);

    used_query = 0;

    for (int i = 0; i < QUERIES_MAX; i++)
    {
        queries[i].offset = i * sizeof(uint64_t);
    }

    cur_query_offset = -1;

    for (int i = 0; i < STAGE_MAX; i++)
    {
      StageBinding& stg = stages[i];
      stg.cbuffer.init();

      for (int j = 0; j < BUFFER_POINT_COUNT; j++)
      {
        stg.buffers[j] = NULL;
        stg.buffers_offset[j] = 0;
        stg.buffers_metal[j] = nil;
      }
      for (int j = 0; j < StageBinding::MAX_STAGE_TEXTURES; j++)
      {
        stg.textures[j] = {};
        stg.textures_metal[j] = nil;
        stg.samplers_metal[j] = nil;
      }
    }

    forceClearOnCreate = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("forceClear", 0);

    uint32_t cube_array_type = RES3D_CUBEARRTEX;
#if _TARGET_IOS
    if (![device supportsFamily:MTLGPUFamilyApple4])
      cube_array_type = RES3D_CUBETEX;
#endif
    blank_tex[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_2d", false, true);
    blank_tex[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, RES3D_ARRTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_array", false, true);
    blank_tex[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_DEPTH24, "blank_depth", false, true);
    blank_tex[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, RES3D_CUBETEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_cube", false, true);
    blank_tex[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, RES3D_VOLTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_3d", false, true);
    blank_tex[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_buffer", false, true);
    blank_tex[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_cube_array", false, true);

    blank_tex_uint[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_2d", false, true);
    blank_tex_uint[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, RES3D_ARRTEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_array", false, true);
    blank_tex_uint[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_DEPTH24, "blank_uint_depth", false, true);
    blank_tex_uint[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, RES3D_CUBETEX, TEXCF_RTARGET, TEXFMT_R32UI, "blank_uint_cube", false, true);
    blank_tex_uint[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, RES3D_VOLTEX, TEXCF_RTARGET, TEXFMT_R32UI, "blank_uint_3d", false, true);
    blank_tex_uint[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_buffer", false, true);
    blank_tex_uint[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_RTARGET, TEXFMT_R32UI, "blank_cube_array", false, true);

    blank_tex_rw[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_2d", false, true);
    blank_tex_rw[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, RES3D_ARRTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_array", false, true);
    blank_tex_rw[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_UNORDERED, TEXFMT_DEPTH24, "blankrw_depth", false, true);
    blank_tex_rw[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, RES3D_CUBETEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_cube", false, true);
    blank_tex_rw[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, RES3D_VOLTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_3d", false, true);
    blank_tex_rw[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_buffer", false, true);
    blank_tex_rw[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_cube_array", false, true);

    for (int i = 0; i < int(MetalImageType::Count); ++i)
    {
      if (MetalImageType(i) == MetalImageType::Tex2DDepth) // don't do this for depth
        continue;
      void *ptr; int stride;
      blank_tex[i]->lockimg(&ptr, stride, 0, TEXLOCK_WRITE);
      memset(ptr, 0, stride);
      blank_tex[i]->unlockimg();

      blank_tex_rw[i]->lockimg(&ptr, stride, 0, TEXLOCK_WRITE);
      memset(ptr, 0, stride);
      blank_tex_rw[i]->unlockimg();

      blank_tex_uint[i]->lockimg(&ptr, stride, 0, TEXLOCK_WRITE);
      memset(ptr, 0, stride);
      blank_tex_uint[i]->unlockimg();
    }

    backbuffer = new Texture(nil, TEXFMT_A8R8G8B8, "backbuffer");
    backbuffer->width = wnd_wd;
    backbuffer->height = wnd_ht;
    backbuffer->cflg = TEXCF_RTARGET;

    ::create_critical_section(acquireSec);
    ::create_critical_section(rcSec);

    pthread_threadid_np(NULL, &main_thread);

    cur_thread = main_thread;

    cur_prog = nil;
    vdecl = nil;

    rt.colors[0].texture = backbuffer;
    for (int i = 1; i < Program::MAX_SIMRT; i++)
      rt.colors[i] = {};

    for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
    {
      cur_rstate.raster_state.blend[i].ablend = false;
      cur_rstate.raster_state.blend[i].ablendOp = MTLBlendOperationAdd;
      cur_rstate.raster_state.blend[i].ablendScr = MTLBlendFactorSourceAlpha;
      cur_rstate.raster_state.blend[i].ablendDst = MTLBlendFactorOneMinusSourceAlpha;
      cur_rstate.raster_state.blend[i].sepblend = false;
    }
    cur_rstate.raster_state.writeMask = WRITEMASK_ALL;
    cur_rstate.raster_state.a2c = 0;
    cur_rstate.raster_state.pad[0] = cur_rstate.raster_state.pad[1] = 0;
    cur_rstate.sample_count = 1;
    cur_rstate.pixelFormat[0] = MTLPixelFormatBGRA8Unorm;
    cur_rstate.depthFormat = mainview.depthPixelFormat;
    cur_rstate.stencilFormat = mainview.stencilPixelFormat;

    depth_clip = MTLDepthClipModeClip;
    depth_bias = 0.0f;
    depth_slopebias = 0.0f;

    cur_state = NULL;

    clear_vshd = createVertexShader((const uint8_t*)clear_vs_metal);
    clear_pshdi = createPixelShader((const uint8_t*)clear_ps_metal_i);
    clear_pshdui = createPixelShader((const uint8_t*)clear_ps_metal_ui);
    clear_pshdf = createPixelShader((const uint8_t*)clear_ps_metal_f);
    clear_vdecl = createVDdecl(clear_dcl);

    PatchShader(clear_vshd, 1, 0);
    PatchShader(clear_pshdi, 1, 1);
    PatchShader(clear_pshdui, 1, 1);
    PatchShader(clear_pshdf, 1, 1);

    clear_progi = createProgram(clear_vshd, clear_pshdi, clear_vdecl, 0, 0);
    clear_progui = createProgram(clear_vshd, clear_pshdui, clear_vdecl, 0, 0);
    clear_progf = createProgram(clear_vshd, clear_pshdf, clear_vdecl, 0, 0);

    copy_vshd = createVertexShader((const uint8_t*)copy_vs_metal);
    copy_pshd = createPixelShader((const uint8_t*)copy_ps_metal);

    PatchShader(copy_vshd, 1, 0);
    PatchShader(copy_pshd, 0, 1);

    copy_prog = createProgram(copy_vshd, copy_pshd, clear_vdecl, 0, 0);

    {
      NSError *error = nil;

      MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
      NSString* src = [NSString stringWithUTF8String:g_clear_cs_src];
      id<MTLLibrary> lib = [drv3d_metal::render.device newLibraryWithSource : src options : options error : &error];
      [options release];

      if (error == nil && [[lib functionNames] count] > 0)
      {
        id<MTLFunction> func = [[lib newFunctionWithName : [[lib functionNames] objectAtIndex:0]] retain];

        clear_cs_pipeline = [device newComputePipelineStateWithFunction : func error : &error];
        if (error)
          clear_cs_pipeline = nil;
      }
    }

    {
      NSError *error = nil;

      MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
      NSString* src = [NSString stringWithUTF8String:g_copy_cs_src];
      id<MTLLibrary> lib = [drv3d_metal::render.device newLibraryWithSource : src options : options error : &error];
      [options release];

      if (error == nil && [[lib functionNames] count] > 0)
      {
        id<MTLFunction> func = [[lib newFunctionWithName : [[lib functionNames] objectAtIndex:0]] retain];

        copy_cs_pipeline = [device newComputePipelineStateWithFunction : func error : &error];
        if (error)
          copy_cs_pipeline = nil;
      }
    }

    {
      float clear_mesh[8] = { -1,-1, -1, 1, 1,-1, 1, 1 };
      clear_mesh_buffer = d3d::create_vb(sizeof(clear_mesh), 0, "metal system clear");

      float *verts = NULL;
      clear_mesh_buffer->lock(0, 0, (void**)&verts, VBLOCK_WRITEONLY);
      if (verts)
        memcpy(verts, clear_mesh, sizeof(clear_mesh));
      clear_mesh_buffer->unlock();
    }

    tmp_copy_buff = createBuffer(tmp_copy_buf_size, MTLResourceStorageModeShared, "tmp_copy_buffer");

    strcpy(device_name, [[device name] UTF8String]);

    can_use_async_pso_compilation = get_aync_pso_compilation_supported();

    [render.mainview isHDRAvailable];

    if (d3d::get_driver_desc().caps.hasBindless)
    {
      bindlessTextureIdBuffer = new Buffer(BINDLESS_TEXTURE_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless texture id buffer");
      bindlessBufferIdBuffer = new Buffer(BINDLESS_BUFFER_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless buffer id buffer");
      bindlessSamplerIdBuffer = new Buffer(BINDLESS_SAMPLER_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless sampler id buffer");

      setBindlessResources();
    }

    if (@available(iOS 17, macOS 11.0, *))
    {
      if (render.device.supportsRaytracing)
      {
        defaultBlas = [render.device newAccelerationStructureWithSize : 4096];
        bottomLevelAccelerationStructures = [[NSMutableArray alloc] init];
      }
    }

#if DAGOR_DBGLEVEL > 0
    signpost_queue = dispatch_queue_create("com.gaijin.signpost_queue", NULL);
    signpost_listener = [[MTLSharedEventListener alloc] initWithDispatchQueue : signpost_queue];

    signpost_event = [device newSharedEvent];
    [signpost_event notifyListener : signpost_listener
                           atValue : ~0ull
                             block : ^(id<MTLSharedEvent> sharedEvent, uint64_t value)
                             {
                             }];
#endif

    return true;
  }

  void Render::executeUpscale(Texture *color, Texture *output, uint32_t colorMode)
  {
    checkRenderAcquired();

#if USE_METALFX_UPSCALE
    if (@available(iOS 16, macos 13, *))
    {
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::None);

      uint64_t hash = 0;
      hash_combine(hash, color->getWidth());
      hash_combine(hash, color->getHeight());
      hash_combine(hash, color->metal_format);
      hash_combine(hash, output->getWidth());
      hash_combine(hash, output->getHeight());
      hash_combine(hash, output->metal_format);
      hash_combine(hash, colorMode);

      if (upscale.spatialScalers.find(hash) == upscale.spatialScalers.end())
      {
        constexpr static MTLFXSpatialScalerColorProcessingMode color_modes[] = {MTLFXSpatialScalerColorProcessingModePerceptual,
          MTLFXSpatialScalerColorProcessingModeLinear, MTLFXSpatialScalerColorProcessingModeHDR};

        MTLFXSpatialScalerDescriptor *spatialScalerDescriptor = [[MTLFXSpatialScalerDescriptor alloc] init];

        spatialScalerDescriptor.inputWidth = color->getWidth();
        spatialScalerDescriptor.inputHeight = color->getHeight();
        spatialScalerDescriptor.colorTextureFormat = color->metal_format;
        spatialScalerDescriptor.outputWidth = output->getWidth();
        spatialScalerDescriptor.outputHeight = output->getHeight();
        spatialScalerDescriptor.outputTextureFormat = output->metal_format;
        spatialScalerDescriptor.colorProcessingMode = color_modes[colorMode];
        upscale.spatialScalers[hash] = [spatialScalerDescriptor newSpatialScalerWithDevice : device];
        [spatialScalerDescriptor release];
      }
      id<MTLFXSpatialScaler> scaler = upscale.spatialScalers[hash];

      scaler.outputTexture = output->apiTex->texture;
      scaler.colorTexture = color->apiTex->texture;
      [scaler encodeToCommandBuffer : commandBuffer];
      scaler.outputTexture = nullptr;
      scaler.colorTexture = nullptr;
    }
#endif
  }

  bool Render::isInited()
  {
    return inited;
  }

  void Render::cleanupFrame()
  {
    stages[STAGE_VS].reset(true);
    stages[STAGE_PS].reset(true);
    stages[STAGE_CS].reset(true);

    int local_frame = (frame + 1) % MAX_FRAMES_TO_RENDER;

    // make sure its safe to free resources
    {
      TIME_PROFILE(wait_prev_frame);

      std::unique_lock<std::mutex> lock(frame_mutex);
      uint64_t frame_submit_number = frames_completed[local_frame];
      frame_condition.wait(lock, [frame_submit_number]
      {
        return frame_submit_number <= render.submits_completed;
      });
    }

    if ((frame + 1) >= MAX_FRAMES_TO_RENDER)
    {
      TIME_PROFILE(free_resources);
      auto & resources2free = resources2delete_frames[local_frame];
      {
        constexpr uint32_t max_const_buffers = 2;
        constexpr uint32_t max_dynamic_buffers = 2;
        std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);
        if (constant_buffers_free.size() > max_const_buffers)
        {
          for (int i = max_const_buffers; i<constant_buffers_free.size(); i++)
          {
            auto& buf = constant_buffers_free[i];
            [buf.buf release];
            buf.buf = nil;
          }
          constant_buffers_free.resize(max_const_buffers);
        }
        if (dynamic_buffers_free.size() > max_dynamic_buffers)
        {
          for (int i = max_dynamic_buffers; i<dynamic_buffers_free.size(); i++)
          {
            auto& buf = dynamic_buffers_free[i];
            [buf.buf release];
            buf.buf = nil;
          }
          dynamic_buffers_free.resize(max_dynamic_buffers);
        }
        for (auto & buf : resources2free.constant_buffers)
          constant_buffers_free.push_back(buf);
        resources2free.constant_buffers.clear();
        for (auto & buf : resources2free.dynamic_buffers)
          dynamic_buffers_free.push_back(buf);
        resources2free.dynamic_buffers.clear();
      }
      progs.lock();
      for (auto index : resources2free.programs)
        progs.freeIndex(index)->release();
      progs.unlock();
      resources2free.programs.clear();

      shaders.lock();
      for (auto index : resources2free.shaders)
        shaders.freeIndex(index)->release();
      shaders.unlock();
      resources2free.shaders.clear();

      vdecls.lock();
      for (auto index : resources2free.vdecls)
      {
        auto decl = vdecls.freeIndex(index);
        if (decl)
          decl->release();
      }
      vdecls.unlock();
      resources2free.vdecls.clear();

      for (auto & buf : resources2free.buffers)
        buf->release();
      resources2free.buffers.clear();

      for (auto & tex : resources2free.textures)
        tex->release();
      resources2free.textures.clear();

      for (auto & res : resources2free.native_resources)
        [res release];
      resources2free.native_resources.clear();

      blases.lock();
      for (auto as : resources2free.acceleration_structs)
      {
        G_ASSERT(as);
        if (as->index >= 0)
        {
          if ([bottomLevelAccelerationStructures count] > as->index)
            [bottomLevelAccelerationStructures replaceObjectAtIndex:as->index withObject:defaultBlas];
          else
            G_ASSERTF(as->was_built == false, "%d of %d", as->index, (int)[bottomLevelAccelerationStructures count]);
          blases.freeIndex(as->index);
        }
        if (as->acceleration_struct)
          [as->acceleration_struct release];
        delete as;
      }
      blases.unlock();
      resources2free.acceleration_structs.clear();
    }

    {
      TIME_PROFILE(swap_frames);
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      eastl::swap(resources2delete, resources2delete_frames[frame%MAX_FRAMES_TO_RENDER]);
    }
    frame++;
  }

  void Render::endFrame()
  {
    flush(false, true);

    last_gpu_time = current_gpu_time.load();
    current_gpu_time.store(0);

    if (number_of_frames_to_skip_after_error)
      number_of_frames_to_skip_after_error--;
    int error = hadError.load();
    if (error)
    {
      if (max_number_of_frames_to_skip_after_error && number_of_frames_to_skip_after_error == 0)
      {
        // 6 - insufficient permission
        if (error == 6 || error == MTLCommandBufferErrorNotPermitted)
        {
          D3D_ERROR("METAL has error during command buffer execution, drawing in background");
        }
        else
        {
          D3D_ERROR("METAL has error during command buffer execution %d, skipping %d frames",
                 error, max_number_of_frames_to_skip_after_error);
        }
        number_of_frames_to_skip_after_error = max_number_of_frames_to_skip_after_error;
      }
      hadError.store(0);
    }

    {
      TIME_PROFILE(precache_tick);
      shadersPreCache.tickCache();
    }

    drawable_acquired = false;
    need_change_rt = 1;
  }

  void Render::doClearTexture(uint16_t tex_width, uint16_t tex_height, uint8_t slices, uint8_t depth, uint8_t levels, id<MTLTexture> tex, int base_format, bool use_dxt)
  {
    G_ASSERT(tex);

    G_ASSERT(blitEncoder != nil);

    // clear buffer so we copy zeroes
    [blitEncoder fillBuffer:tmp_copy_buff range:NSMakeRange(0, tmp_copy_buf_size) value:0];

    for (int slice = 0; slice < slices; ++slice)
      for (int level = 0; level < levels; ++level)
      {
        unsigned local_depth = std::max(depth >> level, 1);
        for (unsigned d = 0; d < local_depth; ++d)
        {
          int row_pitch = 0, slice_pitch = 0;
          Texture::getStride(base_format, tex_width, tex_height, level, row_pitch, slice_pitch);

          unsigned width = max(tex_width >> level, 1);
          unsigned height = max(tex_height >> level, 1);

          G_ASSERT(tmp_copy_buf_size >= row_pitch);
          unsigned height_step = std::min(height, unsigned(tmp_copy_buf_size / row_pitch));
          if (use_dxt)
            height_step = max(height_step & ~3, 4u);

          for (unsigned h = 0; h < height; h += height_step)
          {
            unsigned height_left = std::min(height - h, height_step);
            [blitEncoder copyFromBuffer : tmp_copy_buff
                           sourceOffset : 0
                      sourceBytesPerRow : row_pitch
                    sourceBytesPerImage : tmp_copy_buf_size
                             sourceSize : { width, height_left, 1 }
                              toTexture : tex
                       destinationSlice : slice
                       destinationLevel : level
                      destinationOrigin : { 0, h, d } ];
        }
      }
    }
  }

  void Render::doTexCopyRegion(const TexCopyRegion &cmd)
  {
    MTLOrigin origin = { cmd.src_x, cmd.src_y, cmd.src_z };
    MTLSize size = { cmd.src_w, cmd.src_h, cmd.src_d };
    MTLOrigin destinationOrigin = { cmd.dest_x, cmd.dest_y, cmd.dest_z };

    int koef = 0;

    #if _TARGET_PC_MACOSX
    if (cmd.dst_format == MTLPixelFormatBC1_RGBA ||
        cmd.dst_format == MTLPixelFormatBC1_RGBA_sRGB ||
        cmd.dst_format == MTLPixelFormatBC4_RUnorm)
    {
      koef = 8;
    }
    else
    if (cmd.dst_format == MTLPixelFormatBC3_RGBA_sRGB ||
        cmd.dst_format == MTLPixelFormatBC3_RGBA ||
        cmd.dst_format == MTLPixelFormatBC6H_RGBFloat ||
        cmd.dst_format == MTLPixelFormatBC5_RGUnorm)
    {
      koef = 16;
    }
    #endif
    if (@available(iOS 13, macOS 11.0, *))
    {
      if (cmd.dst_format == MTLPixelFormatEAC_RGBA8_sRGB || cmd.dst_format == MTLPixelFormatEAC_RGBA8
          || cmd.dst_format == MTLPixelFormatEAC_RG11Unorm)
        koef = 16;
    }

    G_ASSERT(cmd.src);
    G_ASSERT(cmd.dst);
    G_ASSERT(blitEncoder != nil);

    int dst_mip = cmd.dest_level % cmd.dest_levels;
    int dst_slice = cmd.dest_level / cmd.dest_levels;
    int src_mip = cmd.src_level % cmd.src_levels;
    int src_slice = cmd.src_level / cmd.src_levels;
    if (koef > 0 && !cmd.src_dxt)
    {
      int stride = cmd.src_w * koef;
      int sz = cmd.src_w * cmd.src_h * koef;

      if (cmd.src_w < 4 || cmd.src_h < 4)
        return;

      id<MTLBuffer> tmp_buf = sz * cmd.src_d <= tmp_copy_buf_size ? tmp_copy_buff :
        createBuffer(sz*cmd.src_d, MTLResourceStorageModeShared, "copy buffer");
      G_ASSERT(sz*cmd.src_d <= tmp_copy_buf_size || tmp_buf != tmp_copy_buff);

      [blitEncoder copyFromTexture : cmd.src
                       sourceSlice : src_slice
                       sourceLevel : src_mip
                      sourceOrigin : origin
                        sourceSize : size
                          toBuffer : tmp_buf
                 destinationOffset : 0
            destinationBytesPerRow : stride
          destinationBytesPerImage : sz];

      size.width = cmd.src_w * 4;
      size.height = cmd.src_h * 4;

      [blitEncoder copyFromBuffer : tmp_buf
                     sourceOffset : 0
                sourceBytesPerRow : stride
              sourceBytesPerImage : sz
                       sourceSize : size
                        toTexture : cmd.dst
                 destinationSlice : dst_slice
                 destinationLevel : dst_mip
                destinationOrigin : destinationOrigin];

      if (sz*cmd.src_d > tmp_copy_buf_size)
        render.queueBufferForDeletion(tmp_buf);
    }
    else
    {
#if DAGOR_DBGLEVEL > 0
      G_ASSERTF_RETURN(cmd.src_format == cmd.dst_format, ,"Can't blit %s(%d) to %s(%d)", [cmd.src.label UTF8String],
                cmd.src_format, [cmd.dst.label UTF8String], cmd.dst_format);
#endif
      MTLSize size = {
        cmd.src_dxt ? max((uint16_t)4, cmd.src_w) : cmd.src_w,
        cmd.src_dxt ? max((uint16_t)4, cmd.src_h) : cmd.src_h,
        cmd.src_d };
      [blitEncoder copyFromTexture : cmd.src
                       sourceSlice : src_slice
                       sourceLevel : src_mip
                      sourceOrigin : origin
                        sourceSize : size
                         toTexture : cmd.dst
                  destinationSlice : dst_slice
                  destinationLevel : dst_mip
                 destinationOrigin : destinationOrigin];
    }
  }

  void Render::flushQueuedCommands()
  {
    TIME_PROFILE(upload);

    ensureHaveEncoderExceptRender(nullptr, Render::EncoderType::None, false);
    if (textures2upload.size() || buffers2copy.size() || regions2copy.size() || textures2clear.size() || buffers2clear.size())
    {
      id<MTLCommandBuffer> cmdBuf = createCommandBuffer(commandQueue, "Command buffer flush");

      id<MTLBuffer> buffers[3] = { nil, nil, nil };
      int buffer_offsets[3] = { -1, -1, -1 };
      for (int i = 0; i < buffers2copy.size(); ++i)
      {
        bool need_set = render.computeEncoder == nil;
        ensureHaveEncoderExceptRender(cmdBuf, Render::EncoderType::Compute, false, "CopyBuffer");
        if (need_set)
          [computeEncoder setComputePipelineState:copy_cs_pipeline];

        auto& rc = buffers2copy[i];
        G_ASSERT((rc.size & 3) == 0);

        int buf_size_offset = 0;
        id<MTLBuffer> buf_size = AllocateConstants(16, buf_size_offset);
        uint32_t * data = (uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset);
        data[0] = rc.size / 4;
        data[1] = rc.src_offset / 4;
        data[2] = rc.dst_offset / 4;

        bindMetalBuffer<STAGE_CS>(buf_size, buffers[0], buffer_offsets[0], 0, buf_size_offset);
        bindMetalBuffer<STAGE_CS>(rc.dst, buffers[1], buffer_offsets[1], 1, 0);
        bindMetalBuffer<STAGE_CS>(rc.src, buffers[2], buffer_offsets[2], 2, 0);
        [render.computeEncoder dispatchThreadgroups  : MTLSizeMake((data[0] + 63) >> 6, 1, 1)
                               threadsPerThreadgroup : MTLSizeMake(64, 1, 1)];
      }
      current_cs_pipeline = nullptr;

      bool needBlit = textures2upload.size() || regions2copy.size() || textures2clear.size() || buffers2clear.size();
      if (needBlit)
        ensureHaveEncoderExceptRender(cmdBuf, Render::EncoderType::Blit, false, "CopyTextures");

      for (int i = 0; i < buffers2clear.size(); ++i)
      {
        Buffer::BufTex *buf = buffers2clear[i];

        G_ASSERT(buf);
        G_ASSERT(buf->buffer);
        G_ASSERT(blitEncoder != nil);
        [blitEncoder fillBuffer : buf->buffer
                          range : NSMakeRange(0, buf->bufsize)
                          value : 0];
        buf->initialized = true;
      }

      for (int i = 0; i < textures2upload.size(); ++i)
      {
        UploadTex& rc = textures2upload[i];
        MTLSize size = { (uint32_t)rc.w, (uint32_t)rc.h, (uint32_t)rc.d };
        MTLOrigin origin = { 0, 0, 0 };

        G_ASSERT([rc.buf length] >= rc.d * rc.imageSize);
        G_ASSERT(blitEncoder != nil);
        [blitEncoder copyFromBuffer:rc.buf sourceOffset:0 sourceBytesPerRow:rc.pitch
                sourceBytesPerImage:rc.imageSize sourceSize:size toTexture:rc.tex
                destinationSlice:rc.layer destinationLevel:rc.level destinationOrigin:origin];
        rc.buf = nil;
      }

      for (int i = 0; i < textures2clear.size(); ++i)
      {
        auto &t = textures2clear[i];
        doClearTexture(t.width, t.height, t.slices, t.depth, t.levels, t.metalTex, t.base_format, t.use_dxt);
      }

      for (int i = 0; i < regions2copy.size(); ++i)
        doTexCopyRegion(regions2copy[i]);

      textures2upload.clear();
      buffers2copy.clear();
      regions2copy.clear();
      textures2clear.clear();
      buffers2clear.clear();

      ensureHaveEncoderExceptRender(cmdBuf, Render::EncoderType::None, false);
      submitCommandBuffer(cmdBuf, false, false);
    }
  }

  void Render::flush(bool wait, bool present)
  {
    TIME_PROFILE(flush);

    checkRenderAcquired();

    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);

    if (save_backBuffer)
    {
      G_ASSERT(drawable_acquired);
      id <MTLTexture> curBackBuffer = [mainview getBackBuffer];
      id <MTLTexture> savedBackBuffer = [mainview getSavedBackBuffer];

      MTLOrigin origin = { 0, 0, 0 };
      MTLSize size = { curBackBuffer.width, curBackBuffer.height, 1 };

      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit);
      [blitEncoder copyFromTexture : curBackBuffer
                       sourceSlice : 0
                       sourceLevel : 0
                      sourceOrigin : origin
                        sourceSize : size
                         toTexture : savedBackBuffer
                  destinationSlice : 0
                  destinationLevel : 0
                 destinationOrigin : origin];

#if _TARGET_PC_MACOSX
      [blitEncoder synchronizeTexture:savedBackBuffer
                               slice : 0
                               level : 0];
#endif
      wait = true;
      save_backBuffer = false;
    }

    if (present)
    {
      if (drawable_acquired)
      {
        TIME_PROFILE(present);
        [mainview presentDrawable : commandBuffer];
        backbuffer->apiTex->rt_texture = nil;
        backbuffer->apiTex->texture = nil;
      }
    }

    flushQueuedCommands();
    frames_completed[frame % MAX_FRAMES_TO_RENDER] = submitCommandBuffer(commandBuffer, wait);

    if (present)
    {
      cleanupFrame();
      setBindlessResources();
    }

    commandBuffer = createCommandBuffer(commandQueue, "Command buffer");
  }

  bool Render::setBlendFactor(E3DCOLOR factor)
  {
    G_ASSERT(renderEncoder);
    checkRenderAcquired();

    G_ASSERT(renderEncoder != nil);
    [renderEncoder setBlendColorRed : factor.r / 255.f
                              green : factor.g / 255.f
                               blue : factor.b / 255.f
                              alpha : factor.a / 255.f];
    return true;
  }

  void Render::clearView(int what, E3DCOLOR color, float z, uint32_t stencil)
  {
    checkRenderAcquired();

    float c[4] = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
    if (rt.isFullscreenViewport())
    {
      if (what & CLEAR_TARGET)
      {
        for (int i = 0; i < Program::MAX_SIMRT; ++i)
        {
          auto &attach = rt.colors[i];
          attach.clear_value[0] = c[0];
          attach.clear_value[1] = c[1];
          attach.clear_value[2] = c[2];
          attach.clear_value[3] = c[3];
          attach.load_action = MTLLoadActionClear;
        }
      }
      if (what & CLEAR_DISCARD_TARGET)
      {
        for (int i = 0; i < Program::MAX_SIMRT; ++i)
        {
          auto &attach = rt.colors[i];
          attach.load_action = MTLLoadActionDontCare;
        }
      }
      if (what & (CLEAR_ZBUFFER | CLEAR_STENCIL))
      {
        G_ASSERT(what & CLEAR_ZBUFFER);
        rt.depth.load_action = MTLLoadActionClear;
        rt.depth.clear_value[0] = z;
        rt.depth.clear_value[1] = stencil;
      }
      if (what & (CLEAR_DISCARD_ZBUFFER | CLEAR_DISCARD_STENCIL))
      {
        G_ASSERT(what & CLEAR_DISCARD_ZBUFFER);
        rt.depth.load_action = MTLLoadActionDontCare;
      }
      updateEncoder(what, c, z, stencil);
    }
    else
    {
      if (renderEncoder == nil)
        updateEncoder();
      doClear(nullptr, 0, 0, z, c, false, what & CLEAR_TARGET, what & CLEAR_ZBUFFER);
    }
  }

  static bool viewportChanged(const MTLViewport& viewport, const Render::Viewport& vp)
  {
    return viewport.originX != vp.x || viewport.originY != vp.y || viewport.width != vp.w
      || viewport.height != vp.h || viewport.znear != vp.minz || viewport.zfar != vp.maxz;
  }

  void Render::setViewport(int x, int y, int w, int h, float minz, float maxz)
  {
    checkRenderAcquired();

    rt.vp.x = x;
    rt.vp.y = y;
    rt.vp.w = w;
    rt.vp.h = h;
    rt.vp.minz = minz;
    rt.vp.maxz = maxz;

    if (viewportChanged(cached_viewport, rt.vp))
      dirty_state |= DirtyFlags::Viewport;
  }

  void Render::setScissor(int x, int y, int w, int h)
  {
    checkRenderAcquired();

    sci_x = x;
    sci_y = y;
    sci_w = w;
    sci_h = h;

    if (scissor_on)
      dirty_state |= DirtyFlags::Scissor;
  }

  void Render::setBuff(unsigned rstage, BufferType bf_type, int slot, Buffer *buffer, int offset, int stride)
  {
    checkRenderAcquired();

    Render::StageBinding &stage = stages[rstage];
    if (rstage == STAGE_VS && bf_type == GEOM_BUFFER)
    {
      G_ASSERT(stride < 256);
      cur_rstate.vbuffer_stride[slot] = stride;
    }
    stage.setBuf(RemapBufferSlot(bf_type, slot), buffer, offset);
  }

  void Render::setImmediateConst(unsigned rstage, const uint32_t *data, unsigned num_words)
  {
    checkRenderAcquired();
    Render::StageBinding &stage = stages[rstage];
    if (num_words)
      memcpy(stage.immediate_dwords, data, num_words * sizeof(uint32_t));
    stage.immediate_dword_count = num_words;
  }

  void Render::setIBuff(Buffer *buffer)
  {
    checkRenderAcquired();

    ibuffer = buffer;
    ibuffertype = buffer && (buffer->bufFlags & SBCF_INDEX32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
  }

  void Render::setTexture(unsigned rstage, int slot, Texture* tex, bool use_sampler, int mip_level, bool as_uint)
  {
    checkRenderAcquired();

    Render::StageBinding &stage = stages[rstage];
    stage.setTex(slot, tex, tex ? tex->isReadStencil() : false, mip_level, as_uint, use_sampler);
  }

  int Render::setSampler(unsigned rstage, int slot, id<MTLSamplerState> sampler)
  {
    checkRenderAcquired();

    Render::StageBinding &stage = stages[rstage];
    stage.setSampler(slot, sampler);

    return 1;
  }

  int Render::updateBindlessResource(uint32_t index, D3dResource *res)
  {
    checkRenderAcquired();

    G_ASSERT(res);
    auto resType = res->restype();
    if (RES3D_SBUF != resType)
    {
      G_ASSERTF(index < BINDLESS_TEXTURE_COUNT, "bindless texture slot out of range: %d (max slot id: %d)", index, BINDLESS_TEXTURE_COUNT);
      if (res)
      {
        auto texture = dynamic_cast<Texture*>(res);
        G_ASSERT(texture);

        bindlessTextures[index] = texture;

        if (maxBindlessTextureId < index + 1)
          maxBindlessTextureId = index + 1;
      }
      else
      {
        bindlessTextures[index] = nullptr;
      }
    }
    else
    {
      G_ASSERTF(index < BINDLESS_BUFFER_COUNT, "bindless buffer slot out of range: %d (max slot id: %d)", index, BINDLESS_BUFFER_COUNT);

      if (res)
      {
        auto buffer = dynamic_cast<Buffer*>(res);
        G_ASSERT(buffer);

        bindlessBuffers[index] = buffer;

        if (maxBindlessBufferId < index + 1)
          maxBindlessBufferId = index + 1;
      }
      else
      {
        bindlessBuffers[index] = nullptr;
      }
    }

    return 1;
  }

  int Render::copyBindlessResources(uint32_t resourceType, uint32_t src, uint32_t dst, uint32_t count)
  {
    checkRenderAcquired();

    for (uint32_t i = 0; i < count; i++)
    {
      if (resourceType != RES3D_SBUF)
      {
        bindlessTextures[dst + i] = bindlessTextures[src + i];
        bindlessTextures[src + i] = nullptr;
      }
      else
      {
        bindlessBuffers[dst + i] = bindlessBuffers[src + i];
        bindlessBuffers[src + i] = nullptr;
      }
    }
    return 1;
  }

  int Render::updateBindlessResourcesToNull(uint32_t resourceType, uint32_t index, uint32_t count)
  {
    checkRenderAcquired();

    for (uint32_t i = 0; i < count; i++)
    {
      if (resourceType != RES3D_SBUF)
        bindlessTextures[index + i] = nullptr;
      else
        bindlessBuffers[index + i] = nullptr;
    }
    return 1;
  }

  bool Render::draw(int prim_type, int start_vertex, int vertex_count, uint32_t num_instances, uint32_t start_instance)
  {
    checkRenderAcquired();

    if (prim_type == 6 || vertex_count == 0)
      return false;

    if (number_of_frames_to_skip_after_error)
      return true;

    if (num_instances == 0)
      return false;

    if (applyStates() == false)
      return true;

    G_ASSERT(renderEncoder);
    G_ASSERT(caps.drawIndxWithBaseVertes || start_instance == 0);
    if (caps.drawIndxWithBaseVertes)
        [renderEncoder drawPrimitives : convertPrimitiveType(prim_type)
                          vertexStart : start_vertex
                          vertexCount : vertex_count
                        instanceCount : num_instances
                         baseInstance : start_instance];
      else
        [renderEncoder drawPrimitives : convertPrimitiveType(prim_type)
                          vertexStart : start_vertex
                          vertexCount : vertex_count
                        instanceCount : num_instances];

    return true;
  }

  bool Render::drawIndexed(int prim_type, int start_index, int index_count, int base_vertex, uint32_t num_instances, uint32_t start_instance)
  {
    checkRenderAcquired();
    if (prim_type == 6 || index_count == 0)
      return false;

    if (number_of_frames_to_skip_after_error)
      return true;

    if (num_instances == 0)
      return false;

    if (applyStates() == false)
      return true;

    int offset = ibuffertype == MTLIndexTypeUInt32 ? start_index * 4 : start_index * 2;
    base_vertex = max(0, base_vertex);

    G_ASSERT(renderEncoder);
    G_ASSERT(ibuffer);
    if (caps.drawIndxWithBaseVertes)
    {
      [renderEncoder drawIndexedPrimitives : convertPrimitiveType(prim_type)
                                indexCount : index_count
                                 indexType : ibuffertype
                               indexBuffer : ibuffer->getBuffer()
                         indexBufferOffset : offset + ibuffer->getDynamicOffset()
                             instanceCount : num_instances
                                baseVertex : base_vertex
                              baseInstance : start_instance];
    }
    else
    {
      if (base_vertex > 0 || start_instance > 0)
      {
        Render::StageBinding &vs_stage = stages[STAGE_VS];
        auto vd = vdecl ? vdecl : cur_prog->vdecl;
        G_ASSERT(vd->num_streams <= GEOM_BUFFER_COUNT);
        for (int streamIdx = 0; streamIdx < vd->num_streams; ++streamIdx)
        {
          const bool isStreamPerVertex = (vd->instanced_mask & (1 << streamIdx)) == 0;
          const unsigned streamOffsetBase = isStreamPerVertex ? base_vertex : start_instance;
          const uint32_t streamOffset = cur_rstate.vbuffer_stride[streamIdx] * streamOffsetBase;
          vs_stage.bindVertexStream(streamIdx, streamOffset);
          vs_stage.buffer_dirty_mask |= 1ull << streamIdx;
        }
      }

      [renderEncoder drawIndexedPrimitives : convertPrimitiveType(prim_type)
                                indexCount : index_count
                                 indexType : ibuffertype
                               indexBuffer : ibuffer->getBuffer()
                         indexBufferOffset : offset + ibuffer->getDynamicOffset()
                             instanceCount : num_instances];
    }

    return true;
  }

  bool Render::drawIndirect(int prim_type, Sbuffer* buffer, uint32_t offset)
  {
    checkRenderAcquired();
    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      D3D_ERROR("Metal: trying to use unsupported drawIndirect");
      return false;
    }

    if (prim_type == 6)
      return false;

    if (number_of_frames_to_skip_after_error)
      return true;

    if (applyStates() == false)
      return true;

    G_ASSERT(renderEncoder);
    G_ASSERT(buffer);
    Buffer *buf = (Buffer *)buffer;
    [renderEncoder drawPrimitives : convertPrimitiveType(prim_type)
                   indirectBuffer : buf->getBuffer()
             indirectBufferOffset : offset + buf->getDynamicOffset()];

    return true;
  }

  bool Render::drawIndirectIndexed(int prim_type, Sbuffer* buffer, uint32_t offset)
  {
    checkRenderAcquired();
    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      D3D_ERROR("Metal: trying to use unsupported drawIndirectIndexed");
      return false;
    }

    if (prim_type == 6)
      return false;

    if (number_of_frames_to_skip_after_error)
      return true;

    if (applyStates() == false)
      return true;

    G_ASSERT(renderEncoder);
    G_ASSERT(ibuffer);
    G_ASSERT(buffer);
    Buffer *buf = (Buffer *)buffer;
    [renderEncoder drawIndexedPrimitives : convertPrimitiveType(prim_type)
                               indexType : ibuffertype
                             indexBuffer : ibuffer->getBuffer()
                       indexBufferOffset : ibuffer->getDynamicOffset()
                          indirectBuffer : buf->getBuffer()
                    indirectBufferOffset : offset + buf->getDynamicOffset()];

    return true;
  }

  bool Render::drawUP(int prim_type, int prim_count, const uint16_t* ind, const void* ptr, int numvert, int stride_bytes)
  {
    checkRenderAcquired();

    G_ASSERT(renderEncoder != nil);

    struct BufferUP : public Buffer
    {
      BufferUP()
      {
        isDynamic = true;
        fast_discard = true;
      }
    };
    BufferUP tempBufferVB, tempBufferIB;

    int vbuf_offset = 0;
    tempBufferVB.dynamic_buffer = AllocateConstants(numvert*stride_bytes, vbuf_offset);
    tempBufferVB.dynamic_frame = frame;
    tempBufferVB.dynamic_offset = 0;

    memcpy((uint8_t*)tempBufferVB.dynamic_buffer.contents + vbuf_offset, ptr, numvert*stride_bytes);
    d3d::setvsrc_ex(0, &tempBufferVB, vbuf_offset, stride_bytes);

    if (ind)
    {
      int ibuf_offset = 0;
      tempBufferIB.dynamic_buffer = AllocateConstants(prim_count * 2, ibuf_offset);
      tempBufferIB.dynamic_frame = frame;
      tempBufferIB.dynamic_offset = 0;

      memcpy((uint8_t*)tempBufferIB.dynamic_buffer.contents + ibuf_offset, ind, prim_count * 2);

      d3d::setind(&tempBufferIB);
      drawIndexed(prim_type, ibuf_offset / 2, prim_count, 0, 1);
    }
    else
      draw(prim_type, 0, prim_count, 1);

    d3d::setind(nullptr);
    d3d::setvsrc_ex(0, nullptr, 0, 0);

    return true;
  }

  int Render::createVertexShader(const uint8_t* code)
  {
    Shader* shader = new Shader();

    if (!shader->compileShader((const char*)code, async_pso_compilation && can_use_async_pso_compilation))
    {
      shader->release();

      return BAD_VPROG;
    }

    shaders.lock();
    int index = shaders.getFreeIndex(shader);
    shaders.unlock();

    return index;
  }

  void Render::deleteVertexShader(int vs)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.shaders.push_back(vs);
  }

  int Render::createPixelShader(const uint8_t* code)
  {
    Shader* shader = new Shader();

    if (!shader->compileShader((const char*)code, async_pso_compilation && can_use_async_pso_compilation))
    {
      delete shader;
      return BAD_FSHADER;
    }

    shaders.lock();
    int index = shaders.getFreeIndex(shader);
    shaders.unlock();

    return index;
  }

  void Render::deletePixelShader(int ps)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.shaders.push_back(ps);
  }

  int Render::createVDdecl(VSDTYPE* d)
  {
    VDecl* decl = new VDecl(d);

    vdecls.lock();
    int index = vdecls.getFreeIndex(decl);
    vdecls.unlock();

    return index;
  }

  void Render::setVDecl(VDECL index)
  {
    checkRenderAcquired();

    vdecls.lock();
    vdecl = vdecls.getObj(index);
    vdecls.unlock();
  }

  void Render::deleteVDecl(VDECL vdecl)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.vdecls.push_back(vdecl);
  }

  int Render::createProgram(int vp, int ps, VDECL vdecl, unsigned *strides, unsigned streams)
  {
    shaders.lock();
    Shader* vshader = shaders.getObj(vp);

    if (!vshader)
    {
      shaders.unlock();
      return BAD_PROGRAM;
    }

    Shader* pshader = shaders.getObj(ps);
    shaders.unlock();

    // pshader can be nullptr

    vdecls.lock();
    VDecl* decl = vdecls.getObj(vdecl);
    vdecls.unlock();

    Program* program = new Program(vshader, pshader, decl);

    progs.lock();
    int index = progs.getFreeIndex(program);
    progs.unlock();

    return index;
  }

  void Render::setProgram(int prog)
  {
    checkRenderAcquired();

    progs.lock();
    Program* new_prog = render.progs.getObj(prog);
    progs.unlock();

    if (new_prog && (!cur_prog || new_prog->vshader != cur_prog->vshader ||
        new_prog->pshader != cur_prog->pshader || new_prog->cshader != cur_prog->cshader))
    {
      if (!new_prog->cshader)
      {
        uint64_t ps_hash_old = !cur_prog || !cur_prog->pshader ? ~0ull : cur_prog->pshader->shader_hash;
        uint64_t ps_hash_new = new_prog->pshader ? new_prog->pshader->shader_hash : ~0ull;
        bool vs_new = !cur_prog || !cur_prog->vshader || cur_prog->vshader->shader_hash != new_prog->vshader->shader_hash;
        bool ps_new = ps_hash_old != ps_hash_new;
        if (vs_new)
        {
          vdecl = NULL;
          stages[STAGE_VS].buffer_dirty_mask |= new_prog->vshader->buffer_mask;
          stages[STAGE_VS].sampler_dirty_mask |= new_prog->vshader->sampler_mask;
          stages[STAGE_VS].texture_dirty_mask |= new_prog->vshader->texture_mask;
        }
        if (ps_new && new_prog->pshader)
        {
          stages[STAGE_PS].buffer_dirty_mask |= new_prog->pshader->buffer_mask;
          stages[STAGE_PS].sampler_dirty_mask |= new_prog->pshader->sampler_mask;
          stages[STAGE_PS].texture_dirty_mask |= new_prog->pshader->texture_mask;
        }
        if (vs_new || ps_new)
          cur_state = NULL;
      }
      else
      {
        stages[STAGE_CS].buffer_dirty_mask |= new_prog->cshader->buffer_mask;
        stages[STAGE_CS].sampler_dirty_mask |= new_prog->cshader->sampler_mask;
        stages[STAGE_CS].texture_dirty_mask |= new_prog->cshader->texture_mask;
      }
    }
    cur_prog = new_prog;
    using_async_pso_compilation = async_pso_compilation && can_use_async_pso_compilation;
  }

  void Render::deleteProgram(int prog)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.programs.push_back(prog);
  }

  int Render::createComputeProgram(const uint8_t* code)
  {
    Shader* shader = new Shader();

    if (!shader->compileShader((const char*)code, false))
    {
      delete shader;
      return BAD_FSHADER;
    }

    Program* program = new Program(shader);

    progs.lock();
    int index = progs.getFreeIndex(program);
    progs.unlock();

    return index;
  }

  void Render::deleteComputeProgram(int cs)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.programs.push_back(cs);
  }

  void Render::setConst(unsigned stage, unsigned reg_base, const float *data, unsigned num_regs)
  {
    checkRenderAcquired();

    stages[stage].cbuffer.applyCmd((void *)data, reg_base*16, num_regs*16);
  }

  void Render::clearRwBuffer(Sbuffer* buff, const float *val)
  {
    checkRenderAcquired();

    clearRwBufferi(buff, (unsigned *)val);
  }

  void Render::clearRwBufferi(Sbuffer *buff, const unsigned *val)
  {
    checkRenderAcquired();

    // handle only this special case for now. i searched through the code and it seems we don't use anything else
    G_ASSERT(buff);
    Buffer *buf = (Buffer*)buff;
    if ((val[0] == 0 || val[0] == 0xffffffff))
    {
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "ClearBuffer");
      G_ASSERT(buf->getDynamicOffset() == 0);
      [blitEncoder fillBuffer : buf->getBuffer()
                        range : NSMakeRange(0, buf->bufSize)
                        value : (val[0] & 0xff)];
    }
    else
    {
      if (computeEncoder == nil)
      {
        stages[STAGE_CS].reset();
        current_cs_pipeline = nullptr;
      }
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Compute, false, "ClearBuffer");

      if (current_cs_pipeline != clear_cs_pipeline)
        [computeEncoder setComputePipelineState:clear_cs_pipeline];
      current_cs_pipeline = clear_cs_pipeline;

      int buf_size_offset = 0;
      id<MTLBuffer> buf_size = AllocateConstants(32, buf_size_offset);
      *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset) = val[0];
      *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 16) = buf->bufSize / 4;

      G_ASSERT(buf->getDynamicOffset() == 0);
      [computeEncoder setBuffer : buf_size
                         offset : buf_size_offset
                        atIndex : 0];
      [computeEncoder setBuffer : buf->getBuffer()
                         offset : 0
                        atIndex : 1];
      [computeEncoder dispatchThreadgroups : MTLSizeMake((buf->bufSize / 4 + 63) >> 6, 1, 1)
                     threadsPerThreadgroup : MTLSizeMake(64, 1, 1)];
    }
  }

  void Render::clearBuffer(Buffer::BufTex* buff)
  {
    // called on buffer creation, can be queued to the beginning of the flush
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    buffers2clear.emplace_back(buff);
  }

  void Render::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
  {
    checkRenderAcquired();

    if (number_of_frames_to_skip_after_error)
      return;

    G_ASSERT(thread_group_x);
    G_ASSERT(thread_group_y);
    G_ASSERT(thread_group_z);

    if (!cur_prog || !cur_prog->cshader)
      return;

    if (!computeEncoder)
    {
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Compute);

      stages[STAGE_CS].reset();
      current_cs_pipeline = nullptr;
    }

    if (current_cs_pipeline != cur_prog->csPipeline)
        [computeEncoder setComputePipelineState : cur_prog->csPipeline];
    current_cs_pipeline = cur_prog->csPipeline;

    stages[STAGE_CS].apply<STAGE_CS>(cur_prog->cshader);

    [computeEncoder dispatchThreadgroups : MTLSizeMake(thread_group_x, thread_group_y, thread_group_z)
                   threadsPerThreadgroup : MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z)];
  }

  void Render::dispatch_indirect(Sbuffer* buffer, uint32_t offset)
  {
    checkRenderAcquired();

    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      D3D_ERROR("Metal: trying to use unsupported dispatch_indirect");
      return;
    }
    if (number_of_frames_to_skip_after_error)
      return;

    if (!cur_prog || !cur_prog->cshader)
      return;

    if (!computeEncoder)
    {
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Compute);

      stages[STAGE_CS].reset();
      current_cs_pipeline = nullptr;
    }

    if (current_cs_pipeline != cur_prog->csPipeline)
        [computeEncoder setComputePipelineState : cur_prog->csPipeline];
    current_cs_pipeline = cur_prog->csPipeline;

    stages[STAGE_CS].apply<STAGE_CS>(cur_prog->cshader);

    G_ASSERT(buffer);
    Buffer *indirect_buffer = (Buffer*)buffer;
    [computeEncoder dispatchThreadgroupsWithIndirectBuffer : indirect_buffer->getBuffer()
                                      indirectBufferOffset : offset + indirect_buffer->getDynamicOffset()
                                     threadsPerThreadgroup : MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z)];
  }

  void Render::generateMips(Texture* tex)
  {
    checkRenderAcquired();

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "GenerateMips");

    G_ASSERT(tex);
    [blitEncoder generateMipmapsForTexture : tex->apiTex->texture];
  }

  static inline bool is_depth_format(unsigned int f)
  {
    f &= TEXFMT_MASK;
    return f >= TEXFMT_FIRST_DEPTH && f <= TEXFMT_LAST_DEPTH;
  }

  void Render::copyTex(Texture* src, Texture* dest)
  {
    checkRenderAcquired();

    if (src->samples > 1)
    {
      StateSaver saver(*this, false, true, false, false);

      for (int i = 0; i < Program::MAX_SIMRT; i++)
        rt.colors[i] = {};
      rt.depth = {};
      dirty_state &= ~DirtyFlags::Viewport;
      need_change_rt = 1;

      auto &attach = is_depth_format(src->cflg) ? rt.depth : rt.colors[0];
      attach.texture = src;
      attach.resolve_target = dest;
      attach.level = 0;
      attach.layer = 0;
      attach.load_action = MTLLoadActionLoad;
      attach.store_action = MTLStoreActionMultisampleResolve;

      checkNoRenderPass();

      updateEncoder();

      return;
    }

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "CopyTex");

    MTLOrigin origin = { 0, 0, 0 };
    MTLSize size = { 1, 1, 1 };

    G_ASSERT(dest);
    G_ASSERT(src);
    for (int i = 0; i < dest->mipLevels; i++)
    {
      size.width = max(dest->width >> i, 1);
      size.height = max(dest->height >> i, 1);

      if (dest->use_dxt)
      {
        size.width = max<unsigned>(size.width, 4u);
        size.height = max<unsigned>(size.height, 4u);
      }

      if (dest->type == RES3D_VOLTEX)
        size.depth = max(dest->depth >> i, 1);

      int count = 1;
      if (dest->type == RES3D_CUBETEX)
        count = 6;
      else
      if (dest->type == RES3D_ARRTEX)
          count = dest->depth;

      G_ASSERT(src != backbuffer || drawable_acquired);
      id<MTLTexture> src_tex = src->apiTex->texture;
      for (int j = 0; j<count; j++)
      {
        [blitEncoder copyFromTexture : src_tex
                         sourceSlice : j
                         sourceLevel : i
                        sourceOrigin : origin
                          sourceSize : size
                           toTexture : dest->apiTex->texture
                    destinationSlice : j
                    destinationLevel : i
                   destinationOrigin : origin];
      }
    }
  }

  void Render::copyTexRegion(Texture* src,
                             int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
                             Texture* dest, int dest_level, int dest_x, int dest_y, int dest_z)
  {
    G_ASSERT(!src->isStub());
    uint64_t cur_thread = 0;
    pthread_threadid_np(NULL, &cur_thread);

    bool from_thread = render.acquire_depth == 0 || cur_thread != render.cur_thread;

    TexCopyRegion region(dest, dest_level, dest_x, dest_y, dest_z, src, src_level, src_x, src_y, src_z, src_w, src_h, src_d);
    if (from_thread)
    {
      std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
      regions2copy.push_back(region);
    }
    else
    {
      checkRenderAcquired();

      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "CopyTexRegion");

      doTexCopyRegion(region);
    }
  }

  void Render::setRT(int index, const RenderAttachment& attach)
  {
    checkRenderAcquired();
    checkNoRenderPass();

    G_ASSERT(!attach.texture || !attach.texture->memoryless || attach.load_action != MTLLoadActionLoad);
    rt.colors[index] = attach;

    need_change_rt = 1;
    cached_viewport.originX = -FLT_MAX;

    dirty_state &= ~DirtyFlags::Viewport;

    if (attach.texture)
    {
      sci_x = 0;
      sci_y = 0;
      sci_w = max(1, attach.texture->width >> attach.level);
      sci_h = max(1, attach.texture->height >> attach.level);
    }
  }

  void Render::setDepth(const RenderAttachment& attach)
  {
    checkNoRenderPass();
    checkRenderAcquired();

    G_ASSERT(!attach.texture || !attach.texture->memoryless || attach.load_action != MTLLoadActionLoad);

    rt.depth = attach;

    need_change_rt = 1;
    cached_viewport.originX = -FLT_MAX;

    dirty_state &= ~DirtyFlags::Viewport;

    if (attach.texture)
    {
      sci_x = 0;
      sci_y = 0;
      sci_w = max(1, attach.texture->width >> attach.level);
      sci_h = max(1, attach.texture->height >> attach.level);
    }
  }

  void Render::clearTex(Texture* tex, const int val[4], int level, int layer)
  {
    acquireOwnerShip();

    doClear(tex, level, layer, 0.0f, (float *)val, true, true, false);

    releaseOwnerShip();
  }

  void Render::clearTex(Texture* tex, const unsigned val[4], int level, int layer)
  {
    acquireOwnerShip();

    doClear(tex, level, layer, 0.0f, (float *)val, true, true, false);

    releaseOwnerShip();
  }

  void Render::clearTex(Texture* tex, const float val[4], int level, int layer)
  {
    acquireOwnerShip();

    doClear(tex, level, layer, 0.0f, (float *)val, false, true, false);

    releaseOwnerShip();
  }

  void Render::clearTexture(Texture* tex)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    textures2clear.emplace_back(tex);
  }

  void Render::copyBuffer(Sbuffer* srcb, int srcOffset, Sbuffer* dstb, int dstOffset, int size)
  {
    checkRenderAcquired();

    Buffer* dst = (Buffer*)dstb;
    Buffer* src = (Buffer*)srcb;

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "CopyBuffer");

    G_ASSERT(dst);
    G_ASSERTF(dst->getDynamicOffset() == 0, "Buffer %s should have zero dynamic offset, but got %d", dst->getResName(),
      dst->getDynamicOffset());
    [blitEncoder copyFromBuffer : src->getBuffer()
                   sourceOffset : srcOffset + src->getDynamicOffset()
                       toBuffer : dst->getBuffer()
              destinationOffset : dstOffset
                           size : size ? size : dst->bufSize];
  }

  void Render::copyTex(Texture *src, Texture *dst, RectInt *rsrc, RectInt *rdst)
  {
    checkRenderAcquired();

    float src_rect[4];
    src_rect[0] = rsrc ? (float)rsrc->left / (float)src->getWidth() : 0.f;
    src_rect[1] = rsrc ? (float)(rsrc->right - rsrc->left) / (float)src->getWidth() : 1.f;
    src_rect[2] = rsrc ? (float)rsrc->top / (float)src->getHeight() : 0.f;
    src_rect[3] = rsrc ? (float)(rsrc->bottom - rsrc->top) / (float)src->getHeight() : 1.f;

    int dst_rect[4];
    if (rdst)
    {
      dst_rect[0] = rdst->left;
      dst_rect[1] = rdst->right - rdst->left;
      dst_rect[2] = rdst->top;
      dst_rect[3] = rdst->bottom - rdst->top;
    }
    else
      dst_rect[0] = -1;

    StateSaver saver(*this, true, true, true, false);

    StageBinding& vs_stage = stages[STAGE_VS];
    StageBinding& ps_stage = stages[STAGE_PS];

    vs_stage.setBuf(0, (Buffer*)clear_mesh_buffer);
    setProgram(copy_prog);
    setRenderTarget(dst, 0, 0);

    ps_stage.setTex(0, src);
    setConstants(vs_stage, src_rect);

    if (applyStates())
    {
      if (dst_rect[0] != -1)
      {
        MTLViewport viewport = { (double)dst_rect[0], (double)dst_rect[2], (double)dst_rect[1], (double)dst_rect[3], 0.f, 1.f };
        [renderEncoder setViewport : viewport];
        cached_viewport.originX = -FLT_MAX;
      }

      [renderEncoder drawPrimitives : MTLPrimitiveTypeTriangleStrip
                        vertexStart : 0
                        vertexCount : 4
                      instanceCount : 1];
    }
  }

  void Render::restoreRT()
  {
    setRT(0, { .texture = backbuffer });
    for (int i = 1; i < Program::MAX_SIMRT; ++i)
      setRT(i, {});
  }

  void Render::restoreDepth()
  {
    setDepth({ .texture = nullptr });
  }

  void Render::doClear(Texture* dst, int dst_level, int dst_layer, float z, float color[4],
                       bool clear_int, bool color_write, bool depth_write)
  {
    StateSaver saver(*this, false, dst != nullptr, color_write, depth_write);

    StageBinding& vs_stage = stages[STAGE_VS];
    StageBinding& ps_stage = stages[STAGE_PS];

    vs_stage.setBuf(0, (Buffer*)clear_mesh_buffer);

    setProgram(clear_int ? (is_signed_int_format(dst) ? clear_progi : clear_progui) : clear_progf);
    if (dst)
      setRenderTarget(dst, dst_level, dst_layer);

    setConstants(vs_stage, z);
    setConstants(ps_stage, color);

    if (applyStates())
      [renderEncoder drawPrimitives : MTLPrimitiveTypeTriangleStrip
                        vertexStart : 0
                        vertexCount : 4
                      instanceCount : 1];
  }

  bool Render::updateProgram()
  {
    if (!cur_prog)
      return false;

    if (cur_state == NULL || (cur_state && !(last_rstate == cur_rstate)))
    {
        cur_state = shadersPreCache.getState(cur_prog, vdecl, cur_rstate, using_async_pso_compilation);

        last_rstate = cur_rstate;

        if (!cur_state)
        {
            return false;
        }
#if DAGOR_DBGLEVEL > 0
        if (cur_prog->pshader)
        {
          bool has_color = false;
          for (int i = 0; i < Program::MAX_SIMRT; ++i)
            has_color |= rt.colors[i].texture != nullptr;
          G_ASSERT(has_color || rt.depth.texture || (!has_color && !rt.depth.texture && cur_prog->pshader->output_mask == 0));
        }
#endif
        [renderEncoder setRenderPipelineState : cur_state];
        put_encoder_label([cur_state.label UTF8String]);
    }
    return true;
  }

  void Render::updateStates()
  {
    if (dirty_state & DirtyFlags::DepthState)
    {
      int prev_zenable = cur_depthstate.zenable;
      int prev_depth_write_on = cur_depthstate.depth_write_on;
      int prev_stencil = cur_depthstate.stencil_enable;

      if (!rt.depth.texture)
      {
        cur_depthstate.zenable = 0;
        cur_depthstate.depth_write_on = 0;
        cur_depthstate.stencil_enable = false;
      }

      updateDepthState();

      if (!rt.depth.texture)
      {
        cur_depthstate.zenable = prev_zenable;
        cur_depthstate.depth_write_on = prev_depth_write_on;
        cur_depthstate.stencil_enable = prev_stencil;
      }

      [renderEncoder setDepthStencilState : cur_depthstate.depthState];
    }

    if (dirty_state & DirtyFlags::StencilRef)
      [renderEncoder setStencilReferenceValue:stencil_ref];

    constexpr float MINIMUM_REPRESENTABLE_D16 = 2e-5f;
    if (dirty_state & DirtyFlags::DepthBias)
      [renderEncoder setDepthBias : depth_bias / MINIMUM_REPRESENTABLE_D16
                       slopeScale : depth_slopebias
                            clamp : 0.f];

    if (dirty_state & DirtyFlags::Viewport)
    {
      MTLViewport viewport;
      viewport.originX = rt.vp.x;
      viewport.originY = rt.vp.y;

      viewport.width = rt.vp.w;
      viewport.height = rt.vp.h;

      viewport.znear = rt.vp.minz;
      viewport.zfar = rt.vp.maxz;

      [renderEncoder setViewport : viewport];
      cached_viewport = viewport;
    }

    if (dirty_state & DirtyFlags::Scissor)
    {
      MTLScissorRect scissor;

      if (scissor_on)
      {
        scissor.x = sci_x;
        scissor.y = sci_y;

        scissor.width = sci_w;
        scissor.height = sci_h;
      }
      else
      {
        scissor.x = 0;
        scissor.y = 0;

        scissor.width = cur_wd;
        scissor.height = cur_ht;
      }

      [renderEncoder setScissorRect : scissor];
    }

    if (dirty_state & DirtyFlags::Cull)
      [renderEncoder setCullMode : cur_cull];

    if (dirty_state & DirtyFlags::Query)
      [renderEncoder setVisibilityResultMode : cur_query_offset != -1 ? MTLVisibilityResultModeCounting : MTLVisibilityResultModeDisabled
                                      offset : cur_query_offset != -1 ? cur_query_offset : 0];

    dirty_state = 0;
  }

  bool Render::applyStates()
  {
    if (need_change_rt)
      updateEncoder();

    if (updateProgram() == false)
      return false;

    if (dirty_state)
      updateStates();

    TIME_PROFILE(applyStates);

    G_ASSERT(renderEncoder);

    stages[STAGE_VS].apply<STAGE_VS>(cur_prog->vshader);
    if (cur_prog->pshader)
      stages[STAGE_PS].apply<STAGE_PS>(cur_prog->pshader);

    return true;
  }

  static void fillAttachment(MTLRenderPassAttachmentDescriptor* desc, const RenderAttachment &attach, int &samples, int &layers, bool depth = false)
  {
    if (attach.texture == nullptr)
      return;

    drv3d_metal::Texture *tex = attach.texture;
    G_ASSERT(tex && tex->apiTex);

    desc.texture = tex->apiTex->rt_texture;
    desc.loadAction = attach.load_action;
    desc.storeAction = attach.store_action;
    desc.level = attach.level;
    if (desc.storeAction == MTLStoreActionMultisampleResolve)
    {
      G_ASSERT(attach.resolve_target && attach.resolve_target->apiTex);
      G_ASSERT(tex->apiTex->rt_texture.sampleCount > 1);
      G_ASSERT(attach.resolve_target->apiTex->texture.sampleCount == 1);
      desc.resolveTexture = attach.resolve_target->apiTex->texture;
    }
    if (tex->type == RES3D_VOLTEX && attach.layer != d3d::RENDER_TO_WHOLE_ARRAY)
      desc.depthPlane = attach.layer;
    else if ((tex->type == RES3D_CUBETEX || tex->type == RES3D_ARRTEX) && attach.layer != d3d::RENDER_TO_WHOLE_ARRAY)
      desc.slice = attach.layer;

    G_ASSERT(samples == -1 || samples == desc.texture.sampleCount);
    samples = desc.texture.sampleCount;

    int layer = -1;
    if (attach.layer == d3d::RENDER_TO_WHOLE_ARRAY && tex->type != RES3D_TEX)
    {
      G_ASSERT(tex->type != RES3D_CUBEARRTEX);
      layer = tex->type == RES3D_CUBETEX ? 6 : tex->depth;
    }
    G_ASSERT(layers == -1 || layer == layers);
    layers = layer;
  }

  bool Render::RenderTargetConfig::isFullscreenViewport() const
  {
    bool vp_used = render.dirty_state & DirtyFlags::Viewport;
    for (int i = 0; i < Program::MAX_SIMRT; ++i)
      if (colors[i].texture)
      {
        if (vp.x == 0 && vp.y == 0 && vp.w == colors[i].texture->width && vp.h == colors[i].texture->height)
        {
          vp_used = false;
          break;
        }
      }
    if (depth.texture && vp.x == 0 && vp.y == 0 && vp.w == depth.texture->width && vp.h == depth.texture->height)
      vp_used = false;

    return vp_used == false;
  }

  void Render::updateEncoder(unsigned mask, float* color, float z, uint8_t stencil)
  {
    TIME_PROFILE(updateEncoder);
    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::None);

    if (rt.colors[0].texture == backbuffer)
    {
      if (!drawable_acquired)
      {
        {
          std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
          flushQueuedCommands();
        }
        submitCommandBuffer(commandBuffer);

        commandBuffer = createCommandBuffer(commandQueue, "Command buffer backbuffer");

        TIME_PROFILE(acquireDrawable);
        [mainview acquireDrawable];

        drawable_acquired = true;
      }
      backbuffer->apiTex->rt_texture = is_rgb_backbuffer ? [mainview getsRGBBackBuffer] : [mainview getBackBuffer];
      backbuffer->apiTex->texture = [mainview getsRGBBackBuffer];

      cur_wd = wnd_wd;
      cur_ht = wnd_ht;
    }

    bool fullscreen_vp = rt.isFullscreenViewport();

    MTLRenderPassDescriptor *render_desc = [MTLRenderPassDescriptor renderPassDescriptor];

    int samples = -1, layers = -1, clear_mask = 0;
    float clear_depth = 0.0f, clear_color[4];

    bool has_color = false;
    for (int i = 0; i < Program::MAX_SIMRT; i++)
    {
      MTLRenderPassColorAttachmentDescriptor* colorAttachment = render_desc.colorAttachments[i];
      RenderAttachment& attach = rt.colors[i];

      fillAttachment(colorAttachment, attach, samples, layers, false);
      if (attach.load_action == MTLLoadActionClear)
      {
        if (!fullscreen_vp && render_pass)
        {
          clear_color[0] = attach.clear_value[0];
          clear_color[1] = attach.clear_value[1];
          clear_color[2] = attach.clear_value[2];
          clear_color[3] = attach.clear_value[3];
          clear_mask |= CLEAR_TARGET;
          colorAttachment.loadAction = MTLLoadActionLoad;
        }
        colorAttachment.clearColor = MTLClearColorMake(attach.clear_value[0], attach.clear_value[1], attach.clear_value[2], attach.clear_value[3]);
      }

      // if there's gonna be pass break ensure its gonna be loaded back properly
      attach.load_action = MTLLoadActionLoad;

      cur_rstate.pixelFormat[i] = colorAttachment.texture ? colorAttachment.texture.pixelFormat : MTLPixelFormatInvalid;
      has_color |= cur_rstate.pixelFormat[i] != MTLPixelFormatInvalid;

      if (attach.texture)
      {
        cur_wd = max(1, attach.texture->width>>attach.level);
        cur_ht = max(1, attach.texture->height>>attach.level);
      }
    }

    if (rt.depth.texture)
    {
      RenderAttachment& attach = rt.depth;
      fillAttachment(render_desc.depthAttachment, attach, samples, layers, true);
      if (attach.load_action == MTLLoadActionClear)
      {
        render_desc.depthAttachment.clearDepth = attach.clear_value[0];
        if (!fullscreen_vp && render_pass)
        {
          clear_depth = attach.clear_value[0];
          clear_mask |= CLEAR_ZBUFFER;
          render_desc.depthAttachment.loadAction = MTLLoadActionLoad;
        }
      }

      if (render_desc.depthAttachment.texture.pixelFormat == MTLPixelFormatDepth32Float_Stencil8)
      {
        fillAttachment(render_desc.stencilAttachment, attach, samples, layers, true);
        if (attach.load_action == MTLLoadActionClear)
          render_desc.stencilAttachment.clearStencil = attach.clear_value[1];
      }

      if (!has_color)
      {
        cur_wd = max(1, attach.texture->width>>attach.level);
        cur_ht = max(1, attach.texture->height>>attach.level);
      }
    }
    else if (!has_color)
    {
      G_ASSERT(dirty_state & DirtyFlags::Viewport);
      G_ASSERT(rt.vp.w > 0);
      G_ASSERT(rt.vp.h > 0);

      // apple hardware only supports 4 so lets use 4 if any msaa is set
      samples = forcedSampleCount < 2 ? 1 : 4;
      render_desc.renderTargetWidth = rt.vp.x + rt.vp.w;
      render_desc.renderTargetHeight = rt.vp.y + rt.vp.h;
      render_desc.defaultRasterSampleCount = samples;
    }

    G_ASSERT(samples > 0);
    cur_rstate.sample_count = samples;
    cur_rstate.is_volume = layers > 0;
    cur_rstate.depthFormat = render_desc.depthAttachment.texture ? render_desc.depthAttachment.texture.pixelFormat : MTLPixelFormatInvalid;
    cur_rstate.stencilFormat = render_desc.stencilAttachment.texture ? render_desc.stencilAttachment.texture.pixelFormat : MTLPixelFormatInvalid;

    render_desc.renderTargetArrayLength = layers > 0 ? layers : 0;
    render_desc.visibilityResultBuffer = query_buffer;

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor : render_desc];
#if DAGOR_DBGLEVEL > 0
    renderEncoder.label = render_desc.colorAttachments[0].texture ? render_desc.colorAttachments[0].texture.label : (render_desc.depthAttachment.texture ? render_desc.depthAttachment.texture.label : @"DagorRenderEncoder");
#endif

    // if there's gonna be pass break ensure its gonna be loaded back properly
    for (int i = 0; i < Program::MAX_SIMRT; i++)
      rt.colors[i].load_action = MTLLoadActionLoad;
    rt.depth.load_action = MTLLoadActionLoad;

    prepareBindlessResourcesGraphics(renderEncoder);

    stages[STAGE_VS].reset();
    stages[STAGE_PS].reset();

    render.dirty_state |= DirtyFlags::DepthState;

    cur_state = NULL;

    [renderEncoder setDepthClipMode : depth_clip];

    if ((fabs(depth_bias) > 0.00000001f && fabs(depth_slopebias) > 0.00000001f))
      dirty_state |= DirtyFlags::DepthBias;
    if (cur_cull != MTLCullModeNone)
      dirty_state |= DirtyFlags::Cull;
    if (cur_query_offset != -1)
      dirty_state |= DirtyFlags::Query;
    if (scissor_on)
      dirty_state |= DirtyFlags::Scissor;

    need_change_rt = 0;

    cached_viewport.originX = -FLT_MAX;

    if (!fullscreen_vp && render_pass && clear_mask)
    {
      render.doClear(nullptr, 0, 0, clear_depth, clear_color, false, clear_mask & CLEAR_TARGET, clear_mask & CLEAR_ZBUFFER);
    }
  }

  void Render::updateDepthState()
  {
    for (int i = 0; i < dstate_cache.size(); i++)
    {
      DepthState& cache = dstate_cache[i];

      if (cur_depthstate == cache)
      {
        cur_depthstate.depthState = cache.depthState;
        return;
      }
    }

    if (!depthStateDesc)
    {
      depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
    }

    depthStateDesc.depthCompareFunction = cur_depthstate.zenable ? (MTLCompareFunction)cur_depthstate.zfunc : MTLCompareFunctionAlways;
    depthStateDesc.depthWriteEnabled = cur_depthstate.depth_write_on;

    if (cur_depthstate.stencil_enable)
    {
      depthStateDesc.frontFaceStencil.stencilCompareFunction = (MTLCompareFunction)cur_depthstate.stencil_func;
      depthStateDesc.frontFaceStencil.stencilFailureOperation = (MTLStencilOperation)cur_depthstate.sfail;
      depthStateDesc.frontFaceStencil.depthFailureOperation = (MTLStencilOperation)cur_depthstate.szfail;
      depthStateDesc.frontFaceStencil.depthStencilPassOperation = (MTLStencilOperation)cur_depthstate.spass;
      depthStateDesc.frontFaceStencil.readMask = cur_depthstate.stencil_read_mask;
      depthStateDesc.frontFaceStencil.writeMask = cur_depthstate.stencil_write_mask;

      depthStateDesc.backFaceStencil.stencilCompareFunction = (MTLCompareFunction)cur_depthstate.stencil_func;
      depthStateDesc.backFaceStencil.stencilFailureOperation = (MTLStencilOperation)cur_depthstate.sfail;
      depthStateDesc.backFaceStencil.depthFailureOperation = (MTLStencilOperation)cur_depthstate.szfail;
      depthStateDesc.backFaceStencil.depthStencilPassOperation = (MTLStencilOperation)cur_depthstate.spass;
      depthStateDesc.backFaceStencil.readMask = cur_depthstate.stencil_read_mask;
      depthStateDesc.backFaceStencil.writeMask = cur_depthstate.stencil_write_mask;
    }
    else
    {
      depthStateDesc.frontFaceStencil = nil;
      depthStateDesc.backFaceStencil = nil;
    }

    cur_depthstate.depthState = [device newDepthStencilStateWithDescriptor : depthStateDesc];

    append_items(dstate_cache, 1, &cur_depthstate);
  }

  static NSAutoreleasePool *g_render_pool = nil;

  void Render::acquireOwnerShip()
  {
    ::enter_critical_section(acquireSec);

    if (acquire_depth == 0)
    {
      uint64_t thread = 0;
      pthread_threadid_np(NULL, &thread);
      cur_thread.store(thread);

      g_render_pool = [[NSAutoreleasePool alloc] init];

      G_ASSERT(commandQueue);
      if (commandBuffer == nil)
        commandBuffer = createCommandBuffer(commandQueue, "Command buffer");
    }

    acquire_depth++;
  }

  void Render::releaseOwnerShip()
  {
    G_ASSERT(acquire_depth > 0);
    if (acquire_depth == 1)
    {
      cur_thread = main_thread;
      if (commandBuffer)
        ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::None);

      [g_render_pool release];
    }
    acquire_depth--;
    ::leave_critical_section(acquireSec);
  }

  int Render::tryReleaseOwnerShip()
  {
    if (is_main_thread())
      return 0;
    if (try_enter_critical_section(acquireSec))
    {
      G_ASSERT(acquire_depth <= 1);
      acquire_depth = 0;
      cur_thread = main_thread;
      return ::full_leave_critical_section(acquireSec);
    }
    return -1;
  }

  Render::Query* Render::createQuery()
  {
    Query* q = NULL;

    for (int i = 0; i < used_query; i++)
    {
      if (!queries[i].used)
      {
        q = &queries[i];
        break;
      }
    }

    if (!q)
    {
      if (used_query >= QUERIES_MAX)
      {
        DAG_FATAL("Count of used queries was exceeded of max available value");
        return NULL;
      }

      q = &queries[used_query];

      used_query++;
    }

    q->status = 0;
    q->used = 1;

    return q;
  }

  void Render::startFence(Query* query)
  {
    query->value = render.submits_scheduled;
    query->status = 2;
  }

  void Render::startQuery(Query* query)
  {
    checkRenderAcquired();

    G_ASSERT(query);
    if (query)
    {
      Render::Query *local_query = query;
      cur_query_offset = local_query->offset;
      [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        local_query->value = *((uint64_t *)((uint8_t *)[query_buffer contents] + local_query->offset));
        local_query->status = 0;
      }];
    }
    else
    {
      cur_query_offset = -1;
    }

    dirty_state |= DirtyFlags::Query;
    query->status = 1;
  }

  void Render::finishQuery(Query* query)
  {
    checkRenderAcquired();

    cur_query_offset = -1;
    query->status = 2;
  }

  void Render::setRenderPass(bool set)
  {
    checkRenderAcquired();

    bool had_render_pass = render_pass;
    render_pass = set;
    // this is mostly for tests that do only clear with render passes
    if (!had_render_pass && set)
      updateEncoder();
  }

  void Render::checkNoRenderPass()
  {
    G_ASSERT(render_pass == false || has_image_blocks == false);
  }

  bool Render::setSrgbBackbuffer(bool set)
  {
    checkRenderAcquired();

    is_rgb_backbuffer = set;
    return set;
  }

  uint64_t Render::getQueryResult(Query* query, bool force_flush)
  {
    if (query->status != 2)
      return 1;
    if (render.submits_completed < query->value && force_flush)
    {
      @autoreleasepool
      {
        render.flush(true);
      }
    }
    return render.submits_completed < query->value && !force_flush ? -1 : 1;
  }

  void Render::releaseQuery(Query* query)
  {
    query->used = 0;
  }

  void Render::release()
  {
    inited = false;

    for (int i = 0; i < int(MetalImageType::Count); ++i)
    {
      blank_tex[i]->destroy();
      blank_tex_uint[i]->destroy();
      blank_tex_rw[i]->destroy();
    }
    backbuffer->apiTex->rt_texture = nil;
    backbuffer->apiTex->texture = nil;
    backbuffer->destroy();

    if (d3d::get_driver_desc().caps.hasBindless)
    {
      bindlessTextureIdBuffer->destroy();
      bindlessBufferIdBuffer->destroy();
      bindlessSamplerIdBuffer->destroy();
    }

    clear_mesh_buffer->destroy();

    acquireOwnerShip();
    flush(false);
    G_ASSERT(commandBuffer);
    submitCommandBuffer(commandBuffer, true);
    commandBuffer = nil;
    releaseOwnerShip();

    shadersPreCache.release();

    [tmp_copy_buff release];
    [copy_cs_pipeline release];
    [clear_cs_pipeline release];
    deleteVertexShader(clear_vshd);
    deleteVertexShader(copy_vshd);
    deletePixelShader(clear_pshdi);
    deletePixelShader(clear_pshdui);
    deletePixelShader(clear_pshdf);
    deletePixelShader(copy_pshd);
    deleteProgram(clear_progi);
    deleteProgram(clear_progui);
    deleteProgram(clear_progf);
    deleteProgram(copy_prog);
    deleteVDecl(clear_vdecl);
    ::destroy_critical_section(acquireSec);
    ::destroy_critical_section(rcSec);
    [query_buffer release];
    [stub_buffer release];

#if DAGOR_DBGLEVEL > 0
    [signpost_event release];
    [signpost_listener release];
    dispatch_release(signpost_queue);
#endif

    for (int local_frame = 0; local_frame < MAX_FRAMES_TO_RENDER; ++local_frame)
      cleanupFrame();

    std::lock_guard<std::mutex> scopedLock(delete_lock);
    for (int local_frame = 0; local_frame < MAX_FRAMES_TO_RENDER; ++local_frame)
    {
      auto & res = resources2delete_frames[local_frame];
      G_ASSERT(res.textures.empty());
      G_ASSERT(res.buffers.empty());
      G_ASSERT(res.vdecls.empty());
      G_ASSERT(res.shaders.empty());
      G_ASSERT(res.programs.empty());
      G_ASSERT(res.native_resources.empty());
      G_ASSERT(res.acceleration_structs.empty());
    }
    G_ASSERT(textures2upload.empty());
    G_ASSERT(buffers2copy.empty());
    G_ASSERT(regions2copy.empty());
    G_ASSERT(textures2clear.empty());
    G_ASSERT(buffers2clear.empty());

    [commandQueue release];

    for (int i = 0; i < STAGE_MAX; i++)
    {
      StageBinding& stg = stages[i];
      stg.cbuffer.destroy();
    }

    Buffer::cleanup();
    Texture::cleanup();

    if (defaultBlas)
      [defaultBlas release];
    defaultBlas = nil;
    if (bottomLevelAccelerationStructures)
      [bottomLevelAccelerationStructures release];
    bottomLevelAccelerationStructures = nil;

    debug("[METAL_INIT] render released at frame %llu", frame);
    frame = 0;
    submits_scheduled = 1;
    submits_completed = 0;
    memset(frames_completed, 0, sizeof(frames_completed));
  }

  int Render::getSupportedMTLVersion(void* mtl_version)
  {
    static int v[4] = {0, 0, 0, 0};
    if (mtl_version)
        *(const void**)mtl_version = v;
    auto mkDrvDesc = [] (char a, char b, char c, char d) { v[0] = a; v[1] = b; v[2] = c; v[3] = d; };
#if _TARGET_TVOS
    if ([device supportsFeatureSet: (MTLFeatureSet)30000 /*MTLFeatureSet_tvOS_GPUFamily1_v1*/])
      mkDrvDesc('T', 'V', '1', '1');
    if ([device supportsFeatureSet: (MTLFeatureSet)30001 /*MTLFeatureSet_tvOS_GPUFamily1_v2*/])
      mkDrvDesc('T', 'V', '1', '2');
    if ([device supportsFeatureSet: (MTLFeatureSet)30002 /*MTLFeatureSet_tvOS_GPUFamily1_v3*/])
      mkDrvDesc('T', 'V', '1', '3');
    if ([device supportsFeatureSet: (MTLFeatureSet)30003 /*MTLFeatureSet_tvOS_GPUFamily2_v1*/])
      mkDrvDesc('T', 'V', '2', '1');
#elif _TARGET_PC_MACOSX
    if ([device supportsFeatureSet: (MTLFeatureSet)10000 /*MTLFeatureSet_macOS_GPUFamily1_v1*/])
      mkDrvDesc('M', 'C', '1', '1');
    if ([device supportsFeatureSet: (MTLFeatureSet)10001 /*MTLFeatureSet_macOS_GPUFamily1_v2*/])
      mkDrvDesc('M', 'C', '1', '2');
    if ([device supportsFeatureSet: (MTLFeatureSet)10003 /*MTLFeatureSet_macOS_GPUFamily1_v3*/])
      mkDrvDesc('M', 'C', '1', '3');
    if ([device supportsFeatureSet: (MTLFeatureSet)10004 /*MTLFeatureSet_macOS_GPUFamily1_v4*/])
      mkDrvDesc('M', 'C', '1', '4');
    if ([device supportsFeatureSet: (MTLFeatureSet)10005 /*MTLFeatureSet_macOS_GPUFamily2_v1*/])
      mkDrvDesc('M', 'C', '2', '1');
#endif

    return 0;
  }

  void Render::readbackTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int offset, int pitch, int imageSize)
  {
    checkRenderAcquired();

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, false, "ReadbackTexture");

    MTLSize size = {(uint32_t)w, (uint32_t)h, (uint32_t)d};
    MTLOrigin origin = {0, 0, 0};

    MTLBlitOption options = MTLBlitOptionNone;
    if (tex.pixelFormat == MTLPixelFormatDepth32Float_Stencil8 || tex.pixelFormat == MTLPixelFormatDepth32Float ||
        tex.pixelFormat == MTLPixelFormatDepth16Unorm)
      options = MTLBlitOptionDepthFromDepthStencil;

    [blitEncoder copyFromTexture : tex
                     sourceSlice : layer
                     sourceLevel : level
                    sourceOrigin : origin
                      sourceSize : size
                        toBuffer : buf
               destinationOffset : offset
          destinationBytesPerRow : pitch
        destinationBytesPerImage : imageSize
                         options : options];
  }

  void Render::uploadTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d,
    id<MTLBuffer> buf, int pitch, int imageSize)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    textures2upload.push_back( {tex, buf, (uint32_t)pitch, (uint32_t)imageSize, (uint16_t)w, (uint16_t)h, (uint16_t)d,
        (uint8_t)level, (uint8_t)layer} );
  }

  void Render::queueBufferForDeletion(id<MTLBuffer> buf)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.native_resources.push_back(buf);
  }

  void Render::queueTextureForDeletion(id<MTLTexture> tex)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.native_resources.push_back(tex);
  }

  void Render::deleteBuffer(Buffer* buff)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.buffers.push_back(buff);
  }

  void Render::deleteTexture(Texture* tex)
  {
    if (tex->isStub())
      tex->apiTex = nullptr; // hack?

    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.textures.push_back(tex);
    if (tex)
      flushTexture(tex);
  }

  void Render::flushTexture(Texture* tex)
  {
    id<MTLTexture> mtlTex = tex->apiTex ? tex->apiTex->texture : nil;
    for (uint32_t i = 0; i < STAGE_MAX; ++i)
    {
      for (uint32_t j = 0; j < StageBinding::MAX_STAGE_TEXTURES; ++j)
      {
        if (stages[i].textures[j].texture == tex)
          stages[i].textures[j] = {};
        if (stages[i].textures_metal[j] == mtlTex)
          stages[i].textures_metal[j] = nil;
      }
    }
    for (uint32_t i = 0; i < Program::MAX_SIMRT; ++i)
    {
      if (rt.colors[i].texture == tex)
        rt.colors[i] = {};
    }
    if (rt.depth.texture == tex)
      rt.depth = {};
  }

  void Render::queueCopyBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    buffers2copy.push_back({ src, dst, src_offset, dst_offset, size });
  }

  void Render::queueUpdateBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size)
  {
    checkRenderAcquired();

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit);
    [blitEncoder copyFromBuffer : src
                   sourceOffset : src_offset
                       toBuffer : dst
              destinationOffset : dst_offset
                           size : size];
  }

  shaders::DriverRenderStateId Render::createRenderState(const shaders::RenderState & state)
  {
    CachedRenderState rstate;
    rstate.scissor_on = state.scissorEnabled;

    G_ASSERT(state.zFunc);
    G_ASSERT(state.cull);

    rstate.depth_state.stencil_enable = state.stencil.func > 0;
    if (state.stencil.func > 0)
    {
      rstate.depth_state.stencil_func = convertCompareFunction(state.stencil.func);
      rstate.depth_state.stencil_read_mask = state.stencil.readMask;
      rstate.depth_state.stencil_write_mask = state.stencil.writeMask;
      rstate.depth_state.sfail = state.stencil.fail;
      rstate.depth_state.szfail = state.stencil.zFail;
      rstate.depth_state.spass = state.stencil.pass;
    }
    rstate.depth_state.zenable = state.ztest;
    rstate.depth_state.depth_write_on = state.zwrite;
    rstate.depth_state.zfunc = convertCompareFunction(state.zFunc);

    rstate.depth_bias = state.zBias;
    rstate.depth_slopebias = state.slopeZBias;

    rstate.depth_clip = state.zClip ? MTLDepthClipModeClip : MTLDepthClipModeClamp;
    rstate.stencil_ref = state.stencilRef;
    rstate.cull = convertCullMode(state.cull);
    rstate.forcedSampleCount = state.forcedSampleCount;

    rstate.raster_state.a2c = state.alphaToCoverage;
    rstate.raster_state.writeMask = state.colorWr;
    for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
    {
      rstate.raster_state.blend[i].ablend = state.blendParams[i].ablend;
      rstate.raster_state.blend[i].ablendScr = convBlendArg(state.blendParams[i].sepablendFactors.src);
      rstate.raster_state.blend[i].ablendDst = convBlendArg(state.blendParams[i].sepablendFactors.dst);
      rstate.raster_state.blend[i].ablendOp = convertBlendOp(state.blendParams[i].sepablendOp);
      rstate.raster_state.blend[i].sepblend = state.blendParams[i].sepablend;
      rstate.raster_state.blend[i].rgbblendScr = convBlendArg(state.blendParams[i].ablendFactors.src);
      rstate.raster_state.blend[i].rgbblendDst = convBlendArg(state.blendParams[i].ablendFactors.dst);
      rstate.raster_state.blend[i].rgbblendOp = convertBlendOp(state.blendParams[i].blendOp);
    }


    return render_states.emplaceOne(rstate);
  }

  bool Render::setRenderState(shaders::DriverRenderStateId state_id)
  {
    checkRenderAcquired();

    const CachedRenderState & state = *render_states.get(state_id);
    if (scissor_on != state.scissor_on)
    {
      scissor_on = state.scissor_on;
      dirty_state |= DirtyFlags::Scissor;
    }
    if (cur_depthstate.depth_state != state.depth_state.depth_state)
    {
      cur_depthstate = state.depth_state;
      render.dirty_state |= DirtyFlags::DepthState;
    }
    if (fabs(depth_bias - state.depth_bias) > 0.00000001f || fabs(depth_slopebias - state.depth_slopebias) > 0.00000001f)
    {
      depth_bias = state.depth_bias;
      depth_slopebias = state.depth_slopebias;
      dirty_state |= DirtyFlags::DepthBias;
    }
    if (depth_clip != state.depth_clip)
    {
      depth_clip = state.depth_clip;
      [renderEncoder setDepthClipMode : depth_clip];
    }
    if (stencil_ref != state.stencil_ref)
    {
      stencil_ref = state.stencil_ref;
      dirty_state |= DirtyFlags::StencilRef;
    }
    if (cur_cull != state.cull)
      setCull(state.cull);
    cur_rstate.raster_state = state.raster_state;
    forcedSampleCount = state.forcedSampleCount;

    return true;
  }

  bool Render::setStencilRef(uint32_t ref)
  {
    checkRenderAcquired();

    if (stencil_ref != ref)
    {
      stencil_ref = ref;
      dirty_state |= DirtyFlags::StencilRef;
    }

    return true;
  }

  void Render::clearRenderStates()
  {
    render_states.clear();
  }

  void Render::beginEvent(const char *name)
  {
    G_ASSERT(commandBuffer);
    if (renderEncoder)
      [renderEncoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    else if (computeEncoder)
      [computeEncoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    else if (blitEncoder)
      [blitEncoder pushDebugGroup:[NSString stringWithUTF8String:name]];
    else
      [commandBuffer pushDebugGroup:[NSString stringWithUTF8String:name]];
  }

  void Render::endEvent()
  {
    G_ASSERT(commandBuffer);
    if (renderEncoder)
      [renderEncoder popDebugGroup];
    else if (computeEncoder)
      [computeEncoder popDebugGroup];
    else if (blitEncoder)
      [blitEncoder popDebugGroup];
    else
      [commandBuffer popDebugGroup];
  }

#if DAGOR_DBGLEVEL > 0
  void Render::put_encoder_label(const char *label)
  {
    if (!use_signposts)
      return;
    debug("[signpost] %llu - %s", signpost + 1, label);
  }

  id<MTLCommandBuffer> Render::wait_for_encoder()
  {
    if (!use_signposts)
      return commandBuffer;

    checkRenderAcquired();

    G_ASSERT(commandBuffer != nil);
    [commandBuffer encodeSignalEvent:signpost_event value:++signpost];

    {
      std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
      flushQueuedCommands();
    }
    submitCommandBuffer(commandBuffer);

    debug("[signpost] %llu wait", signpost);
    [signpost_event notifyListener : signpost_listener
                           atValue : signpost
                             block : ^(id<MTLSharedEvent> sharedEvent, uint64_t value)
                             {
                               signpost_condition.notify_all();
                             }];
    std::unique_lock<std::mutex> lock(signpost_mutex);
    signpost_condition.wait(lock, [this]
    {
        return signpost_event.signaledValue >= signpost;
    });

    commandBuffer = createCommandBuffer(commandQueue, "Command buffer");
    return commandBuffer;
  }
#endif

  uint64_t Render::getGPUTimeStampFreq()
  {
    MTLTimestamp cpuTimestamp, gpuTimestamp;
    [device sampleTimestamps: &cpuTimestamp gpuTimestamp: &gpuTimestamp];
    return gpuTimestamp;
  }

  id<MTLBuffer> Render::createBuffer(uint32_t size, uint32_t flags, const char *name)
  {
    id<MTLBuffer> buf = [device newBufferWithLength : size
                                            options : flags];
    if (buf == nil)
      DAG_FATAL("Failed to allocate buffer %s with size %d", name, size);
#if DAGOR_DBGLEVEL > 0
    buf.label = [NSString stringWithUTF8String : name];
#endif
    return buf;
  }

  RaytraceAccelerationStructure *Render::createAccelerationStructure(RaytraceBuildFlags flags, bool blas)
  {
    RaytraceAccelerationStructure *as = new RaytraceAccelerationStructure { .createFlags = flags };
    if (blas)
    {
      blases.lock();
      as->index = blases.getFreeIndex(as);
      blases.unlock();
      G_ASSERT(as->index >= 0);
    }
    return as;
  }

  void Render::deleteAccelerationStructure(RaytraceAccelerationStructure *as)
  {
    if (as)
    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.acceleration_structs.push_back(as);
    }
  }

  void Render::setTLAS(RaytraceTopAccelerationStructure *tlas, ShaderStage rstage, int slot)
  {
    checkRenderAcquired();

    G_ASSERT(rstage == STAGE_CS); // For now metal supports only CS
    G_ASSERT(slot >= 0);
    G_ASSERT(slot < MAX_SHADER_ACCELERATION_STRUCTURES);

    Render::StageBinding &stage = render.stages[rstage];
    stage.setAccStruct(slot, tlas ? tlas->acceleration_struct : nil);
  }

  void Render::buildAccelerationStructure(RaytraceAccelerationStructure *as, MTLAccelerationStructureDescriptor *desc, Sbuffer *space_buffer, uint32_t space_buffer_offset, bool refit)
  {
    G_ASSERT(space_buffer);

    MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor: desc];

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Acceleration);

    if (refit)
    {
      G_ASSERT(as->acceleration_struct);
      G_ASSERT(desc.usage & MTLAccelerationStructureUsageRefit);
      id<MTLAccelerationStructure> accStruct = (id<MTLAccelerationStructure>)as->acceleration_struct;

      G_ASSERT(accStruct.size >= sizes.accelerationStructureSize);

      id<MTLBuffer> spaceBuf = ((Buffer*)space_buffer)->getBuffer();
      G_ASSERT(sizes.refitScratchBufferSize <= spaceBuf.length);

      [accelerationEncoder refitAccelerationStructure: accStruct
                                           descriptor: desc
                                          destination: accStruct
                                        scratchBuffer: spaceBuf
                                  scratchBufferOffset: space_buffer_offset];
    }
    else
    {
      if (as->acceleration_struct)
      {
        std::lock_guard<std::mutex> scopedLock(delete_lock);
        resources2delete.native_resources.push_back(as->acceleration_struct);
      }

      as->acceleration_struct = [device newAccelerationStructureWithSize: sizes.accelerationStructureSize];

      id<MTLBuffer> spaceBuf = ((Buffer*)space_buffer)->getBuffer();
      G_ASSERT(sizes.buildScratchBufferSize <= spaceBuf.length);

      [accelerationEncoder buildAccelerationStructure: (id<MTLAccelerationStructure>)as->acceleration_struct
                                           descriptor: desc
                                        scratchBuffer: spaceBuf
                                  scratchBufferOffset: space_buffer_offset];
    }
    as->was_built = true;

    // Here struct compression can be done to reduce the size
    // but it is really performance heavy (requires a flush ([commandBuffer waitUntilCompleted]))
    // and doing it every frame is too much
  }

  void Render::buildTLAS(RaytraceTopAccelerationStructure *as, const ::raytrace::TopAccelerationStructureBuildInfo &tasbi)
  {
    checkRenderAcquired();

    G_ASSERT(as);
    G_ASSERT(as->index == -1);

    if (@available(macOS 11.0, iOS 15.0, *))
    {
      MTLInstanceAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getTLASDescriptor(tasbi.instanceCount, tasbi.flags);

      G_ASSERT(tasbi.instanceBuffer);

      id<MTLBuffer> instanceBuf = ((drv3d_metal::Buffer *)tasbi.instanceBuffer)->getBuffer();
      G_ASSERT(instanceBuf.length >= d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize * tasbi.instanceCount);

      accDesc.instanceDescriptorBuffer = instanceBuf;
      accDesc.instancedAccelerationStructures = bottomLevelAccelerationStructures;
      buildAccelerationStructure(as, accDesc, tasbi.scratchSpaceBuffer, tasbi.scratchSpaceBufferOffsetInBytes, tasbi.doUpdate);
    }
  }

  void Render::buildBLAS(RaytraceBottomAccelerationStructure *blas, const ::raytrace::BottomAccelerationStructureBuildInfo &basbi)
  {
    checkRenderAcquired();

    G_ASSERT(blas);
    G_ASSERT(blas->index >= 0);
    G_ASSERT(blas->acceleration_struct == nil || basbi.doUpdate);
    G_ASSERT(blas->acceleration_struct || !basbi.doUpdate);

    if (@available(macOS 11.0, iOS 15.0, *))
    {
      MTLPrimitiveAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getBLASDescriptor(basbi.geometryDesc, basbi.geometryDescCount, basbi.flags);
      buildAccelerationStructure(blas, accDesc, basbi.scratchSpaceBuffer, basbi.scratchSpaceBufferOffsetInBytes, basbi.doUpdate);
    }

    if ([bottomLevelAccelerationStructures count] <= blas->index)
      for (uint32_t i = [bottomLevelAccelerationStructures count]; i <= blas->index; ++i)
        [bottomLevelAccelerationStructures addObject:defaultBlas];
    [bottomLevelAccelerationStructures replaceObjectAtIndex:blas->index withObject:blas->acceleration_struct];
  }

  API_AVAILABLE(ios(16.0), macos(13.0)) static uint64_t ToResourceID(MTLResourceID res)
  {
    uint64_t ret = 0;
    memcpy(&ret, &res, sizeof(res));
    return ret;
  }

  void Render::prepareBindlessResourcesGraphics(id<MTLRenderCommandEncoder> renderCommandEncoder)
  {
    if (!d3d::get_driver_desc().caps.hasBindless)
      return;

    prepareBindlessResources(resourcesToUse);
    if (resourcesToUse.size())
      [renderCommandEncoder useResources : resourcesToUse.data()
                                   count : resourcesToUse.size()
                                   usage : MTLResourceUsageRead
                                  stages : MTLRenderStageVertex|MTLRenderStageFragment];
  }

  void Render::prepareBindlessResourcesCompute(id<MTLComputeCommandEncoder> renderComputeEncoder)
  {
    if (!d3d::get_driver_desc().caps.hasBindless)
      return;

    prepareBindlessResources(resourcesToUse);
    if (resourcesToUse.size())
      [renderComputeEncoder useResources : resourcesToUse.data()
                                   count : resourcesToUse.size()
                                   usage : MTLResourceUsageRead];
  }

  void Render::prepareBindlessResources(eastl::vector<id<MTLResource>>& resourcesToUse)
  {
    resourcesToUse.clear();

    const auto bindessTextureResourceIdSize = BINDLESS_TEXTURE_COUNT; //*/ max(maxBindlessTextureId, 1u);
    const auto bindessBufferResourceIdSize = BINDLESS_BUFFER_COUNT; //*/ max(maxBindlessBufferId, 1u);
    const auto bindessSamplerResourceIdSize = BINDLESS_SAMPLER_COUNT; //*/ max(maxBindlessSamplerId, 1u);

    auto mappedBindlessTextureIds = (uint64_t*)bindlessTextureIdBuffer->lock(0, bindessTextureResourceIdSize*sizeof(uint64_t), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    for(uint32_t i = 0; i < BINDLESS_TEXTURE_COUNT; i++)
    {
      auto tex = i < maxBindlessTextureId && bindlessTextures[i] ? bindlessTextures[i] : blank_tex[0];

      id<MTLTexture> mtlTexture = nil;
      tex->apply(mtlTexture, false, 0, false, false);
      G_ASSERT(mtlTexture != nil);
      if (i < maxBindlessTextureId)
        resourcesToUse.push_back(mtlTexture);

      if (@available(macOS 13.0, iOS 16.0, *))
      {
        mappedBindlessTextureIds[i] = ToResourceID(mtlTexture.gpuResourceID);
      }
    }
    bindlessTextureIdBuffer->unlock();

    auto mappedBindlessBufferIds = (uint64_t*)bindlessBufferIdBuffer->lock(0, bindessBufferResourceIdSize*sizeof(uint64_t), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    for(uint32_t i = 0; i < BINDLESS_BUFFER_COUNT; i++)
    {
      id<MTLBuffer> mtlBuffer = i < maxBindlessBufferId && bindlessBuffers[i] ? bindlessBuffers[i]->getBuffer() : stub_buffer;
      G_ASSERT(mtlBuffer != nil);
      if (i < maxBindlessTextureId)
        resourcesToUse.push_back(mtlBuffer);

      if (@available(macOS 13.0, iOS 16.0, *))
      {
        mappedBindlessBufferIds[i] = mtlBuffer.gpuAddress;
      }
    }
    bindlessBufferIdBuffer->unlock();

    Texture::SamplerState sampler_state_dummy;
    drv3d_metal::Texture::getSampler(sampler_state_dummy);

    auto mappedBindlessSamplerIds = (uint64_t*)bindlessSamplerIdBuffer->lock(0, bindessSamplerResourceIdSize*sizeof(uint64_t), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    for(uint32_t i = 0; i < BINDLESS_SAMPLER_COUNT; i++)
    {
      auto smp = i < maxBindlessSamplerId ? bindlessSamplers[i] : sampler_state_dummy.sampler;
      G_ASSERT(smp);
      if (@available(macOS 13.0, iOS 16.0, *))
      {
        mappedBindlessSamplerIds[i] = ToResourceID(smp.gpuResourceID);
      }
    }
    bindlessSamplerIdBuffer->unlock();
  }

  void Render::setBindlessResources()
  {
    if (!d3d::get_driver_desc().caps.hasBindless)
      return;

    for (int i = 0; i < BINDLESS_TEXTURE_ID_BUFFER_COUNT; i++)
    {
      uint32_t slot = RemapBufferSlot(BINDLESS_TEXTURE_ID_BUFFER, i);
      render.stages[STAGE_PS].setBuf(slot, bindlessTextureIdBuffer, 0);
      render.stages[STAGE_VS].setBuf(slot, bindlessTextureIdBuffer, 0);
      render.stages[STAGE_CS].setBuf(slot, bindlessTextureIdBuffer, 0);
    }

    for (int i = 0; i < BINDLESS_BUFFER_ID_BUFFER_COUNT; i++)
    {
      uint32_t slot = RemapBufferSlot(BINDLESS_BUFFER_ID_BUFFER, i);
      render.stages[STAGE_PS].setBuf(slot, bindlessBufferIdBuffer, 0);
      render.stages[STAGE_VS].setBuf(slot, bindlessBufferIdBuffer, 0);
      render.stages[STAGE_CS].setBuf(slot, bindlessBufferIdBuffer, 0);
    }

    for (int i = 0; i < BINDLESS_SAMPLER_ID_BUFFER_COUNT; i++)
    {
      uint32_t slot = RemapBufferSlot(BINDLESS_SAMPLER_ID_BUFFER, i);
      render.stages[STAGE_PS].setBuf(slot, bindlessSamplerIdBuffer, 0);
      render.stages[STAGE_VS].setBuf(slot, bindlessSamplerIdBuffer, 0);
      render.stages[STAGE_CS].setBuf(slot, bindlessSamplerIdBuffer, 0);
    }
  }

  void Render::pushAsyncPsoCompilation(bool enable)
  {
    checkRenderAcquired();

    async_pso_compilation = enable;
    async_pso_compilation_stack.push_back(enable);
  }

  void Render::popAsyncPsoCompilation()
  {
    checkRenderAcquired();

    async_pso_compilation_stack.pop_back();
    async_pso_compilation = async_pso_compilation_stack.size() ? async_pso_compilation_stack.back() : false;
  }
}
