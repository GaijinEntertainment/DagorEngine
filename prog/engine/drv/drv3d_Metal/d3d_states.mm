// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_streamOutput.h>
#include <drv/3d/dag_heap.h>

#include "render.h"
#include "drv_assert_defs.h"
#include <drv/3d/dag_res.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_metal;

static int sizeofAccelerationStruct()
{
  if (@available(iOS 15.0, macos 12.0, *))
  {
    return sizeof(MTLAccelerationStructureUserIDInstanceDescriptor);
  }
  return 0;
}

namespace drv3d_metal
{
  bool sRGB_BackBuffer_on = false;
  int  d3d_prev_func = CMPF_GREATEREQUAL;

  bool metal_use_queries = true;

  bool desc_prepared = false;

  Driver3dDesc g_device_desc =
  {
    0, 0,   //int zcmpfunc,acmpfunc;
    0, 0,   //int sblend,dblend;

    1, 1,     //int mintexw,mintexh;
    16384, 16384, //int maxtexw,maxtexh;
    1, 16384, //int mincubesize,maxcubesize;
    1, 16384, //int minvolsize,maxvolsize;
    0,    //int maxtexaspect; ///< 1 means texture should be square, 0 means no limit
    65536,  //int maxtexcoord;
    16,     //int maxsimtex;
    16,    //int maxlights;
    0,    //int maxclipplanes;
    64,     //int maxstreams;
    64,     //int maxstreamstr;
    1024,   //int maxvpconsts;


    65536 * 4, 65536 * 4, //int maxprims,maxvertind;
    0.0f, 0.0f, //float upixofs,vpixofs;
    Program::MAX_SIMRT,   //int maxSimRT;
    true, //bool is20ArbitrarySwizzleAvailable;
    32,//minWarpSiz
  };

  VSDTYPE debug_vdecl[] =
  {
    VSD_STREAM(0),
    VSD_REG(VSDR_POS, VSDT_FLOAT3),
    VSD_REG(VSDR_DIFF, VSDT_E3DCOLOR),
    VSD_END
  };

  // METAL
  const char debug_vs_metal[] = ""
  "#include <metal_stdlib>\n"
  "#include <simd/simd.h>\n"
  "// debug_vs_metal\n"
  "using namespace metal;\n"
  "typedef struct {\n"
  "matrix_float4x4 gtm;\n"
  "} cnst_t;\n"
  "typedef struct {\n"
  "float3 position [[ attribute(0) ]];\n"
  "float4 color [[ attribute(1) ]];\n"
  "} Triangle;\n"
  "typedef struct {\n"
  "float4 position [[position]];\n"
  "float4 color;\n"
  "} TriangleOutput;\n"
  "vertex TriangleOutput VertexColor(Triangle Vertices [[stage_in]],\n"
  "                  constant cnst_t& cnst [[ buffer("
  BIND_POINT_STR
  ")]])\n"
  "{\n"
  "  TriangleOutput out;\n"
  "  float4 pos = float4(Vertices.position, 1.0);\n"
  "  out.position = pos * cnst.gtm;\n"
  "  out.color = Vertices.color;\n"
  "  return out;\n"
  "}";

  const char debug_ps_metal[] = ""
  "#include <metal_stdlib>\n"
  "#include <metal_texture>\n"
  "using namespace metal;\n"
  "typedef struct {\n"
  "float4 position [[position]];\n"
  "float4 color;\n"
  "} TriangleOutput;\n"
  "fragment half4 texturedQuadFragment(TriangleOutput   inFrag  [[ stage_in ]])\n"
  "{\n"
  "  half4 color = (half4)inFrag.color;\n"
  "  return color;\n"
  "};";

  VDECL   d3d_debug_vdecl = BAD_VDECL;
  PROGRAM d3d_debug_prog = BAD_PROGRAM;
}

extern void mac_get_vdevice_info(int& vram, GpuVendor& vendor, String &gpu_desc, bool &web_driver);

const char *d3d::get_driver_name()
{
  return "Metal";
}

#if _TARGET_PC_MACOSX
DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::metal); }
#endif

const char *d3d::get_device_driver_version()
{
  return "1.0";
}

const char *d3d::get_device_name()
{
  return render.device_name;
}

const char *d3d::get_last_error()
{
  return render.isInited() ? "unspecified error" : "Metal driver not inited";
}

