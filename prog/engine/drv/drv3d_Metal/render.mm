
#include <math/dag_TMatrix.h>
#include <pthread.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>

#include "render.h"
#include "render_inline_shaders.h"
#include "render_command_buffer.h"

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
  Render render;

  CommandBuffer command_buffers[1 + CMD_USE_ATOMICS];
  CommandBuffer* command_buffer = command_buffers;

  MTLDepthStencilDescriptor* Render::depthStateDesc = NULL;

  VDECL   copy_vdecl = BAD_VDECL;
  VSDTYPE clear_dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END };

  Sbuffer* clear_mesh_buffer = nullptr;

  int tmp_copy_buf_size = 131072;

  int clear_vshd = -1;
  int clear_pshdi = -1;
  int clear_pshdf = -1;
  int clear_vdecl = -1;
  int clear_progi = -1;
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

  MTLBlendFactor convBlendArg(int arg)
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
          NSLog(@"Achtung!");
          if (buffer.error)
            NSLog(@"%@", buffer.error);
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
    return cmdBuf;
  }

  int Render::ConstBuffer::shared_cmd_offset;
  int Render::ConstBuffer::shared_cmd_size;
  uint8_t *Render::ConstBuffer::shared_cmd;

  void Render::ConstBuffer::destroy()
  {
    if (shared_cmd)
    {
      free(shared_cmd);
      shared_cmd = nullptr;
    }
    if (cbuffer)
    {
      free(cbuffer);
      cbuffer = nullptr;
    }
  }

  void Render::ConstBuffer::init()
  {
    device_buffer_offset = 0;

    is_binded = nil;

    cbuffer_offset = 0;
    cbuffer = (uint8_t*)malloc(64 * 1024);

    if (shared_cmd == nullptr)
    {
      shared_cmd_offset = 0;
      shared_cmd_size = 1024 * 1024;
      shared_cmd = (uint8_t*)malloc(shared_cmd_size);
    }

    num_strored = 0;
  }

  void Render::ConstBuffer::applyCmd(void* data, int reg_base, int num_regs)
  {
    memcpy(&cbuffer[reg_base], data, num_regs);

    num_strored = max(num_strored, reg_base + num_regs);
  }

  void* Render::ConstBuffer::storeCmd(const float* data, int num_regs)
  {
    G_ASSERT(render.acquare_depth > 0);
    if (shared_cmd_offset + num_regs > shared_cmd_size)
      return nullptr;

    int offset = shared_cmd_offset;
    memcpy(shared_cmd + offset, data, num_regs);
    shared_cmd_offset += num_regs;

    return shared_cmd + offset;
  }

  __forceinline static void bindMetalBuffer(unsigned stage, id<MTLBuffer> buffer, id<MTLBuffer>& buffer_cache,
                                            int& cache_offset, int slot, unsigned offset)
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

  void Render::ConstBuffer::applyBuffer(unsigned stage, int num_reg)
  {
    if (num_reg == 0)
    {
      return;
    }

    if (num_strored > num_reg)
    {
      num_reg = num_strored;
    }

    id<MTLBuffer> buf = render.AllocateConstants(num_reg, device_buffer_offset);
    uint8_t* ptr = (uint8_t*)buf.contents + device_buffer_offset;
    G_ASSERT(device_buffer_offset + num_reg <= RingBufferItem::max_size);

    memcpy(ptr, cbuffer, num_reg);

    bindMetalBuffer(stage, buf, is_binded, is_binded_offset, BIND_POINT, device_buffer_offset);

    num_strored = 0;
  }

  void Render::StageBinding::bindVertexStream(uint32_t stream, uint32_t offset)
  {
    id<MTLBuffer> buffer = NULL;

    uint32_t dynamic_offset = 0;
    if (buffers[stream])
      buffer = buffers[stream]->getBuffer(), dynamic_offset = buffers[stream]->getDynamicOffset();

    bindMetalBuffer(STAGE_VS, buffer, buffers_metal[stream], buffers_metal_offset[stream], stream,
                    buffers_offset[stream] + dynamic_offset + offset);
  }

  void Render::StageBinding::apply(unsigned stage, Shader* shader)
  {
    if (stage == STAGE_VS)
      for (int i = 0; i < GEOM_BUFFER_COUNT; ++i)
        bindVertexStream(i, 0);

    if (shader->immediate_slot >= 0)
    {
      G_ASSERTF(immediate_dword_count > 0, "Shader name and variant: %s", getShaderName(shader->src == nil ? "" : [shader->src cStringUsingEncoding:[NSString defaultCStringEncoding]]).c_str());
      int upload_size = ((immediate_dword_count+3) & ~3) * sizeof(uint32_t);
      bool changed = immediate_slot != shader->immediate_slot ||
        memcmp(immediate_dwords_metal, immediate_dwords, immediate_dword_count*sizeof(uint32_t));
      if (changed)
      {
        memcpy(immediate_dwords_metal, immediate_dwords, upload_size);
        immediate_slot = shader->immediate_slot;
      }
      if (stage == STAGE_VS && changed)
      {
        [render.renderEncoder setVertexBytes:immediate_dwords length:upload_size atIndex:shader->immediate_slot];
      }
      else if (stage == STAGE_PS && changed)
      {
        [render.renderEncoder setFragmentBytes:immediate_dwords length:upload_size atIndex:shader->immediate_slot];
      }
      else if (stage == STAGE_CS && changed)
      {
        [render.computeEncoder setBytes :immediate_dwords length:upload_size atIndex:shader->immediate_slot];
      }
    }
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

      bindMetalBuffer(stage, buffer, buffers_metal[index], buffers_metal_offset[index],
                      shader->buffers[i].slot, offset + dynamic_offset);
    }

    for (int i = 0; i < shader->num_tex; i++)
    {
      int slot = shader->tex_binding[i];
      int tslot = shader->tex_remap[i];

      id<MTLTexture> texture = nil;

      if (textures[slot])
        textures[slot]->apply(texture, textures_read_stencil[slot], textures_mip_level[slot]);
      else
      {
        if (slot >= MAX_SHADER_TEXTURES)
            render.blank_tex_rw[shader->tex_type[i]]->apply(texture, false, 0);
        else
            render.blank_tex[shader->tex_type[i]]->apply(texture, false, 0);
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
    for (int i = 0; i < shader->num_samplers; i++)
    {
      int slot = shader->sampler_binding[i];
      int tslot = shader->sampler_remap[i];

      id<MTLSamplerState> sampler = nil;

      G_ASSERT(slot < 16);
      if (textures[slot])
        textures[slot]->applySampler(sampler);
      else
        render.blank_tex[shader->tex_type[i]]->applySampler(sampler);

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

    for (int i = 0; i < shader->accelerationStructureCount; i++)
    {
      const int inDriverSlot = shader->acceleration_structure_remap[i];
      const int inShaderSlot = shader->acceleration_structure_binding[inDriverSlot];

      if (@available(iOS 15.0, macos 11.0, *))
      {
        [render.computeEncoder setAccelerationStructure:acceleration_structures[inDriverSlot] atBufferIndex:inShaderSlot];
      }
    }

    if (shader)
    {
      cbuffer.applyBuffer(stage, shader->num_reg);
    }
  }

  void Render::StageBinding::resetBuffer(int buf)
  {
    buffers_metal[buf] = nil;
  }

  void Render::StageBinding::resetBuffers(Shader* old_shader, Shader* shader)
  {
    if (shader)
    {
      bool changed = !old_shader || old_shader->shader_hash != shader->shader_hash;
      if (changed)
        for (int i = 0; i < shader->num_buffers; ++i)
          resetBuffer(shader->buffers[i].remapped_slot);
    }
    else
    {
      for (int i = 0; i < BUFFER_POINT_COUNT; i++)
        resetBuffer(i);
      cbuffer.is_binded = nil;
      immediate_slot = -1;
    }
  }

  void Render::StageBinding::reset(bool full)
  {
    for (int i = 0; i < MAX_SHADER_TEXTURES*2; i++)
    {
      textures_metal[i] = nil;
      samplers_metal[i] = nil;
      if (full)
        textures[i] = nullptr;
    }
    for (int i = 0; i < BUFFER_POINT_COUNT; i++)
    {
      if (full)
      {
        buffers[i] = nil;
      }
    }

    resetBuffers(nullptr, nullptr);
  }

  void Render::StageBinding::removeBuf(Buffer* buf)
  {
    for (int i = 0; i < 32; i++)
    {
      if (buffers[i] == buf)
      {
        buffers[i] = nullptr;
      }
    }
  }

  void Render::StageBinding::removeTex(Texture* tex)
  {
    for (int i = 0; i < MAX_SHADER_TEXTURES*2; i++)
    {
      if (textures[i] == tex)
      {
        textures[i] = nullptr;
      }
    }
  }

  Render::Render()
    : shaders(midmem_ptr(), 128)
    , progs(midmem_ptr(), 128)
    , vdecls(midmem_ptr(), 128)
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

    need_prepare_depth_state = true;

    need_change_rt = 0;

    depth_clip = true;

    main_thread = 0;

    device_name[0] = 0;

    drawable_aquared = false;

    cached_viewport.originX = -FLT_MAX;

    start_capture = false;

    save_backBuffer = false;
  }

  static void PatchShader(int shader, int num_reg, int num_tex)
  {
    Shader* vshader = render.shaders.getObj(shader);
    vshader->num_reg = num_reg;
    vshader->num_tex = num_tex;
    vshader->tex_type[0] = 0;
    vshader->tex_binding[0] = 0;
    vshader->tex_remap[0] = 0;
    vshader->num_samplers = num_tex;
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

    max_number_of_frames_to_skip_after_error = ::dgs_get_settings()->getBlockByNameEx("metal")->getInt("framesToSkip", 0);
    hadError = false;

    #if _TARGET_TVOS
    caps.samplerHaveCmprFun = [device supportsFeatureSet:MTLFeatureSet_tvOS_GPUFamily2_v1];
    caps.drawIndxWithBaseVertes = [device supportsFeatureSet:MTLFeatureSet_tvOS_GPUFamily2_v1];
    #elif _TARGET_IOS
    caps.samplerHaveCmprFun = [device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
    caps.drawIndxWithBaseVertes = [device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
    #endif

    #if _TARGET_PC_MACOSX
    caps.readWriteTextureTier1 = [device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v2];
    caps.readWriteTextureTier2 = [device supportsFeatureSet:MTLFeatureSet_macOS_ReadWriteTextureTier2];
    #else
    caps.readWriteTextureTier1 = device.readWriteTextureSupport == MTLReadWriteTextureTier1 || device.readWriteTextureSupport == MTLReadWriteTextureTier2;
    caps.readWriteTextureTier2 = device.readWriteTextureSupport == MTLReadWriteTextureTier2;
    #endif

    shadersPreCache.init(get_shader_cache_version());

    commandQueue = [device newCommandQueue];

    query_buffer = [device newBufferWithLength : QUERIES_MAX * 8 options : MTLResourceStorageModeShared];
    stub_buffer = [device newBufferWithLength : 64*1024 options : MTLResourceStorageModeShared];
    stub_buffer.label = @"stub buffer";
    memset(stub_buffer.contents, 0, 64*1024);

    used_query = 0;

    for (int i = 0; i < QUERIES_MAX; i++)
    {
        queries[i].offset = i * 8;
    }

    cur_query_offset = -1;

    for (int i = 0; i < 3; i++)
    {
      StageBinding& stg = stages[i];
      stg.cbuffer.init();

      for (int j = 0; j < BUFFER_POINT_COUNT; j++)
      {
        stg.buffers[j] = NULL;
        stg.buffers_offset[j] = 0;
        stg.buffers_metal[j] = nil;
      }
      for (int j = 0; j < MAX_SHADER_TEXTURES*2; j++)
      {
        stg.textures[j] = NULL;
        stg.textures_read_stencil[j] = false;
        stg.textures_mip_level[j] = 0;
        stg.textures_metal[j] = nil;
        stg.samplers_metal[j] = nil;
      }
    }

    forceClearOnCreate = ::dgs_get_settings()->getBlockByNameEx("metal")->getBool("forceClear", 0);

    blank_tex[0] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_2d", false, true);
    blank_tex[1] = new Texture(1, 1, 1, 1, RES3D_ARRTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blanktex_array", false, true);
    blank_tex[2] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_RTARGET, TEXFMT_DEPTH24, "blank_depth", false, true);
    blank_tex[3] = new Texture(1, 1, 1, 1, RES3D_CUBETEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_cube", false, true);
    blank_tex[4] = new Texture(1, 1, 1, 1, RES3D_VOLTEX, TEXCF_RTARGET, TEXFMT_A8R8G8B8, "blank_3d", false, true);

    blank_tex_rw[0] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_2d", false, true);
    blank_tex_rw[1] = new Texture(1, 1, 1, 1, RES3D_ARRTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blanktexrw_array", false, true);
    blank_tex_rw[2] = new Texture(1, 1, 1, 1, RES3D_TEX, TEXCF_UNORDERED, TEXFMT_DEPTH24, "blankrw_depth", false, true);
    blank_tex_rw[3] = new Texture(1, 1, 1, 1, RES3D_CUBETEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_cube", false, true);
    blank_tex_rw[4] = new Texture(1, 1, 1, 1, RES3D_VOLTEX, TEXCF_UNORDERED, TEXFMT_A8R8G8B8, "blankrw_3d", false, true);

    for (int i = 0; i < 5; ++i)
    {
      if (i == 2) // don't do this for depth
        continue;
      void *ptr; int stride;
      blank_tex[i]->lockimg(&ptr, stride, 0, TEXLOCK_WRITE);
      memset(ptr, 0, stride);
      blank_tex[i]->unlockimg();

      blank_tex_rw[i]->lockimg(&ptr, stride, 0, TEXLOCK_WRITE);
      memset(ptr, 0, stride);
      blank_tex_rw[i]->unlockimg();
    }

    ::create_critical_section(aquareSec);
    ::create_critical_section(rcSec);

    pthread_threadid_np(NULL, &main_thread);

    cur_thread = main_thread;

    acquare_depth = 0;

    cur_prog = nil;
    vdecl = nil;

    rt.rt[0] = (Texture*)-1;
    rt.rt_levels[0] = 0;
    rt.rt_layers[0] = 0;

    for (int i = 1; i < Program::MAX_SIMRT; i++)
    {
      rt.rt[i] = 0;
      rt.rt_levels[i] = 0;
      rt.rt_layers[i] = 0;
    }

    rt.depth = (Texture*)-1;
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

    render_desc = [MTLRenderPassDescriptor renderPassDescriptor];

    depth_clip = 1;
    depth_bias = 0.0f;
    depth_slopebias = 0.0f;
    bias_changed = 0;
    stencil_ref_changed = false;

    need_set_qeury = 0;

    cur_state = NULL;

    clear_vshd = createVertexShader((const uint8_t*)clear_vs_metal);
    clear_pshdi = createPixelShader((const uint8_t*)clear_ps_metal_i);
    clear_pshdf = createPixelShader((const uint8_t*)clear_ps_metal_f);
    clear_vdecl = createVDdecl(clear_dcl);

    PatchShader(clear_vshd, 1, 0);
    PatchShader(clear_pshdi, 1, 1);
    PatchShader(clear_pshdf, 1, 1);

    clear_progi = createProgram(clear_vshd, clear_pshdi, clear_vdecl, 0, 0);
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
      clear_mesh_buffer = d3d::create_vb(sizeof(clear_mesh), SBCF_MAYBELOST, "metal system clear");

      float *verts = NULL;
      clear_mesh_buffer->lock(0, 0, (void**)&verts, VBLOCK_WRITEONLY);
      if (verts)
        memcpy(verts, clear_mesh, sizeof(clear_mesh));
      clear_mesh_buffer->unlock();
    }

    tmp_copy_buff = [device newBufferWithLength : tmp_copy_buf_size options : MTLResourceStorageModeShared];

    strcpy(device_name, [[device name] UTF8String]);

    can_use_async_pso_compilation = get_aync_pso_compilation_supported();

    [render.mainview isHDRAvailable];

    bottomLevelAccelerationStructures = [[NSMutableArray alloc] init];

    return true;
  }

  void Render::executeUpscale(Texture *color, Texture *output, uint32_t colorMode)
  {
    command_buffer->push<CommandMtlfxUpscale>(color, output, colorMode);
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
      std::unique_lock<std::mutex> lock(frame_mutex);
      uint64_t lframe = frames_completed[local_frame];
      frame_condition.wait(lock, [lframe]
      {
        return lframe <= render.frame_completed;
      });
    }

    if ((frame + 1) >= MAX_FRAMES_TO_RENDER)
    {
      auto & resources2free = resources2delete_frames[local_frame];
      {
        std::lock_guard<std::mutex> scopedLock(constant_buffer_lock);
        if (constant_buffers_free.size() > 1)
        {
          for (int i = 1; i<constant_buffers_free.size(); i++)
          {
            auto& buf = constant_buffers_free[i];
            [buf.buf release];
            buf.buf = nil;
          }
          constant_buffers_free.resize(1);
        }
        for (auto & buf : resources2free.constant_buffers)
          constant_buffers_free.push_back(buf);
        resources2free.constant_buffers.clear();
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

      if (@available(iOS 15.0, macos 11.0, *))
      {
        for (auto & as : resources2free.acceleration_structures)
          [as release];
        resources2free.acceleration_structures.clear();

        for (auto & as : resources2free.tlases)
          if (as->instanceAccelerationStructure != nil)
            [as->instanceAccelerationStructure release];

        for (auto & as : resources2free.blases)
          if (as->primitiveAccelerationStructure != nil)
            [as->primitiveAccelerationStructure release];
      }
      resources2free.tlases.clear();
      resources2free.blases.clear();
    }

    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      eastl::swap(resources2delete, resources2delete_frames[frame%MAX_FRAMES_TO_RENDER]);
    }
    frame++;
  }

  void Render::endFrame()
  {
    flush(false, true);

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
          logerr("METAL has error during command buffer execution, drawing in background");
        }
        else
        {
          logerr("METAL has error during command buffer execution %d, skipping %d frames",
                 error, max_number_of_frames_to_skip_after_error);
        }
        number_of_frames_to_skip_after_error = max_number_of_frames_to_skip_after_error;
      }
      hadError.store(0);
    }

    shadersPreCache.tickCache();

    drawable_aquared = false;
    need_change_rt = 1;
  }

  void Render::doClearTexture(uint16_t tex_width, uint16_t tex_height, uint8_t slices, uint8_t depth, uint8_t levels, id<MTLTexture> tex, int base_format, bool use_dxt)
  {
    G_ASSERT(tex);

    ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

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

  void Render::doTexCopyRegion(const CommandTexCopyRegion& cmd)
  {
    ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

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
        [device newBufferWithLength : sz*cmd.src_d options : MTLResourceStorageModeShared];
#if DAGOR_DBGLEVEL > 0
      if (sz*cmd.src_d > tmp_copy_buf_size)
        tmp_buf.label = [NSString stringWithFormat:@"copy for %llu", render.frame];
#endif
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

  void Render::flush(bool wait, bool present)
  {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    if (@available(macos 10.13, *))
    {
      if (start_capture)
      {
        if (@available(macos 10.15, iOS 13, *))
        {
          [[MTLCaptureManager sharedCaptureManager] startCaptureWithDescriptor:[MTLCaptureDescriptor alloc] error:nil];
        }
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101500
        else
        {
          [[MTLCaptureManager sharedCaptureManager] startCaptureWithDevice:device];
        }
#endif
      }
    }
#endif

    @autoreleasepool
    {
      commandBuffer = createCommandBuffer(commandQueue, "Command buffer");
      need_change_rt = 1;

      RCAutoLock autoclock;
      {
        TIME_PROFILE(upload);
        std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
        if (textures2upload.size() || buffers2copy.size())
        {
          uint32_t data[4] = { 0 };
          for (int i = 0; i < buffers2copy.size(); ++i)
          {
            ensureHaveEncoderExceptRender(Render::EncoderType::Compute);
            [computeEncoder setComputePipelineState:copy_cs_pipeline];

            auto& rc = buffers2copy[i];
            G_ASSERT((rc.size & 3) == 0);
            data[0] = rc.size / 4;
            data[1] = rc.src_offset / 4;
            data[2] = rc.dst_offset / 4;
            [render.computeEncoder setBytes:data length:16 atIndex:0];
            [render.computeEncoder setBuffer:rc.dst offset:0 atIndex:1];
            [render.computeEncoder setBuffer:rc.src offset:0 atIndex:2];
            [render.computeEncoder dispatchThreadgroups  : MTLSizeMake((data[0] + 63) >> 6, 1, 1)
                                   threadsPerThreadgroup : MTLSizeMake(64, 1, 1)];
          }
          stages[STAGE_CS].reset();
          current_cs_pipeline = nullptr;

          for (int i = 0; i < textures2upload.size(); ++i)
          {
            ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

            UploadTex& rc = textures2upload[i];
            MTLSize size = { (uint32_t)rc.w, (uint32_t)rc.h, (uint32_t)rc.d };
            MTLOrigin origin = { 0, 0, 0 };

            G_ASSERT([rc.buf length] >= rc.d * rc.imageSize);
            [blitEncoder copyFromBuffer:rc.buf sourceOffset:0 sourceBytesPerRow:rc.pitch
                    sourceBytesPerImage:rc.imageSize sourceSize:size toTexture:rc.tex
                    destinationSlice:rc.layer destinationLevel:rc.level destinationOrigin:origin];
            rc.buf = nil;
          }

          textures2upload.clear();
          buffers2copy.clear();
        }
      }

      if (max_commands < command_buffer->offset)
      {
        debug("[METAL] max command memory used %i", command_buffer->offset);
        for (int i = 0; i < Type_CommandCount; ++i)
          if (command_buffer->size_stats[i])
            debug("[METAL] command memory used %i - %i", i, command_buffer->size_stats[i]);
        max_commands = command_buffer->offset;
      }

  #if CMD_USE_ATOMICS
      CommandBuffer* new_buffer = command_buffer == command_buffers ? command_buffers + 1 : command_buffers;
      CommandBuffer* local_buffer = __atomic_exchange_n(&command_buffer, new_buffer, __ATOMIC_SEQ_CST);
  #else
      CommandBuffer* local_buffer = command_buffer;
  #endif
      local_buffer->execute(*this);

      stages[0].cbuffer.shared_cmd_offset = 0;

      if (save_backBuffer)
      {
        G_ASSERT(drawable_aquared);
        id <MTLTexture> curBackBuffer = [mainview getBackBuffer:false];
        id <MTLTexture> savedBackBuffer = [mainview getSavedBackBuffer];

        MTLOrigin origin = { 0, 0, 0 };
        MTLSize size = { curBackBuffer.width, curBackBuffer.height, 1 };

        ensureHaveEncoderExceptRender(Render::EncoderType::Blit);
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
      ensureHaveEncoderExceptRender(Render::EncoderType::None);

      if (present)
      {
        if (drawable_aquared)
        {
          TIME_PROFILE(present);
          [mainview presentDrawable : commandBuffer];
        }

        {
          TIME_PROFILE(wait_for_drawable);
          uint64_t local_frame = 0;
          {
            std::unique_lock<std::mutex> lock(render.frame_mutex);
            local_frame = ++frame_scheduled;
            frames_completed[frame % MAX_FRAMES_TO_RENDER] = local_frame;
          }
          [commandBuffer addCompletedHandler : ^ (id<MTLCommandBuffer> buffer)
          {
            {
              std::unique_lock<std::mutex> lock(render.frame_mutex);
              G_ASSERT(local_frame <= render.frame_scheduled);
              G_ASSERT(local_frame == render.frame_completed + 1);
              render.frame_completed = local_frame;
            }
            render.frame_condition.notify_all();
          }];
        }
      }

      setupCommadBufferErrorHandler(commandBuffer);
      [commandBuffer commit];

      if (wait)
      {
        TIME_PROFILE(wait_for_cmd_buffer);
        [commandBuffer waitUntilCompleted];
      }

      commandBuffer = nil;

      if (present)
        cleanupFrame();
    }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    if (@available(macos 10.13, *))
    {
      if (start_capture)
      {
        [[MTLCaptureManager sharedCaptureManager] stopCapture];
        start_capture = false;
      }
    }
#endif
  }

  bool Render::setBlendFactor(E3DCOLOR color)
  {
    command_buffer->push<CommandSetBlendFactor>(color);
    return true;
  }

  void Render::clearView(int what, E3DCOLOR c, float z, uint32_t stencil)
  {
    command_buffer->push<CommandClear>(what, c, z, stencil);
  }

  void Render::setViewport(int x, int y, int w, int h, float minz, float maxz)
  {
    if (minz == 0.0f && maxz == 1.f)
      command_buffer->push<CommandSetViewportNoZ>(x, y, w, h);
    else
      command_buffer->push<CommandSetViewport>(x, y, w, h, minz, maxz);
  }

  void Render::setScissor(int x, int y, int w, int h)
  {
    command_buffer->push<CommandSetScissor>(x, y, w, h);
  }

  void Render::setBuff(unsigned stage, BufferType bf_type, int slot, Buffer *buffer, int offset, int stride)
  {
    command_buffer->push<CommandSetBuf>(stage, RemapBufferSlot(bf_type, slot), slot, buffer, offset, stride);
  }

  void Render::setImmediateConst(unsigned stage, const uint32_t *data, unsigned num_words)
  {
    command_buffer->push<CommandSetImmediate>(stage, num_words, (unsigned*)data);
  }

  void Render::setIBuff(Buffer *buffer)
  {
    command_buffer->push<CommandSetIB>(buffer, buffer && (buffer->bufFlags & SBCF_INDEX32) ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16);
  }

  void Render::discardBuffer(Buffer *buffer, Buffer::BufTex* buftex, id<MTLBuffer> buf, int dynamic_offset)
  {
    command_buffer->push<CommandBufDiscard>(buffer, buftex, buf, dynamic_offset);
  }

  void Render::swapTextures(Texture* src, BaseTexture* dst)
  {
    command_buffer->push<CommandSwapTextures>(src, dst);
  }

  void Render::setTexture(unsigned stage, int slot, Texture* tex, int mip_level)
  {
    command_buffer->push<CommandSetTex>(stage, slot, tex, tex ? tex->isReadStencil() : false, mip_level);
  }

  int Render::setTextureAddr(Texture* tex, int u, int v, int w)
  {
    return command_buffer->push<CommandSetTextureAddr>(tex, u, v, w) != nullptr;
  }

  int Render::setTextureBorder(Texture* tex, E3DCOLOR color)
  {
    return command_buffer->push<CommandSetTextureBorder>(tex, color) != nullptr;
  }

  int Render::setTextureFilter(Texture* tex, int filter)
  {
    return command_buffer->push<CommandSetTextureFilter>(tex, filter, -1) != nullptr;
  }

  int Render::setTextureMipmapFilter(Texture* tex, int filter)
  {
    return command_buffer->push<CommandSetTextureFilter>(tex, -1, filter) != nullptr;
  }

  int Render::setTextureAnisotropy(Texture* tex, int level)
  {
    return command_buffer->push<CommandSetTextureAniso>(tex, level) != nullptr;
  }

  bool Render::draw(int prim_type, int start_vertex, int vertex_count, uint32_t num_inst, uint32_t start_instance)
  {
    if (prim_type == 6 || vertex_count == 0)
    {
      return false;
    }

    if (number_of_frames_to_skip_after_error)
      return true;

    command_buffer->push<CommandDraw>(prim_type - 1, start_vertex, vertex_count, num_inst, start_instance);

    return true;
  }

  bool Render::drawIndexed(int prim_type, int start_index, int index_count, int base_vertex, uint32_t num_inst, uint32_t start_instance)
  {
    if (prim_type == 6 || index_count == 0)
    {
      return false;
    }

    if (number_of_frames_to_skip_after_error)
      return true;

    command_buffer->push<CommandDrawIndexed>(prim_type - 1, base_vertex, start_index, index_count, num_inst, start_instance);

    return true;
  }

  bool Render::drawIndirect(int prim_type, Sbuffer* buffer, uint32_t offset)
  {
    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      logerr("Metal: trying to use unsupported drawIndirect");
      return false;
    }

    if (prim_type == 6)
    {
      return false;
    }

    if (number_of_frames_to_skip_after_error)
      return true;

    command_buffer->push<CommandDrawIndirect>(prim_type - 1, (Buffer*)buffer, offset);

    return true;
  }

  bool Render::drawIndirectIndexed(int prim_type, Sbuffer* buffer, uint32_t offset)
  {
    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      logerr("Metal: trying to use unsupported drawIndirectIndexed");
      return false;
    }

    if (prim_type == 6)
    {
      return false;
    }

    if (number_of_frames_to_skip_after_error)
      return true;

    command_buffer->push<CommandDrawIndexedIndirect>(prim_type - 1, (Buffer*)buffer, offset);

    return true;
  }

  struct BufferUP : public Buffer
  {
    BufferUP()
    {
      isDynamic = true;
      fast_discard = true;
    }
  };
  BufferUP tempBufferVB, tempBufferIB;

  bool Render::drawUP(int prim_type, int prim_count, const uint16_t* ind, const void* ptr, int numvert, int stride_bytes)
  {
    int vbuf_offset = 0;
    id<MTLBuffer> dynamic_vb = AllocateConstants(numvert*stride_bytes, vbuf_offset);
    tempBufferVB.dynamic_frame = frame;
    render.discardBuffer(&tempBufferVB, nullptr, dynamic_vb, 0);

    memcpy((uint8_t*)dynamic_vb.contents + vbuf_offset, ptr, numvert*stride_bytes);
    d3d::setvsrc_ex(0, &tempBufferVB, vbuf_offset, stride_bytes);

    if (ind)
    {
      int ibuf_offset = 0;
      id<MTLBuffer> dynamic_ib = AllocateConstants(prim_count * 2, ibuf_offset);
      tempBufferIB.dynamic_frame = frame;
      render.discardBuffer(&tempBufferIB, nullptr, dynamic_ib, 0);

      memcpy((uint8_t*)dynamic_ib.contents + ibuf_offset, ind, prim_count * 2);

      d3d::setind(&tempBufferIB);
      drawIndexed(prim_type, ibuf_offset / 2, prim_count, 0, 1);
    }
    else
      draw(prim_type, 0, prim_count, 1);
    return true;
  }

  int Render::createVertexShader(const uint8_t* code)
  {
    Shader* shader = new Shader();

    if (!shader->compileShader((const char*)code, using_async_pso_compilation))
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

    if (!shader->compileShader((const char*)code, using_async_pso_compilation))
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

  int  Render::createVDdecl(VSDTYPE* d)
  {
    VDecl* decl = new VDecl(d);

    vdecls.lock();
    int index = vdecls.getFreeIndex(decl);
    vdecls.unlock();

    return index;
  }

  void Render::setVDecl(VDECL vdecl)
  {
    command_buffer->push<CommandSetVDecl>(vdecl);
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
    command_buffer->push<CommandSetProg>(prog);
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
    command_buffer->push<CommandSetConst>(stage, data, reg_base*16, num_regs*16);
  }

  void Render::clearRwBuffer(Sbuffer* buff, const float val[4])
  {
    command_buffer->push<CommandClearRW>((Buffer*)buff, (unsigned*)val);
  }

  void Render::clearRwBufferi(Sbuffer *tex, const unsigned val[4])
  {
    command_buffer->push<CommandClearRW>((Buffer*)tex, (unsigned*)val);
  }

  void Render::clearBuffer(Buffer::BufTex* buff)
  {
    command_buffer->push<CommandClearBuffer>(buff);
  }

  void Render::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
  {
    if (number_of_frames_to_skip_after_error)
      return;
    G_ASSERT(thread_group_x);
    G_ASSERT(thread_group_y);
    G_ASSERT(thread_group_z);
    command_buffer->push<CommandDispatch>(thread_group_x, thread_group_y, thread_group_z);
  }

  void Render::dispatch_indirect(Sbuffer* buffer, uint32_t offset)
  {
    if (!d3d::get_driver_desc().caps.hasIndirectSupport)
    {
      logerr("Metal: trying to use unsupported dispatch_indirect");
      return;
    }
    if (number_of_frames_to_skip_after_error)
      return;
    command_buffer->push<CommandDispatchIndirect>((Buffer*)buffer, offset);
  }

  void Render::setTextureMipLevel(Texture* texture, int min_level, int max_level)
  {
    command_buffer->push<CommandSetTexMipLevel>(texture, min_level, max_level);
  }

  void Render::generateMips(Texture* texture)
  {
    command_buffer->push<CommandTexGenMips>(texture);
  }

  void Render::copyTex(Texture* src, Texture* dest)
  {
    command_buffer->push<CommandTexCopy>(dest, src);
  }

  void Render::copyTexRegion(Texture* src,
                             int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
                             Texture* dest, int dest_level, int dest_x, int dest_y, int dest_z)
  {
    G_ASSERT(!src->isStub());
    command_buffer->push<CommandTexCopyRegion>(dest, dest_level, dest_x, dest_y, dest_z, src, src_level, src_x, src_y, src_z, src_w, src_h, src_d);
  }

  void Render::setRT(int index, Texture* rt, int level, int layer)
  {
    // -1 is used for backbuffer texture
    if (rt && rt != (Texture*)-1)
      rt->last_render_frame = frame;
    command_buffer->push<CommandSetRT>(index, rt, level, layer);
  }

  void Render::setDepth(Texture* depth, int layer, bool ro)
  {
    // -1 is used for backbuffer depth
    if (depth && depth != (Texture*)-1)
      depth->last_render_frame = frame;
    command_buffer->push<CommandSetDepth>(depth, layer, ro);
  }

  void Render::clearTex(Texture* tex, const unsigned val[4], int level, int layer)
  {
    command_buffer->push<CommandClearTex>(tex, (unsigned*)val, level, layer, true);
  }

  void Render::clearTex(Texture* tex, const float val[4], int level, int layer)
  {
    command_buffer->push<CommandClearTex>(tex, (unsigned*)val, level, layer, false);
  }

  void Render::clearTexture(Texture* tex)
  {
    command_buffer->push<CommandClearTexture>(tex);
  }

  void Render::copyBuffer(Sbuffer* src, int srcOffset, Sbuffer* dstb, int dstOffset, int size)
  {
    Buffer* dst = (Buffer*)dstb;
    command_buffer->push<CommandCopyBuffer>(dst, dstOffset, (Buffer*)src, srcOffset, size ? size : dst->bufSize);
  }

  void Render::copyTex(Texture* src, Texture* dst, RectInt *rsrc, RectInt *rdst)
  {
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

    command_buffer->push<CommandCopyTex>(src, src_rect, dst, dst_rect);
  }

  void Render::restoreRT()
  {
    setRT(0, (Texture*)-1, 0, 0);
  }

  void Render::restoreDepth()
  {
    setDepth((Texture*)-1, 0, false);
  }

  void Render::doSetState(shaders::DriverRenderStateId state_id)
  {
    const CachedRenderState & state = *render_states.get(state_id);
    if (scissor_on != state.scissor_on)
    {
      scissor_on = state.scissor_on;
      need_set_scissor = true;
    }
    if (cur_depthstate.depth_state != state.depth_state.depth_state)
    {
      cur_depthstate = state.depth_state;
      need_prepare_depth_state = true;
    }
    if (fabs(depth_bias - state.depth_bias) > 0.00000001f || fabs(depth_slopebias - state.depth_slopebias) > 0.00000001f)
    {
      depth_bias = state.depth_bias;
      depth_slopebias = state.depth_slopebias;
      bias_changed = 1;
    }
    if (depth_clip != state.depth_clip)
    {
      depth_clip = state.depth_clip;
      [renderEncoder setDepthClipMode : (MTLDepthClipMode)(1 - depth_clip)];
    }
    if (stencil_ref != state.stencil_ref)
    {
      stencil_ref = state.stencil_ref;
      stencil_ref_changed = true;
    }
    if (cur_cull != state.cull)
      setCull(state.cull);
    cur_rstate.raster_state = state.raster_state;
  }

  void Render::doDispatch(Buffer* indirect_buffer, int offset, int tx, int ty, int tz)
  {
    if (!cur_prog || !cur_prog->cshader)
    {
      return;
    }

    if (!computeEncoder)
    {
      ensureHaveEncoderExceptRender(Render::EncoderType::Compute);

      stages[STAGE_CS].reset();
      current_cs_pipeline = nullptr;
    }

    if (current_cs_pipeline != cur_prog->csPipeline)
        [computeEncoder setComputePipelineState : cur_prog->csPipeline];
    current_cs_pipeline = cur_prog->csPipeline;

    stages[STAGE_CS].apply(STAGE_CS, cur_prog->cshader);

    if (indirect_buffer)
    {
      [computeEncoder dispatchThreadgroupsWithIndirectBuffer:indirect_buffer->getBuffer()
                                        indirectBufferOffset:offset + indirect_buffer->getDynamicOffset()
                   threadsPerThreadgroup : MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z)];
    }
    else
      [computeEncoder dispatchThreadgroups : MTLSizeMake(tx, ty, tz)
                   threadsPerThreadgroup : MTLSizeMake(cur_prog->cshader->tgrsz_x, cur_prog->cshader->tgrsz_y, cur_prog->cshader->tgrsz_z)];
  }

  static void setRenderTarget(Texture* tex, int level, int layer)
  {
    render.rt.rt[0] = tex;
    render.rt.rt_levels[0] = level;
    render.rt.rt_layers[0] = layer;

    for (int i = 1; i < Program::MAX_SIMRT; i++)
      render.rt.rt[i] = NULL;

    render.rt.depth = NULL;
    render.rt.vp.used = false;
    render.need_change_rt = 1;
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
    Render::StageBinding old_vs_stage;
    Render::StageBinding old_ps_stage;
    Render::DepthState old_depthstate;
    Render::RenderTargetConfig old_rt;
    int old_cull;
    bool save_tex, save_rt;

    float vs_cbuffer[4];
    float ps_cbuffer[4];

    StateSaver(Render& render, bool save_tex, bool save_rt, bool color_write, bool depth_write)
      : save_tex(save_tex)
      , save_rt(save_rt)
    {
      old_rstate = render.cur_rstate;
      old_vs_stage = render.stages[STAGE_VS];
      old_ps_stage = render.stages[STAGE_PS];
      old_depthstate = render.cur_depthstate;
      old_rt = render.rt;
      old_cull = render.cur_cull;

      memcpy(vs_cbuffer, old_vs_stage.cbuffer.cbuffer, 16);
      memcpy(ps_cbuffer, old_ps_stage.cbuffer.cbuffer, 16);

      render.cur_depthstate.zenable = 0;
      render.cur_depthstate.depth_write_on = depth_write ? 1 : 0;
      render.need_prepare_depth_state = true;

      render.setCull(0);

      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
        render.cur_rstate.raster_state.blend[i].ablend = false;
      render.cur_rstate.raster_state.writeMask = color_write ? 0xffffffff : MTLColorWriteMaskNone;
      render.cur_rstate.raster_state.a2c = 0;
      render.cur_rstate.raster_state.pad[0] = render.cur_rstate.raster_state.pad[1] = 0;
    }

    ~StateSaver()
    {
      render.cur_depthstate.zenable = old_depthstate.zenable;
      render.cur_depthstate.depth_write_on = old_depthstate.depth_write_on;
      render.need_prepare_depth_state = true;

      for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
        render.cur_rstate.raster_state.blend[i].ablend = old_rstate.raster_state.blend[i].ablend;
      render.cur_rstate.raster_state.writeMask = old_rstate.raster_state.writeMask;
      render.cur_rstate.raster_state.a2c = 0;
      render.cur_rstate.raster_state.pad[0] = render.cur_rstate.raster_state.pad[1] = 0 ;

      render.stages[STAGE_VS].setBuf(0, old_vs_stage.buffers[0], old_vs_stage.buffers_offset[0]);
      render.setCull(old_cull);

      if (save_rt)
      {
        render.rt = old_rt;
        render.need_change_rt = 1;
      }

      if (save_tex)
        render.stages[STAGE_PS].setTex(0, old_ps_stage.textures[0], old_ps_stage.textures_read_stencil[0], old_ps_stage.textures_mip_level[0]);

      memcpy(old_vs_stage.cbuffer.cbuffer, vs_cbuffer, 16);
      memcpy(old_ps_stage.cbuffer.cbuffer, ps_cbuffer, 16);
    }
  };

  void Render::doSetProgram(int prog, bool is_async)
  {
    progs.lock();
    Program* new_prog = render.progs.getObj(prog);
    progs.unlock();
    if (new_prog && (!cur_prog || new_prog->vshader != cur_prog->vshader ||
        new_prog->pshader != cur_prog->pshader || new_prog->cshader != cur_prog->cshader))
    {
      if (new_prog->vshader)
        render.stages[STAGE_VS].resetBuffers(cur_prog ? cur_prog->vshader : nullptr, new_prog->vshader);
      if (new_prog->pshader)
        render.stages[STAGE_PS].resetBuffers(cur_prog ? cur_prog->pshader : nullptr, new_prog->pshader);
      if (new_prog->cshader)
        render.stages[STAGE_CS].resetBuffers(cur_prog ? cur_prog->cshader : nullptr, new_prog->cshader);
      if (!new_prog->cshader)
      {
        uint64_t ps_hash_old = !cur_prog || !cur_prog->pshader ? ~0ull : cur_prog->pshader->shader_hash;
        uint64_t ps_hash_new = new_prog->pshader ? new_prog->pshader->shader_hash : ~0ull;
        bool vs_new = !cur_prog || !cur_prog->vshader || cur_prog->vshader->shader_hash != new_prog->vshader->shader_hash;
        bool ps_new = ps_hash_old != ps_hash_new;
        if (vs_new)
          vdecl = NULL;
        if (vs_new || ps_new)
          cur_state = NULL;
      }
    }
    cur_prog = new_prog;
    using_async_pso_compilation = is_async && can_use_async_pso_compilation;
  }

  void Render::doCopyTexture(Texture* dst, Texture* src, float* src_rect, int* vp)
  {
    StateSaver saver(*this, true, true, true, false);

    StageBinding& vs_stage = stages[STAGE_VS];
    StageBinding& ps_stage = stages[STAGE_PS];

    vs_stage.setBuf(0, (Buffer*)clear_mesh_buffer);
    doSetProgram(copy_prog, false);
    setRenderTarget(dst, 0, 0);

    ps_stage.setTex(0, src);
    setConstants(vs_stage, src_rect);

    if (applyStates())
    {
      if (vp[0] != -1)
      {
        MTLViewport viewport = { (double)vp[0], (double)vp[2], (double)vp[1], (double)vp[3], 0.f, 1.f };
        [renderEncoder setViewport : viewport];
        cached_viewport.originX = -FLT_MAX;
      }

      [renderEncoder drawPrimitives : MTLPrimitiveTypeTriangleStrip
                        vertexStart : 0
                        vertexCount : 4
                      instanceCount : 1];
    }
  }

  void Render::doClear(Texture* dst, int dst_level, int dst_layer, float z, float color[4],
                       bool clear_int, bool color_write, bool depth_write)
  {
    StateSaver saver(*this, false, dst != nullptr, color_write, depth_write);

    StageBinding& vs_stage = stages[STAGE_VS];
    StageBinding& ps_stage = stages[STAGE_PS];

    vs_stage.setBuf(0, (Buffer*)clear_mesh_buffer);

    doSetProgram(clear_int ? clear_progi : clear_progf, false);
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

  void Render::doTexCopy(Texture* dst, Texture* src)
  {
    ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

    MTLOrigin origin = { 0, 0, 0 };
    MTLSize size = { 1, 1, 1 };

    G_ASSERT(dst);
    G_ASSERT(src);
    for (int i = 0; i < dst->mipLevels; i++)
    {
      size.width = max(dst->width >> i, 1);
      size.height = max(dst->height >> i, 1);

      if (dst->use_dxt)
      {
        size.width = max<unsigned>(size.width, 4u);
        size.height = max<unsigned>(size.height, 4u);
      }

      if (dst->type == RES3D_VOLTEX)
        size.depth = max(dst->depth >> i, 1);

      int count = 1;
      if (dst->type == RES3D_CUBETEX)
        count = 6;
      else
      if (dst->type == RES3D_ARRTEX)
          count = dst->depth;

      G_ASSERT(src != (Texture*)-1 || drawable_aquared);
      id<MTLTexture> src_tex = src == (Texture*)-1 ? [mainview getBackBuffer:false] : src->apiTex->texture;
      for (int j = 0; j<count; j++)
      {
        [blitEncoder copyFromTexture : src_tex
                         sourceSlice : j
                         sourceLevel : i
                        sourceOrigin : origin
                          sourceSize : size
                           toTexture : dst->apiTex->texture
                    destinationSlice : j
                    destinationLevel : i
                   destinationOrigin : origin];
      }
    }
  }

  static bool viewportChanged(const MTLViewport& viewport, const Render::Viewport& vp)
  {
    return viewport.originX != vp.x || viewport.originY != vp.y || viewport.width != vp.w
      || viewport.height != vp.h || viewport.znear != vp.minz || viewport.zfar != vp.maxz;
  }

  bool Render::applyStates()
  {
    if (need_change_rt)
      updateEncoder();

    TIME_PROFILE(applyStates);

    G_ASSERT(renderEncoder);

    if (need_prepare_depth_state)
    {
      int prev_zenable = cur_depthstate.zenable;
      int prev_depth_write_on = cur_depthstate.depth_write_on;
      int prev_stencil = cur_depthstate.stencil_enable;

      if (!rt.depth)
      {
        cur_depthstate.zenable = 0;
        cur_depthstate.depth_write_on = 0;
        cur_depthstate.stencil_enable = false;
      }

      updateDepthState();

      if (!rt.depth)
      {
        cur_depthstate.zenable = prev_zenable;
        cur_depthstate.depth_write_on = prev_depth_write_on;
        cur_depthstate.stencil_enable = prev_stencil;
      }

      [renderEncoder setDepthStencilState : cur_depthstate.depthState];
      need_prepare_depth_state = false;
    }

    if (stencil_ref_changed)
    {
      [renderEncoder setStencilReferenceValue:stencil_ref];
      stencil_ref_changed = false;
    }

    if (bias_changed)
    {
      #define MINIMUM_REPRESENTABLE_D16   2e-5
      float db = depth_bias / MINIMUM_REPRESENTABLE_D16;
      [renderEncoder setDepthBias : db slopeScale : depth_slopebias clamp : 0.f];
      bias_changed = 0;
    }

    if (rt.vp.used && viewportChanged(cached_viewport, rt.vp))
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
    rt.vp.used = false;

    if (need_set_scissor)
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
      need_set_scissor = false;
    }

    if (!cur_prog)
    {
      return false;
    }

    if (cur_state == NULL || (cur_state && !(last_rstate == cur_rstate)))
    {
        cur_state = shadersPreCache.getState(cur_prog, vdecl, cur_rstate, using_async_pso_compilation);

        last_rstate = cur_rstate;

        if (!cur_state)
        {
            return false;
        }

        [renderEncoder setRenderPipelineState : cur_state];
    }

    stages[STAGE_VS].apply(STAGE_VS, cur_prog->vshader);
    if (cur_prog->pshader)
      stages[STAGE_PS].apply(STAGE_PS, cur_prog->pshader);

    if (need_set_cull)
    {
      int cull = cur_cull;

      [renderEncoder setCullMode : (MTLCullMode)cull];

      need_set_cull = 0;
    }

    if (need_set_qeury)
    {
      if (cur_query_offset != -1)
      {
        [renderEncoder setVisibilityResultMode : MTLVisibilityResultModeCounting
                                        offset : cur_query_offset];
      }
      else
      {
        [renderEncoder setVisibilityResultMode : MTLVisibilityResultModeDisabled
                                        offset : 0];
      }

      need_set_qeury = 0;
    }

    return true;
  }

  struct CachedMSAATexture
  {
      uint32_t width = 0;
      uint32_t height = 0;
      uint32_t mips = 0;
      MTLPixelFormat format;
      id<MTLTexture> texture;
  };
  static eastl::vector<CachedMSAATexture> g_msaa_textures;

  static void fillAttachement(MTLRenderPassAttachmentDescriptor* desc, int i, Texture* tex, int level, bool msaa)
  {
    G_ASSERT(!tex || tex->apiTex);
#if _TARGET_IOS || _TARGET_TVOS
    if (msaa)
    {
      if (tex)
      {
        id<MTLTexture> texture = nil;
        for (auto & c : g_msaa_textures)
        {
          if (c.width == tex->width && c.height == tex->height && c.mips == tex->mipLevels && c.format == tex->metal_format)
          {
            texture = c.texture;
            break;
          }
        }
        if (!texture)
        {
          MTLTextureDescriptor *pTexDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : tex->metal_format
                  width : tex->width
                  height : tex->height
                  mipmapped : NO];

          pTexDesc.usage = MTLTextureUsageRenderTarget;
          pTexDesc.storageMode = MTLStorageModeMemoryless;
          pTexDesc.resourceOptions = MTLResourceStorageModeMemoryless;

          pTexDesc.mipmapLevelCount = tex->mipLevels;
          pTexDesc.textureType = MTLTextureType2DMultisample;

          pTexDesc.sampleCount = 4;

          texture = [render.device newTextureWithDescriptor : pTexDesc];
          g_msaa_textures.push_back({tex->width, tex->height, tex->mipLevels, tex->metal_format, texture});
        }

        desc.texture = texture;
        desc.level = level;

        desc.resolveTexture = tex->apiTex->rt_texture;
        desc.resolveLevel = level;
        desc.resolveSlice = 0;
      }

      desc.storeAction = MTLStoreActionMultisampleResolve;
    }
    else
#endif
    {
      desc.texture = tex ? tex->apiTex->rt_texture : nil;
      desc.storeAction = tex && (tex->cflg & TEXFMT_MASK) == TEXFMT_MSAA_MAX_SAMPLES ?  MTLStoreActionDontCare : MTLStoreActionStore;
      desc.level = level;
    }
  }

  void Render::PrepareMetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode)
  {
#if USE_METALFX_UPSCALE
    if (@available(iOS 16, macos 13, *))
    {
      if (!upscale.spatialScalerDescriptor)
        upscale.spatialScalerDescriptor = [[MTLFXSpatialScalerDescriptor alloc] init];

      bool upscaleChanged = upscale.spatialScaler == nil ||
          upscale.spatialScalerDescriptor.inputWidth != color->getWidth() ||
          upscale.spatialScalerDescriptor.inputHeight != color->getHeight() ||
          upscale.spatialScalerDescriptor.colorTextureFormat != color->metal_format ||
          upscale.spatialScalerDescriptor.outputWidth != output->getWidth() ||
          upscale.spatialScalerDescriptor.outputHeight != output->getHeight() ||
          upscale.spatialScalerDescriptor.outputTextureFormat != output->metal_format ||
          upscale.spatialScalerDescriptor.colorProcessingMode != colorMode;

      if (upscaleChanged)
      {
        upscale.spatialScalerDescriptor.inputWidth = color->getWidth();
        upscale.spatialScalerDescriptor.inputHeight = color->getHeight();
        upscale.spatialScalerDescriptor.colorTextureFormat = color->metal_format;
        upscale.spatialScalerDescriptor.outputWidth = output->getWidth();
        upscale.spatialScalerDescriptor.outputHeight = output->getHeight();
        upscale.spatialScalerDescriptor.outputTextureFormat = output->metal_format;
        upscale.spatialScalerDescriptor.colorProcessingMode = upscale.color_modes[colorMode];
        upscale.spatialScaler = [upscale.spatialScalerDescriptor newSpatialScalerWithDevice: device];
      }
    }
#endif
  }

  void Render::MetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode)
  {
#if USE_METALFX_UPSCALE
    if (@available(iOS 16, macos 13, *))
    {
      ensureHaveEncoderExceptRender(Render::EncoderType::None);

      PrepareMetalfxUpscale(color, output, colorMode);

      upscale.spatialScaler.outputTexture = output->apiTex->texture;
      upscale.spatialScaler.colorTexture = color->apiTex->texture;
      [upscale.spatialScaler encodeToCommandBuffer:commandBuffer];
    }
#endif
  }

  void Render::updateEncoder(unsigned mask, float* color, float z, uint8_t stencil)
  {
    TIME_PROFILE(updateEncoder);
    ensureHaveEncoderExceptRender(Render::EncoderType::None);

    if (rt.rt[0] == (Texture*)-1)
    {
      if (!drawable_aquared)
      {
        setupCommadBufferErrorHandler(commandBuffer);
        [commandBuffer commit];
        commandBuffer = createCommandBuffer(commandQueue, "Command buffer backbuffer");

        TIME_PROFILE(acquireDrawable);
        [mainview aquareDrawable];

        drawable_aquared = true;
      }

      cur_wd = wnd_wd;
      cur_ht = wnd_ht;
    }

    bool msaa = false;
    bool depthResolve = false;
#if _TARGET_IOS || _TARGET_TVOS
    msaa = msaa_pass;
    msaa_pass = false;
    depthResolve = depth_resolve;
    depth_resolve = false;
#endif
    int samples = -1;

    render_desc = [MTLRenderPassDescriptor renderPassDescriptor];

    bool has_color = false;
    int max_slices = -1;
    for (int i = 0; i < Program::MAX_SIMRT; i++)
    {
      MTLRenderPassColorAttachmentDescriptor* colorAttachment = render_desc.colorAttachments[i];

      colorAttachment.depthPlane = 0;
//      colorAttachment.slice = 0;
//      colorAttachment.level = 0;
      colorAttachment.texture = NULL;

      cur_rstate.pixelFormat[i] = MTLPixelFormatInvalid;

      if (!rt.rt[i])
        continue;

      if (rt.rt[i] == (Texture*)-1)
      {
        cur_rstate.pixelFormat[i] = [mainview getLayerPixelFormat];
        colorAttachment.texture = [mainview getBackBuffer : false];
//        colorAttachment.level = 0;
//        colorAttachment.slice = 0;
        samples = 1;
      }
      else
      {
        fillAttachement(colorAttachment, i, rt.rt[i], rt.rt_levels[i], msaa);

        cur_rstate.pixelFormat[i] = rt.rt[i]->metal_rt_format;
        samples = colorAttachment.texture ? colorAttachment.texture.sampleCount : samples;
//        colorAttachment.level = 0;
        uint32_t layer = rt.rt_layers[i];
        if (layer == d3d::RENDER_TO_WHOLE_ARRAY && rt.rt[i]->type != RES3D_TEX)
        {
          G_ASSERT(rt.rt[i]->type != RES3D_CUBEARRTEX);
          int max_layers = rt.rt[i]->type == RES3D_CUBETEX ? 6 : rt.rt[i]->depth;
          G_ASSERT(max_slices == -1 || max_slices == max_layers);
          max_slices = max_layers;
          layer = 0;
        }
        if (rt.rt[i]->type == RES3D_VOLTEX)
          colorAttachment.depthPlane = layer;
        else if (rt.rt[i]->type == RES3D_CUBETEX || rt.rt[i]->type == RES3D_ARRTEX)
          colorAttachment.slice = layer;

        if (i == 0)
        {
          cur_wd = max(1, rt.rt[i]->width>>colorAttachment.level);
          cur_ht = max(1, rt.rt[i]->height>>colorAttachment.level);
        }
      }

      if (mask & CLEAR_TARGET)
      {
        G_ASSERT(color);
        colorAttachment.loadAction = MTLLoadActionClear;
        colorAttachment.clearColor = MTLClearColorMake(color[0], color[1], color[2], color[3]);
      }
      else
      {
        colorAttachment.loadAction = mask & CLEAR_DISCARD_TARGET ? MTLLoadActionDontCare : MTLLoadActionLoad;
      }
      has_color = true;
    }

    if (!has_color && !rt.depth)
    {
      G_ASSERT(rt.vp.used);
      G_ASSERT(rt.vp.w > 0);
      G_ASSERT(rt.vp.h > 0);

      samples = 1;
      render_desc.renderTargetWidth = rt.vp.x + rt.vp.w;
      render_desc.renderTargetHeight = rt.vp.y + rt.vp.h;
      render_desc.defaultRasterSampleCount = samples;
    }

    if (rt.depth)
    {
      if (rt.depth == (Texture*)-1)
      {
        render_desc.depthAttachment.slice = 0;
        render_desc.depthAttachment.texture = [mainview getDepthBuffer];
        render_desc.stencilAttachment.texture = NULL;
        cur_rstate.depthFormat = mainview.depthPixelFormat;
        cur_rstate.stencilFormat = MTLPixelFormatInvalid;

        if (!rt.rt[0])
        {
          cur_wd = scr_wd;
          cur_ht = scr_ht;
        }
      }
      else
      {
        G_ASSERT(rt.depth->apiTex);
        cur_rstate.depthFormat = rt.depth->metal_format;
        cur_rstate.stencilFormat = rt.depth->metal_format == MTLPixelFormatDepth32Float_Stencil8 ? rt.depth->metal_format : MTLPixelFormatInvalid;

        fillAttachement(render_desc.depthAttachment, -1, rt.depth, 0, msaa);
        fillAttachement(render_desc.stencilAttachment, -1, cur_rstate.stencilFormat != MTLPixelFormatInvalid ? rt.depth : nullptr, 0, msaa);

        G_ASSERT(samples == -1 || samples == render_desc.depthAttachment.texture.sampleCount);

        render_desc.depthAttachment.slice = 0;
        render_desc.stencilAttachment.slice = 0;

        samples = render_desc.depthAttachment.texture.sampleCount;

        G_ASSERT(rt.depth->type != RES3D_CUBEARRTEX && rt.depth->type != RES3D_VOLTEX);
        if (rt.depth->type == RES3D_CUBETEX || rt.depth->type == RES3D_ARRTEX)
        {
          uint32_t layer = rt.depth_layer;
          if (layer == d3d::RENDER_TO_WHOLE_ARRAY)
          {
            int max_layers = rt.depth->type == RES3D_CUBETEX ? 6 : rt.depth->depth;
            G_ASSERT(max_slices == -1 || max_slices == max_layers);
            max_slices = max_layers;
            layer = 0;
          }

          render_desc.depthAttachment.slice = layer;
          if (cur_rstate.stencilFormat != MTLPixelFormatInvalid)
            render_desc.stencilAttachment.slice = layer;
        }

        if (!rt.rt[0])
        {
          cur_wd = rt.depth->width;
          cur_ht = rt.depth->height;
        }
      }
    }
    else
    {
      render_desc.depthAttachment.texture = NULL;
      render_desc.stencilAttachment.texture = NULL;
      cur_rstate.depthFormat = MTLPixelFormatInvalid;
      cur_rstate.stencilFormat = MTLPixelFormatInvalid;
    }

    G_ASSERT(samples > 0);
    cur_rstate.sample_count = samples;
    cur_rstate.is_volume = max_slices > 0;
    render_desc.renderTargetArrayLength = max_slices > 0 ? max_slices : 0;

#if _TARGET_IOS || _TARGET_TVOS
    bool memoryless = rt.depth ? (rt.depth == (Texture*)-1 ? true : rt.depth->memoryless) : false;
#else
    bool memoryless = rt.depth && rt.depth != (Texture*)-1 ? rt.depth->memoryless : false;
#endif
    if (!msaa)
    {
      render_desc.depthAttachment.storeAction = memoryless || rt.depth_readonly ? MTLStoreActionDontCare : MTLStoreActionStore;
      render_desc.stencilAttachment.storeAction = memoryless || rt.depth_readonly ? MTLStoreActionDontCare : MTLStoreActionStore;
    }
    else if (msaa)
    {
      if (depthResolve)
      {
        render_desc.depthAttachment.resolveTexture = rt.depth->apiTex->texture;
        render_desc.depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
        // By using min filter we select the most far subsample.
        // This allows to override such pixels with overlapping VFX in the future,
        // resulting in better quality (no 1px border around opaque objects).
        render_desc.depthAttachment.depthResolveFilter = MTLMultisampleDepthResolveFilterMin;
      }
      else
      {
        render_desc.depthAttachment.resolveTexture = nil;
        render_desc.depthAttachment.storeAction = MTLStoreActionDontCare;
      }
      render_desc.stencilAttachment.resolveTexture = nil;
      render_desc.stencilAttachment.storeAction = MTLStoreActionDontCare;
    }

    if (mask & CLEAR_ZBUFFER)
    {
      render_desc.depthAttachment.loadAction = MTLLoadActionClear;
      render_desc.depthAttachment.clearDepth = z;
      render_desc.stencilAttachment.loadAction = MTLLoadActionClear;
      render_desc.stencilAttachment.clearStencil = stencil;
    }
    else
    {
      render_desc.depthAttachment.loadAction = memoryless || mask & CLEAR_DISCARD_ZBUFFER
        ? MTLLoadActionDontCare : MTLLoadActionLoad;
      render_desc.stencilAttachment.loadAction = memoryless || mask & CLEAR_DISCARD_STENCIL
        ? MTLLoadActionDontCare : MTLLoadActionLoad;
    }

    render_desc.visibilityResultBuffer = query_buffer;

    renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor : render_desc];
