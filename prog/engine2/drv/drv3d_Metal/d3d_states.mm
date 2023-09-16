
#include <3d/dag_drv3d.h>

#include "render.h"
#include <3d/dag_drv3d_res.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_metal;

const bool d3d::HALF_TEXEL_OFS = false;
const float d3d::HALF_TEXEL_OFSFU = 0.0f;

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

    FSHVER_50, //unsigned vprogver;

    65536 * 4, 65536 * 4, //int maxprims,maxvertind;
    0.0f, 0.0f, //float upixofs,vpixofs;
    DDFSH_3_0 | DDFSH_4_0, //int fshver; ///< fragment shader version flags
    0,   // cshver
    Program::MAX_SIMRT,   //int maxSimRT;
    true, //bool is20ArbitrarySwizzleAvailable;
    32,//minWarpSiz
    sizeof(MTLAccelerationStructureInstanceDescriptor) //raytraceTopAccelerationInstanceElementSize
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

extern void mac_get_vdevice_info(int& vram, int& vendor, String &gpu_desc, bool &web_driver);

const char *d3d::get_driver_name()
{
  return "Metal";
}

unsigned d3d::get_driver_code()
{
  return _MAKE4C('MTL');
}

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

const Driver3dDesc &d3d::get_driver_desc()
{
  G_ASSERTF(render.device, "d3d::get_driver_desc: render.device is nil. The function must be called after video init");
  if (!desc_prepared)
  {
#if _TARGET_PC_MACOSX
    int vendor;
    int vram;
    String gpu_desc;
    bool web_driver;

    mac_get_vdevice_info(vram, vendor, gpu_desc, web_driver);

    g_device_desc.fshver |= DDFSH_5_0;
    g_device_desc.cshver |= DDCSH_5_0;

    metal_use_queries = (vendor != D3D_VENDOR_NVIDIA);

    g_device_desc.caps.hasBindless = false;
    g_device_desc.caps.hasUAVOnlyForcedSampleCount = false;
    g_device_desc.caps.hasNativeRenderPassSubPasses = false;
#else //_TARGET_IOS | _TARGET_TVOS
    metal_use_queries = false;
    g_device_desc.fshver |= DDFSH_5_0;
    g_device_desc.cshver |= DDCSH_5_0;

    g_device_desc.maxSimRT = 4;

    g_device_desc.caps.hasIndirectSupport = [render.device supportsFamily:MTLGPUFamilyApple3];
    g_device_desc.caps.hasCompareSampler = [render.device supportsFamily:MTLGPUFamilyApple3];
#endif
    g_device_desc.caps.hasRenderPassDepthResolve = [render.device supportsFamily:MTLGPUFamilyApple3];

#if APPLE_RT_SUPPORTED && D3D_HAS_RAY_TRACING
#if DAGOR_DBGLEVEL > 0
  const bool isRTAllowed = true;
#else
  const bool isRTAllowed = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("allowRt", false);
#endif
  if (isRTAllowed && render.device.supportsRaytracing)
    g_device_desc.caps.hasRaytracingT11 = true;
#else
  g_device_desc.caps.hasRaytracingT11 = false;
#endif

    desc_prepared = true;
  }

  return g_device_desc;
}