uint32_t d3d::get_last_error_code()
{
  return 0;
}

bool supports_any_raytracing()
{
  return render.device.supportsRaytracing;
}

bool supports_hw_raytracing()
{
  return supports_any_raytracing() && [render.device supportsFamily: MTLGPUFamilyApple9];
}

const Driver3dDesc &d3d::get_driver_desc()
{
  D3D_CONTRACT_ASSERTF(render.device, "d3d::get_driver_desc: render.device is nil. The function must be called after video init");
  if (!desc_prepared)
  {
#if _TARGET_PC_MACOSX
    GpuVendor vendor;
    int vram;
    String gpu_desc;
    bool web_driver;

    mac_get_vdevice_info(vram, vendor, gpu_desc, web_driver);

    metal_use_queries = (vendor != GpuVendor::NVIDIA);

    g_device_desc.caps.hasBindless = false;
    const bool isBindlessAllowed = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("allowBindless", false);
    if (@available(macOS 15.0, iOS 18.0, *))
    {
      if (isBindlessAllowed)
        g_device_desc.caps.hasBindless = [render.device supportsFamily:MTLGPUFamilyMetal3];
    }

    g_device_desc.caps.hasUAVOnlyForcedSampleCount = true;

    g_device_desc.shaderModel = 5.0_sm;
#else //_TARGET_IOS | _TARGET_TVOS
    metal_use_queries = false;

    g_device_desc.maxSimRT = 4;

    g_device_desc.caps.hasIndirectSupport = [render.device supportsFamily:MTLGPUFamilyApple3];
    g_device_desc.caps.hasCompareSampler = [render.device supportsFamily:MTLGPUFamilyApple3];
#endif
    g_device_desc.caps.hasRenderPassDepthResolve = [render.device supportsFamily:MTLGPUFamilyApple3];
    g_device_desc.caps.hasBaseVertexSupport = [render.device supportsFamily:MTLGPUFamilyApple3];
    if (@available(iOS 16, macos 13, *))
    {
      g_device_desc.caps.hasMeshShader = [render.device supportsFamily:MTLGPUFamilyApple9];
    }
    else
      g_device_desc.caps.hasMeshShader = false;
    if (g_device_desc.caps.hasRenderPassDepthResolve)
    {
      g_device_desc.depthResolveModes =
        DepthResolveMode::DEPTH_RESOLVE_MODE_SAMPLE_ZERO |
        DepthResolveMode::DEPTH_RESOLVE_MODE_MIN |
        DepthResolveMode::DEPTH_RESOLVE_MODE_MAX;
    }

    G_ASSERT(render.clear_cs_pipeline.threadExecutionWidth > 0);
    g_device_desc.minWarpSize = render.clear_cs_pipeline.threadExecutionWidth;
    g_device_desc.maxWarpSize = render.clear_cs_pipeline.threadExecutionWidth;
    g_device_desc.caps.hasRayAccelerationStructure = false;
    g_device_desc.caps.hasRayQuery = false;
    g_device_desc.caps.hasGeometryIndexInRayAccelerationStructure = false;
    g_device_desc.caps.hasSkipPrimitiveTypeInRayTracingShaders = false;
    g_device_desc.caps.hasNativeRayTracePipelineExpansion = false;
    g_device_desc.raytrace.topAccelerationStructureInstanceElementSize = sizeofAccelerationStruct();

#if D3D_HAS_RAY_TRACING
  const bool isRTAllowed = (DAGOR_DBGLEVEL > 0) ? supports_any_raytracing() : supports_hw_raytracing();
  if (@available(iOS 17, macOS 12.0, *))
  {
    if (isRTAllowed)
    {
      g_device_desc.caps.hasRayAccelerationStructure = true;
      g_device_desc.caps.hasRayQuery = true;
      g_device_desc.caps.hasEmulatedRaytracing = supports_any_raytracing() && !supports_hw_raytracing();
      debug("d3d::get_driver_desc: Raytracing is emulated: %d", g_device_desc.caps.hasEmulatedRaytracing);
      g_device_desc.raytrace.accelerationStructureBuildScratchBufferOffsetAlignment = 32;
      // TODO: may support?
      // hasGeometryIndexInRayAccelerationStructure
      // hasSkipPrimitiveTypeInRayTracingShaders
    }
  }
#endif

    g_device_desc.caps.hasResourceHeaps = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("allowResourceHeaps", true);

    desc_prepared = true;
  }

  return g_device_desc;
}