#if DAGOR_DBGLEVEL > 0
    NSString* label = render_desc.colorAttachments[0].texture ? render_desc.colorAttachments[0].texture.label : @"DagorRenderEncoder";
    renderEncoder.label = label;
#endif

    stages[STAGE_VS].reset();
    stages[STAGE_PS].reset();

    need_prepare_depth_state = true;

    cur_state = NULL;

    [renderEncoder setDepthClipMode : (MTLDepthClipMode)(1 - depth_clip)];

    bias_changed = (fabs(depth_bias) > 0.00000001f && fabs(depth_slopebias) > 0.00000001f) ? 1 : 0;
    need_set_cull = cur_cull ? 1 : 0;
    need_set_qeury = (cur_query_offset != -1) ? 1 : 0;

    if (scissor_on)
    {
      need_set_scissor = true;
    }

    need_change_rt = 0;

    cached_viewport.originX = -FLT_MAX;
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

  void Render::aquareOwnerShip()
  {
    ::enter_critical_section(aquareSec);

    if (acquare_depth == 0)
    {
      pthread_threadid_np(NULL, &cur_thread);
    }

    acquare_depth++;
  }

  void Render::releaseOwnerShip()
  {
    G_ASSERT(acquare_depth > 0);
    acquare_depth--;

    if (acquare_depth == 0)
    {
      cur_thread = main_thread;
    }
    ::leave_critical_section(aquareSec);
  }

  int Render::tryReleaseOwnerShip()
  {
    if (is_main_thread())
      return 0;
    if (try_enter_critical_section(aquareSec))
    {
      G_ASSERT(acquare_depth <= 1);
      acquare_depth = 0;
      cur_thread = main_thread;
      return ::full_leave_critical_section(aquareSec);
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
        fatal("Count of used queries was exceeded of max available value");
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
    query->value = render.frame;
    query->status = 2;
  }

  void Render::startQuery(Query* query)
  {
    command_buffer->push<CommandSetQuery>(query);
    query->status = 1;
  }

  void Render::finishQuery(Query* query)
  {
    command_buffer->push<CommandSetQuery>(nullptr);
    query->status = 2;
  }

  void Render::setMSAAPass()
  {
    command_buffer->push<CommandSetMSAAPass>();
  }

  void Render::setDepthResolve()
  {
    command_buffer->push<CommandSetDepthResolve>();
  }

  uint64_t Render::getQueryResult(Query* query, bool force_flush)
  {
    if (query->status != 2)
      return 1;
    if (render.frame - query->value < MAX_FRAMES_TO_RENDER && force_flush)
    {
      @autoreleasepool
      {
        render.flush(true);
      }
    }
    return render.frame - query->value < MAX_FRAMES_TO_RENDER && !force_flush ? -1 : 1;
  }

  void Render::releaseQuery(Query* query)
  {
    query->used = 0;
  }

  void Render::release()
  {
    inited = false;

    shadersPreCache.release();

    for (int i = 0; i < 5; ++i)
    {
      blank_tex[i]->destroy();
      blank_tex_rw[i]->destroy();
    }

    clear_mesh_buffer->destroy();

    flush(true);

    [tmp_copy_buff release];
    [copy_cs_pipeline release];
    [clear_cs_pipeline release];
    deleteVertexShader(clear_vshd);
    deleteVertexShader(copy_vshd);
    deletePixelShader(clear_pshdi);
    deletePixelShader(clear_pshdf);
    deletePixelShader(copy_pshd);
    deleteProgram(clear_progi);
    deleteProgram(clear_progf);
    deleteProgram(copy_prog);
    deleteVDecl(clear_vdecl);
    ::destroy_critical_section(aquareSec);
    ::destroy_critical_section(rcSec);
    [query_buffer release];
    [stub_buffer release];

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
      if (@available(iOS 15.0, macos 11.0, *))
      {
        G_ASSERT(res.acceleration_structures.empty());
      }
      G_ASSERT(res.tlases.empty());
      G_ASSERT(res.blases.empty());
    }
    G_ASSERT(textures2upload.empty());
    G_ASSERT(buffers2copy.empty());

    [commandQueue release];

    for (int i = 0; i < 3; i++)
    {
      StageBinding& stg = stages[i];
      stg.cbuffer.destroy();
    }

    Buffer::cleanup();
    Texture::cleanup();

    [bottomLevelAccelerationStructures release];
    [accelerationStructureScratchBuffer release];
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
    command_buffer->push<CommandReadbackTex>(tex, layer, level, w, h, d, buf, offset, pitch, imageSize);
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
    command_buffer->push<CommandDeleteNativeBuffer>(buf);
  }

  void Render::queueTextureForDeletion(id<MTLTexture> tex)
  {
    command_buffer->push<CommandDeleteNativeTexture>(tex);
  }

  void Render::deleteBuffer(Buffer* buff)
  {
    command_buffer->push<CommandDeleteBuffer>(buff);
  }

  void Render::deleteTexture(Texture* tex)
  {
    if (tex->isStub())
      tex->apiTex = nullptr; // hack?
    command_buffer->push<CommandDeleteTexture>(tex);
  }

  void Render::flushTexture(Texture* tex)
  {
    id<MTLTexture> mtlTex = tex->apiTex ? tex->apiTex->texture : nil;
    for (uint32_t i = 0; i < STAGE_MAX; ++i)
    {
      for (uint32_t j = 0; j < MAX_SHADER_TEXTURES*2; ++j)
      {
        if (stages[i].textures[j] == tex)
          stages[i].textures[j] = nullptr;
        if (stages[i].textures_metal[j] == mtlTex)
          stages[i].textures_metal[j] = nil;
      }
    }
    for (uint32_t i = 0; i < Program::MAX_SIMRT; ++i)
    {
      if (rt.rt[i] == tex)
        rt.rt[i] = nullptr;
    }
    if (rt.depth == tex)
      rt.depth = nullptr;
  }

  void Render::queueCopyBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size)
  {
    std::lock_guard<std::mutex> scopedLock(copy_tex_lock);
    buffers2copy.push_back({ src, dst, src_offset, dst_offset, size });
  }

  void Render::queueUpdateBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size)
  {
    command_buffer->push<CommandUpdateBuffer>(dst, dst_offset, src, src_offset, size);
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
      rstate.depth_state.stencil_func = state.stencil.func-1;
      rstate.depth_state.stencil_read_mask = state.stencil.readMask;
      rstate.depth_state.stencil_write_mask = state.stencil.writeMask;
      rstate.depth_state.sfail = state.stencil.fail;
      rstate.depth_state.szfail = state.stencil.zFail;
      rstate.depth_state.spass = state.stencil.pass;
    }
    rstate.depth_state.zenable = state.ztest;
    rstate.depth_state.depth_write_on = state.zwrite;
    rstate.depth_state.zfunc = state.zFunc-1;

    rstate.depth_bias = state.zBias;
    rstate.depth_slopebias = state.slopeZBias;

    rstate.depth_clip = state.zClip;
    rstate.stencil_ref = state.stencilRef;
    rstate.cull = state.cull-1;

    rstate.raster_state.a2c = state.alphaToCoverage;
    rstate.raster_state.writeMask = state.colorWr;
    for (int i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
    {
      rstate.raster_state.blend[i].ablend = state.blendParams[i].ablend;
      rstate.raster_state.blend[i].ablendScr = convBlendArg(state.blendParams[i].sepablendFactors.src);
      rstate.raster_state.blend[i].ablendDst = convBlendArg(state.blendParams[i].sepablendFactors.dst);
      rstate.raster_state.blend[i].ablendOp = state.blendParams[i].sepablendOp-1;
      rstate.raster_state.blend[i].sepblend = state.blendParams[i].sepablend;
      rstate.raster_state.blend[i].rgbblendScr = convBlendArg(state.blendParams[i].ablendFactors.src);
      rstate.raster_state.blend[i].rgbblendDst = convBlendArg(state.blendParams[i].ablendFactors.dst);
      rstate.raster_state.blend[i].rgbblendOp = state.blendParams[i].blendOp-1;
    }


    return render_states.emplaceOne(rstate);
  }

  bool Render::setRenderState(shaders::DriverRenderStateId id)
  {
    command_buffer->push<CommandSetState>(id);

    return true;
  }

  bool Render::setStencilRef(uint32_t ref)
  {
    command_buffer->push<CommandStencilRef>(ref);

    return true;
  }

  void Render::clearRenderStates()
  {
    render_states.clear();
  }

  void Render::beginEvent(const char *name)
  {
    command_buffer->push<CommandBeginEvent>(name);
  }

  void Render::endEvent()
  {
    command_buffer->push<CommandEndEvent>();
  }

  uint64_t Render::getGPUTimeStampFreq()
  {
    MTLTimestamp cpuTimestamp, gpuTimestamp;
    [device sampleTimestamps: &cpuTimestamp gpuTimestamp: &gpuTimestamp];
    return gpuTimestamp;
  }

  void Render::addBLASStubToList()
  {
    [bottomLevelAccelerationStructures addObject:[NSNull null]];
  }

  void Render::deleteTLAS(RaytraceTopAccelerationStructure *tlas)
  {
    std::lock_guard<std::mutex> scopedLock(tlas_lock);
    auto search = eastl::find_if(topStructs.begin(), topStructs.end(), [tlas](const auto &as) { return as.get() == tlas; });
    const bool isPresent = search != topStructs.end();
    if (!isPresent)
    {
      logerr("Trying to delete unrecognised tlas");
      return;
    }

    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.tlases.push_back(eastl::move(*search));
    }

    topStructs.erase(search);
  }

  void Render::deleteBLAS(RaytraceBottomAccelerationStructure *botAccStruct)
  {
    std::lock_guard<std::mutex> scopedLock(blas_lock);
    auto search = eastl::find_if(bottomStructs.begin(), bottomStructs.end(), [botAccStruct](const auto &blas) { return blas.get() == botAccStruct; });
    const bool isPresent = search != bottomStructs.end();
    if (!isPresent)
    {
      debug("Trying to delete unrecognised blas");
      return;
    }

    const int deleteAtIndex = (*search)->index;
    for (auto it = search + 1; it != bottomStructs.end(); ++it)
      (*it)->index--;

    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.blases.push_back(eastl::move(*search));
    }

    bottomStructs.erase(search);

    command_buffer->push<CommandRemoveBLASFromList>(deleteAtIndex);
  }

  void Render::removeBLASFromList(int delete_index)
  {
    [bottomLevelAccelerationStructures removeObjectAtIndex: delete_index];
  }

  void Render::scheduleTLASSet(RaytraceTopAccelerationStructure *tlas, ShaderStage stage, int slot)
  {
    command_buffer->push<CommandSetTLAS>(tlas, stage, slot);
  }

  RaytraceTopAccelerationStructure *Render::createTopAccelerationStructure(RaytraceBuildFlags flags)
  {
    std::lock_guard<std::mutex> scopedLock(tlas_lock);
    return topStructs.emplace_back(new RaytraceTopAccelerationStructure{ nil, flags }).get();
  }

  RaytraceBottomAccelerationStructure *Render::createBottomAccelerationStructure(RaytraceBuildFlags flags)
  {
    std::lock_guard<std::mutex> scopedLock(blas_lock);
    // Add Null struct to bottomLevelAccelerationStructures
    command_buffer->push<CommandAddBLASToList>();
    return bottomStructs.emplace_back(new RaytraceBottomAccelerationStructure{ nil , bottomStructs.size(), flags }).get();
  }

  void Render::adjustASScratchBuffer(NSUInteger required_size)
  {
    if (accelerationStructureScratchBuffer == nil)
    {
      accelerationStructureScratchBuffer = [render.device newBufferWithLength: required_size
                                                                      options: MTLResourceStorageModePrivate];
      return;
    }

    if (required_size > [accelerationStructureScratchBuffer length])
    {
      {
        std::lock_guard<std::mutex> scopedLock(delete_lock);
        resources2delete.native_resources.push_back(accelerationStructureScratchBuffer);
      }
      constexpr float IncreaseFactor = 1.5;
      accelerationStructureScratchBuffer = [render.device newBufferWithLength: required_size * IncreaseFactor
                                                                      options: MTLResourceStorageModePrivate];
    }
  }

  void Render::createAccelerationStructure(id<MTLAccelerationStructure> &as, MTLAccelerationStructureDescriptor *desc, bool refit)
  {
    MTLAccelerationStructureSizes sizes = [render.device accelerationStructureSizesWithDescriptor: desc];

    ensureHaveEncoderExceptRender(Render::EncoderType::Acceleration);

    if (refit)
    {
      // According to documentation refit can be done in place with destination = nil or source,
      // but for unknown reason it causes a crash
      adjustASScratchBuffer(sizes.refitScratchBufferSize);
      id<MTLAccelerationStructure> accStruct = [render.device newAccelerationStructureWithSize: sizes.accelerationStructureSize];
      [accelerationEncoder refitAccelerationStructure: as
                                           descriptor: desc
                                          destination: accStruct
                                        scratchBuffer: accelerationStructureScratchBuffer
                                  scratchBufferOffset: 0];
      as = accStruct;
    }
    else
    {
      adjustASScratchBuffer(sizes.buildScratchBufferSize);
      as = [render.device newAccelerationStructureWithSize: sizes.accelerationStructureSize];
      [accelerationEncoder buildAccelerationStructure: as
                                           descriptor: desc
                                        scratchBuffer: accelerationStructureScratchBuffer
                                  scratchBufferOffset: 0];
    }

    // Here struct compression can be done to reduce the size
    // but it is really performance heavy (requires a flush ([commandBuffer waitUntilCompleted]))
    // and doing it every frame is too much
  }

  void Render::scheduleTLASBuild(RaytraceTopAccelerationStructure *as, MTLInstanceAccelerationStructureDescriptor *desc, bool update)
  {
    command_buffer->push<CommandBuildTLAS>(as, desc, update);
  }

  void Render::buildTLAS(RaytraceTopAccelerationStructure *tlas, MTLInstanceAccelerationStructureDescriptor *desc, bool update)
  {
    if (tlas->instanceAccelerationStructure != nil)
    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.acceleration_structures.push_back(tlas->instanceAccelerationStructure);
    }

    desc.instancedAccelerationStructures = bottomLevelAccelerationStructures;

    createAccelerationStructure(tlas->instanceAccelerationStructure, desc, update);
  }

  void Render::scheduleBLASBuild(RaytraceBottomAccelerationStructure *blas, MTLPrimitiveAccelerationStructureDescriptor *desc, bool update)
  {
    command_buffer->push<CommandBuildBLAS>(blas, desc, update);
  }

  void Render::buildBLAS(RaytraceBottomAccelerationStructure *blas, MTLPrimitiveAccelerationStructureDescriptor *desc, bool update)
  {
    const NSUInteger previousIndex = [bottomLevelAccelerationStructures indexOfObject: blas->primitiveAccelerationStructure];
    const bool wasPresentPreviously = previousIndex != NSNotFound;

    if (wasPresentPreviously)
    {
      std::lock_guard<std::mutex> scopedLock(delete_lock);
      resources2delete.acceleration_structures.push_back(blas->primitiveAccelerationStructure);
    }

    createAccelerationStructure(blas->primitiveAccelerationStructure, desc, update);

    // If blas is new then NSNull in bottomLevelAccelerationStructures is replaced,
    // otherwise previous blas is replaced
    const NSUInteger replaceIndex = wasPresentPreviously ? previousIndex : blas->index;
    [bottomLevelAccelerationStructures replaceObjectAtIndex: replaceIndex
                                                  withObject: blas->primitiveAccelerationStructure];
  }
}