int d3d::driver_command(int command, void *par1, void *par2, void *par3)
{
  switch (command)
  {
    case DRV3D_COMMAND_D3D_FLUSH:
      break;
    case DRV3D_COMMAND_FLUSH_STATES:
      @autoreleasepool
      {
        render.aquareOwnerShip();
        render.flush(false);
        render.releaseOwnerShip();
      }
      break;
    case DRV3D_COMMAND_PROCESS_APP_INACTIVE_UPDATE:
      @autoreleasepool
      {
        render.aquareOwnerShip();
        render.endFrame();
        render.releaseOwnerShip();
      }
      break;
      //  case DRV3D_COMMAND_GETALLTEXS:
      //  case DRV3D_COMMAND_SET_STAT3D_HANDLER:
      //  case DRV3D_COMMAND_GETTEXTUREMEM:
      //  break;
    case DRV3D_COMMAND_GETVISIBILITYBEGIN:
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
    case DRV3D_COMMAND_GETVISIBILITYEND:
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
    case DRV3D_COMMAND_GETVISIBILITYCOUNT:
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
    case DRV3D_COMMAND_RELEASE_QUERY:
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
      //  break;
      //  case DRV3D_COMMAND_ENABLEDEBUGTEXTURES:
      //  case DRV3D_COMMAND_DISABLEDEBUGTEXTURES:
      //  break;
      //  case DRV3D_COMMAND_ENABLE_MT: // ignore
      //  case DRV3D_COMMAND_DISABLE_MT:// ignore
      //  break;
    case DRV3D_COMMAND_ENTER_RESOURCE_LOCK_CS:
    case DRV3D_COMMAND_LEAVE_RESOURCE_LOCK_CS:
      break;
    case DRV3D_COMMAND_ACQUIRE_OWNERSHIP:
      render.aquareOwnerShip();
      break;
    case DRV3D_COMMAND_RELEASE_OWNERSHIP:
      if (par3)
        return render.tryReleaseOwnerShip();
      else
        render.releaseOwnerShip();
      break;
      //  case DRV3D_COMMAND_PREALLOCATE_RT:
      //  break;
      //  case DRV3D_COMMAND_SUSPEND:   // ignore
      //  break;
      //  case DRV3D_COMMAND_RESUME:  // ignore
      //  break;
      //  case DRV3D_COMMAND_GET_SUSPEND_COUNT: // ignore
      //  break;
      //  case DRV3D_COMMAND_GET_MEM_USAGE:
      //  case DRV3D_COMMAND_GET_AA_LEVEL:
      //  break;
      //  case DRV3D_COMMAND_THREAD_ENTER:
      //  break;
      //  case DRV3D_COMMAND_THREAD_LEAVE:
      //  break;
      //  case DRV3D_COMMAND_GET_GPU_FRAME_TIME:  // ignore
      //  break;
      //  case DRV3D_COMMAND_ENTER_RESOURCE_LOCK_CS:
      //  case DRV3D_COMMAND_LEAVE_RESOURCE_LOCK_CS:
      //  break;
      //  case DRV3D_COMMAND_INVALIDATE_STATES: // ignore
      //  break;
      // vulkan has no no timing queries as of yet
      case D3V3D_COMMAND_TIMESTAMPFREQ:
        *reinterpret_cast<uint64_t *>(par1) = render.getGPUTimeStampFreq();
        return 1;
      //  case D3V3D_COMMAND_TIMESTAMPISSUE:// ignore
      case D3V3D_COMMAND_TIMESTAMPGET:
        {
          Render::Query *q = static_cast<Render::Query *>(par1);
          if (q)
          {
            *reinterpret_cast<uint64_t *>(par2) = render.getQueryResult(q, false);
            return 1;
          }
        }
      case D3V3D_COMMAND_TIMECLOCKCALIBRATION:
        // TODO:: stub, defaults to dx12 defaults
        if (par1)
            *reinterpret_cast<uint64_t *>(par1) = 0;
        if (par2)
            *reinterpret_cast<uint64_t *>(par2) = 0;
        if (par3)
            *reinterpret_cast<int *>(par3) = DRV3D_CPU_FREQ_TYPE_QPC;
        return 1;
      //  break;
      //  case DRV3D_COMMAND_GET_SECONDARY_BACKBUFFER:
      //  break;
    case DRV3D_COMMAND_GET_VENDOR:
#if _TARGET_TVOS
    {
      if (par1)
        *(const void**)par1 = render.device_name;
      render.getSupportedMTLVersion(par2);
      return D3D_VENDOR_APPLE;
    }
#endif
      //return drv3d_vulkan::api_state.device.getDeviceVendor();
      break;
    case DRV3D_COMMAND_GET_RESOLUTION:
      /*if (par1 && par2)
      {
        *((int *)par1) = api_state.windowState.settings.resolutionX;
        *((int *)par2) = api_state.windowState.settings.resolutionY;
        return 1;
      }*/
      break;
    case DRV3D_COMMAND_START_CAPTURE_FRAME:
    {
      render.start_capture = true;
      break;
    }
    case DRV3D_COMMAND_SET_PIPELINE_COMPILATION_TIME_BUDGET:
    {
      render.async_pso_compilation = par1 == nullptr;
      break;
    }
    case DRV3D_COMMAND_GET_PIPELINE_COMPILATION_QUEUE_LENGTH:
    {
      if (par1)
        *static_cast<uint32_t*>(par1) = 1;
      return render.async_pso_compilation_length;
    }
    case DRV3D_COMMAND_LOAD_PIPELINE_CACHE:
    {
      render.shadersPreCache.loadPreCache();
      break;
    }
    case DRV3D_COMMAND_IS_HDR_ENABLED:
    {
      return (int)render.mainview.hdrEnabled;
    }
    case DRV3D_COMMAND_INT10_HDR_BUFFER:
      return (int)render.mainview.int10HDRBuffer;
    case DRV3D_COMMAND_HDR_OUTPUT_MODE:
    {
      if (render.mainview.hdrEnabled)
        return (int)(render.mainview.int10HDRBuffer ? HdrOutputMode::HDR10_ONLY : HdrOutputMode::HDR_ONLY);
      return (int)HdrOutputMode::SDR_ONLY;
    }
    case DRV3D_COMMAND_IS_HDR_AVAILABLE:
    {
      return [render.mainview isHDRAvailable];
    }
    case DRV3D_COMMAND_SET_HDR:
    {
      bool enable = *static_cast<bool*>(par1);
      [render.mainview setHDR: enable];
    }
    case DRV3D_COMMAND_GET_METALFX_UPSCALE_STATE:
    {
#if USE_METALFX_UPSCALE
      return render.upscale.supported ? (int)MtlfxUpscaleState::READY : (int)MtlfxUpscaleState::UNSUPPORTED;
#else
      return (int)MtlfxUpscaleState::UNSUPPORTED;
#endif
  }
    case DRV3D_COMMAND_EXECUTE_METALFX_UPSCALE:
    {
      MtlFxUpscaleParams *params = static_cast<MtlFxUpscaleParams*>(par1);
      render.executeUpscale((drv3d_metal::Texture*)params->color, (drv3d_metal::Texture*)params->output, (uint32_t)params->colorMode);
      break;
    }
  }

  return 0;
}

bool d3d::setgamma(float p)
{
  if (render.mainview.hdrEnabled)
    return false;
  [render.mainview setGamma:p];
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

bool d3d::isVcolRgba()
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
  return set;
}

bool d3d::set_msaa_pass()
{
  render.setMSAAPass();
  return true;
}

bool d3d::set_depth_resolve()
{
  render.setDepthResolve();
  return true;
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

void d3d::resource_barrier(ResourceBarrierDesc, GpuPipeline)
{
}