int d3d::driver_command(Drv3dCommand command, void *par1, void *par2, void *par3)
{
  switch (command)
  {
    case Drv3dCommand::D3D_FLUSH:
    case Drv3dCommand::GPU_BARRIER_WAIT_ALL_COMMANDS: // TODO: Implement GPU_BARRIER_WAIT_ALL_COMMANDS separately
      @autoreleasepool
      {
        render.acquireOwnership();
        render.flush(true);
        render.releaseOwnership();
      }
      break;
    case Drv3dCommand::CONVERT_TLAS_INSTANCES:
    {
      struct DECLSPEC_ALIGN(16) Dx12HWInstance
      {
        mat43f transform;
        unsigned instanceID : 24;
        unsigned instanceMask : 8;
        unsigned instanceContributionToHitGroupIndex : 24;
        unsigned flags : 8;
        uint64_t blasGpuAddress;
      } ATTRIBUTE_ALIGN(16);

      if (@available(iOS 15, macOS 12.0, *))
      {
        Dx12HWInstance *src = (Dx12HWInstance *)par2;
        int instance_count = intptr_t(par3);
        MTLAccelerationStructureUserIDInstanceDescriptor *dst = (MTLAccelerationStructureUserIDInstanceDescriptor *)par1;
        for (int i = 0; i < instance_count; ++i)
        {
          const Dx12HWInstance &inst = *src++;
          MTLAccelerationStructureUserIDInstanceDescriptor &ret = *dst++;

          G_ASSERTF(inst.blasGpuAddress < render.nativeBlases.count, "Blas index is out of bounds, %llu is bigger than %d", inst.blasGpuAddress, int(render.nativeBlases.count));
          ret.accelerationStructureIndex = inst.blasGpuAddress;
          ret.options = 0;
          ret.intersectionFunctionTableOffset = 0; // instance.instanceContributionToHitGroupIndex;
          ret.mask = inst.instanceMask;
          ret.userID = inst.instanceID;

          const float *matrix = (float *)&inst.transform;
          for (int row = 0; row < 3; row++)
          {
            for (int column = 0; column < 4; column++)
            {
              ret.transformationMatrix.columns[column][row] = matrix[4*row + column];
            }
          }

          if (inst.flags & RaytraceGeometryInstanceDescription::Flags::TRIANGLE_CULL_DISABLE)
            ret.options |= MTLAccelerationStructureInstanceOptionDisableTriangleCulling;
          if (inst.flags & RaytraceGeometryInstanceDescription::Flags::TRIANGLE_CULL_FLIP_WINDING)
            ret.options |= MTLAccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
          if (inst.flags & RaytraceGeometryInstanceDescription::Flags::FORCE_OPAQUE)
            ret.options |= MTLAccelerationStructureInstanceOptionOpaque;
          if (inst.flags & RaytraceGeometryInstanceDescription::Flags::FORCE_NO_OPAQUE)
            ret.options |= MTLAccelerationStructureInstanceOptionNonOpaque;
        }
      }
      return 1;
    }
    case Drv3dCommand::LOGERR_ON_SYNC:
      render.report_stalls = par1 != nullptr;
      break;
    case Drv3dCommand::PROCESS_PENDING_RESOURCE_UPDATED:
    case Drv3dCommand::FLUSH_STATES:
      @autoreleasepool
      {
        render.acquireOwnership();
        if (par3)
          render.capture_command_gpu_time = true;
        render.flush(false);
        render.capture_command_gpu_time = false;
        render.releaseOwnership();
      }
      break;
    case Drv3dCommand::PROCESS_APP_INACTIVE_UPDATE:
      @autoreleasepool
      {
        render.acquireOwnership();
        render.endFrame();
        render.releaseOwnership();
      }
      break;
    case Drv3dCommand::GETVISIBILITYBEGIN:
    {
      if (!metal_use_queries)
      {
        return 0;
      }

      Render::Query** q = (Render::Query**)par1;

      if (!q[0])
      {
        q[0] = render.createQuery();
      }

      render.startQuery(q[0]);
      return 0;
    }
    case Drv3dCommand::GETVISIBILITYEND:
    {
      if (!metal_use_queries)
      {
        return 0;
      }

      Render::Query* q = (Render::Query*)par1;

      if (!q)
      {
        return 0;
      }

      render.finishQuery(q);
      return 0;
    }
    case Drv3dCommand::GETVISIBILITYCOUNT:
    {
      if (!metal_use_queries)
      {
        return 0;
      }

      Render::Query* q = (Render::Query*)par1;

      if (!q)
      {
        return 0;
      }

      return (int)render.getQueryResult(q, par2 ? 0 : 1);
    }
    case Drv3dCommand::RELEASE_QUERY:
    {
      if (!metal_use_queries)
      {
        return 0;
      }

      Render::Query** q = (Render::Query**)par1;

      if (!q[0])
      {
        return 0;
      }

      render.releaseQuery(q[0]);
      q[0] = 0;
      return 0;
    }
    case Drv3dCommand::ACQUIRE_OWNERSHIP:
      render.acquireOwnership();
      break;
    case Drv3dCommand::RELEASE_OWNERSHIP:
      if (par3)
        return render.tryReleaseOwnerShip();
      else
        render.releaseOwnership();
      break;
    case Drv3dCommand::TIMESTAMPFREQ:
      // this has to be comparable with cpu ticks frequency otherwise its gonna be way off and no events
      *reinterpret_cast<uint64_t *>(par1) = 1000000000;
      return 1;
    case Drv3dCommand::TIMESTAMPISSUE:
    {
      G_ASSERTF(par1, "metal: par1 for Drv3dCommand::TIMESTAMPISSUE must be not null");
      uint64_t &query = *(reinterpret_cast<uint64_t *>(par1));
      // we're using floor, so render.nextSample effectively means last scheduled sample + 1
      query = render.nextSample;
      break;
    }
    case Drv3dCommand::TIMESTAMPGET:
      {
        // convert to ms from us
        if (par3)
        {
          *reinterpret_cast<uint64_t *>(par3) = render.current_command_gpu_time.load();
          return 1;
        }

        if (!par1)
          return 0;

        uint64_t gpuTimeStamp = render.getTimestampResult(reinterpret_cast<uint64_t>(par1));
        if (!gpuTimeStamp)
          return 0;

        *reinterpret_cast<uint64_t *>(par2) = gpuTimeStamp;
        return 1;
      }
    case Drv3dCommand::TIMECLOCKCALIBRATION:
    {
      MTLTimestamp cpuTimestamp, gpuTimestamp;
      [render.device sampleTimestamps : &cpuTimestamp
                         gpuTimestamp : &gpuTimestamp];
      if (par1)
        *reinterpret_cast<uint64_t *>(par1) = cpuTimestamp;
      if (par2)
        *reinterpret_cast<uint64_t *>(par2) = gpuTimestamp;
      if (par3)
        *reinterpret_cast<int *>(par3) = DRV3D_CPU_FREQ_NSEC;
      return 1;
    }
    case Drv3dCommand::GET_VIDEO_MEMORY_BUDGET:
    {
      int gpuMaxAvailableKb = 0;
      #if _TARGET_PC_MACOSX
        GpuVendor vendor;
        String gpu_desc;
        bool web_driver;

        mac_get_vdevice_info(gpuMaxAvailableKb, vendor, gpu_desc, web_driver);
      #else
        gpuMaxAvailableKb = d3d::get_dedicated_gpu_memory_size_kb();
      #endif

      if (par1)
        *reinterpret_cast<uint32_t *>(par1) = gpuMaxAvailableKb;

      if (par2 || par3)
      {
        const int usedKB = min(int(render.device.currentAllocatedSize >> 10), gpuMaxAvailableKb);
        if (par2)
          *reinterpret_cast<uint32_t *>(par2) = gpuMaxAvailableKb - usedKB; // free to use
        if (par3)
          *reinterpret_cast<uint32_t *>(par3) = usedKB;
      }
      return gpuMaxAvailableKb;
    }
    case Drv3dCommand::GET_VENDOR:
#if _TARGET_TVOS
    {
      if (par1)
        *(const void**)par1 = render.device_name;
      render.getSupportedMTLVersion(par2);
      return GpuVendor::APPLE;
    }
#endif
      //return drv3d_vulkan::api_state.device.getDeviceVendor();
      break;
    case Drv3dCommand::START_CAPTURE_FRAME:
    {
      if (par1 == nullptr)
        d3d::driver_command(Drv3dCommand::FLUSH_STATES);
      {
        MTLCaptureManager *man = [MTLCaptureManager sharedCaptureManager];
        if (par1 != nullptr)
        {
          NSError *error = nil;
          MTLCaptureDescriptor *desc = [[MTLCaptureDescriptor alloc] init];
          desc.captureObject = render.device;
          [man startCaptureWithDescriptor : desc
                                    error : &error];
          if (error)
            logerr("Failed to start capture, error %s",
                  [[error localizedDescription] UTF8String]);
          [desc release];

          d3d::driver_command(Drv3dCommand::FLUSH_STATES);
        }
        else
          [man stopCapture];
      }
      break;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN:
    {
      render.pushAsyncPsoCompilation(par1 == nullptr);
      return 1;
    }
    case Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END:
    {
      render.popAsyncPsoCompilation();
      return 1;
    }
    case Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH:
    {
      if (par1)
        *static_cast<uint32_t*>(par1) = 1;
      return render.async_pso_compilation_length;
    }
    case Drv3dCommand::LOAD_PIPELINE_CACHE:
    {
      render.shadersPreCache.loadPreCache();
      break;
    }
    case Drv3dCommand::IS_HDR_ENABLED:
    {
      return (int)render.mainview.hdrEnabled;
    }
    case Drv3dCommand::INT10_HDR_BUFFER:
      return (int)render.mainview.int10HDRBuffer;
    case Drv3dCommand::HDR_OUTPUT_MODE:
    {
      if (render.mainview.hdrEnabled)
        return (int)(render.mainview.int10HDRBuffer ? HdrOutputMode::HDR10_ONLY : HdrOutputMode::HDR_ONLY);
      return (int)HdrOutputMode::SDR_ONLY;
    }
    case Drv3dCommand::IS_HDR_AVAILABLE:
    {
      return [render.mainview isHDRAvailable];
    }
    case Drv3dCommand::SET_HDR:
    {
      bool enable = *static_cast<bool*>(par1);
      [render.mainview setHDR: enable];
      break;
    }
    case Drv3dCommand::GET_METALFX_UPSCALE_STATE:
    {
#if USE_METALFX_UPSCALE
      return render.upscale.supported ? (int)MtlfxUpscaleState::READY : (int)MtlfxUpscaleState::UNSUPPORTED;
#else
      return (int)MtlfxUpscaleState::UNSUPPORTED;
#endif
  }
    case Drv3dCommand::EXECUTE_METALFX_UPSCALE:
    {
      MtlFxUpscaleParams *params = static_cast<MtlFxUpscaleParams*>(par1);
      render.executeUpscale((drv3d_metal::Texture*)params->color, (drv3d_metal::Texture*)params->output, (uint32_t)params->colorMode);
      break;
    }
    default:
	  break;
  }

  return 0;
}

bool d3d::setgamma(float p)
{
  if (render.mainview.hdrEnabled)
    return false;
  dispatch_async(dispatch_get_main_queue(), ^{
    [render.mainview setGamma:p];
  });
  return true;
}

bool d3d::device_lost(bool *can_reset_now)
{
  return false;
}
bool d3d::is_in_device_reset_now() { return false; }

bool d3d::is_window_occluded() { return false; }

bool d3d::should_use_compute_for_image_processing(std::initializer_list<unsigned>) { return false; }

bool d3d::reset_device()
{
  return true;
}

bool d3d::setstencil(uint32_t ref)
{
  return render.setStencilRef(ref);
}

// Miscellaneous

//returns previous result. switch on/off srgb write to backbuffer (default is off)
bool d3d::set_srgb_backbuffer_write(bool set)
{
  return render.setSrgbBackbuffer(set);
}

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState & state)
{
  return render.createRenderState(state);
}

