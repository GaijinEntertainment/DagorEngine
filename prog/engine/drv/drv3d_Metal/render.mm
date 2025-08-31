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
#include <EASTL/sort.h>

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

    uint64_t ToResourceID(id<MTLTexture> tex)
    {
      G_ASSERT(tex);
      if (@available(iOS 16, macOS 13.0, *))
      {
        MTLResourceID res = tex.gpuResourceID;
        uint64_t ret = 0;
        memcpy(&ret, &res, sizeof(res));
        return ret;
      }
      return 0;
    }

    uint64_t ToResourceID(id<MTLSamplerState> smp)
    {
      G_ASSERT(smp);
      if (@available(iOS 16, macOS 13.0, *))
      {
        MTLResourceID res = smp.gpuResourceID;
        uint64_t ret = 0;
        memcpy(&ret, &res, sizeof(res));
        return ret;
      }
      return 0;
    }
  }

  static bool are_compatible_for_blit(MTLPixelFormat src, MTLPixelFormat dst)
  {
    if (src == dst)
      return true;
    if (src == MTLPixelFormatDepth16Unorm && (dst == MTLPixelFormatR16Float))
      return true;
    if (src == MTLPixelFormatDepth32Float && (dst == MTLPixelFormatR32Float))
      return true;
    return false;
  }

  Render render;

  MTLDepthStencilDescriptor* Render::depthStateDesc = NULL;

  VDECL   copy_vdecl = BAD_VDECL;
  VSDTYPE clear_dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END };

  Sbuffer* clear_mesh_buffer = nullptr;

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
    "uint channels;\n"
    "};\n"
    "kernel void cs_main(uint global_index[[thread_position_in_grid]],\n"
    "    constant Const &consts[[buffer(0)]], device uint* buffer[[buffer(1)]])\n"
    "{\n"
    "   if (global_index < consts.len) buffer[global_index] = consts.val[global_index % consts.channels];\n"
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
    case EXT_BLEND_SRC1COLOR: return MTLBlendFactorSource1Color;
    case EXT_BLEND_INVSRC1COLOR: return MTLBlendFactorOneMinusSource1Color;
    case EXT_BLEND_SRC1ALPHA: return MTLBlendFactorSource1Alpha;
    case EXT_BLEND_INVSRC1ALPHA: return MTLBlendFactorOneMinusSource1Alpha;
    default:
      G_ASSERT(0 && "Unknown blend mode");
    }

    return MTLBlendFactorOne;
  }
  static MTLBlendFactor convBlendArgForAlpha(int arg)
  {
    switch (arg)
    {
    case BLEND_ZERO:      return MTLBlendFactorZero;
    case BLEND_ONE:       return MTLBlendFactorOne;
    case BLEND_SRCCOLOR:    return MTLBlendFactorSourceAlpha; // MTLBlendFactorSourceColor
    case BLEND_INVSRCCOLOR:   return MTLBlendFactorOneMinusSourceAlpha; // MTLBlendFactorOneMinusSourceColor
    case BLEND_SRCALPHA:    return MTLBlendFactorSourceAlpha;
    case BLEND_INVSRCALPHA:   return MTLBlendFactorOneMinusSourceAlpha;
    case BLEND_DESTALPHA:     return MTLBlendFactorDestinationAlpha;
    case BLEND_INVDESTALPHA:  return MTLBlendFactorOneMinusDestinationAlpha;
    case BLEND_DESTCOLOR:     return MTLBlendFactorDestinationAlpha; // MTLBlendFactorDestinationColor
    case BLEND_INVDESTCOLOR:  return MTLBlendFactorOneMinusDestinationAlpha; // MTLBlendFactorOneMinusDestinationColor
    case BLEND_SRCALPHASAT:   return MTLBlendFactorSourceAlphaSaturated;
    case BLEND_BOTHINVSRCALPHA: return  MTLBlendFactorSourceAlphaSaturated;
    case BLEND_BLENDFACTOR:   return MTLBlendFactorBlendAlpha; // MTLBlendFactorBlendColor
    case BLEND_INVBLENDFACTOR:  return MTLBlendFactorOneMinusBlendAlpha; // MTLBlendFactorOneMinusBlendColor
    case EXT_BLEND_SRC1COLOR: return MTLBlendFactorSource1Color;
    case EXT_BLEND_INVSRC1COLOR: return MTLBlendFactorOneMinusSource1Color;
    case EXT_BLEND_SRC1ALPHA: return MTLBlendFactorSource1Alpha;
    case EXT_BLEND_INVSRC1ALPHA: return MTLBlendFactorOneMinusSource1Alpha;
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
    MTLTriangleFillMode old_fill;
    bool save_tex, save_rt;

    bool scissor_on = false;
    int sci_x = 0;
    int sci_y = 0;
    int sci_w = 0;
    int sci_h = 0;

    Render::Viewport viewport;

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
      old_fill = render.cur_fill;

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

      render.cur_depthstate.zenable = depth_write ? 1 : 0;
      render.cur_depthstate.depth_write_on = depth_write ? 1 : 0;
      render.cur_depthstate.zfunc = MTLCompareFunctionAlways;
      render.dirty_state |= DirtyFlags::DepthState;

      render.setCull(MTLCullModeNone);
      render.setFill(MTLTriangleFillModeFill);

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

      viewport = render.rt.vp;
    }

    ~StateSaver()
    {
      render.cur_depthstate.zenable = old_depthstate.zenable;
      render.cur_depthstate.depth_write_on = old_depthstate.depth_write_on;
      render.cur_depthstate.zfunc = old_depthstate.zfunc;
      render.dirty_state |= DirtyFlags::DepthState;

      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
        render.cur_rstate.raster_state.blend[i].ablend = old_rstate.raster_state.blend[i].ablend;
      render.cur_rstate.raster_state.writeMask = old_rstate.raster_state.writeMask;
      render.cur_rstate.raster_state.a2c = 0;
      render.cur_rstate.raster_state.pad[0] = render.cur_rstate.raster_state.pad[1] = 0;
      render.cur_rstate.vbuffer_stride[0] = old_rstate.vbuffer_stride[0];

      render.stages[STAGE_VS].setBuf(render.storages[STAGE_VS], 0, old_buffers_0, old_buffers_offset_0);
      render.setCull(old_cull);
      render.setFill(old_fill);

      if (save_rt)
      {
        render.rt = old_rt;
        render.need_change_rt = 1;
        render.checkNoRenderPass();

        render.setViewport(viewport.x, viewport.y, viewport.w, viewport.h, viewport.minz, viewport.maxz);
      }

      if (save_tex)
        render.stages[STAGE_PS].setTex(render.storages[STAGE_PS], 0, old_texture.texture, old_texture.read_stencil, old_texture.mip_level, old_texture.slice, old_texture.as_uint);

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

#if DAGOR_DBGLEVEL > 0
  #define ADD_SIGNPOSTS 1

  static const char * MTLCommandEncoderErrorStateToString(MTLCommandEncoderErrorState state)
  {
     switch (state)
     {
       case MTLCommandEncoderErrorStateUnknown:
         return "unknown";
       case MTLCommandEncoderErrorStateCompleted:
         return "completed";
       case MTLCommandEncoderErrorStateAffected:
         return "affected";
       case MTLCommandEncoderErrorStatePending:
         return "pending";
       case MTLCommandEncoderErrorStateFaulted:
         return "faulted";
     }
     return "unknown";
   }
#endif

  static id<MTLCommandBuffer> createCommandBuffer(id<MTLCommandQueue> queue, const char* name)
  {
    id<MTLCommandBuffer> cmdBuf = nil;
#if DAGOR_DBGLEVEL > 0
    MTLCommandBufferDescriptor* desc = [[MTLCommandBufferDescriptor alloc] init];
    desc.retainedReferences = NO;
    desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    cmdBuf = [queue commandBufferWithDescriptor:desc];
    [desc release];

    [cmdBuf addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
    {
      NSError *error = buffer.error;
      if (error)
      {
        logerr("[METAL] Achtung! command buffer %s failed to execute\nerror summary: %s", [buffer.label UTF8String], [error.localizedDescription UTF8String]);
        NSArray<id<MTLCommandBufferEncoderInfo>>* infos = error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
        for (id<MTLCommandBufferEncoderInfo> info in infos)
        {
          debug("[METAL] encoder %s, status %s", [info.label UTF8String], MTLCommandEncoderErrorStateToString(info.errorState));
          auto signposts = [info.debugSignposts componentsJoinedByString:@", "];
          if (signposts.length > 0u && info.errorState == MTLCommandEncoderErrorStateFaulted)
            debug("[METAL] faulted encoder signposts %s", [signposts UTF8String]);
        }
      }
    }];
    {
      std::unique_lock<std::mutex> lock(render.frame_mutex);
      uint64_t submit_number = render.submits_scheduled;
      cmdBuf.label = [NSString stringWithFormat:@"%s %llu (%llu)", name, render.frame, submit_number];
    }
#else
    cmdBuf = [queue commandBufferWithUnretainedReferences];
#endif
    return [cmdBuf retain];
  }

  static void setupCommadBufferErrorHandler(id<MTLCommandBuffer> commandBuffer)
  {
    if (render.max_number_of_frames_to_skip_after_error == 0 && !render.report_gpu_errors)
      return;
    [commandBuffer addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
    {
      if (buffer.status == MTLCommandBufferStatusError)
      {
        render.hadError.store(buffer.error ? buffer.error.code : 100500);
      }
    }];
  }

  static void sampleSampleBuffer(uint64_t start, uint64_t end)
  {
#if HAVE_SAMPLE_QUERIES
    static uint64_t lastGpuTick = 1;

    if (start == end)
      return;

    const NSData *data = [render.sampleBuffer resolveCounterRange : NSMakeRange(start, end - start)];
    if (data == nil || data.length / sizeof(MTLCounterResultTimestamp) != end - start)
    {
      for (int i = 0; i < end - start; i++)
        render.sampleSamples[start + i] = lastGpuTick;
      return;
    }

    MTLCounterResultTimestamp *stamps = (MTLCounterResultTimestamp *)data.bytes;
    for (int i = 0; i < end - start; i++)
    {
      if (stamps[i].timestamp == 0 || stamps[i].timestamp == MTLCounterErrorValue)
        render.sampleSamples[start + i] = lastGpuTick;
      else
      {
        render.sampleSamples[start + i] = stamps[i].timestamp;
        lastGpuTick = stamps[i].timestamp;
      }
    }
#endif
  }

  static void setupCommadBufferSampleHandler(id<MTLCommandBuffer> commandBuffer, uint64_t start, uint64_t end)
  {
#if HAVE_SAMPLE_QUERIES
    if (start == end || render.sampleBuffer == nil)
      return;
    [commandBuffer addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
    {
      uint64_t wrapped_start = start % Render::MAX_SAMPLE_SAMPLES;
      uint64_t wrapped_end = end % Render::MAX_SAMPLE_SAMPLES;
      if (wrapped_start > wrapped_end)
      {
        sampleSampleBuffer(0, wrapped_end);
        sampleSampleBuffer(wrapped_start, Render::MAX_SAMPLE_SAMPLES);
      }
      else
        sampleSampleBuffer(wrapped_start, wrapped_end);
      render.processedSample = end;
    }];
#endif
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
      G_ASSERT(render.nextSample >= render.nextSubmittedSample);
      setupCommadBufferSampleHandler(commandBuffer, render.nextSubmittedSample, render.nextSample);
      render.nextSubmittedSample = render.nextSample;
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

  void Render::ConstBuffer::destroy()
  {
  }

  void Render::ConstBuffer::init()
  {
    num_stored = 0;
  }

  static void flushResources(const Render::ResourceArray &resources)
  {
    for (const auto &res : resources)
    {
      switch (res.resource_type)
      {
        case Render::Resource::Type::AccelerationStructure:
          G_ASSERT(render.computeEncoder);
          if (res.resource)
            render.track_resource_read(*res.resource, res.is_from_vs);
          [render.computeEncoder setAccelerationStructure : res.metal_resource
                                            atBufferIndex : res.slot];
          break;
        case Render::Resource::Type::Sampler:
          if (res.stage == STAGE_VS)
            [render.renderEncoder setVertexSamplerState : res.metal_resource atIndex : res.slot];
          else if (res.stage == STAGE_PS)
            [render.renderEncoder setFragmentSamplerState : res.metal_resource atIndex : res.slot];
          else if (res.stage == STAGE_CS)
          {
            G_ASSERT(render.computeEncoder);
            [render.computeEncoder setSamplerState : res.metal_resource atIndex : res.slot];
          }
          break;
        case Render::Resource::Type::Texture:
          if (res.resource)
          {
            if (res.uav)
              render.track_resource_write(*res.resource, res.is_from_vs);
            else
              render.track_resource_read(*res.resource, res.is_from_vs);
          }
          if (res.stage == STAGE_VS)
            [render.renderEncoder setVertexTexture : res.metal_resource atIndex : res.slot];
          else if (res.stage == STAGE_PS)
            [render.renderEncoder setFragmentTexture : res.metal_resource atIndex : res.slot];
          else if (res.stage == STAGE_CS)
          {
            G_ASSERT(render.computeEncoder);
            [render.computeEncoder setTexture : res.metal_resource atIndex : res.slot];
          }
          break;
        case Render::Resource::Type::Buffer:
          if (res.resource)
          {
            if (res.uav)
              render.track_resource_write(*res.resource, res.is_from_vs);
            else
              render.track_resource_read(*res.resource, res.is_from_vs);
          }
          if (res.stage == STAGE_VS)
          {
            if (res.offset_only)
              [render.renderEncoder setVertexBufferOffset : res.offset atIndex : res.slot];
            else
              [render.renderEncoder setVertexBuffer : res.metal_resource offset : res.offset atIndex : res.slot];
          }
          else if (res.stage == STAGE_PS)
          {
            if (res.offset_only)
              [render.renderEncoder setFragmentBufferOffset : res.offset atIndex : res.slot];
            else
              [render.renderEncoder setFragmentBuffer : res.metal_resource offset : res.offset atIndex : res.slot];
          }
          else if (res.stage == STAGE_CS)
          {
            G_ASSERT(render.computeEncoder);
            if (res.offset_only)
            {
              G_ASSERT(res.slot >= 2);
              [render.computeEncoder setBufferOffset : res.offset atIndex : res.slot];
            }
            else
              [render.computeEncoder setBuffer : res.metal_resource offset : res.offset atIndex : res.slot];
          }
          break;
      }
    }
  }

  void Render::ConstBuffer::applyCmd(void* data, int reg_base, int num_regs)
  {
    G_ASSERT(reg_base + num_regs <= MAX_CBUFFER_SIZE);

    memcpy(&cbuffer[reg_base], data, num_regs);
    num_stored = max(num_stored, reg_base + num_regs);
  }

  void Render::ConstBuffer::applyBuffer(StageStorage &storage, int num_reg, ResourceArray &resources)
  {
    if (num_reg == 0)
    {
      return;
    }

    if (num_stored > num_reg)
    {
      num_reg = num_stored;
    }

    if (storage.cbuffer_num_bound >= num_reg && memcmp(cbuffer, storage.cbuffer, num_reg) == 0)
      return;

    int offset = -1;
    id<MTLBuffer> buf = render.AllocateConstants(num_reg, offset);
    uint8_t* ptr = (uint8_t*)buf.contents + offset;
    G_ASSERT(offset + num_reg <= RingBufferItem::max_constant_size);

    memcpy(ptr, cbuffer, num_reg);
    memcpy(storage.cbuffer, cbuffer, num_reg);
    storage.cbuffer_num_bound = num_reg;

    storage.bindBuffer(BIND_POINT, nullptr, buf, offset, false, resources);

    num_stored = 0;
  }

  void Render::StageBinding::bindVertexStream(StageStorage &storage, uint32_t stream, uint32_t offset, ResourceArray &resources)
  {
    G_ASSERT(stream < 2);

    id<MTLBuffer> buffer = NULL;

    G_ASSERT(storage.stage == STAGE_VS || storage.stage == STAGE_MS || storage.stage == STAGE_OS);
    if (buffers[stream])
    {
      uint32_t dynamic_offset = buffers[stream]->getDynamicOffset();
      storage.bindBuffer(stream, buffers[stream], buffers[stream]->getBuffer(), buffers_offset[stream] + dynamic_offset + offset, false, resources);
    }
  }

  static __forceinline void apply_immediate_constants(Render::StageStorage &storage, Shader* shader, Render::StageBinding& stage, Render::ResourceArray &resources)
  {
    G_ASSERTF(stage.immediate_dword_count > 0, "Shader name and variant: %s", getShaderName(shader->src == nil ? "" : [shader->src cStringUsingEncoding:[NSString defaultCStringEncoding]]).c_str());

    if (shader->immediate_slot == storage.immediate_slot && stage.immediate_dword_count <= storage.immediate_dword_count
        && memcmp(stage.immediate_dwords, storage.immediate_dwords, stage.immediate_dword_count*sizeof(uint32_t)) == 0)
      return;

    int buf_imm_offset = 0;
    id<MTLBuffer> buf_imm = render.AllocateConstants(stage.immediate_dword_count*sizeof(uint32_t), buf_imm_offset);
    memcpy((uint8_t *)buf_imm.contents + buf_imm_offset, stage.immediate_dwords, stage.immediate_dword_count*sizeof(uint32_t));
    memcpy(storage.immediate_dwords, stage.immediate_dwords, stage.immediate_dword_count*sizeof(uint32_t));

    storage.immediate_dword_count = stage.immediate_dword_count;
    storage.immediate_slot = shader->immediate_slot;

    storage.bindBuffer(shader->immediate_slot, nullptr, buf_imm, buf_imm_offset, false, resources);
  }

  void Render::StageBinding::apply_vertex_streams(StageStorage &storage, ResourceArray &resources)
  {
    for (int i = 0; i < GEOM_BUFFER_COUNT; ++i)
      bindVertexStream(storage, i, 0, resources);
    storage.buffer_dirty_mask &= ~GEOM_BUFFER_MASK;
  }

  void Render::StageBinding::apply_buffers(StageStorage &storage, Shader* shader, ResourceArray &resources)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_buffers; ++i)
    {
      int index = shader->buffers[i].remapped_slot;

      id<MTLBuffer> buffer = NULL;
      unsigned offset = buffers_offset[index];
      bool is_uav = index >= RW_BUFFER_POINT && index < RW_BUFFER_POINT + RW_BUFFER_COUNT;

      uint32_t dynamic_offset = 0;
      if (buffers[index])
      {
        buffer = buffers[index]->getBuffer();
        dynamic_offset = buffers[index]->getDynamicOffset();
      }

      if (buffer == nil && index >= CONST_BUFFER_POINT)
      {
        buffer = render.stub_buffer;
        offset = 0;
        dynamic_offset = 0;
      }

      int target = shader->buffers[i].slot;

      // reset cached immediate binding if something overrides the slot
      if (shader->immediate_slot == -1 && target == storage.immediate_slot)
        storage.immediate_slot = -1;

      storage.bindBuffer(target, buffers[index], buffer, offset + dynamic_offset, is_uav, resources);
    }
    storage.buffer_dirty_mask &= ~shader->buffer_mask;
  }

  static MTLTextureType ConvertTextureType(MetalImageType type)
  {
    switch (type)
    {
      case MetalImageType::Tex2D: return MTLTextureType2D;
      case MetalImageType::TexBuffer: return MTLTextureType2D;
      case MetalImageType::Tex2DArray: return MTLTextureType2DArray;
      case MetalImageType::TexCube: return MTLTextureTypeCube;
      case MetalImageType::TexCubeArray: return MTLTextureTypeCubeArray;
      case MetalImageType::Tex3D: return MTLTextureType3D;
      default:break;
    }
    return MTLTextureType1D;
  }

  void Render::StageBinding::apply_textures(StageStorage &storage, Shader* shader, ResourceArray &resources)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_tex; i++)
    {
      int slot = shader->tex_binding[i];
      uint32_t tslot = shader->tex_remap[i];
      MetalImageType type = shader->tex_type[i].type;

      id<MTLTexture> texture = nil;
      HazardTracker *resource = nullptr;

      bool is_uav = slot >= MAX_SHADER_TEXTURES;

      const auto &cache = textures[slot];
      if (type == MetalImageType::TexBuffer)
      {
        auto buf_tex = buffers[slot] ? buffers[slot]->getTexture() : nullptr;
        if (buf_tex)
        {
          buf_tex->apply(texture, false, 0, 0, false, false);
          resource = buffers[slot];
        }
      }
      else if (cache.texture)
      {
        cache.texture->apply(texture, cache.read_stencil, cache.mip_level, cache.slice, is_uav, cache.as_uint);
        resource = cache.texture;
      }

      if (texture == nil)
      {
        int index = int(type);
        G_ASSERT(index < int(MetalImageType::Count));
        if (is_uav)
          render.blank_tex_rw[index]->apply(texture, false, 0, 0, false, false);
        else if (shader->tex_type[i].is_uint)
          render.blank_tex_uint[index]->apply(texture, false, 0, 0, false, false);
        else
          render.blank_tex[index]->apply(texture, false, 0, 0, false, false);
      }

      if (texture && storage.textures[tslot] != texture)
      {
        G_ASSERTF(texture.textureType == ConvertTextureType(type), "Texture %s is bound as %d but expected to be %d", [texture.label UTF8String], (int)texture.textureType, (int)ConvertTextureType(type));
        storage.textures[tslot] = texture;

        resources.push_back({
                            .resource = resource,
                            .metal_resource = texture,
                            .offset = 0,
                            .stage = storage.stage,
                            .uav = is_uav,
                            .is_from_vs = storage.is_vertex(),
                            .slot = tslot,
                            .offset_only = 0,
                            .resource_type = Render::Resource::Type::Texture,
                          });
      }
    }
    storage.texture_dirty_mask &= ~shader->texture_mask;
  }

  void Render::StageBinding::apply_samplers(StageStorage& storage, Shader* shader, ResourceArray &resources)
  {
    G_ASSERT(shader);
    for (int i = 0; i < shader->num_samplers; i++)
    {
      int slot = shader->sampler_binding[i];
      uint32_t tslot = shader->sampler_remap[i];

      id<MTLSamplerState> sampler = nil;

      G_ASSERT(slot < 16);
      if (samplers[slot])
        sampler = samplers[slot];
      else
        sampler = render.sampler_states[render.getSampler({})].sampler;

      if (sampler && storage.samplers[tslot] != sampler)
      {
        storage.samplers[tslot] = sampler;

        resources.push_back({
                            .resource = nullptr,
                            .metal_resource = sampler,
                            .offset = 0,
                            .stage = storage.stage,
                            .uav = 0,
                            .slot = tslot,
                            .offset_only = 0,
                            .resource_type = Render::Resource::Type::Sampler,
                          });
      }
    }
    storage.sampler_dirty_mask &= ~shader->sampler_mask;
  }

  void Render::StageBinding::apply_acceleration_structs(StageStorage &storage, Shader* shader, ResourceArray &resources)
  {
    G_ASSERT(shader);

    for (int i = 0; i < shader->accelerationStructureCount; i++)
    {
      const int inDriverSlot = shader->acceleration_structure_remap[i];
      const uint32_t inShaderSlot = shader->acceleration_structure_binding[inDriverSlot];

      id<MTLAccelerationStructure> as = nil;
      if (acceleration_structures[inDriverSlot])
        as = acceleration_structures[inDriverSlot]->acceleration_struct;
      if (as == nil)
        as = render.defaultTlas;
      G_ASSERT(as);

      resources.push_back({
                          .resource = acceleration_structures[inDriverSlot],
                          .metal_resource = as,
                          .offset = 0,
                          .stage = storage.stage,
                          .uav = 0,
                          .slot = inShaderSlot,
                          .offset_only = 0,
                          .resource_type = Render::Resource::Type::AccelerationStructure,
                        });
    }
  }

  void Render::StageBinding::apply(StageStorage &storage, Shader* shader, ResourceArray &resources)
  {
    G_ASSERT(shader);

    // early out if nothing was bound since last time
    if (storage.is_vertex() && (storage.buffer_dirty_mask & GEOM_BUFFER_MASK))
      apply_vertex_streams(storage, resources);
    if (storage.buffer_dirty_mask & shader->buffer_mask)
      apply_buffers(storage, shader, resources);

    if (shader->num_bindless_buffers)
    {
      G_ASSERT(d3d::get_driver_desc().caps.hasBindless);
      render.prepareBindlessResources(shader->bindless_type_mask);
      for (int i = 0; i < shader->num_bindless_buffers; ++i)
      {
        const auto &remap = shader->bindless_buffers[i];
        int target = remap.slot;

        Buffer *bindless_buffer = nullptr;
        if (remap.remap_type == EncodedBufferRemap::RemapType::Sampler)
          bindless_buffer = render.bindlessSamplerIdBuffer;
        else if (remap.remap_type == EncodedBufferRemap::RemapType::Buffer)
          bindless_buffer = render.bindlessBufferIdBuffer;
        else
        {
          if (remap.texture_type == MetalImageType::Tex2D)
            bindless_buffer = render.bindlessTexture2DIdBuffer;
          else if (remap.texture_type == MetalImageType::TexCube)
            bindless_buffer = render.bindlessTextureCubeIdBuffer;
          else if (remap.texture_type == MetalImageType::Tex2DArray)
            bindless_buffer = render.bindlessTexture2DArrayIdBuffer;
          else
            G_ASSERTF(0, "Unsupported bindless texture array type %d", int(remap.texture_type));
        }
        G_ASSERT(bindless_buffer);

        storage.bindBuffer(target, bindless_buffer, bindless_buffer->getBuffer(), bindless_buffer->getDynamicOffset(), false, resources);
      }
    }

    if (shader->immediate_slot >= 0)
      apply_immediate_constants(storage, shader, *this, resources);

    if (storage.texture_dirty_mask & shader->texture_mask)
      apply_textures(storage, shader, resources);

    if (storage.sampler_dirty_mask & shader->sampler_mask)
      apply_samplers(storage, shader, resources);

    if (storage.stage == STAGE_CS)
      apply_acceleration_structs(storage, shader, resources);

    cbuffer.applyBuffer(storage, shader->num_reg, resources);
  }

  void Render::StageBinding::reset()
  {
    for (int i = 0; i < MAX_STAGE_TEXTURES; i++)
        textures[i] = {};
    for (int i = 0; i < BUFFER_POINT_COUNT; i++)
    {
      if (buffers[i])
        buffers[i]->bound_slots = 0;
      buffers[i] = nil;
    }
    for (uint32_t j = 0; j < MAX_SHADER_ACCELERATION_STRUCTURES; ++j)
        acceleration_structures[j] = nullptr;
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

  static void PatchVertexShader(int shader)
  {
    Shader* vshader = render.shaders.getObj(shader);
    vshader->num_va = 1;
    vshader->va[0].reg = 0;
    vshader->va[0].vsdr = VSDR_POS;
  }

  id<MTLAccelerationStructure> createDefaultBlas(id<MTLDevice> device)
  {
    id<MTLBuffer> vertexBuffer = [device newBufferWithLength : sizeof(float)*9
                                                     options : MTLResourceStorageModeShared];
    float *vdata = (float *)vertexBuffer.contents;
    vdata[0] = 0.0f; vdata[1] = 0.0f; vdata[2] = 0.f;
    vdata[3] = 0.1f; vdata[4] = 0.0f; vdata[5] = 0.f;
    vdata[6] = 0.0f; vdata[7] = 0.1f; vdata[8] = 0.f;

    MTLAccelerationStructureTriangleGeometryDescriptor *geomDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

    geomDesc.vertexBuffer = vertexBuffer;
    geomDesc.vertexStride = 12;
    geomDesc.vertexBufferOffset = 0;
    if (@available(iOS 17, macOS 13.0, *))
    {
      geomDesc.vertexFormat = MTLAttributeFormatFloat3;
    }
    geomDesc.triangleCount = 1;
    geomDesc.intersectionFunctionTableOffset = 0; // not implemented
    geomDesc.opaque = true;
    geomDesc.allowDuplicateIntersectionFunctionInvocation = false;

    NSMutableArray *geometries = [[NSMutableArray alloc] init];
    [geometries addObject : geomDesc];

    MTLPrimitiveAccelerationStructureDescriptor *accDesc = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
    accDesc.geometryDescriptors = geometries;
    accDesc.usage = MTLAccelerationStructureUsageNone;

    MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor : accDesc];
    id<MTLAccelerationStructure> defaultBlas = [render.device newAccelerationStructureWithSize : sizes.accelerationStructureSize];
    defaultBlas.label = @"default blas";

    id<MTLBuffer> scratchBuffer = [device newBufferWithLength : sizes.buildScratchBufferSize
                                                      options : MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cmdBuf = [render.commandQueue commandBufferWithUnretainedReferences];
    id<MTLAccelerationStructureCommandEncoder> encoder = [cmdBuf accelerationStructureCommandEncoder];

    [encoder buildAccelerationStructure : defaultBlas
                             descriptor : accDesc
                          scratchBuffer : scratchBuffer
                    scratchBufferOffset : 0];

    [encoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

    [vertexBuffer release];
    [scratchBuffer release];

    return defaultBlas;
  }

  id<MTLAccelerationStructure> createDefaultTlas(id<MTLDevice> device)
  {
    id<MTLBuffer> instanceBuf = nil;
    if (@available(iOS 17, macOS 12.0, *))
    {
      instanceBuf = [device newBufferWithLength : sizeof(MTLAccelerationStructureUserIDInstanceDescriptor)
                                        options : MTLResourceStorageModeShared];

      MTLAccelerationStructureUserIDInstanceDescriptor *data = (MTLAccelerationStructureUserIDInstanceDescriptor *)instanceBuf.contents;
      data[0].accelerationStructureIndex = 0;
      data[0].options = 0;
      data[0].intersectionFunctionTableOffset = 0;
      data[0].mask = 0;
      data[0].userID = 0;
      data[0].transformationMatrix.columns[0] = MTLPackedFloat3Make(1, 0, 0);
      data[0].transformationMatrix.columns[1] = MTLPackedFloat3Make(0, 1, 0);
      data[0].transformationMatrix.columns[2] = MTLPackedFloat3Make(0, 0, 1);
      data[0].transformationMatrix.columns[3] = MTLPackedFloat3Make(0, 0, 0);
    }
    G_ASSERT(instanceBuf);

    MTLInstanceAccelerationStructureDescriptor *accDescInstance = [MTLInstanceAccelerationStructureDescriptor descriptor];
    accDescInstance.usage = MTLAccelerationStructureUsageNone;
    if (@available(iOS 17, macOS 12.0, *))
    {
      accDescInstance.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeUserID;
    }
    accDescInstance.instanceCount = 1;
    accDescInstance.instanceDescriptorBuffer = instanceBuf;
    accDescInstance.instancedAccelerationStructures = @[ render.defaultBlas ];

    MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor : accDescInstance];
    id<MTLAccelerationStructure> defaultTlas = [device newAccelerationStructureWithSize : sizes.accelerationStructureSize];
    defaultTlas.label = @"default tlas";

    id<MTLBuffer> scratchBuffer = [device newBufferWithLength : sizes.buildScratchBufferSize
                                                      options : MTLResourceStorageModeShared];

    id<MTLCommandBuffer> cmdBuf = [render.commandQueue commandBufferWithUnretainedReferences];
    id<MTLAccelerationStructureCommandEncoder>encoder = [cmdBuf accelerationStructureCommandEncoder];

    [encoder buildAccelerationStructure : defaultTlas
                             descriptor : accDescInstance
                          scratchBuffer : scratchBuffer
                    scratchBufferOffset : 0];

    [encoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

    [instanceBuf release];
    [scratchBuffer release];

    return defaultTlas;
  }

  bool Render::init()
  {
    if (inited)
    {
      return false;
    }

    sampler_states.reserve(256);
#if HAVE_SAMPLE_QUERIES
    if ([device supportsCounterSampling : MTLCounterSamplingPointAtStageBoundary])
    {
      id<MTLCounterSet> timestampSet = nil;
      for (id<MTLCounterSet> counterSet in device.counterSets)
        if ([counterSet.name isEqualToString : MTLCommonCounterSetTimestamp])
          timestampSet = counterSet;
      if (timestampSet)
      {
        MTLCounterSampleBufferDescriptor *desc = [[MTLCounterSampleBufferDescriptor alloc] init];
        desc.counterSet = timestampSet;
        desc.storageMode = MTLStorageModeShared;
        desc.sampleCount = MAX_SAMPLE_SAMPLES;
        desc.label = @"counter sampling buffer";

        NSError *error = nil;
        sampleBuffer = [device newCounterSampleBufferWithDescriptor : desc
                                                              error : &error];
        if (error != nil)
        {
          debug("[METAL Profiler] Device failed to create a counter sample buffer. Error: %s", [[error localizedDescription] UTF8String]);
        }
        [desc release];
      }
    }
#endif

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

    caps.readWriteTextureTier1 = device.readWriteTextureSupport == MTLReadWriteTextureTier1 || device.readWriteTextureSupport == MTLReadWriteTextureTier2;
    caps.readWriteTextureTier2 = device.readWriteTextureSupport == MTLReadWriteTextureTier2;

    // we need to init caps before loading pre cache
    // and caps use clear_cs_pipeline
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
    d3d::get_driver_desc();
    shadersPreCache.init(get_shader_cache_version());

    max_number_of_frames_to_skip_after_error = ::dgs_get_settings()->getBlockByNameEx("metal")->getInt("framesToSkip", 0);
    report_gpu_errors = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("report_gpu_errors", true);
    validate_framemem_bounds = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("validate_framem_bounds", false);
    manual_hazard_tracking = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("enable_manual_hazard_tracking", true);
    hadError = false;

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

    for (auto &stg : stages)
    {
      stg.cbuffer.init();
      stg.reset();
    }

    forceClearOnCreate = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("forceClear", 0);

    D3DResourceType cube_array_type = D3DResourceType::CUBEARRTEX;
#if _TARGET_IOS
    if (![device supportsFamily:MTLGPUFamilyApple4])
      cube_array_type = D3DResourceType::CUBETEX;
#endif
    blank_tex[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_2d", false, true);
    blank_tex[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, D3DResourceType::ARRTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_array", false, true);
    blank_tex[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_DEPTH24, "blank_depth", false, true);
    blank_tex[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, D3DResourceType::CUBETEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_cube", false, true);
    blank_tex[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, D3DResourceType::VOLTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_3d", false, true);
    blank_tex[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_buffer", false, true);
    blank_tex[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_cube_array", false, true);

    blank_tex_uint[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_2d", false, true);
    blank_tex_uint[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, D3DResourceType::ARRTEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_array", false, true);
    blank_tex_uint[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_DEPTH24, "blank_uint_depth", false, true);
    blank_tex_uint[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, D3DResourceType::CUBETEX, TEXCF_RTARGET, TEXFMT_R32UI, "blank_uint_cube", false, true);
    blank_tex_uint[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, D3DResourceType::VOLTEX, TEXCF_RTARGET, TEXFMT_R32UI, "blank_uint_3d", false, true);
    blank_tex_uint[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_RTARGET, TEXFMT_R32UI, "blanktex_uint_buffer", false, true);
    blank_tex_uint[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_RTARGET, TEXFMT_R32UI, "blank_cube_array", false, true);

    blank_tex_rw[int(MetalImageType::Tex2D)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_2d", false, true);
    blank_tex_rw[int(MetalImageType::Tex2DArray)] = new Texture(1, 1, 1, 1, D3DResourceType::ARRTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_array", false, true);
    blank_tex_rw[int(MetalImageType::Tex2DDepth)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_UNORDERED, TEXFMT_DEPTH24, "blankrw_depth", false, true);
    blank_tex_rw[int(MetalImageType::TexCube)] = new Texture(1, 1, 1, 1, D3DResourceType::CUBETEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_cube", false, true);
    blank_tex_rw[int(MetalImageType::Tex3D)] = new Texture(1, 1, 1, 1, D3DResourceType::VOLTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_3d", false, true);
    blank_tex_rw[int(MetalImageType::TexBuffer)] = new Texture(1, 1, 1, 1, D3DResourceType::TEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_buffer", false, true);
    blank_tex_rw[int(MetalImageType::TexCubeArray)] = new Texture(1, 1, 1, 1, cube_array_type, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_cube_array", false, true);

    blank_tex[int(MetalImageType::Tex2D)]->apply(blank_tex_2d, false, 0, 0, false, false);
    G_ASSERT(blank_tex_2d);

    blank_tex[int(MetalImageType::TexCube)]->apply(blank_tex_cube, false, 0, 0, false, false);
    G_ASSERT(blank_tex_cube);

    blank_tex[int(MetalImageType::Tex2DArray)]->apply(blank_tex_2dArray, false, 0, 0, false, false);
    G_ASSERT(blank_tex_2dArray);

    // create default one
    free_encoders.clear();
    if (manual_hazard_tracking)
      current_enqueued_encoder = allocate_encoder();
    current_enqueued_resources = 0;

    last_encoders.clear();
    last_encoders_current.clear();

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
    PatchVertexShader(clear_vshd);
    PatchShader(clear_pshdi, 1, 0);
    PatchShader(clear_pshdui, 1, 0);
    PatchShader(clear_pshdf, 1, 0);

    clear_progi = createProgram(clear_vshd, clear_pshdi, clear_vdecl, 0, 0);
    clear_progui = createProgram(clear_vshd, clear_pshdui, clear_vdecl, 0, 0);
    clear_progf = createProgram(clear_vshd, clear_pshdf, clear_vdecl, 0, 0);

    copy_vshd = createVertexShader((const uint8_t*)copy_vs_metal);
    copy_pshd = createPixelShader((const uint8_t*)copy_ps_metal);

    PatchShader(copy_vshd, 1, 0);
    PatchVertexShader(copy_vshd);
    PatchShader(copy_pshd, 0, 1);

    copy_prog = createProgram(copy_vshd, copy_pshd, clear_vdecl, 0, 0);

    {
      float clear_mesh[8] = { -1,-1, -1, 1, 1,-1, 1, 1 };
      clear_mesh_buffer = d3d::create_vb(sizeof(clear_mesh), 0, "metal system clear");

      float *verts = NULL;
      clear_mesh_buffer->lock(0, 0, (void**)&verts, VBLOCK_WRITEONLY);
      if (verts)
        memcpy(verts, clear_mesh, sizeof(clear_mesh));
      clear_mesh_buffer->unlock();
    }

    strcpy(device_name, [[device name] UTF8String]);

    can_use_async_pso_compilation = get_aync_pso_compilation_supported();

    [render.mainview isHDRAvailable];

    if (d3d::get_driver_desc().caps.hasBindless)
    {
      bindlessTexture2DIdBuffer = new Buffer(BINDLESS_TEXTURE_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless texture 2d id buffer");
      bindlessTexture2DArrayIdBuffer = new Buffer(BINDLESS_TEXTURE_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless texture 2d array id buffer");
      bindlessTextureCubeIdBuffer = new Buffer(BINDLESS_TEXTURE_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless texture cube id buffer");
      bindlessBufferIdBuffer = new Buffer(BINDLESS_BUFFER_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless buffer id buffer");
      bindlessSamplerIdBuffer = new Buffer(BINDLESS_SAMPLER_COUNT, sizeof(uint64_t), SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, "bindless sampler id buffer");

      if (@available(iOS 18, macOS 15.0, *))
      {
        MTLResidencySetDescriptor *setDescriptor = [[MTLResidencySetDescriptor alloc] init];
        setDescriptor.label = @"Primary residency set";
        setDescriptor.initialCapacity = 4096;

        NSError *error = nil;
        residencySet = [device newResidencySetWithDescriptor : setDescriptor
                                                       error : &error];
        [setDescriptor release];

        [residencySet addAllocation : blank_tex_2d];
        [residencySet addAllocation : blank_tex_2dArray];
        [residencySet addAllocation : blank_tex_cube];
        [residencySet addAllocation : stub_buffer];

        if (error)
          DAG_FATAL("Couldn't create residency set because of %s", [error.localizedDescription UTF8String]);

        bindlessTextures2D.init(ToResourceID(blank_tex_2d));
        bindlessTexturesCube.init(ToResourceID(blank_tex_cube));
        bindlessTextures2DArray.init(ToResourceID(blank_tex_2dArray));
        bindlessBuffers.init(stub_buffer.gpuAddress);
      }
    }

    if (@available(iOS 17, macOS 13.0, *))
    {
      if (render.device.supportsRaytracing)
      {
        defaultBlas = createDefaultBlas(device);
        G_ASSERT(defaultBlas);

        int defaultIndex = blases.getFreeIndex(nullptr);
        G_ASSERT(defaultIndex == 0);
        nativeBlases = [[NSMutableArray alloc] initWithCapacity : 4096];
        [nativeBlases addObject : defaultBlas];

        defaultTlas = createDefaultTlas(device);
        G_ASSERT(defaultTlas);
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
    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::None);
    command_encoder.write(CommandType::DoMetalFX).write(color).write(output).write(colorMode);
#endif
  }

  bool Render::isInited()
  {
    return inited;
  }

  void Render::freeResources()
  {
    {
      TIME_PROFILE(free_queued);

      std::lock_guard<std::mutex> scopedLock(delete_lock);

      if (resources2delete.empty() || resources2delete[0].submit > submits_completed)
        return;

      progs.lock();
      shaders.lock();
      vdecls.lock();
      int freed_items = 0;
      for (freed_items; freed_items < resources2delete.size(); ++freed_items)
      {
        const auto &res = resources2delete[freed_items];
        if (res.submit > submits_completed)
          break;
        switch (res.type)
        {
          case DeletedResource::Type::None:
            G_ASSERT(0 && "we shouldn't be here");
            break;
          case DeletedResource::Type::Texture:
            G_ASSERT(res.texture);
#if DAGOR_DBGLEVEL > 0
            for (auto &tex : bindlessTextures2D.cache)
              G_ASSERTF(res.texture != tex.tex, "Texture %s beging released is in bindless 2d array", res.texture->getName());
            for (auto &tex : bindlessTextures2DArray.cache)
              G_ASSERTF(res.texture != tex.tex, "Texture %s beging released is in bindless 2darray array", res.texture->getName());
            for (auto &tex : bindlessTexturesCube.cache)
              G_ASSERTF(res.texture != tex.tex, "Texture %s beging released is in bindless cube array", res.texture->getName());
#endif
            res.texture->release();
            break;
          case DeletedResource::Type::Buffer:
            G_ASSERT(res.buffer);
#if DAGOR_DBGLEVEL > 0
            for (auto &buf : bindlessBuffers.cache)
              G_ASSERTF(res.buffer != buf.buf, "Buffers %s beging released is in bindless array", res.buffer->getName());
#endif
            res.buffer->release();
            break;
          case DeletedResource::Type::VDecl:
          {
            auto decl = vdecls.freeIndex(res.vdecl);
            if (decl)
              decl->release();
            break;
          }
          case DeletedResource::Type::Shader:
            shaders.freeIndex(res.shader)->release();
            break;
          case DeletedResource::Type::Program:
            progs.freeIndex(res.program)->release();
            break;
          case DeletedResource::Type::NativeResource:
            G_ASSERT(resource_residency.find(res.native_resource) == resource_residency.end());
            [res.native_resource release];
            break;
          case DeletedResource::Type::Heap:
            [res.heap release];
            break;
        }
      }
      progs.unlock();
      shaders.unlock();
      vdecls.unlock();

      if (freed_items)
        resources2delete.erase(resources2delete.begin(), resources2delete.begin() + freed_items);
    }
  }

  void Render::cleanupFrame()
  {
    stages[STAGE_VS].reset();
    stages[STAGE_PS].reset();
    stages[STAGE_CS].reset();

    int local_frame = (frame + 1) % MAX_FRAMES_TO_RENDER;

    // make sure its safe to free resources
    {
      TIME_PROFILE(wait_prev_frame);

      std::unique_lock<std::mutex> lock(frame_mutex);
      uint64_t frame_submit_number = frames_completed[local_frame];
      while (!frame_condition.wait_for(lock, std::chrono::seconds(1), [frame_submit_number]
      {
        return frame_submit_number <= render.submits_completed;
      }))
      {
        LOGERR_ONCE("[METAL] waiting for previous frame to finish for at least a second");
      }
    }

    freeResources();
    for (auto &func : frame_callbacks[local_frame])
      func();
    frame_callbacks[local_frame].clear();
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
      if ((report_gpu_errors || max_number_of_frames_to_skip_after_error) && number_of_frames_to_skip_after_error == 0)
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

    constexpr uint32_t copy_size = 131072;

    int src_offset = 0;
    id<MTLBuffer> copy_src = AllocateDynamicBuffer(copy_size, src_offset);

     // clear buffer so we copy zeroes from it
    if (base_format == TEXFMT_ASTC4)
    {
      // For astc "all zeroes" isn't considered a valid compressed block
      // so this uint4{ 98371, 0, 0, 0 } (tied to quant method used in shader)
      // value should be placed instead to be treated like a black color
      const uint32_t black_block[] = { 98371, 0, 0, 0 };
      const int blocked_size = copy_size / (sizeof(black_block));
      uint8_t *dst = (uint8_t *)copy_src.contents + src_offset;
      for (int i = 0; i < blocked_size; i++, dst += sizeof(black_block))
        memcpy(dst, black_block, sizeof(black_block));
    }
    else
      [blitEncoder fillBuffer:copy_src range:NSMakeRange(src_offset, copy_size) value:0];

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

          G_ASSERT(copy_size >= row_pitch);
          unsigned height_step = std::min(height, unsigned(copy_size / row_pitch));
          if (use_dxt)
            height_step = max(height_step & ~3, 4u);

          for (unsigned h = 0; h < height; h += height_step)
          {
            unsigned height_left = std::min(height - h, height_step);
            [blitEncoder copyFromBuffer : copy_src
                           sourceOffset : src_offset
                      sourceBytesPerRow : row_pitch
                    sourceBytesPerImage : copy_size
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
    if (cmd.dst_format == MTLPixelFormatEAC_RGBA8_sRGB || cmd.dst_format == MTLPixelFormatEAC_RGBA8
        || cmd.dst_format == MTLPixelFormatEAC_RG11Unorm
        || cmd.dst_format == MTLPixelFormatASTC_4x4_LDR || cmd.dst_format == MTLPixelFormatASTC_4x4_sRGB)
        koef = 16;

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

      constexpr uint32_t copy_size = 131072;

      int src_offset = 0;
      id<MTLBuffer> tmp_buf = sz * cmd.src_d <= copy_size ? [AllocateDynamicBuffer(sz * cmd.src_d, src_offset) retain] :
        createBuffer(sz*cmd.src_d, MTLResourceStorageModePrivate, "copy buffer");

      [blitEncoder copyFromTexture : cmd.src
                       sourceSlice : src_slice
                       sourceLevel : src_mip
                      sourceOrigin : origin
                        sourceSize : size
                          toBuffer : tmp_buf
                 destinationOffset : src_offset
            destinationBytesPerRow : stride
          destinationBytesPerImage : sz];

      size.width = cmd.src_w * 4;
      size.height = cmd.src_h * 4;

      [blitEncoder copyFromBuffer : tmp_buf
                     sourceOffset : src_offset
                sourceBytesPerRow : stride
              sourceBytesPerImage : sz
                       sourceSize : size
                        toTexture : cmd.dst
                 destinationSlice : dst_slice
                 destinationLevel : dst_mip
                destinationOrigin : destinationOrigin];

      render.queueResourceForDeletion(tmp_buf);
    }
    else
    {
#if DAGOR_DBGLEVEL > 0
      G_ASSERTF_RETURN(are_compatible_for_blit(cmd.src_format, cmd.dst_format), ,"Can't blit %s(%d) to %s(%d)", [cmd.src.label UTF8String],
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

  bool Render::copyBackbuffer(id<MTLCommandBuffer> commandBuffer)
  {
    if (!save_backBuffer)
      return false;

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
    save_backBuffer = false;

    return true;
  }

  struct ASBuildInfo
  {
    Buffer *scratch = nullptr;
    uint32_t offset = 0;
    bool update = false;
  };

  void Render::executeCommands(bool wait, bool present)
  {
    bool needBlit = buffers2copy.size() || textures2upload.size() || regions2copy.size() || textures2clear.size() || buffers2clear.size();
    if (command_encoder.empty() && !needBlit && !present && !wait)
      return;

    FRAMEMEM_VALIDATE;

    id<MTLCommandBuffer> commandBuffer = createCommandBuffer(commandQueue, "Command buffer");

    if (@available(macOS 15.0, iOS 18.0, *))
    {
      if (d3d::get_driver_desc().caps.hasBindless)
        [commandBuffer useResidencySet:residencySet];
    }

    if (needBlit)
      ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit);

    G_ASSERT(!needBlit || blitEncoder != nil);
    for (const auto &[buf_ptr, buf] : buffers2clear)
    {
      G_ASSERT(buf_ptr);
      track_resource_write(*buf_ptr);

      G_ASSERT(buf->buffer);
      [blitEncoder fillBuffer : buf->buffer
                        range : NSMakeRange(0, buf->bufsize)
                        value : 0];
      buf->initialized = true;
    }

    for (const auto &rc : buffers2copy)
    {
      G_ASSERT((rc.size & 3) == 0);

      G_ASSERT(rc.dst_ptr);
      track_resource_write(*rc.dst_ptr);

      G_ASSERT(rc.dst);
      G_ASSERT(rc.src);
      [blitEncoder copyFromBuffer : rc.src
                     sourceOffset : rc.src_offset
                         toBuffer : rc.dst
                destinationOffset : rc.dst_offset
                             size : rc.size];
    }

    for (const auto &rc : textures2upload)
    {
      MTLSize size = { (uint32_t)rc.w, (uint32_t)rc.h, (uint32_t)rc.d };
      MTLOrigin origin = { (uint32_t)rc.x, (uint32_t)rc.y, (uint32_t)rc.z };

      G_ASSERT(rc.tex_ptr);
      track_resource_write(*rc.tex_ptr);

      [blitEncoder copyFromBuffer : rc.buf
                     sourceOffset : 0
                sourceBytesPerRow : rc.pitch
              sourceBytesPerImage : rc.imageSize
                       sourceSize : size
                        toTexture : rc.tex
                 destinationSlice : rc.layer
                 destinationLevel : rc.level
                destinationOrigin : origin];
    }

    for (const auto &t : textures2clear)
    {
      G_ASSERT(t.tex);
      track_resource_write(*t.tex);
      doClearTexture(t.width, t.height, t.slices, t.depth, t.levels, t.metalTex, t.base_format, t.use_dxt);
    }

    for (const auto &r : regions2copy)
    {
      track_resource_read(*r.src_ptr);
      track_resource_write(*r.dst_ptr);

      doTexCopyRegion(r);
    }

    textures2upload.clear();
    buffers2copy.clear();
    regions2copy.clear();
    textures2clear.clear();
    buffers2clear.clear();

    CommandType command;
    while (!command_encoder.empty())
    {
      command_encoder.read(command);
      switch (command)
      {
        case CommandType::BeginPass:
        {
          RenderTargetConfig render_targets;
          MTLRenderPassDescriptor *render_desc = nullptr;
          command_encoder.read(render_targets).read(render_desc);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::None);
          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Render);
          G_ASSERT(render_desc);
          renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor : render_desc];
      #if DAGOR_DBGLEVEL > 0
          renderEncoder.label = render_desc.colorAttachments[0].texture ? render_desc.colorAttachments[0].texture.label : (render_desc.depthAttachment.texture ? render_desc.depthAttachment.texture.label : @"DagorRenderEncoder");
      #endif
          if (renderEncoder == nil)
            DAG_FATAL("Failed to allocate render encoder");
          [render_desc release];

          for (int i = 0; i < Program::MAX_SIMRT; i++)
          {
            if (render_targets.colors[i].texture)
            {
              if (render_targets.colors[i].store_action == MTLStoreActionDontCare)
                track_resource_read(*render_targets.colors[i].texture, true);
              else
                track_resource_write(*render_targets.colors[i].texture, true);
            }
            if (render_targets.colors[i].resolve_target)
              track_resource_write(*render_targets.colors[i].resolve_target);
          }
          if (render_targets.depth.texture)
          {
            if (render_targets.depth.store_action == MTLStoreActionDontCare)
              track_resource_read(*render_targets.depth.texture, true);
            else
              track_resource_write(*render_targets.depth.texture, true);
          }
          if (render_targets.depth.resolve_target)
            track_resource_write(*render_targets.depth.resolve_target);
        }
        break;
        case CommandType::SetResources:
        {
          uint8_t resource_count;
          command_encoder.read(resource_count);

          eastl::vector<Resource, framemem_allocator> resources(resource_count);
          command_encoder.read(resources.data(), resource_count * sizeof(Resource));

          G_ASSERT(renderEncoder || computeEncoder);
          flushResources(resources);
        }
        break;
        case CommandType::SetPipelineState:
        {
          id<MTLRenderPipelineState> state = nil;
          command_encoder.read(state);

          G_ASSERT(renderEncoder);
          G_ASSERT(state);
          [renderEncoder setRenderPipelineState : state];
        }
        break;
        case CommandType::SetComputePipelineState:
        {
          id<MTLComputePipelineState> state = nil;
          command_encoder.read(state);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Compute);

          G_ASSERT(computeEncoder);
          G_ASSERT(state);
          [computeEncoder setComputePipelineState : state];
        }
        break;
        case CommandType::SetDepthState:
        {
          id<MTLDepthStencilState> state = nil;
          command_encoder.read(state);

          G_ASSERT(renderEncoder);
          G_ASSERT(state);
          [renderEncoder setDepthStencilState : state];
        }
        break;
        case CommandType::SetStencilRef:
        {
          uint32_t ref = 0;
          command_encoder.read(ref);

          G_ASSERT(renderEncoder);
          [renderEncoder setStencilReferenceValue:ref];
        }
        break;
        case CommandType::SetBlendFactor:
        {
          float factor[4] = {};
          command_encoder.read(factor, sizeof(factor));

          G_ASSERT(renderEncoder);
          [renderEncoder setBlendColorRed : factor[0]
                                    green : factor[1]
                                     blue : factor[2]
                                    alpha : factor[3]];
        }
        break;
        case CommandType::SetDepthBias:
        {
          float bias = 0.f, slopebias = 0.f;
          command_encoder.read(bias).read(slopebias);

          G_ASSERT(renderEncoder);
          constexpr float MINIMUM_REPRESENTABLE_D16 = 2e-5f;
          [renderEncoder setDepthBias : bias / MINIMUM_REPRESENTABLE_D16
                           slopeScale : slopebias
                                clamp : 0.f];
        }
        break;
        case CommandType::SetViewport:
        {
          MTLViewport viewport;
          command_encoder.read(viewport);

          G_ASSERT(renderEncoder);
          [renderEncoder setViewport : viewport];
        }
        break;
        case CommandType::SetScissor:
        {
          MTLScissorRect scissor;
          command_encoder.read(scissor);

          G_ASSERT(renderEncoder);
          [renderEncoder setScissorRect : scissor];
        }
        break;
        case CommandType::SetCull:
        {
          MTLCullMode mode;
          command_encoder.read(mode);

          G_ASSERT(renderEncoder);
          [renderEncoder setCullMode : mode];
        }
        break;
        case CommandType::SetFill:
        {
          MTLTriangleFillMode mode;
          command_encoder.read(mode);

          G_ASSERT(renderEncoder);
          [renderEncoder setTriangleFillMode : mode];
        }
        break;
        case CommandType::SetQuery:
        {
          int32_t offset = 0;
          command_encoder.read(offset);

          G_ASSERT(renderEncoder);
          [renderEncoder setVisibilityResultMode : offset != -1 ? MTLVisibilityResultModeCounting : MTLVisibilityResultModeDisabled
                                          offset : offset != -1 ? offset : 0];
        }
        break;
        case CommandType::SetDepthClip:
        {
          MTLDepthClipMode mode;
          command_encoder.read(mode);

          G_ASSERT(renderEncoder);
          [renderEncoder setDepthClipMode : mode];
        }
        break;
        case CommandType::Draw:
        {
          MTLPrimitiveType type;
          uint32_t start_vertex = 0, vertex_count = 0, num_instances = 0, start_instance = 0;
          command_encoder.read(type).read(start_vertex).read(vertex_count).read(num_instances).read(start_instance);

          G_ASSERT(renderEncoder);
          [renderEncoder drawPrimitives : type
                            vertexStart : start_vertex
                            vertexCount : vertex_count
                          instanceCount : num_instances
                           baseInstance : start_instance];
        }
        break;
        case CommandType::DrawIndexed:
        {
          MTLPrimitiveType type;
          MTLIndexType index_type;
          Buffer *ibuffer = nullptr;
          id<MTLBuffer> metal_ibuffer = nil;
          uint32_t index_count = 0, ibuffer_offset = 0, num_instances = 0, base_vertex = 0, start_instance = 0;
          command_encoder.read(type).read(ibuffer).read(index_count).read(index_type).read(metal_ibuffer).read(ibuffer_offset).read(num_instances).read(base_vertex).read(start_instance);

          G_ASSERT(ibuffer);
          track_resource_read(*ibuffer, true);

          G_ASSERT(renderEncoder);
          G_ASSERT(metal_ibuffer);
          [renderEncoder drawIndexedPrimitives : type
                                    indexCount : index_count
                                     indexType : index_type
                                   indexBuffer : metal_ibuffer
                             indexBufferOffset : ibuffer_offset
                                 instanceCount : num_instances
                                    baseVertex : base_vertex
                                  baseInstance : start_instance];
        }
        break;
        case CommandType::DrawIndirect:
        {
          MTLPrimitiveType type;
          Buffer *buf = nullptr;
          id<MTLBuffer> metal_buf = nil;
          uint32_t buf_offset = 0;
          command_encoder.read(type).read(buf).read(metal_buf).read(buf_offset);

          G_ASSERT(buf);
          track_resource_read(*buf, true);

          G_ASSERT(renderEncoder);
          G_ASSERT(metal_buf);

          [renderEncoder drawPrimitives : type
                         indirectBuffer : metal_buf
                   indirectBufferOffset : buf_offset];
        }
        break;
        case CommandType::DrawIndexedIndirect:
        {
          MTLPrimitiveType type;
          Buffer *buf = nullptr, *ibuffer = nullptr;
          MTLIndexType itype;
          id<MTLBuffer> metal_buf = nil, metal_ibuffer = nil;
          uint32_t buf_offset = 0, ibuffer_offset = 0;
          command_encoder.read(type).read(buf).read(ibuffer).read(itype).read(metal_ibuffer).read(ibuffer_offset).read(metal_buf).read(buf_offset);

          G_ASSERT(buf);
          G_ASSERT(ibuffer);
          track_resource_read(*buf, true);
          track_resource_read(*ibuffer, true);

          G_ASSERT(renderEncoder);
          G_ASSERT(metal_buf);
          G_ASSERT(metal_ibuffer);
          [renderEncoder drawIndexedPrimitives : type
                                     indexType : itype
                                   indexBuffer : metal_ibuffer
                             indexBufferOffset : ibuffer_offset
                                indirectBuffer : metal_buf
                          indirectBufferOffset : buf_offset];
        }
        break;
        case CommandType::DispatchMesh:
        {
          MTLSize threadgroups, mthreads, othreads;
          command_encoder.read(threadgroups).read(mthreads).read(othreads);

          G_ASSERT(renderEncoder);
          if (@available(iOS 16, macos 13, *))
          {
            [renderEncoder drawMeshThreads : threadgroups
               threadsPerObjectThreadgroup : othreads
                 threadsPerMeshThreadgroup : mthreads];
          }
        }
        break;
        case CommandType::DispatchMeshIndirect:
        {
          Buffer *indirect_buf = nullptr;
          id<MTLBuffer> indirect_buf_metal = nil;
          MTLSize mthreads, othreads;
          uint32_t offset = 0, count = 0, stride = 0;
          command_encoder.read(indirect_buf).read(indirect_buf_metal).read(offset).read(count).read(stride).read(mthreads).read(othreads);

          G_ASSERT(indirect_buf);
          track_resource_read(*indirect_buf, true);

          G_ASSERT(renderEncoder);
          G_ASSERT(indirect_buf_metal != nil);
          G_ASSERT(count);
          G_ASSERT(stride);
          if (@available(iOS 16, macos 13, *))
          {
            for (uint32_t i = 0; i < count; ++i, offset += stride)
              [renderEncoder drawMeshThreadgroupsWithIndirectBuffer : indirect_buf_metal
                                               indirectBufferOffset : offset
                                        threadsPerObjectThreadgroup : othreads
                                          threadsPerMeshThreadgroup : mthreads];
          }
        }
        break;
        case CommandType::ClearBuffer:
        {
          Buffer *buf = nullptr;
          id<MTLBuffer> buf_metal = nil;
          uint32_t buffer_size = 0, channels = 0;
          unsigned val[4];
          command_encoder.read(buf).read(buf_metal).read(buffer_size).read(channels).read(val, sizeof(unsigned)*4);

          bool all_the_same = val[0] == val[1] && val[1] == val[2] && val[2] == val[3];
          if (all_the_same && (val[0] == 0 || val[0] == 0xffffffff) && computeEncoder == nil)
          {
            ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "ClearBuffer");

            G_ASSERT(buf);
            track_resource_write(*buf);

            [blitEncoder fillBuffer : buf_metal
                              range : NSMakeRange(0, buffer_size)
                              value : (val[0] & 0xff)];
          }
          else
          {
            ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Compute, "ClearBuffer");

            G_ASSERT(buf);
            track_resource_write(*buf);

            int buf_size_offset = 0;
            id<MTLBuffer> buf_size = AllocateConstants(32, buf_size_offset);
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 0) = val[0];
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 4) = val[1];
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 8) = val[2];
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 12) = val[3];
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 16) = buffer_size / 4;
            *(uint32_t *)((uint8_t *)buf_size.contents + buf_size_offset + 20) = channels;

            [computeEncoder setBuffer : buf_size
                               offset : buf_size_offset
                              atIndex : 0];
            [computeEncoder setBuffer : buf_metal
                               offset : 0
                              atIndex : 1];
            [computeEncoder dispatchThreadgroups : MTLSizeMake((buffer_size / 4 + 63) >> 6, 1, 1)
                           threadsPerThreadgroup : MTLSizeMake(64, 1, 1)];
          }
        }
        break;
        case CommandType::Dispatch:
        {
          MTLSize thread_groups, threads;
          command_encoder.read(thread_groups).read(threads);

          G_ASSERT(computeEncoder);
          [computeEncoder dispatchThreadgroups : thread_groups
                         threadsPerThreadgroup : threads];
        }
        break;
        case CommandType::DispatchIndirect:
        {
          Buffer *indirect_buffer = nullptr;
          MTLSize threads;
          id<MTLBuffer> indirect_buffer_metal = nil;
          uint32_t indirect_offset = 0;
          command_encoder.read(indirect_buffer).read(indirect_buffer_metal).read(indirect_offset).read(threads);

          G_ASSERT(computeEncoder);
          track_resource_read(*indirect_buffer);
          [computeEncoder dispatchThreadgroupsWithIndirectBuffer : indirect_buffer_metal
                                            indirectBufferOffset : indirect_offset
                                           threadsPerThreadgroup : threads];
        }
        break;
        case CommandType::GenerateMips:
        {
          Texture *tex = nullptr;
          id<MTLTexture> tex_metal = nil;
          command_encoder.read(tex).read(tex_metal);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "GenerateMips");

          track_resource_write(*tex);
          [blitEncoder generateMipmapsForTexture : tex_metal];
        }
        break;
        case CommandType::CopyTex:
        {
          Texture *src = nullptr, *dest = nullptr;
          command_encoder.read(src).read(dest);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "CopyTex");

          track_resource_read(*src);
          track_resource_write(*dest);

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

            if (dest->type == D3DResourceType::VOLTEX)
              size.depth = max(dest->depth >> i, 1);

            int count = 1;
            if (dest->type == D3DResourceType::CUBETEX)
              count = 6;
            else
            if (dest->type == D3DResourceType::ARRTEX)
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
        break;
        case CommandType::CopyBuf:
        {
          Buffer *src = nullptr, *dst = nullptr;
          id<MTLBuffer> src_metal = nil, dst_metal = nil;
          uint32_t srcOffset = 0, dstOffset = 0, size = 0;
          command_encoder.read(src).read(src_metal).read(dst).read(dst_metal).read(srcOffset).read(dstOffset).read(size);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "CopyBuffer");

          if (src)
            track_resource_read(*src);
          if (dst)
            track_resource_write(*dst);

          [blitEncoder copyFromBuffer : src_metal
                         sourceOffset : srcOffset
                             toBuffer : dst_metal
                    destinationOffset : dstOffset
                                 size : size];
        }
        break;
        case CommandType::ReadbackTexture:
        {
          Texture *tex_ptr = nullptr;
          id<MTLTexture> tex = nil;
          id<MTLBuffer> buf = nil;
          uint32_t level = 0, layer = 0, offset = 0, pitch = 0, imageSize = 0;
          MTLSize size;
          MTLOrigin origin;
          MTLBlitOption options;
          command_encoder.read(tex_ptr).read(tex).read(layer).read(level).read(origin).read(size).read(buf).read(offset).read(pitch).read(imageSize).read(options);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "ReadbackTexture");

          G_ASSERT(tex_ptr);
          track_resource_read(*tex_ptr);

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
        break;
        case CommandType::CopyTexRegion:
        {
          TexCopyRegion region;
          command_encoder.read(region);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Blit, "CopyTexRegion");

          track_resource_read(*region.src_ptr);
          track_resource_write(*region.dst_ptr);

          doTexCopyRegion(region);
        }
        break;
        case CommandType::TrackBindless:
        {
          uint32_t type = 0;
          command_encoder.read(type);

          if (type == BindlessTypeBuffer)
          {
            for (auto &buf : bindlessBuffers.cache)
              if (buf.buf)
                track_resource_read(*buf.buf);
          }
          else if (type == BindlessTypeTexture2DArray)
          {
            for (auto &tex : bindlessTextures2DArray.cache)
              if (tex.tex)
                track_resource_read(*tex.tex);
          }
          else if (type == BindlessTypeTextureCube)
          {
            for (auto &tex : bindlessTexturesCube.cache)
              if (tex.tex)
                track_resource_read(*tex.tex);
          }
          else if (type == BindlessTypeTexture2D)
          {
            for (auto &tex : bindlessTextures2D.cache)
              if (tex.tex)
                track_resource_read(*tex.tex);
          }
        }
        break;
        case CommandType::CopyAccelerationStruct:
        {
          RaytraceAccelerationStructure *dst = nullptr, *src = nullptr;
          id<MTLAccelerationStructure> as_dst = nil, as_src = nil;
          command_encoder.read(src).read(dst).read(as_src).read(as_dst);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Acceleration);

          G_ASSERT(as_src);
          G_ASSERT(as_dst);

          track_resource_write(*dst);
          track_resource_read(*src);

          [accelerationEncoder copyAccelerationStructure : as_src
                                 toAccelerationStructure : as_dst];
        }
        break;
        case CommandType::BuildAccelerationStructure:
        {
          RaytraceAccelerationStructure *as = nullptr;
          uint8_t count = 0;
          ASBuildInfo info;
          MTLAccelerationStructureDescriptor *desc = nullptr;
          command_encoder.read(as).read(desc).read(info).read(count);

          ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::Acceleration);

          for (uint32_t i = 0; i < count; ++i)
          {
            Buffer *buf = nullptr;
            command_encoder.read(buf);
            track_resource_read(*buf);
          }

          if (as->index == -1)
          {
            blases.lock();
            for (size_t i = 0; i < blases.getCount(); ++i)
            {
              if (blases.getObj(i))
                track_resource_read(*blases.getObj(i));
            }
            blases.unlock();
          }

          buildAccelerationStructure(as, desc, info.scratch, info.offset, info.update);
          G_ASSERT(as->acceleration_struct);
        }
        break;
        case CommandType::DoMetalFX:
        {
#if USE_METALFX_UPSCALE
          Texture *color = nullptr, *output = nullptr;
          uint32_t colorMode = 0;
          command_encoder.read(color).read(output).read(colorMode);

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
        break;
        case CommandType::DoQuery:
        {
          Query *query = nullptr;
          command_encoder.read(query);

          G_ASSERT(query);
          [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            query->value = *((uint64_t *)((uint8_t *)[query_buffer contents] + query->offset));
            query->status = 0;
          }];
        }
        break;
        case CommandType::PresentDrawable:
        {
          id<CAMetalDrawable> draw = nil;
          command_encoder.read(draw);

          [commandBuffer presentDrawable : draw];
          [draw release];
        }
        break;
        case CommandType::BeginEvent:
        {
          NSString *str = nil;
          command_encoder.read(str);

          G_ASSERT(commandBuffer);
          if (renderEncoder)
            [renderEncoder pushDebugGroup:str];
          else if (computeEncoder)
            [computeEncoder pushDebugGroup:str];
          else if (blitEncoder)
            [blitEncoder pushDebugGroup:str];
          else
            [commandBuffer pushDebugGroup:str];

          [str release];
        }
        break;
        case CommandType::EndEvent:
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
        break;
        default:
          G_ASSERT(0 && "Unknown command");
      }
    }

    wait |= copyBackbuffer(commandBuffer);

    ensureHaveEncoderExceptRender(commandBuffer, Render::EncoderType::None);
    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::None);

    if (present && drawable_acquired)
    {
      TIME_PROFILE(present);
      [mainview presentDrawable : commandBuffer];
      backbuffer->apiTex->rt_texture = nil;
      backbuffer->apiTex->texture = nil;
      backbuffer->apiTex->sub_texture = nil;
    }
    frames_completed[frame % MAX_FRAMES_TO_RENDER] = submitCommandBuffer(commandBuffer, wait);
    commandBuffer = nil;
  }

  void Render::flush(bool wait, bool present)
  {
    TIME_PROFILE(flush);

    checkRenderAcquired();

    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);

    executeCommands(wait, present);
    command_encoder.clear();

    if (present)
    {
      {
        std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);
        frame_callbacks[frame % MAX_FRAMES_TO_RENDER] = std::move(frame_callback);
        G_ASSERT(frame_callback.empty());
      }
      cleanupFrame();
      ++frame;
    }
    else
      freeResources();

    cur_prog = nullptr;
    cur_state = nullptr;
    bindless_resources_bound = 0;
  }

  bool Render::setBlendFactor(E3DCOLOR factor)
  {
    checkRenderAcquired();

    float c[4] = {factor.r / 255.f, factor.g / 255.f, factor.b / 255.f, factor.a / 255.f};
    if (blend_factor[0] == c[0] && blend_factor[1] == c[1] && blend_factor[2] == c[2] && blend_factor[3] == c[3])
      return true;

    blend_factor[0] = c[0];
    blend_factor[1] = c[1];
    blend_factor[2] = c[2];
    blend_factor[3] = c[3];

    dirty_state |= DirtyFlags::BlendFactor;

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
      if (encoder_type != EncoderType::Render)
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
    stage.setBuf(storages[rstage], RemapBufferSlot(bf_type, slot), buffer, offset);
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

  void Render::setTexture(unsigned rstage, int slot, Texture* tex, int mip_level, int slice, bool as_uint)
  {
    checkRenderAcquired();

    Render::StageBinding &stage = stages[rstage];
    stage.setTex(storages[rstage], slot, tex, tex ? tex->isReadStencil() : false, mip_level, slice, as_uint);
  }

  int Render::setSampler(unsigned rstage, int slot, int sampler, float bias)
  {
    checkRenderAcquired();

    Render::StageBinding &stage = stages[rstage];
    stage.setSampler(storages[rstage], slot, sampler >= 0 ? sampler_states[sampler].sampler : nil, bias);

    return 1;
  }

  void Render::removeResource(id<MTLResource> res)
  {
    if (res == nil)
      return;
    auto it = resource_residency.find(res);
    if (it != resource_residency.end())
    {
      if (it->second == 1)
      {
        if (@available(iOS 18, macOS 15.0, *))
        {
          [residencySet removeAllocation:res];
          bindless_set_dirty = true;
        }
        resource_residency.erase(it);
      }
      else
        it->second--;
    }
  }

  void Render::addResource(id<MTLResource> res)
  {
    G_ASSERT(res);
    auto it = resource_residency.find(res);
    if (it == resource_residency.end())
    {
      if (@available(iOS 18, macOS 15.0, *))
      {
        [residencySet addAllocation:res];
        bindless_set_dirty = true;
      }
      resource_residency[res] = 1;
    }
    else
      it->second++;
  }

  bool Render::BindlessTextureCache::update(uint32_t index, D3dResource *res)
  {
    G_ASSERTF(index < cache.size(), "bindless texture slot out of range: %d (max slot id: %d)", index, int(cache.size()));

    auto texture = res ? dynamic_cast<Texture*>(res) : nullptr;

    id<MTLTexture> metal_tex = nil;
    if (texture)
      texture->apply(metal_tex, false, 0, 0, false, false);

    if (cache[index].tex == texture && cache[index].metal_tex == metal_tex)
      return false;

    if (res)
      render.removeResource(cache[index].metal_tex);
    cache[index] = { texture, metal_tex };
    if (res)
      render.addResource(cache[index].metal_tex);

    id_cache[index] = metal_tex ? ToResourceID(metal_tex) : default_tex;

    return true;
  }

  bool Render::BindlessBufferCache::update(uint32_t index, D3dResource *res)
  {
    G_ASSERTF(index < cache.size(), "bindless buffer slot out of range: %d (max slot id: %d)", index, int(cache.size()));

    Buffer *buffer = res ? dynamic_cast<Buffer *>(res) : nullptr;
    id<MTLBuffer> metal_buffer = res ? buffer->getBuffer() : nil;
    if (buffer)
    {
      G_ASSERT(buffer->is_fast_discard() == false);
      G_ASSERT(buffer->is_dynamic() == false);
    }

    if (cache[index].buf == buffer && cache[index].metal_buf == metal_buffer)
      return false;

    if (res)
      render.removeResource(cache[index].metal_buf);
    cache[index] = { buffer, metal_buffer };
    if (res)
      render.addResource(cache[index].metal_buf);

    if (@available(macOS 13.0, iOS 16.0, *))
    {
      id_cache[index] = metal_buffer ? metal_buffer.gpuAddress : default_buf;
    }

    return true;
  }

  bool Render::updateBindlessResource(D3DResourceType range_type, uint32_t index, D3dResource *res)
  {
    checkRenderAcquired();

    G_ASSERT(res);
    auto resType = res->getType();
    G_ASSERT(range_type == resType);
    if (D3DResourceType::SBUF == resType)
    {
      if (!bindlessBuffers.update(index, res))
        return false;
      bindless_resources_bound &= ~BindlessTypeBuffer;
    }
    else if (D3DResourceType::TEX == resType)
    {
      if (!bindlessTextures2D.update(index, res))
        return false;
      bindless_resources_bound &= ~BindlessTypeTexture2D;
    }
    else if (D3DResourceType::CUBETEX == resType)
    {
      if (!bindlessTexturesCube.update(index, res))
        return false;
      bindless_resources_bound &= ~BindlessTypeTextureCube;
    }
    else if (D3DResourceType::ARRTEX == resType)
    {
      if (!bindlessTextures2DArray.update(index, res))
        return false;
      bindless_resources_bound &= ~BindlessTypeTexture2DArray;
    }
    return true;
  }

  int Render::copyBindlessResources(D3DResourceType type, uint32_t src, uint32_t dst, uint32_t count)
  {
    checkRenderAcquired();

    if (D3DResourceType::SBUF == type)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        bindlessBuffers.update(dst + i, bindlessBuffers.cache[src + i].buf);
        bindlessBuffers.update(src + i, nullptr);
      }
      bindless_resources_bound &= ~BindlessTypeBuffer;
    }
    else if (D3DResourceType::TEX == type)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        bindlessTextures2D.update(dst + i, bindlessTextures2D.cache[src + i].tex);
        bindlessTextures2D.update(src + i, nullptr);
      }
      bindless_resources_bound &= ~BindlessTypeTexture2D;
    }
    else if (D3DResourceType::CUBETEX == type)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        bindlessTexturesCube.update(dst + i, bindlessTexturesCube.cache[src + i].tex);
        bindlessTexturesCube.update(src + i, nullptr);
      }
      bindless_resources_bound &= ~BindlessTypeTextureCube;
    }
    else if (D3DResourceType::ARRTEX == type)
    {
      for (uint32_t i = 0; i < count; i++)
      {
        bindlessTextures2DArray.update(dst + i, bindlessTextures2DArray.cache[src + i].tex);
        bindlessTextures2DArray.update(src + i, nullptr);
      }
      bindless_resources_bound &= ~BindlessTypeTexture2DArray;
    }
    return 1;
  }

  int Render::updateBindlessResourcesToNull(D3DResourceType type, uint32_t index, uint32_t count)
  {
    checkRenderAcquired();

    if (D3DResourceType::SBUF == type)
    {
      bool update = false;
      for (uint32_t i = 0; i < count; i++)
        update |= bindlessBuffers.update(index + i, nullptr);
      if (update)
        bindless_resources_bound &= ~BindlessTypeBuffer;
    }
    else if (D3DResourceType::TEX == type)
    {
      bool update = false;
      for (uint32_t i = 0; i < count; i++)
        update |= bindlessTextures2D.update(index + i, nullptr);
      if (update)
        bindless_resources_bound &= ~BindlessTypeTexture2D;
    }
    else if (D3DResourceType::CUBETEX == type)
    {
      bool update = false;
      for (uint32_t i = 0; i < count; i++)
        update |= bindlessTexturesCube.update(index + i, nullptr);
      if (update)
        bindless_resources_bound &= ~BindlessTypeTextureCube;
    }
    else if (D3DResourceType::ARRTEX == type)
    {
      bool update = false;
      for (uint32_t i = 0; i < count; i++)
        update |= bindlessTextures2DArray.update(index + i, nullptr);
      if (update)
        bindless_resources_bound &= ~BindlessTypeTexture2DArray;
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

    MTLPrimitiveType type = convertPrimitiveType(prim_type);
    command_encoder.write(CommandType::Draw).write(type).write(start_vertex).write(vertex_count).write(num_instances).write(start_instance);

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

    G_ASSERT(ibuffer);
    id<MTLBuffer> metal_ibuffer = ibuffer->getBuffer();
    uint32_t ibuffer_offset = offset + ibuffer->getDynamicOffset();

    MTLPrimitiveType type = convertPrimitiveType(prim_type);
    command_encoder.write(CommandType::DrawIndexed).write(type).write(ibuffer).write(index_count).write(ibuffertype).write(metal_ibuffer).write(ibuffer_offset).write(num_instances).write(base_vertex).write(start_instance);

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

    G_ASSERT(buffer);
    Buffer *buf = (Buffer *)buffer;

    id<MTLBuffer> metal_buf = buf->getBuffer();
    uint32_t buf_offset = offset + buf->getDynamicOffset();

    MTLPrimitiveType type = convertPrimitiveType(prim_type);
    command_encoder.write(CommandType::DrawIndirect).write(type).write(buf).write(metal_buf).write(buf_offset);

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

    G_ASSERT(ibuffer);
    G_ASSERT(buffer);
    Buffer *buf = (Buffer *)buffer;

    id<MTLBuffer> metal_ibuffer = ibuffer->getBuffer();
    uint32_t ibuffer_offset = ibuffer->getDynamicOffset();

    id<MTLBuffer> metal_buf = buf->getBuffer();
    uint32_t buf_offset = offset + buf->getDynamicOffset();

    MTLPrimitiveType type = convertPrimitiveType(prim_type);
    command_encoder.write(CommandType::DrawIndexedIndirect).write(type).write(buf).write(ibuffer).write(ibuffertype).write(metal_ibuffer).write(ibuffer_offset).write(metal_buf).write(buf_offset);

    return true;
  }

  bool Render::drawUP(int prim_type, int prim_count, const uint16_t* ind, const void* ptr, int numvert, int stride_bytes)
  {
    checkRenderAcquired();

    int vbuf_offset = 0;
    upBufferVB.dynamic_buffer = AllocateConstants(numvert*stride_bytes, vbuf_offset);
    upBufferVB.dynamic_frame = frame;
    upBufferVB.dynamic_offset = 0;

    memcpy((uint8_t*)upBufferVB.dynamic_buffer.contents + vbuf_offset, ptr, numvert*stride_bytes);
    d3d::setvsrc_ex(0, &upBufferVB, vbuf_offset, stride_bytes);

    if (ind)
    {
      int ibuf_offset = 0;
      upBufferIB.dynamic_buffer = AllocateConstants(prim_count * 2, ibuf_offset);
      upBufferIB.dynamic_frame = frame;
      upBufferIB.dynamic_offset = 0;

      memcpy((uint8_t*)upBufferIB.dynamic_buffer.contents + ibuf_offset, ind, prim_count * 2);

      d3d::setind(&upBufferIB);
      drawIndexed(prim_type, ibuf_offset / 2, prim_count, 0, 1);
    }
    else
      draw(prim_type, 0, prim_count, 1);

    d3d::setind(nullptr);
    d3d::setvsrc_ex(0, nullptr, 0, 0);

    return true;
  }

  bool Render::dispatchMesh(uint32_t x, uint32_t y, uint32_t z)
  {
    checkRenderAcquired();

    if (!d3d::get_driver_desc().caps.hasMeshShader)
    {
      D3D_ERROR("Metal: trying to use unsupported dispatchMesh");
      return false;
    }

    if (applyStates() == false)
      return true;

    G_ASSERT(cur_prog->mshader);
    MTLSize threadgroups = MTLSizeMake(x, y, z);
    MTLSize mthreads = MTLSizeMake(cur_prog->mshader->tgrsz_x, cur_prog->mshader->tgrsz_y, cur_prog->mshader->tgrsz_z);
    MTLSize othreads = mthreads;
    if (cur_prog->ashader)
      othreads = MTLSizeMake(cur_prog->ashader->tgrsz_x, cur_prog->ashader->tgrsz_y, cur_prog->ashader->tgrsz_z);
    command_encoder.write(CommandType::DispatchMesh).write(threadgroups).write(mthreads).write(othreads);

    return true;
  }

  bool Render::dispatchMeshIndirect(Buffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
  {
    checkRenderAcquired();

    if (!d3d::get_driver_desc().caps.hasMeshShader)
    {
      D3D_ERROR("Metal: trying to use unsupported dispatchMeshIndirect");
      return false;
    }

    G_ASSERT(args);

    if (applyStates() == false)
      return true;

    id<MTLBuffer> metal_args = args->getBuffer();
    byte_offset += args->getDynamicOffset();

    G_ASSERT(cur_prog->mshader);
    MTLSize mthreads = MTLSizeMake(cur_prog->mshader->tgrsz_x, cur_prog->mshader->tgrsz_y, cur_prog->mshader->tgrsz_z);
    MTLSize othreads = mthreads;
    if (cur_prog->ashader)
      othreads = MTLSizeMake(cur_prog->ashader->tgrsz_x, cur_prog->ashader->tgrsz_y, cur_prog->ashader->tgrsz_z);

    command_encoder.write(CommandType::DispatchMeshIndirect).write(args).write(metal_args).write(byte_offset).write(dispatch_count).write(stride_bytes).write(mthreads).write(othreads);

    return true;
  }

  uint64_t Render::getTimestampResult(uint64_t timestamp)
  {
#if HAVE_SAMPLE_QUERIES
    if (sampleBuffer == nil || timestamp == 0)
      return 0;
    if (timestamp - 1 >= processedSample)
      return 0;
    return sampleSamples[(timestamp - 1) % MAX_SAMPLE_SAMPLES];
#else
    return 1;
#endif
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
    resources2delete.push_back({ .type = DeletedResource::Type::Shader, .submit = submits_scheduled, .shader = vs });
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
    resources2delete.push_back({ .type = DeletedResource::Type::Shader, .submit = submits_scheduled, .shader = ps });
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
    resources2delete.push_back({ .type = DeletedResource::Type::VDecl, .submit = submits_scheduled, .vdecl = vdecl });
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
        new_prog->mshader != cur_prog->mshader || new_prog->mshader != cur_prog->mshader ||
        new_prog->pshader != cur_prog->pshader || new_prog->cshader != cur_prog->cshader))
    {
      if (!new_prog->cshader)
      {
        uint64_t vs_hash_old = !cur_prog || !cur_prog->vshader ? ~0ull : cur_prog->vshader->shader_hash;
        uint64_t vs_hash_new = new_prog->vshader ? new_prog->vshader->shader_hash : ~0ull;

        uint64_t ms_hash_old = !cur_prog || !cur_prog->mshader ? ~0ull : cur_prog->mshader->shader_hash;
        uint64_t ms_hash_new = new_prog->mshader ? new_prog->mshader->shader_hash : ~0ull;

        uint64_t as_hash_old = !cur_prog || !cur_prog->ashader ? ~0ull : cur_prog->ashader->shader_hash;
        uint64_t as_hash_new = new_prog->ashader ? new_prog->ashader->shader_hash : ~0ull;

        uint64_t ps_hash_old = !cur_prog || !cur_prog->pshader ? ~0ull : cur_prog->pshader->shader_hash;
        uint64_t ps_hash_new = new_prog->pshader ? new_prog->pshader->shader_hash : ~0ull;

        bool vs_new = vs_hash_old != vs_hash_new;
        bool ms_new = ms_hash_old != ms_hash_new;
        bool as_new = as_hash_old != as_hash_new;
        bool ps_new = ps_hash_old != ps_hash_new;
        if (vs_new && new_prog->vshader)
        {
          vdecl = NULL;
          storages[STAGE_VS].buffer_dirty_mask |= new_prog->vshader->buffer_mask;
          storages[STAGE_VS].sampler_dirty_mask |= new_prog->vshader->sampler_mask;
          storages[STAGE_VS].texture_dirty_mask |= new_prog->vshader->texture_mask;
          storages[STAGE_VS].buffer_dirty_mask |= new_prog->vshader->bindless_mask;
        }
        if (ms_new && new_prog->mshader)
        {
          vdecl = NULL;
          storages[STAGE_VS].buffer_dirty_mask |= new_prog->mshader->buffer_mask;
          storages[STAGE_VS].sampler_dirty_mask |= new_prog->mshader->sampler_mask;
          storages[STAGE_VS].texture_dirty_mask |= new_prog->mshader->texture_mask;
          storages[STAGE_VS].buffer_dirty_mask |= new_prog->mshader->bindless_mask;
        }
        if (ps_new && new_prog->pshader)
        {
          storages[STAGE_PS].buffer_dirty_mask |= new_prog->pshader->buffer_mask;
          storages[STAGE_PS].sampler_dirty_mask |= new_prog->pshader->sampler_mask;
          storages[STAGE_PS].texture_dirty_mask |= new_prog->pshader->texture_mask;
          storages[STAGE_PS].buffer_dirty_mask |= new_prog->pshader->bindless_mask;
        }
        if (vs_new || ps_new || ms_new || as_new)
          cur_state = NULL;
      }
      else
      {
        storages[STAGE_CS].buffer_dirty_mask |= new_prog->cshader->buffer_mask;
        storages[STAGE_CS].buffer_dirty_mask |= new_prog->cshader->bindless_mask;
        storages[STAGE_CS].sampler_dirty_mask |= new_prog->cshader->sampler_mask;
        storages[STAGE_CS].texture_dirty_mask |= new_prog->cshader->texture_mask;
      }
    }
    cur_prog = new_prog;
    using_async_pso_compilation = async_pso_compilation && can_use_async_pso_compilation;
  }

  void Render::deleteProgram(int prog)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.push_back({ .type = DeletedResource::Type::Program, .submit = submits_scheduled, .program = prog });
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
    resources2delete.push_back({ .type = DeletedResource::Type::Shader, .submit = submits_scheduled, .shader = cs });
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

  static uint32_t format2channelCount(Texture *tex)
  {
    if (tex == nullptr)
      return 1;
    if (tex->metal_format == MTLPixelFormatRGBA32Float || tex->metal_format == MTLPixelFormatRGBA32Uint)
      return 4;
    if (tex->metal_format == MTLPixelFormatRG32Float || tex->metal_format == MTLPixelFormatRG32Uint)
      return 2;
    if (tex->metal_format == MTLPixelFormatR32Float || tex->metal_format == MTLPixelFormatR32Uint)
      return 1;
    G_ASSERTF(0, "Unsupported texture clear format %d", tex->metal_format);
    return 1;
  }

  void Render::clearRwBufferi(Sbuffer *buff, const unsigned *val)
  {
    checkRenderAcquired();

    // handle only this special case for now. i searched through the code and it seems we don't use anything else
    G_ASSERT(buff);
    Buffer *buf = (Buffer*)buff;
    id<MTLBuffer> buf_metal = buf->getBuffer();
    uint32_t buffer_size = buf->bufSize;
    uint32_t channels = format2channelCount(buf->getTexture());

    G_ASSERT(buf->getDynamicOffset() == 0);
    G_ASSERT(!buf->is_fast_discard());

    bool all_the_same = val[0] == val[1] && val[1] == val[2] && val[2] == val[3];
    if (all_the_same && (val[0] == 0 || val[0] == 0xffffffff) && encoder_type != EncoderType::Compute)
      ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
    else
    {
      ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Compute);
      if (current_cs_pipeline != clear_cs_pipeline)
        command_encoder.write(CommandType::SetComputePipelineState).write(clear_cs_pipeline);
      current_cs_pipeline = clear_cs_pipeline;
    }
    command_encoder.write(CommandType::ClearBuffer).write(buf).write(buf_metal).write(buffer_size).write(channels).write(val, sizeof(unsigned)*4);
  }

  void Render::clearBuffer(Buffer *buf, Buffer::BufTex* buff)
  {
    // called on buffer creation, can be queued to the beginning of the flush
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    G_ASSERT(buf);
    track_resource_write_enqueued(*buf);
    buffers2clear.emplace_back(buf, buff);
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

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Compute);

    if (current_cs_pipeline != cur_prog->csPipeline)
      command_encoder.write(CommandType::SetComputePipelineState).write(cur_prog->csPipeline);
    current_cs_pipeline = cur_prog->csPipeline;

    eastl::vector<Resource, framemem_allocator> resources;
    stages[STAGE_CS].apply(storages[STAGE_CS], cur_prog->cshader, resources);

    G_ASSERT(resources.size() < 256);
    G_ASSERT(encoder_type == EncoderType::Compute);
    uint8_t resource_count = uint8_t(resources.size());
    command_encoder.write(CommandType::SetResources).write(resource_count).write(resources.data(), resource_count * sizeof(Resource));

    MTLSize thread_groups = MTLSizeMake(thread_group_x, thread_group_y, thread_group_z);
    MTLSize threads = MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z);

    command_encoder.write(CommandType::Dispatch).write(thread_groups).write(threads);
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

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Compute);

    if (current_cs_pipeline != cur_prog->csPipeline)
      command_encoder.write(CommandType::SetComputePipelineState).write(cur_prog->csPipeline);
    current_cs_pipeline = cur_prog->csPipeline;

    eastl::vector<Resource, framemem_allocator> resources;
    stages[STAGE_CS].apply(storages[STAGE_CS], cur_prog->cshader, resources);

    G_ASSERT(resources.size() < 256);
    G_ASSERT(encoder_type == EncoderType::Compute);
    uint8_t resource_count = uint8_t(resources.size());
    command_encoder.write(CommandType::SetResources).write(resource_count).write(resources.data(), resource_count * sizeof(Resource));

    G_ASSERT(buffer);
    Buffer *indirect_buffer = (Buffer*)buffer;

    MTLSize threads = MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z);
    id<MTLBuffer> indirect_buffer_metal = indirect_buffer->getBuffer();
    uint32_t indirect_offset = offset + indirect_buffer->getDynamicOffset();

    command_encoder.write(CommandType::DispatchIndirect).write(indirect_buffer).write(indirect_buffer_metal).write(indirect_offset).write(threads);
  }

  void Render::generateMips(Texture* tex)
  {
    checkRenderAcquired();

    G_ASSERT(tex);
    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
    command_encoder.write(CommandType::GenerateMips).write(tex).write(tex->apiTex->texture);
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

    command_encoder.write(CommandType::CopyTex).write(src).write(dest);
    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
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
      track_resource_write_enqueued(*dest);
      regions2copy.push_back(region);
    }
    else
    {
      checkRenderAcquired();

      command_encoder.write(CommandType::CopyTexRegion).write(region);
      ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
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
    acquireOwnership();

    doClear(tex, level, layer, 0.0f, (float *)val, true, true, false);

    releaseOwnership();
  }

  void Render::clearTex(Texture* tex, const unsigned val[4], int level, int layer)
  {
    acquireOwnership();

    doClear(tex, level, layer, 0.0f, (float *)val, true, true, false);

    releaseOwnership();
  }

  void Render::clearTex(Texture* tex, const float val[4], int level, int layer)
  {
    acquireOwnership();

    doClear(tex, level, layer, 0.0f, (float *)val, false, true, false);

    releaseOwnership();
  }

  void Render::clearTexture(Texture* tex)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    track_resource_write_enqueued(*tex);
    textures2clear.emplace_back(tex);
  }

  void Render::copyBuffer(Sbuffer* srcb, int srcOffset, Sbuffer* dstb, int dstOffset, int size)
  {
    checkRenderAcquired();

    Buffer* dst = (Buffer*)dstb;
    id<MTLBuffer> dst_metal = dst->getBuffer();
    Buffer* src = (Buffer*)srcb;
    id<MTLBuffer> src_metal = src->getBuffer();

    size = size ? size : src->bufSize;

    srcOffset += src->getDynamicOffset();

    G_ASSERT(dst);
    G_ASSERTF(dst->getDynamicOffset() == 0, "Buffer %s should have zero dynamic offset, but got %d", dst->getName(),
              dst->getDynamicOffset());

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
    command_encoder.write(CommandType::CopyBuf).write(src).write(src_metal).write(dst).write(dst_metal).write(srcOffset).write(dstOffset).write(size);
  }

  void Render::copyTex(Texture *src, Texture *dst, const RectInt *rsrc, const RectInt *rdst)
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

    vs_stage.setBuf(storages[STAGE_VS], 0, (Buffer*)clear_mesh_buffer);
    cur_rstate.vbuffer_stride[0] = 8;

    setProgram(copy_prog);
    setRenderTarget(dst, 0, 0);

    ps_stage.setTex(storages[STAGE_PS], 0, src);
    setConstants(vs_stage, src_rect);

    if (dst_rect[0] != -1)
      setViewport(dst_rect[0], dst_rect[2], dst_rect[1], dst_rect[3], 0.f, 1.f);

    draw(PRIM_TRISTRIP, 0, 4, 1, 0);
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

    vs_stage.setBuf(storages[STAGE_VS], 0, (Buffer*)clear_mesh_buffer);
    cur_rstate.vbuffer_stride[0] = 8;

    setProgram(clear_int ? (is_signed_int_format(dst) ? clear_progi : clear_progui) : clear_progf);
    if (dst)
      setRenderTarget(dst, dst_level, dst_layer);

    setConstants(vs_stage, z);
    setConstants(ps_stage, color);

    draw(PRIM_TRISTRIP, 0, 4, 1, 0);
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
      command_encoder.write(CommandType::SetPipelineState).write(cur_state);
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

      command_encoder.write(CommandType::SetDepthState).write(cur_depthstate.depthState);
    }

    if (dirty_state & DirtyFlags::StencilRef)
      command_encoder.write(CommandType::SetStencilRef).write(stencil_ref);

    if (dirty_state & DirtyFlags::BlendFactor)
      command_encoder.write(CommandType::SetBlendFactor).write(blend_factor, sizeof(blend_factor));

    constexpr float MINIMUM_REPRESENTABLE_D16 = 2e-5f;
    if (dirty_state & DirtyFlags::DepthBias)
      command_encoder.write(CommandType::SetDepthBias).write(depth_bias).write(depth_slopebias);

    if (dirty_state & DirtyFlags::Viewport)
    {
      MTLViewport viewport;
      viewport.originX = rt.vp.x;
      viewport.originY = rt.vp.y;

      viewport.width = rt.vp.w;
      viewport.height = rt.vp.h;

      viewport.znear = rt.vp.minz;
      viewport.zfar = rt.vp.maxz;

      cached_viewport = viewport;

      command_encoder.write(CommandType::SetViewport).write(viewport);
    }

    if (dirty_state & DirtyFlags::Scissor)
    {
      MTLScissorRect scissor;

      if (scissor_on)
      {
        G_ASSERT(cur_wd >= sci_x + sci_w);
        G_ASSERT(cur_ht >= sci_y + sci_h);
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

      command_encoder.write(CommandType::SetScissor).write(scissor);
    }

    if (dirty_state & DirtyFlags::Cull)
      command_encoder.write(CommandType::SetCull).write(cur_cull);

    if (dirty_state & DirtyFlags::Fill)
      command_encoder.write(CommandType::SetFill).write(cur_fill);

    if (dirty_state & DirtyFlags::DepthClip)
      command_encoder.write(CommandType::SetDepthClip).write(depth_clip);

    if (dirty_state & DirtyFlags::Query)
      command_encoder.write(CommandType::SetQuery).write(cur_query_offset);

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

    ResourceArray resources;
    if (cur_prog->vshader)
      stages[STAGE_VS].apply(storages[STAGE_VS], cur_prog->vshader, resources);
    else
    {
      G_ASSERT(cur_prog->mshader);
      stages[STAGE_VS].apply(storages[STAGE_MS], cur_prog->mshader, resources);
      if (cur_prog->ashader)
        stages[STAGE_VS].apply(storages[STAGE_OS], cur_prog->ashader, resources);
    }
    if (cur_prog->pshader)
      stages[STAGE_PS].apply(storages[STAGE_PS], cur_prog->pshader, resources);

    G_ASSERT(resources.size() < 256);
    G_ASSERT(encoder_type == EncoderType::Render);
    uint8_t resource_count = uint8_t(resources.size());
    command_encoder.write(CommandType::SetResources).write(resource_count).write(resources.data(), resource_count * sizeof(Resource));

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
    if (tex->type == D3DResourceType::VOLTEX && attach.layer != d3d::RENDER_TO_WHOLE_ARRAY)
      desc.depthPlane = attach.layer;
    else if ((tex->type == D3DResourceType::CUBETEX || tex->type == D3DResourceType::ARRTEX) && attach.layer != d3d::RENDER_TO_WHOLE_ARRAY)
      desc.slice = attach.layer;

    G_ASSERT(samples == -1 || samples == desc.texture.sampleCount);
    samples = desc.texture.sampleCount;

    int layer = -1;
    if (attach.layer == d3d::RENDER_TO_WHOLE_ARRAY && tex->type != D3DResourceType::TEX)
    {
      G_ASSERT(tex->type != D3DResourceType::CUBEARRTEX);
      layer = tex->type == D3DResourceType::CUBETEX ? 6 : tex->depth;
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
    if (rt.colors[0].texture == backbuffer)
    {
      if (!drawable_acquired)
      {
        TIME_PROFILE(acquireDrawable);
        [mainview acquireDrawable];

        drawable_acquired = true;
      }
      backbuffer->apiTex->rt_texture = is_rgb_backbuffer ? [mainview getsRGBBackBuffer] : [mainview getBackBuffer];
      backbuffer->apiTex->texture = backbuffer->apiTex->sub_texture = [mainview getsRGBBackBuffer];

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

#if HAVE_SAMPLE_QUERIES
    if (sampleBuffer)
    {
      MTLRenderPassSampleBufferAttachmentDescriptor *sampleAttachment = render_desc.sampleBufferAttachments[0];
      sampleAttachment.sampleBuffer = sampleBuffer;
      sampleAttachment.startOfVertexSampleIndex   = MTLCounterDontSample;
      sampleAttachment.endOfVertexSampleIndex     = MTLCounterDontSample;
      sampleAttachment.startOfFragmentSampleIndex = (nextSample++) % MAX_SAMPLE_SAMPLES;
      sampleAttachment.endOfFragmentSampleIndex   = MTLCounterDontSample;
    }
#endif

    command_encoder.write(CommandType::BeginPass).write(rt).write([render_desc retain]);
    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Render);

    // if there's gonna be pass break ensure its gonna be loaded back properly
    for (int i = 0; i < Program::MAX_SIMRT; i++)
      rt.colors[i].load_action = MTLLoadActionLoad;
    rt.depth.load_action = MTLLoadActionLoad;

    storages[STAGE_VS].reset();
    storages[STAGE_MS].reset();
    storages[STAGE_OS].reset();
    storages[STAGE_PS].reset();

    render.dirty_state |= DirtyFlags::DepthState;

    cur_state = NULL;

    if (depth_clip != MTLDepthClipModeClip)
      dirty_state |= DirtyFlags::DepthClip;
    if ((fabs(depth_bias) > 0.00000001f && fabs(depth_slopebias) > 0.00000001f))
      dirty_state |= DirtyFlags::DepthBias;
    if (cur_cull != MTLCullModeNone)
      dirty_state |= DirtyFlags::Cull;
    if (cur_fill != MTLTriangleFillModeFill)
      dirty_state |= DirtyFlags::Fill;
    if (cur_query_offset != -1)
      dirty_state |= DirtyFlags::Query;
    if (scissor_on)
      dirty_state |= DirtyFlags::Scissor;
    if (blend_factor[0] != 0 || blend_factor[1] != 0 || blend_factor[2] != 0 || blend_factor[3] != 0)
      dirty_state |= DirtyFlags::BlendFactor;

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
    depthStateDesc.depthWriteEnabled = cur_depthstate.zenable && cur_depthstate.depth_write_on;

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

  void Render::acquireOwnership()
  {
    ::enter_critical_section(acquireSec);

    if (acquire_depth == 0)
    {
      uint64_t thread = 0;
      pthread_threadid_np(NULL, &thread);
      cur_thread.store(thread);

      g_render_pool = [[NSAutoreleasePool alloc] init];
    }

    acquire_depth++;
  }

  void Render::releaseOwnership()
  {
    G_ASSERT(acquire_depth > 0);
    if (acquire_depth == 1)
    {
      cur_thread = main_thread;
      ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::None);

      cur_prog = nullptr;
      cur_state = nullptr;

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
      command_encoder.write(CommandType::DoQuery).write(query);
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
    backbuffer->apiTex->sub_texture = nil;
    backbuffer->destroy();

    if (d3d::get_driver_desc().caps.hasBindless)
    {
      bindlessTexture2DIdBuffer->destroy();
      bindlessTexture2DArrayIdBuffer->destroy();
      bindlessTextureCubeIdBuffer->destroy();
      bindlessBufferIdBuffer->destroy();
      bindlessSamplerIdBuffer->destroy();

      bindlessTexture2DIdBuffer = nullptr;
      bindlessTexture2DArrayIdBuffer = nullptr;
      bindlessTextureCubeIdBuffer = nullptr;
      bindlessBufferIdBuffer = nullptr;
      bindlessSamplerIdBuffer = nullptr;

      G_ASSERT(bindlessManager.empty());
    }

    clear_mesh_buffer->destroy();

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

    acquireOwnership();
    flush(true);
    releaseOwnership();

    shadersPreCache.release();

    [clear_cs_pipeline release];
    ::destroy_critical_section(acquireSec);
    ::destroy_critical_section(rcSec);
    [query_buffer release];
    [stub_buffer release];

    for (auto fence : encoders)
      [fence.fence release];
    encoders.clear();
    free_encoders.clear();
    current_enqueued_encoder = nullptr;
    current_enqueued_resources = 0;

#if DAGOR_DBGLEVEL > 0
    [signpost_event release];
    [signpost_listener release];
    dispatch_release(signpost_queue);
#endif

    for (int local_frame = 0; local_frame < MAX_FRAMES_TO_RENDER; ++local_frame)
    {
      cleanupFrame();
      ++frame;
    }

    std::lock_guard<std::mutex> scopedLock(delete_lock);
    G_ASSERT(resources2delete.empty());
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
    {
      blases.freeIndex(0);
      [defaultBlas release];
    }
    defaultBlas = nil;
    if (defaultTlas)
      [defaultTlas release];
    defaultTlas = nil;
#if HAVE_SAMPLE_QUERIES
    if (sampleBuffer)
        [sampleBuffer release];
#endif
    if (nativeBlases)
      [nativeBlases release];
    nativeBlases = nil;

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

  void Render::readbackTexture(Texture *tex_ptr, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int offset, int pitch, int imageSize)
  {
    checkRenderAcquired();
    G_ASSERT(tex_ptr);

    id<MTLTexture> tex = tex_ptr->apiTex->texture;

    if (tex_ptr->type != D3DResourceType::VOLTEX)
      d = 1;

    MTLSize size = {(uint32_t)w, (uint32_t)h, (uint32_t)d};
    MTLOrigin origin = {0, 0, 0};

    MTLBlitOption options = MTLBlitOptionNone;
    if (tex.pixelFormat == MTLPixelFormatDepth32Float_Stencil8 || tex.pixelFormat == MTLPixelFormatDepth32Float ||
        tex.pixelFormat == MTLPixelFormatDepth16Unorm)
      options = MTLBlitOptionDepthFromDepthStencil;

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
    command_encoder.write(CommandType::ReadbackTexture).write(tex_ptr).write(tex).write(layer).write(level).write(origin).write(size).write(buf).write(offset).write(pitch).write(imageSize).write(options);
  }

  void Render::uploadTexture(Texture *tex_ptr, id<MTLTexture> tex, int level, int layer, int x, int y, int z, int w, int h, int d,
    id<MTLBuffer> buf, int pitch, int imageSize)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    G_ASSERT(tex_ptr);
    track_resource_write_enqueued(*tex_ptr);
    textures2upload.push_back( {tex_ptr, tex, buf, (uint32_t)pitch, (uint32_t)imageSize, (uint16_t)x, (uint16_t)y, (uint16_t)z, (uint16_t)w, (uint16_t)h, (uint16_t)d,
        (uint8_t)level, (uint8_t)layer} );
  }

  void Render::queueResourceForDeletion(id<MTLResource> buf)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.push_back({ .type = DeletedResource::Type::NativeResource, .submit = submits_scheduled, .native_resource = buf });
  }

  void Render::queueHeapForDeletion(id<MTLHeap> heap)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.push_back({ .type = DeletedResource::Type::Heap, .submit = submits_scheduled, .heap = heap });
  }

  void Render::deleteBuffer(Buffer* buff)
  {
    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.push_back({ .type = DeletedResource::Type::Buffer, .submit = submits_scheduled, .buffer = buff });
  }

  void Render::deleteTexture(Texture* tex)
  {
    if (tex->isStub())
      tex->apiTex = nullptr; // hack?

    std::lock_guard<std::mutex> scopedLock(delete_lock);
    resources2delete.push_back({ .type = DeletedResource::Type::Texture, .submit = submits_scheduled, .texture = tex });
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

  void Render::queueCopyBuffer(Buffer *dst_buf, id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    G_ASSERT(dst_buf);
    track_resource_write_enqueued(*dst_buf);
    buffers2copy.push_back({ dst_buf, src, dst, src_offset, dst_offset, size });
  }

  void Render::queueUpdateBuffer(id<MTLBuffer> src, int src_offset, Buffer *dst_buf, int dst_offset, int size)
  {
    checkRenderAcquired();
    G_ASSERT(dst_buf);

    id<MTLBuffer> dst = dst_buf->getBuffer();
    G_ASSERT(dst);

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Blit);
    command_encoder.write(CommandType::CopyBuf).write(nullptr).write(src).write(dst_buf).write(dst).write(src_offset).write(dst_offset).write(size);
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
    if (state.dualSourceBlendEnabled)
    {
      rstate.raster_state.blend[0].ablend = state.dualSourceBlend.params.ablend;
      rstate.raster_state.blend[0].ablendScr = convBlendArgForAlpha(state.dualSourceBlend.params.sepablendFactors.src);
      rstate.raster_state.blend[0].ablendDst = convBlendArgForAlpha(state.dualSourceBlend.params.sepablendFactors.dst);
      rstate.raster_state.blend[0].ablendOp = convertBlendOp(state.dualSourceBlend.params.sepablendOp);
      rstate.raster_state.blend[0].sepblend = state.dualSourceBlend.params.sepablend;
      rstate.raster_state.blend[0].rgbblendScr = convBlendArg(state.dualSourceBlend.params.ablendFactors.src);
      rstate.raster_state.blend[0].rgbblendDst = convBlendArg(state.dualSourceBlend.params.ablendFactors.dst);
      rstate.raster_state.blend[0].rgbblendOp = convertBlendOp(state.dualSourceBlend.params.blendOp);
    }
    else
      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
      {
        rstate.raster_state.blend[i].ablend = state.blendParams[i].ablend;
        rstate.raster_state.blend[i].ablendScr = convBlendArgForAlpha(state.blendParams[i].sepablendFactors.src);
        rstate.raster_state.blend[i].ablendDst = convBlendArgForAlpha(state.blendParams[i].sepablendFactors.dst);
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
      dirty_state |= DirtyFlags::DepthClip;
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
    acquireOwnership();
    NSString *str = [NSString stringWithUTF8String:name];
    command_encoder.write(CommandType::BeginEvent).write([str retain]);
    releaseOwnership();
  }

  void Render::endEvent()
  {
    acquireOwnership();
    command_encoder.write(CommandType::EndEvent);
    releaseOwnership();
  }

  id<MTLBuffer> Render::createBuffer(uint32_t size, uint32_t flags, const char *name)
  {
    if (manual_hazard_tracking)
      flags |= MTLResourceHazardTrackingModeUntracked;
    id<MTLBuffer> buf = [device newBufferWithLength : size
                                            options : flags];
    if (buf == nil)
      DAG_FATAL("Failed to allocate buffer %s with size %d", name, size);
#if DAGOR_DBGLEVEL > 0
    buf.label = [NSString stringWithUTF8String : name];
#endif
    return buf;
  }

  RaytraceAccelerationStructure *Render::createAccelerationStructure(RaytraceBuildFlags flags, uint32_t size, bool blas)
  {
    RaytraceAccelerationStructure *as = new RaytraceAccelerationStructure { .createFlags = flags };
    G_ASSERT(size);
    as->acceleration_struct = [device newAccelerationStructureWithSize : size];
    if (blas)
    {
      blases.lock();
      as->index = blases.getFreeIndex(as);
      while (as->index >= nativeBlases.count)
        [nativeBlases addObject : defaultBlas];
      [nativeBlases replaceObjectAtIndex : as->index withObject : as->acceleration_struct];
#if DAGOR_DBGLEVEL > 0
      as->acceleration_struct.label = [NSString stringWithFormat:@"blas_%d %llu", as->index, render.frame];
#endif
      blases.unlock();
      G_ASSERT(as->index >= 0);
    }
    return as;
  }

  void Render::deleteAccelerationStructure(RaytraceAccelerationStructure *as)
  {
    if (as)
    {
      if (as->index >= 0)
      {
        blases.lock();
        if (nativeBlases.count > as->index)
          [nativeBlases replaceObjectAtIndex : as->index withObject : defaultBlas];
        blases.freeIndex(as->index);
        blases.unlock();
        as->index = -1;
      }
      else
      {
        uint64_t cur_thread = 0;
        pthread_threadid_np(NULL, &cur_thread);

        bool from_thread = render.acquire_depth == 0 || cur_thread != render.cur_thread;
        if (!from_thread)
        {
          for (uint32_t i = 0; i < STAGE_MAX; ++i)
          {
            for (uint32_t j = 0; j < MAX_SHADER_ACCELERATION_STRUCTURES; ++j)
            {
              if (stages[i].acceleration_structures[j] == as)
                stages[i].acceleration_structures[j] = nullptr;
            }
          }
        }
      }

      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.push_back({ .type = DeletedResource::Type::NativeResource, .submit = submits_scheduled, .native_resource = as->acceleration_struct });

      delete as;
    }
  }

  void Render::setTLAS(RaytraceTopAccelerationStructure *tlas, ShaderStage rstage, int slot)
  {
    checkRenderAcquired();

    G_ASSERT(rstage == STAGE_CS); // For now metal supports only CS
    G_ASSERT(slot >= 0);
    G_ASSERT(slot < MAX_SHADER_ACCELERATION_STRUCTURES);

    Render::StageBinding &stage = render.stages[rstage];
    stage.setAccStruct(slot, tlas);
  }

  void Render::copyAccelerationStruct(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src)
  {
    checkRenderAcquired();

    G_ASSERT(src->acceleration_struct);
    G_ASSERT(dst->acceleration_struct);

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Acceleration);
    command_encoder.write(CommandType::CopyAccelerationStruct).write(src).write(dst).write(src->acceleration_struct).write(dst->acceleration_struct);
  }

  void Render::buildAccelerationStructure(RaytraceAccelerationStructure *as, MTLAccelerationStructureDescriptor *desc, Sbuffer *space_buffer, uint32_t space_buffer_offset, bool refit)
  {
    checkRenderAcquired();

    G_ASSERT(space_buffer);

    MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor: desc];

    G_ASSERT(as->acceleration_struct);
    track_resource_write(*as);

    id<MTLAccelerationStructure> accStruct = as->acceleration_struct;
    G_ASSERT(accStruct.size >= sizes.accelerationStructureSize);

    Buffer *sbuffer = (Buffer *)space_buffer;
    G_ASSERT(sbuffer && !sbuffer->is_fast_discard());
    track_resource_read(*sbuffer);

    id<MTLBuffer> spaceBuf = sbuffer->getBuffer();

    if (refit && as->was_built)
    {
      G_ASSERT(as->was_built);

      G_ASSERT(desc.usage & MTLAccelerationStructureUsageRefit);

      G_ASSERT(sizes.refitScratchBufferSize + space_buffer_offset <= spaceBuf.length);

      [accelerationEncoder refitAccelerationStructure: accStruct
                                           descriptor: desc
                                          destination: accStruct
                                        scratchBuffer: spaceBuf
                                  scratchBufferOffset: space_buffer_offset];
    }
    else
    {
      G_ASSERT(sizes.buildScratchBufferSize + space_buffer_offset <= spaceBuf.length);

      [accelerationEncoder buildAccelerationStructure: accStruct
                                           descriptor: desc
                                        scratchBuffer: spaceBuf
                                  scratchBufferOffset: space_buffer_offset];
      as->was_built = true;
    }

    // Here struct compression can be done to reduce the size
    // but it is really performance heavy (requires a flush ([commandBuffer waitUntilCompleted]))
    // and doing it every frame is too much
    [desc release];
  }

  void Render::buildTLAS(RaytraceTopAccelerationStructure *as, const ::raytrace::TopAccelerationStructureBuildInfo &tasbi)
  {
    checkRenderAcquired();

    G_ASSERT(as);
    G_ASSERT(as->index == -1);

    MTLInstanceAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getTLASDescriptor(tasbi.instanceCount, tasbi.flags);

    G_ASSERT(tasbi.instanceBuffer);

    Buffer *ibuffer = (Buffer *)tasbi.instanceBuffer;
    G_ASSERT(!ibuffer->is_fast_discard());
    id<MTLBuffer> instanceBuf = ibuffer->getBuffer();
    G_ASSERT(instanceBuf.length >= d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize * tasbi.instanceCount);

    blases.lock();
    accDesc.instanceDescriptorBuffer = instanceBuf;
    accDesc.instancedAccelerationStructures = nativeBlases;
    blases.unlock();

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Acceleration);

    ASBuildInfo info {.scratch = (Buffer *)tasbi.scratchSpaceBuffer, .offset = tasbi.scratchSpaceBufferOffsetInBytes, .update = tasbi.doUpdate};

    uint8_t count = 1;
    command_encoder.write(CommandType::BuildAccelerationStructure).write(as).write([accDesc retain]).write(info).write(count).write(ibuffer);
  }

  void Render::buildBLAS(RaytraceBottomAccelerationStructure *blas, const ::raytrace::BottomAccelerationStructureBuildInfo &basbi)
  {
    checkRenderAcquired();

    G_ASSERT(blas);
    G_ASSERT(blas->index >= 0);

    eastl::vector<Buffer *, framemem_allocator> resources;
    MTLPrimitiveAccelerationStructureDescriptor *accDesc = acceleration_structure_descriptors::getBLASDescriptor(basbi.geometryDesc, basbi.geometryDescCount, basbi.flags, &resources);

    ensureHaveEncoderExceptRenderFrontend(Render::EncoderType::Acceleration);

    ASBuildInfo info {.scratch = (Buffer *)basbi.scratchSpaceBuffer, .offset = basbi.scratchSpaceBufferOffsetInBytes, .update = basbi.doUpdate};

    uint8_t count = uint8_t(resources.size());
    command_encoder.write(CommandType::BuildAccelerationStructure).write(blas).write([accDesc retain]).write(info).write(count).write(resources.data(), resources.size()*sizeof(Buffer *));
  }

  void Render::prepareBindlessResources(uint32_t requestedTypes)
  {
    if (!d3d::get_driver_desc().caps.hasBindless || requestedTypes == 0)
      return;

    if ((requestedTypes & bindless_resources_bound) == requestedTypes)
      return;

    TIME_PROFILE(prepareBindlessResources);

    if (bindless_set_dirty)
    {
      if (@available(iOS 18, macOS 15.0, *))
      {
        [residencySet commit];
      }
      bindless_set_dirty = false;
    }

    uint64_t tex_2d = 0;
    uint64_t tex_2dArray = 0;
    uint64_t tex_cube = 0;
    uint64_t buffer_stub = 0;
    if (@available(macOS 13.0, iOS 16.0, *))
    {
      G_ASSERT(blank_tex_2d);
      G_ASSERT(blank_tex_2dArray);
      G_ASSERT(blank_tex_cube);
      tex_2d = ToResourceID(blank_tex_2d);
      tex_2dArray = ToResourceID(blank_tex_2dArray);
      tex_cube = ToResourceID(blank_tex_cube);
      buffer_stub = stub_buffer.gpuAddress;
    }

    uint32_t mask = (requestedTypes ^ bindless_resources_bound) & requestedTypes;
    if (mask & (BindlessTypeTexture2D))
    {
      command_encoder.write(CommandType::TrackBindless).write(BindlessTypeTexture2D);
      if (bindlessTextures2D.id_cache.empty())
        bindlessTexture2DIdBuffer->updateDataWithLock(0, sizeof(uint64_t), &tex_2d, VBLOCK_DISCARD);
      else
        bindlessTexture2DIdBuffer->updateDataWithLock(0, sizeof(uint64_t)*bindlessTextures2D.id_cache.size(), bindlessTextures2D.id_cache.data(), VBLOCK_DISCARD);
    }
    if (mask & (BindlessTypeTextureCube))
    {
      command_encoder.write(CommandType::TrackBindless).write(BindlessTypeTextureCube);
      if (bindlessTexturesCube.id_cache.empty())
        bindlessTextureCubeIdBuffer->updateDataWithLock(0, sizeof(uint64_t), &tex_cube, VBLOCK_DISCARD);
      else
        bindlessTextureCubeIdBuffer->updateDataWithLock(0, sizeof(uint64_t)*bindlessTexturesCube.id_cache.size(), bindlessTexturesCube.id_cache.data(), VBLOCK_DISCARD);
    }
    if (mask & (BindlessTypeTexture2DArray))
    {
      command_encoder.write(CommandType::TrackBindless).write(BindlessTypeTexture2DArray);
      if (bindlessTextures2DArray.id_cache.empty())
        bindlessTexture2DArrayIdBuffer->updateDataWithLock(0, sizeof(uint64_t), &tex_2dArray, VBLOCK_DISCARD);
      else
        bindlessTexture2DArrayIdBuffer->updateDataWithLock(0, sizeof(uint64_t)*bindlessTextures2DArray.id_cache.size(), bindlessTextures2DArray.id_cache.data(), VBLOCK_DISCARD);
    }
    if (mask & BindlessTypeBuffer)
    {
      command_encoder.write(CommandType::TrackBindless).write(BindlessTypeBuffer);
      if (bindlessBuffers.cache.empty())
          bindlessBufferIdBuffer->updateDataWithLock(0, sizeof(uint64_t), &buffer_stub, VBLOCK_DISCARD);
      else
        bindlessBufferIdBuffer->updateDataWithLock(0, sizeof(uint64_t)*bindlessBuffers.id_cache.size(), bindlessBuffers.id_cache.data(), VBLOCK_DISCARD);
    }

    if (mask & BindlessTypeSampler)
    {
      if (bindlessSamplers.empty())
      {
        id<MTLSamplerState> sampler_state_dummy = sampler_states[render.getSampler({})].sampler;

        if (@available(macOS 13.0, iOS 16.0, *))
        {
          uint64_t sampler = ToResourceID(sampler_state_dummy);
          bindlessSamplerIdBuffer->updateDataWithLock(0, sizeof(uint64_t), &sampler, VBLOCK_DISCARD);
        }
      }
      else
        bindlessSamplerIdBuffer->updateDataWithLock(0, sizeof(uint64_t)*bindlessSamplersCache.size(), bindlessSamplersCache.data(), VBLOCK_DISCARD);
    }

    bindless_resources_bound |= requestedTypes;
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

  static MTLSamplerAddressMode getAddressMode(int mode)
  {
    switch (mode)
    {
      case TEXADDR_WRAP:     return MTLSamplerAddressModeRepeat;
      case TEXADDR_MIRROR:   return MTLSamplerAddressModeMirrorRepeat;
      case TEXADDR_CLAMP:    return MTLSamplerAddressModeClampToEdge;
      case TEXADDR_MIRRORONCE: return MTLSamplerAddressModeMirrorClampToEdge;
  #if !_TARGET_IOS
      case TEXADDR_BORDER:   return MTLSamplerAddressModeClampToBorderColor;
  #else
      case TEXADDR_BORDER:   return MTLSamplerAddressModeClampToZero;
  #endif
    }

    return MTLSamplerAddressModeRepeat;
  }

  int Render::getSampler(SamplerState sampler_state)
  {
    sampler_state.sampler = NULL;

    std::unique_lock lock(samplerStatesMutex);

    for (int i = 0; i < sampler_states.size(); i++)
    {
      if (sampler_states[i] == sampler_state)
      {
        sampler_state.sampler = sampler_states[i].sampler;
        return i;
      }
    }

    MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    samplerDescriptor.sAddressMode = getAddressMode(sampler_state.addrU);
    samplerDescriptor.tAddressMode = getAddressMode(sampler_state.addrV);
    samplerDescriptor.rAddressMode = getAddressMode(sampler_state.addrW);

    if (sampler_state.texFilter == d3d::FilterMode::Point)
    {
      samplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
      samplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
    }
    else
    {
      samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
      samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    }

    if (sampler_state.mipmap == d3d::MipMapMode::Point)
    {
      samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;
    }
    else
    {
      samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
    }

    samplerDescriptor.lodMinClamp = 0;
    samplerDescriptor.lodMaxClamp = sampler_state.max_level;

    samplerDescriptor.maxAnisotropy = min(16, sampler_state.anisotropy);

#if !_TARGET_IOS
    if (sampler_state.border_color.r > 0 ||
        sampler_state.border_color.g > 0 ||
        sampler_state.border_color.b > 0)
    {
      samplerDescriptor.borderColor = MTLSamplerBorderColorOpaqueWhite;
    }
    else
    {
      samplerDescriptor.borderColor = sampler_state.border_color.a > 0 ? MTLSamplerBorderColorOpaqueBlack : MTLSamplerBorderColorTransparentBlack;
    }
#endif

    samplerDescriptor.compareFunction = MTLCompareFunctionLessEqual;
    samplerDescriptor.supportArgumentBuffers = d3d::get_driver_desc().caps.hasBindless;

    sampler_state.sampler = [render.device newSamplerStateWithDescriptor : samplerDescriptor];
    [samplerDescriptor release];

    append_items(sampler_states, 1, &sampler_state);
    return sampler_states.size() - 1;
  }
}