bool d3d::set_render_state(shaders::DriverRenderStateId id)
{
  return render.setRenderState(id);
}

bool d3d::set_blend_factor(E3DCOLOR color)
{
    return render.setBlendFactor(color);
}

void d3d::clear_render_states()
{
  render.clearRenderStates();
}

PROGRAM d3d::get_debug_program()
{
  if (d3d_debug_prog == BAD_PROGRAM)
  {
    d3d_debug_vdecl = render.createVDdecl(debug_vdecl);
    d3d_debug_prog = create_program((const uint32_t*)debug_vs_metal, (const uint32_t*)debug_ps_metal, d3d_debug_vdecl, 0, 0);
  }

  return d3d_debug_prog;
}

d3d::EventQuery* d3d::create_event_query()
{
  return (d3d::EventQuery*)render.createQuery();
}

void d3d::release_event_query(d3d::EventQuery *q)
{
  if (!q)
  {
    return;
  }

  render.releaseQuery((Render::Query*)q);
}

bool d3d::issue_event_query(d3d::EventQuery *q)
{
  if (!q)
  {
    return false;
  }

  render.startFence((Render::Query*)q);

  return true;
}

bool d3d::setwire(bool in)
{
  render.setFill(in ? MTLTriangleFillModeLines : MTLTriangleFillModeFill);
  return true;
}

bool d3d::get_event_query_status(d3d::EventQuery *q, bool force_flush)
{
  if (!q)
  {
    return true;
  }

  return render.getQueryResult((Render::Query*)q, force_flush) != -1;
}

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps,
  int max_vdecl, int max_vb, int max_ib, int max_stblk)
{
  (void)strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk;
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps,
  int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = 0;//api_state.textures.getSet().capacity();
  max_vb = 0;//api_state.vertexBuffers.getSet().capacity();
  max_ib = 0;//api_state.indexBuffers.getSet().capacity();
  max_ps = max_vs = 0;// api_state.shaderProgramDatabase.getShadersTap().capacity();
  max_vdecl = 0;// api_state.shaderProgramDatabase.getLayoutTab().capacity();
  max_stblk = 0; // ???
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps,
  int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = 0;// api_state.textures.getSet().size();
  max_vb = 0;//api_state.vertexBuffers.getSet().size();
  max_ib = 0;//api_state.indexBuffers.getSet().size();
  max_ps = max_vs = 0;// api_state.shaderProgramDatabase.getShadersTap().capacity();
  max_vdecl = 0;// api_state.shaderProgramDatabase.getLayoutTab().capacity();
  max_stblk = 0; // ???
}

void d3d::set_variable_rate_shading(unsigned, unsigned, VariableRateShadingCombiner,
                                    VariableRateShadingCombiner)
{
}
void d3d::set_variable_rate_shading_texture(BaseTexture *)
{
}

void d3d::set_stream_output_buffer(int, const StreamOutputBufferSetup &)
{
}

void d3d::resource_barrier(const ResourceBarrierDesc &, GpuPipeline)
{
}

namespace d3d
{

static bool is_depth_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg >= TEXFMT_FIRST_DEPTH && cflg <= TEXFMT_LAST_DEPTH;
}

static MTLTextureDescriptor *createTextureDescriptor(const ResourceDescription &desc, bool& is_rt, bool& is_depth_stencil)
{
  MTLTextureDescriptor *pTexDesc = [[MTLTextureDescriptor alloc] init];
  switch (desc.type)
  {
    case D3DResourceType::TEX:
      pTexDesc.textureType = MTLTextureType2D;
      pTexDesc.width = desc.asTexRes.width;
      pTexDesc.height = desc.asTexRes.height;
      pTexDesc.depth = 1;
      pTexDesc.mipmapLevelCount = desc.asTexRes.mipLevels;
      pTexDesc.arrayLength = 1;
      break;
    case D3DResourceType::CUBETEX:
      pTexDesc.textureType = MTLTextureTypeCube;
      pTexDesc.width = desc.asCubeTexRes.extent;
      pTexDesc.height = desc.asCubeTexRes.extent;
      pTexDesc.depth = 1;
      pTexDesc.mipmapLevelCount = desc.asCubeTexRes.mipLevels;
      pTexDesc.arrayLength = 1;
      break;
    case D3DResourceType::VOLTEX:
      pTexDesc.textureType = MTLTextureType3D;
      pTexDesc.width = desc.asVolTexRes.depth;
      pTexDesc.height = desc.asVolTexRes.depth;
      pTexDesc.depth = desc.asVolTexRes.depth;
      pTexDesc.mipmapLevelCount = desc.asVolTexRes.mipLevels;
      pTexDesc.arrayLength = 1;
      break;
    case D3DResourceType::ARRTEX:
      pTexDesc.textureType = MTLTextureType2DArray;
      pTexDesc.width = desc.asArrayTexRes.width;
      pTexDesc.height = desc.asArrayTexRes.height;
      pTexDesc.depth = 1;
      pTexDesc.mipmapLevelCount = desc.asArrayTexRes.mipLevels;
      pTexDesc.arrayLength = desc.asArrayTexRes.arrayLayers;
      break;
    case D3DResourceType::CUBEARRTEX:
      pTexDesc.textureType = MTLTextureTypeCubeArray;
      pTexDesc.width = desc.asArrayCubeTexRes.extent;
      pTexDesc.height = desc.asArrayCubeTexRes.extent;
      pTexDesc.depth = 1;
      pTexDesc.mipmapLevelCount = desc.asArrayCubeTexRes.mipLevels;
      pTexDesc.arrayLength = desc.asArrayCubeTexRes.cubes;
      break;
    default: G_ASSERTF(0, "vulkan: non image resource desc is used to generate image create info"); break;
  }
  uint32_t cflag = desc.asBasicRes.cFlags;
  G_ASSERT(get_sample_count(cflag) == 1);

  pTexDesc.storageMode = MTLStorageModePrivate;
  pTexDesc.pixelFormat = drv3d_metal::Texture::format2Metal(cflag & TEXFMT_MASK);

  if (pTexDesc.pixelFormat == MTLPixelFormatDepth32Float ||
      pTexDesc.pixelFormat == MTLPixelFormatDepth32Float_Stencil8)
    pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
  else if (cflag & TEXCF_UNORDERED)
    pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
  else if (cflag & TEXCF_RTARGET)
    pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;

  is_rt = cflag & TEXCF_RTARGET;
  is_depth_stencil = is_rt && is_depth_format_flg(cflag);

  return pTexDesc;
}

static ResourceHeapGroup *g_heap_group_tex = (ResourceHeapGroup *)1;
static ResourceHeapGroup *g_heap_group_rt = (ResourceHeapGroup *)2;
static ResourceHeapGroup *g_heap_group_ds = (ResourceHeapGroup *)3;
static ResourceHeapGroup *g_heap_group_buf = (ResourceHeapGroup *)4;

ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc)
{
  ResourceAllocationProperties ret {};

  MTLSizeAndAlign size_and_align;
  switch (desc.type)
  {
    case D3DResourceType::TEX:
    case D3DResourceType::CUBETEX:
    case D3DResourceType::VOLTEX:
    case D3DResourceType::ARRTEX:
    case D3DResourceType::CUBEARRTEX:
    {
      bool is_depth_stencil = false, is_rt = false;
      MTLTextureDescriptor *tex_desc = createTextureDescriptor(desc, is_rt, is_depth_stencil);
      size_and_align = [render.device heapTextureSizeAndAlignWithDescriptor : tex_desc];
      [tex_desc release];

      ret.heapGroup = is_depth_stencil ? g_heap_group_ds : (is_rt ? g_heap_group_rt : g_heap_group_tex);
    }
    break;
    case D3DResourceType::SBUF:
    {
      size_and_align = [render.device heapBufferSizeAndAlignWithLength : desc.asBufferRes.elementSizeInBytes * desc.asBufferRes.elementCount
                                                               options : MTLResourceStorageModePrivate];
      ret.heapGroup = g_heap_group_buf;
    }
    break;
    default:
      // unreachable code
      ret = {};
      G_ASSERTF(0, "metal: unhandled/unknown resource type (%u)", eastl::to_underlying(desc.type));
      break;
  }
  ret.sizeInBytes = size_and_align.size;
  ret.offsetAlignment = size_and_align.align;
  return ret;
}

ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  ResourceHeap *ret = new ResourceHeap;

  MTLHeapDescriptor *desc = [[MTLHeapDescriptor alloc] init];
  desc.type = MTLHeapTypePlacement;
  desc.storageMode = MTLStorageModePrivate;
  desc.size = size;
  ret->heap = [render.device newHeapWithDescriptor : desc];
  G_ASSERT(ret->heap);
  [desc release];

  ret->group = heap_group;
  ret->size = size;

  return ret;
}

void destroy_resource_heap(ResourceHeap *heap)
{
  if (heap == nullptr)
    return;

  render.queueHeapForDeletion(heap->heap);

  delete heap;
}

Sbuffer *place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
                                            const ResourceAllocationProperties &alloc_info, const char *name)
{
  if (heap == nullptr)
    return nullptr;

  G_UNUSED(alloc_info);
  G_ASSERT(alloc_info.heapGroup == heap->group);
  G_ASSERT(alloc_info.sizeInBytes + offset <= heap->size);
  G_ASSERT(heap->heap);

  id<MTLBuffer> buf = [heap->heap newBufferWithLength : desc.asBufferRes.elementSizeInBytes * desc.asBufferRes.elementCount
                                                 options : MTLResourceStorageModePrivate
                                                  offset : offset];
  if (buf == nullptr)
    return nullptr;

  drv3d_metal::Buffer *buffer = new drv3d_metal::Buffer(buf, desc.asBasicRes.cFlags, name);
  buffer->heap_offset = uint32_t(offset);
  buffer->heap_size = alloc_info.sizeInBytes;
  buffer->heap = heap;
  return buffer;
}

BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
                                                 const ResourceAllocationProperties &alloc_info, const char *name)
{
  if (heap == nullptr)
    return nullptr;

  G_UNUSED(alloc_info);
  G_ASSERT(alloc_info.heapGroup == heap->group);
  G_ASSERT(alloc_info.sizeInBytes + offset <= heap->size);
  G_ASSERT(heap->heap);

  bool is_depth_stencil = false, is_rt = false;
  MTLTextureDescriptor *tex_desc = createTextureDescriptor(desc, is_rt, is_depth_stencil);

  id<MTLTexture> tex = [heap->heap newTextureWithDescriptor : tex_desc
                                                     offset : offset];
  if (tex == nullptr)
    return nullptr;

  drv3d_metal::Texture *texture = new drv3d_metal::Texture(tex, tex_desc, desc.asBasicRes.cFlags, name);
  texture->heap_offset = uint32_t(offset);
  texture->heap_size = alloc_info.sizeInBytes;
  texture->heap = heap;
  return texture;
}

ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  if (heap_group == nullptr)
    return {};
  constexpr uint32_t max_size = 256 << 20;
  ResourceHeapGroupProperties ret;
  ret.isCPUVisible = false;
  ret.isGPULocal = true;
  ret.isDedicatedFastGPULocal = false;
  ret.maxResourceSize = max_size;
  ret.maxHeapSize = max_size;
  ret.optimalMaxHeapSize = max_size;
  return ret;
}
}

void ResourceHeap::track_resource_read(drv3d_metal::HazardTracker &resource)
{
  resources.erase(eastl::remove_if(resources.begin(), resources.end(), [this](const Resource &a) { return a.enc.submit <= render.submits_completed; }), resources.end());

  for (auto &res : resources)
    if (res.offset < resource.heap_offset + resource.heap_size && res.offset + res.size > resource.heap_offset && res.type != Resource::Access::Read)
      render.track_resource_read_impl(res.enc);

  drv3d_metal::HazardEncoder enc = {render.submits_scheduled, render.current_encoder->index};
  if (resources.size())
  {
    const auto &last = resources.back();
    if (last.enc == enc && last.type == Resource::Access::Read)
      return;
  }
  resources.emplace_back(enc, resource.heap_offset, resource.heap_size, Resource::Access::Read);
}

void ResourceHeap::track_resource_write(drv3d_metal::HazardTracker &resource)
{
  resources.erase(eastl::remove_if(resources.begin(), resources.end(), [this](const Resource &a) { return a.enc.submit <= render.submits_completed; }), resources.end());

  for (auto it = resources.begin(); it != resources.end(); )
  {
    const auto& res = *it;
    if (res.offset < resource.heap_offset + resource.heap_size && res.offset + res.size > resource.heap_offset)
      render.track_resource_read_impl(res.enc);
    // when tracking writes we basically wait for everything that was before so replacing all accesses with current write would be enough
    if (res.offset == resource.heap_offset && res.size == resource.heap_size)
      it = resources.erase(it);
    else
      ++it;
  }

  drv3d_metal::HazardEncoder enc = {render.submits_scheduled, render.current_encoder->index};
  resources.emplace_back(enc, resource.heap_offset, resource.heap_size, Resource::Access::Write);
}
