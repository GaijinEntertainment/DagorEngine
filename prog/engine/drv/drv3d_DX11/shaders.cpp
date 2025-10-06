// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_critSec.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <EASTL/unique_ptr.h>

#include "driver.h"

#include <image/dag_texPixel.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_asyncIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>

#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <util/dag_string.h>
#include <3d/parseShaders.h>
#include <util/dag_globDef.h>

#include <debug/dag_except.h>

#include <d3d11_1.h>

#define SHOW_DISASM   0 // show shader disassambled in create_shader
#define SHOW_SETCONST 0 // trace shader constants setting

#define D3DSPR(r) ((((r) << D3DSP_REGTYPE_SHIFT) & D3DSP_REGTYPE_MASK) | (((r) << D3DSP_REGTYPE_SHIFT2) & D3DSP_REGTYPE_MASK2))

using namespace drv3d_dx11;

#if SHOW_DISASM
static void show_disasm(const void *shader_bin, uint32_t size, const char *prefix, int max_const)
{
  ID3D10Blob *disasm = NULL;
  D3DDisassemble((DWORD *)shader_bin, size, 0, NULL, &disasm);
  if (disasm)
  {
    debug("%s disasm (%p,%d max_const=%d):\n%.*s", prefix, shader_bin, size, max_const, disasm->GetBufferSize(),
      disasm->GetBufferPointer());
    disasm->Release();
  }
}
#endif

namespace drv3d_dx11
{
#include "shaderSource.inc.cpp"

static ObjectPoolWithLock<PixelShader> g_pixel_shaders("pixel_shaders");
static ObjectPoolWithLock<VertexShader> g_vertex_shaders("vertex_shaders");
static ObjectPoolWithLock<ComputeShader, 32> g_compute_shaders("compute_shaders");
static ObjectPoolWithLock<Program, 256> g_programs("programs");
static ObjectPoolWithLock<ObjectProxyPtr<InputLayout>> g_input_layouts("input_layouts");
void get_shaders_mem_used(String &str)
{
  str.printf(128, "g_pixel_shaders.size() =%d\n", g_pixel_shaders.size());
  str.aprintf(128, "g_vertex_shaders.size() =%d\n", g_vertex_shaders.size());
  str.aprintf(128, "g_compute_shaders.size() =%d\n", g_compute_shaders.size());
  str.aprintf(128, "g_programs.size() =%d\n", g_programs.size());
  str.aprintf(128, "g_input_layouts.size() =%d\n", g_input_layouts.size());
  int maxIl = 0, totalIl = 0;
  ITERATE_OVER_OBJECT_POOL(g_vertex_shaders, i)
    if (g_vertex_shaders[i].ilCache)
    {
      totalIl += (int)g_vertex_shaders[i].ilCache->size();
      maxIl = max(maxIl, (int)g_vertex_shaders[i].ilCache->size());
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_vertex_shaders);
  str.aprintf(128, "g_vertex_shaders ilcache max=%d, avg = %f\n", maxIl, float(totalIl) / g_vertex_shaders.size());
}

void clear_pools_garbage()
{
  g_pixel_shaders.clearGarbage();
  g_vertex_shaders.clearGarbage();
  g_compute_shaders.clearGarbage();
  g_programs.clearGarbage();
  g_input_layouts.clearGarbage();
  clear_buffers_pool_garbage();
  clear_textures_pool_garbage();
}

FSHADER g_default_copy_ps = BAD_FSHADER;
VPROG g_default_copy_vs = BAD_VPROG;
VDECL g_default_pos_vdecl = BAD_VDECL;

FSHADER g_default_clear_ps = BAD_FSHADER;
VPROG g_default_clear_vs = BAD_VPROG;
VDECL g_default_clear_vdecl = BAD_VDECL;

FSHADER g_default_debug_ps = BAD_FSHADER;
VPROG g_default_debug_vs = BAD_VPROG;
VDECL g_default_debug_vdecl = BAD_VDECL;
PROGRAM g_default_debug_program = BAD_PROGRAM;

d3d::SamplerHandle g_default_clamp_sampler = d3d::INVALID_SAMPLER_HANDLE;

int ConstantBuffers::getVsConstBufferId(int required_size)
{
  for (int i = 0; i < g_vsbin_sizes_count; ++i)
  {
    if (g_vsbin_sizes[i] >= required_size)
      return i;
  }

  return g_vsbin_sizes_count - 1;
}

void ConstantBuffers::create()
{
  vsConstsUsed = psConstsUsed = csConstsUsed = 0;
  mem_set_0(constsRequired);
  vsCurrentBuffer = -1;

  D3D11_BUFFER_DESC desc;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  {
    G_STATIC_ASSERT(MAX_PS_CONSTS <= MIN_PS_CONSTS + PS_CONSTS_STEP * (PS_BINS - 1));
    for (int i = 0; i < PS_BINS; ++i)
    {
      desc.ByteWidth = (MIN_PS_CONSTS + PS_CONSTS_STEP * i) * sizeof(vec4f);
      DXFATAL(dx_device->CreateBuffer(&desc, NULL, &psConstBuffer[i]), "ps ConstBuffer");
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
      if (psConstBuffer[i])
        psConstBuffer[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (int)strlen("PS_CONST_BUFFER"), "PS_CONST_BUFFER");
#endif
    }

    for (int i = 0; i < g_vsbin_sizes_count; ++i)
      vsConstBuffer[i] = NULL;

    mem_set_0(csConstBuffer);
    desc.ByteWidth = DEF_CS_CONSTS * sizeof(vec4f);
    DXFATAL(dx_device->CreateBuffer(&desc, NULL, &csConstBuffer[0]), "cs ConstBuffer");
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
    if (csConstBuffer[0])
      csConstBuffer[0]->SetPrivateData(WKPDID_D3DDebugObjectName, (int)strlen("CS_CONST_BUFFER0"), "CS_CONST_BUFFER0");
#endif
    csCurrentBuffer = 0;
  }
  {
    ContextAutoLock contextLock;
    dx_context->PSSetConstantBuffers(0, 1, &psConstBuffer[psCurrentBuffer = 0]);
    dx_context->CSSetConstantBuffers(0, 1, &csConstBuffer[csCurrentBuffer]);
  }
};

static ID3D11Buffer *create_const_buffer(int size, const char *name)
{
  D3D11_BUFFER_DESC desc;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;
  desc.ByteWidth = size * sizeof(vec4f);
  ID3D11Buffer *buffer = 0;

  DXFATAL(dx_device->CreateBuffer(&desc, NULL, &buffer), "cbuffer: %s", name);
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  if (buffer)
    buffer->SetPrivateData(WKPDID_D3DDebugObjectName, (int)strlen(name), name);
#endif

  return buffer;
}

ID3D11Buffer *ConstantBuffers::createVsBuffer(int id)
{
  D3D_CONTRACT_ASSERT(id >= 0 && id < g_vsbin_sizes_count);
  D3D_CONTRACT_ASSERT(vsConstBuffer[id] == NULL);

  int size = g_vsbin_sizes[id];
  debug("creating vsConstBuffer, id:%d, size:%d", id, size);
  vsConstBuffer[id] = create_const_buffer(size, "VS_CONST_BUFFER");
  return vsConstBuffer[id];
}

void ConstantBuffers::destroy()
{
  for (int i = 0; i < csConstBuffer.size(); ++i)
    if (csConstBuffer[i])
    {
      csConstBuffer[i]->Release();
      csConstBuffer[i] = NULL;
    }
  for (int i = 0; i < psConstBuffer.size(); ++i)
    if (psConstBuffer[i])
    {
      psConstBuffer[i]->Release();
      psConstBuffer[i] = NULL;
    }
  for (int i = 0; i < vsConstBuffer.size(); ++i)
    if (vsConstBuffer[i])
    {
      vsConstBuffer[i]->Release();
      vsConstBuffer[i] = NULL;
    }

  psCurrentBuffer = vsCurrentBuffer = csCurrentBuffer = 0;
  mem_set_0(csConstBuffer);
  g_render_state.modified = true;
  mem_set_ff(constantsModified);
  mem_set_0(constantsBufferChanged);
  mem_set_0(customBuffers);
  vsConstsUsed = psConstsUsed = csConstsUsed = 0;
}

static ID3D11Buffer *nullCbuf = 0;
#define SET_CONST_BUFFERS(_ST, _START, _NUMS, _BUFFERS, _OFFSETS, _SIZES)                         \
  if (g_device_desc.caps.hasConstBufferOffset && needOffsets)                                     \
  {                                                                                               \
    if (command_list_wa)                                                                          \
    {                                                                                             \
      dx_context->##_ST##SetConstantBuffers((_START), 1, &nullCbuf);                              \
      if ((_NUMS) > 1)                                                                            \
        dx_context->##_ST##SetConstantBuffers((_START) + (_NUMS) - 1, 1, &nullCbuf);              \
    }                                                                                             \
    dx_context1->##_ST##SetConstantBuffers1((_START), (_NUMS), (_BUFFERS), (_OFFSETS), (_SIZES)); \
  }                                                                                               \
  else                                                                                            \
    dx_context->##_ST##SetConstantBuffers((_START), (_NUMS), (_BUFFERS));

void ConstantBuffers::flush(uint32_t vsc, uint32_t psc, uint32_t hdg_bits)
{
  if (!constantsBufferChanged[STAGE_VS] && !constantsBufferChanged[STAGE_PS] && !constantsModified[STAGE_VS] &&
      !constantsModified[STAGE_PS] && vsConstsUsed >= vsc && psConstsUsed >= psc && lastHDGBitsApplied == hdg_bits)
    return;

  D3D11_MAPPED_SUBRESOURCE msr;
  if (!customBuffers[STAGE_PS][0])
  {
    if (psc)
    {
      if (constantsModified[STAGE_PS] || psc > psConstsUsed)
      {
        constantsModified[STAGE_PS] = false;
        msr.pData = NULL;
        int buffer = int(psc - MIN_PS_CONSTS + PS_CONSTS_STEP - 1) / PS_CONSTS_STEP;
        G_ASSERT(buffer < PS_BINS);
        buffer = min(int(PS_BINS) - 1, buffer);
        ID3D11Buffer *psConstBufferToMap = psConstBuffer[buffer];
        ContextAutoLock contextLock;
        HRESULT hr = dx_context->Map(psConstBufferToMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        if (SUCCEEDED(hr))
        {
          psConstsUsed = psc;
          if (msr.pData)
            memcpy(msr.pData, psConsts, psc * sizeof(vec4f));
          dx_context->Unmap(psConstBufferToMap, 0);
          if (psCurrentBuffer != buffer)
            dx_context->PSSetConstantBuffers(0, 1, &psConstBufferToMap);
          psCurrentBuffer = buffer;
        }
      }
    }
  }
  if (!customBuffers[STAGE_VS][0])
  {
    if (vsc)
    {
      if (constantsModified[STAGE_VS] || vsc > vsConstsUsed || lastHDGBitsApplied != hdg_bits)
      {
        constantsModified[STAGE_VS] = false;
        int buffer = getVsConstBufferId(vsc);

        ID3D11Buffer *vsConstBufferToMap = vsConstBuffer[buffer];
        if (!vsConstBufferToMap)
          vsConstBufferToMap = createVsBuffer(buffer);

        msr.pData = NULL;
        ContextAutoLock contextLock;
        HRESULT hr = dx_context->Map(vsConstBufferToMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        if (SUCCEEDED(hr))
        {
          vsConstsUsed = vsc;
          if (msr.pData)
            memcpy(msr.pData, vsConsts, vsc * sizeof(vec4f));
          dx_context->Unmap(vsConstBufferToMap, 0);
          if (vsCurrentBuffer != buffer || lastHDGBitsApplied != hdg_bits)
          {
            if (hdg_bits & HAS_HS)
              dx_context->HSSetConstantBuffers(0, 1, &vsConstBufferToMap);
            if (hdg_bits & HAS_DS)
              dx_context->DSSetConstantBuffers(0, 1, &vsConstBufferToMap);
            if (hdg_bits & HAS_GS)
              dx_context->GSSetConstantBuffers(0, 1, &vsConstBufferToMap);
          }
          if (vsCurrentBuffer != buffer)
          {
            vsCurrentBuffer = buffer;
            dx_context->VSSetConstantBuffers(0, 1, &vsConstBufferToMap);
          }
        }
        lastHDGBitsApplied = hdg_bits;
      }
    }
  }

  for (int stage = STAGE_CS + 1; stage < STAGE_MAX; ++stage)
  {
    if (!constantsBufferChanged[stage])
      continue;
    carray<ID3D11Buffer *, MAX_CONST_BUFFERS> constBuffers;
    carray<uint32_t, MAX_CONST_BUFFERS> cbOffsets;
    carray<uint32_t, MAX_CONST_BUFFERS> cbSizes;
    mem_copy_from(constBuffers, customBuffers[stage].data());
    mem_copy_from(cbOffsets, buffersFirstConstants[stage].data());
    mem_copy_from(cbSizes, buffersNumConstants[stage].data());

    int slotsToSet = customBuffersMaxSlotUsed[stage] + 1, nextMaxUsedSlot = 0;
    bool needOffsets = false;
    for (int slotI = 1; slotI < slotsToSet; ++slotI) //&& customBuffers[stage][maxUsedSlot]
    {
      if (constBuffers[slotI])
        nextMaxUsedSlot = slotI;
      needOffsets |= cbOffsets[slotI] != 0;
    }

    ContextAutoLock contextLock;
    switch (stage)
    {
      case STAGE_VS:
        if (!constBuffers[0] && vsCurrentBuffer >= 0)
        {
          constBuffers[0] = vsConstBuffer[vsCurrentBuffer];
          cbOffsets[0] = 0;
          cbSizes[0] = 4096;
        }
        SET_CONST_BUFFERS(VS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
        if (hdg_bits & HAS_HS)
        {
          SET_CONST_BUFFERS(HS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
        }
        if (hdg_bits & HAS_DS)
        {
          SET_CONST_BUFFERS(DS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
        }
        if (hdg_bits & HAS_GS)
        {
          SET_CONST_BUFFERS(GS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
        }
        break;
      case STAGE_PS:
        if (!constBuffers[0])
        {
          constBuffers[0] = psConstBuffer[psCurrentBuffer];
          cbOffsets[0] = 0;
          cbSizes[0] = 4096;
        }

        SET_CONST_BUFFERS(PS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
        break;
      default: G_ASSERT(0);
    }
    customBuffersMaxSlotUsed[stage] = nextMaxUsedSlot;
    constantsBufferChanged[stage] = 0;
  }
};

void ConstantBuffers::flush_cs(uint32_t csc, bool async)
{
  bool &constsModified = async ? constantsModified[STAGE_CS_ASYNC_STATE] : constantsModified[STAGE_CS];
  if (!constantsBufferChanged[STAGE_CS] && !constsModified && csConstsUsed >= csc)
    return;

  const int stage = async ? (int)STAGE_CS_ASYNC_STATE : (int)STAGE_CS; // Buffers are common, but update flags are separate.
  D3D11_MAPPED_SUBRESOURCE msr;

  if (!customBuffers[STAGE_CS][0])
  {
    if (csc)
    {
      if (constsModified || csc > csConstsUsed)
      {
        int nextBuffer = clamp(get_bigger_log2(csc) - DEF_CS_CONSTS_SHIFT, 0u, csConstBuffer.size() - 1u);
        if (!csConstBuffer[nextBuffer])
          csConstBuffer[nextBuffer] = create_const_buffer((DEF_CS_CONSTS << nextBuffer), "CS_CONST_BUFFER");

        constsModified = false;
        msr.pData = NULL;
        ContextAutoLock contextLock;
        HRESULT hr = dx_context->Map(csConstBuffer[nextBuffer], 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
        if (SUCCEEDED(hr))
        {
          csConstsUsed = csc;
          if (msr.pData)
            memcpy(msr.pData, csConsts, csc * sizeof(vec4f));
          dx_context->Unmap(csConstBuffer[nextBuffer], 0);
          if (csCurrentBuffer != nextBuffer)
          {
            csCurrentBuffer = nextBuffer;
            constantsBufferChanged[STAGE_CS] = 0xFFFFFFFF; // Async shaders always set buffers, no need to flag ASYNC_CS.
          }
        }
      }
    }
    else
    {
      csConstsUsed = 0;
    }
  }

  /*
  Async compute contexts don't support Map/Unmap functions, without setting them with CSSetConstantBuffers, they are not updated.
  There may be some opportunities for optimization using SetPlacement APIs if needed.

  XDK docs, article "Optimizing Monolithic Driver Performance":

  For all of the "Set" APIs (traditional or placement style), the following rules apply. Note that these rules are similar to the
  SetFast family of APIs on the D3D 11.X device context:
    1. Resources and views of resources that have DYNAMIC usage are not supported. In order to submit dynamically created data as a
       constant buffer, use the SetPlacement APIs or direct memory access on placement created resources.
    ...
  */

  if (!constantsBufferChanged[STAGE_CS] && !async)
    return;

  carray<ID3D11Buffer *, MAX_CONST_BUFFERS> constBuffers;
  carray<uint32_t, MAX_CONST_BUFFERS> cbOffsets;
  carray<uint32_t, MAX_CONST_BUFFERS> cbSizes;
  mem_copy_from(constBuffers, customBuffers[STAGE_CS].data());
  mem_copy_from(cbOffsets, buffersFirstConstants[STAGE_CS].data());
  mem_copy_from(cbSizes, buffersNumConstants[STAGE_CS].data());
  int slotsToSet = customBuffersMaxSlotUsed[stage] + 1, nextMaxUsedSlot = 0;
  bool needOffsets = false;
  for (int slotI = 0; slotI < slotsToSet; ++slotI) //&& customBuffers[stage][maxUsedSlot]
  {
    if (constBuffers[slotI])
      nextMaxUsedSlot = slotI;
    needOffsets |= cbOffsets[slotI] != 0;
  }
  if (!constBuffers[0])
  {
    constBuffers[0] = csConstBuffer[csCurrentBuffer];
    cbOffsets[0] = 0;
    cbSizes[0] = 4096;
  }

  {
    ContextAutoLock contextLock;
    SET_CONST_BUFFERS(CS, 0, slotsToSet, constBuffers.data(), cbOffsets.data(), cbSizes.data())
  }

  customBuffersMaxSlotUsed[stage] = nextMaxUsedSlot;
  constantsBufferChanged[stage] = 0;
};


VSDTYPE simple_pos_vdecl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT4), VSD_END};

VSDTYPE clear_vdecl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_REG(VSDR_TEXC0, VSDT_FLOAT4), VSD_END};

VSDTYPE debug_vdecl[] = {VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_REG(VSDR_DIFF, VSDT_E3DCOLOR), VSD_END};

VPROG create_vertex_shader_internal(const void *shader_bin, uint32_t size, uint32_t vs_consts_count, InputLayoutCache *ilCache,
  bool do_fatal)
{
  G_ASSERT(vs_consts_count <= g_vsbin_max_size);
  VertexShader e;
  e.ilCache = ilCache;
  e.dataOwned = 1;
  e.constsUsed = vs_consts_count;
  e.shader = NULL;
  e.hs = NULL;
  e.ds = NULL;
  e.gs = NULL;
  e.hsTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
  e.hdgBits = 0;
  if (device_is_lost == S_OK)
  {
    const uint32_t *u32 = (const uint32_t *)shader_bin;
    if (u32[0] == _MAKE4C('DX11'))
    {
      e.hsTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED + (u32[2] >> 24);
      uint32_t vs_ofs = 5, vs_len = u32[1];
      uint32_t hs_ofs = vs_ofs + vs_len, hs_len = u32[2] & 0x00FFFFFF;
      uint32_t ds_ofs = hs_ofs + hs_len, ds_len = u32[3];
      uint32_t gs_ofs = ds_ofs + ds_len, gs_len = u32[4];

      last_hres = dx_device->CreateVertexShader(&u32[vs_ofs], vs_len * sizeof(uint32_t), NULL, &e.shader);
      if (hs_len && SUCCEEDED(last_hres))
        last_hres = dx_device->CreateHullShader(&u32[hs_ofs], hs_len * sizeof(uint32_t), NULL, &e.hs);
      if (ds_len && SUCCEEDED(last_hres))
        last_hres = dx_device->CreateDomainShader(&u32[ds_ofs], ds_len * sizeof(uint32_t), NULL, &e.ds);
      if (gs_len && SUCCEEDED(last_hres))
        last_hres = dx_device->CreateGeometryShader(&u32[gs_ofs], gs_len * sizeof(uint32_t), NULL, &e.gs);
      if (e.hs)
        e.hdgBits |= HAS_HS;
      if (e.ds)
        e.hdgBits |= HAS_DS;
      if (e.gs)
        e.hdgBits |= HAS_GS;
    }
    else
      last_hres = dx_device->CreateVertexShader(shader_bin, size, NULL, &e.shader);
    if (!device_should_reset(last_hres, "CreateVertexShader"))
      if (do_fatal)
      {
        DXFATAL(last_hres, "CreateVertexShader");
      }

    if (FAILED(last_hres))
      e.shader = NULL;
    else
    {
      if (e.shader == NULL || IsBadReadPtr(e.shader, 1))
      {
        D3D_ERROR("CreateVertexShader (or hull/domain/geometry) failed: vs=0x%08X, hres=0x%08X", e.shader, last_hres);
        DAG_FATAL("dx11 error: broken driver");
      }
    }
  }

  return (SUCCEEDED(last_hres) || FAILED(device_is_lost)) ? g_vertex_shaders.safeAllocAndSet(e) : BAD_VPROG;
}

VPROG create_vertex_shader_unpacked(const void *shader_bin, uint32_t size, uint32_t vs_consts_count, bool do_fatal)
{
  void *copy = memalloc(size, midmem);
  memcpy(copy, shader_bin, size);
  shader_bin = copy;
#if SHOW_DISASM
  show_disasm(shader_bin, size, "VS", vs_consts_count);
#endif
  return create_vertex_shader_internal(shader_bin, size, vs_consts_count, new InputLayoutCache(shader_bin, size), do_fatal);
}

FSHADER create_pixel_shader_unpacked(const void *shader_bin, uint32_t size, uint32_t ps_consts_count, bool do_fatal)
{
  G_ASSERT(ps_consts_count <= MAX_PS_CONSTS);
  PixelShader e;
  e.shader = NULL;
  e.constsUsed = ps_consts_count;
#if SHOW_DISASM
  show_disasm(shader_bin, size, "PS", ps_consts_count);
#endif
  if (device_is_lost == S_OK)
  {
    last_hres = dx_device->CreatePixelShader(shader_bin, size, NULL, &e.shader);
    if (!device_should_reset(last_hres, "CreatePixelShader"))
      if (do_fatal)
      {
        DXFATAL(last_hres, "CreatePixelShader");
      }

    if (FAILED(last_hres))
      e.shader = NULL;
    else
    {
      if (e.shader == NULL || IsBadReadPtr(e.shader, 1))
      {
        D3D_ERROR("CreatePixelShader failed: ps=0x%08X, hres=0x%08X", e.shader, last_hres);
        DAG_FATAL("dx11 error: broken driver");
      }
    }
  }

  return (SUCCEEDED(last_hres) || FAILED(device_is_lost)) ? g_pixel_shaders.safeAllocAndSet(e) : BAD_FSHADER;
}
VPROG create_vertex_shader(const void *source_data, uint32_t size, uint32_t vs_consts_count)
{
  const uint32_t *native_code = (const uint32_t *)source_data;
  return create_vertex_shader_unpacked(native_code, size, vs_consts_count, true);
}
FSHADER create_pixel_shader(const void *source_data, uint32_t size, uint32_t vs_consts_count)
{
  const uint32_t *native_code = (const uint32_t *)source_data;
  return create_pixel_shader_unpacked(native_code, size, vs_consts_count, true);
}

SHADER_ID create_compute_shader(const uint32_t *native_code, uint32_t size, uint32_t max_const)
{
  ComputeShader e;
  e.shader = NULL;
  e.constsUsed = max_const;
  if (device_is_lost == S_OK)
  {
    last_hres = dx_device->CreateComputeShader(native_code, size, NULL, &e.shader);
    if (!device_should_reset(last_hres, "CreateComputeShader"))
    {
      DXFATAL(last_hres, "CreateComputeShader");
    }

    if (FAILED(last_hres))
      e.shader = NULL;
  }

  return (SUCCEEDED(last_hres) || FAILED(device_is_lost)) ? g_compute_shaders.safeAllocAndSet(e) : BAD_SHADER_ID;
}

void set_vertex_shader_debug_info(VPROG vpr, const char *debug_info)
{
  if (g_vertex_shaders.isIndexValid(vpr))
  {
    VertexShader &vs = g_vertex_shaders[vpr];
    if (vs.shader)
      vs.shader->SetPrivateData(WKPDID_D3DDebugObjectName, (int)strlen(debug_info), debug_info);
  }
}

void set_pixel_shader_debug_info(FSHADER fsh, const char *debug_info)
{
  if (g_pixel_shaders.isIndexValid(fsh))
  {
    PixelShader &ps = g_pixel_shaders[fsh];
    if (ps.shader)
      ps.shader->SetPrivateData(WKPDID_D3DDebugObjectName, (int)strlen(debug_info), debug_info);
  }
}

void init_default_shaders()
{
  g_default_copy_vs = create_vertex_shader(g_default_copy_vs_src, sizeof(g_default_copy_vs_src), 0);
  g_default_copy_ps = create_pixel_shader(g_default_copy_ps_src, sizeof(g_default_copy_ps_src), 0);
  g_default_pos_vdecl = d3d::create_vdecl(simple_pos_vdecl);

  g_default_clear_vs = create_vertex_shader(g_default_clear_vs_src, sizeof(g_default_clear_vs_src), 0);
  g_default_clear_ps = create_pixel_shader(g_default_clear_ps_src, sizeof(g_default_clear_ps_src), 0);
  g_default_clear_vdecl = d3d::create_vdecl(clear_vdecl);

  g_default_debug_vs = create_vertex_shader(g_default_debug_vs_src, sizeof(g_default_debug_vs_src), 4);
  g_default_debug_ps = create_pixel_shader(g_default_debug_ps_src, sizeof(g_default_debug_ps_src), 0);
  g_default_debug_vdecl = d3d::create_vdecl(debug_vdecl);
  g_default_debug_program = d3d::create_program(g_default_debug_vs, g_default_debug_ps, g_default_debug_vdecl);

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    g_default_clamp_sampler = d3d::request_sampler(smpInfo);
  }
}

void close_default_shaders()
{
  d3d::delete_pixel_shader(g_default_copy_ps);
  d3d::delete_vertex_shader(g_default_copy_vs);
  d3d::delete_vdecl(g_default_pos_vdecl);

  d3d::delete_pixel_shader(g_default_clear_ps);
  d3d::delete_vertex_shader(g_default_clear_vs);
  d3d::delete_vdecl(g_default_clear_vdecl);

  d3d::delete_program(g_default_debug_program);
  d3d::delete_pixel_shader(g_default_debug_ps);
  d3d::delete_vertex_shader(g_default_debug_vs);
  d3d::delete_vdecl(g_default_debug_vdecl);

  g_default_copy_ps = BAD_FSHADER;
  g_default_copy_vs = BAD_VPROG;
  g_default_pos_vdecl = BAD_VDECL;

  g_default_clear_ps = BAD_FSHADER;
  g_default_clear_vs = BAD_VPROG;
  g_default_clear_vdecl = BAD_VDECL;

  g_default_debug_ps = BAD_FSHADER;
  g_default_debug_vs = BAD_VPROG;
  g_default_debug_vdecl = BAD_VDECL;
  g_default_debug_program = BAD_PROGRAM;

  g_default_clamp_sampler = d3d::INVALID_SAMPLER_HANDLE;
}


PROGRAM get_debug_program() { return g_default_debug_program; }


bool init_shaders(RenderState &rs)
{
  rs.nextVdecl = BAD_HANDLE;
  rs.nextVertexShader = BAD_HANDLE;
  rs.nextPixelShader = BAD_HANDLE;
  rs.nextComputeShader = BAD_HANDLE;

  init_default_shaders();

  rs.constants.create();
  return true;
}

void flush_cs_shaders(RenderState &rs, bool flushConsts, bool async)
{
  ComputeShader *cs = (rs.nextComputeShader == BAD_SHADER_ID) ? NULL : &g_compute_shaders[rs.nextComputeShader];
  if (rs.csModified)
  {
    rs.csModified = false;
    {
      ContextAutoLock contextLock;
      dx_context->CSSetShader(cs ? cs->shader : NULL, 0, 0);
    }
  }

  if (flushConsts)
  {
    uint32_t csConstsCount = cs ? max((unsigned)rs.constants.constsRequired[STAGE_CS], (unsigned)cs->constsUsed) : 0;
    rs.constants.flush_cs(csConstsCount, async);
  }
}

void flush_shaders(RenderState &rs, bool flushConsts)
{
  PixelShader *ps = (rs.nextPixelShader == BAD_FSHADER) ? NULL : &g_pixel_shaders[rs.nextPixelShader];

  if (rs.psModified || rs.vsModified)
    Stat3D::updateProgram();
  if (rs.psModified)
  {
    ContextAutoLock contextLock;
    dx_context->PSSetShader(ps ? ps->shader : NULL, 0, 0);
    rs.psModified = false;
  }

  VertexShader *vs = (rs.nextVertexShader == BAD_VPROG) ? NULL : &g_vertex_shaders[rs.nextVertexShader];
  if (rs.vsModified)
  {
    {
      ContextAutoLock contextLock;
      dx_context->VSSetShader(vs ? vs->shader : NULL, 0, 0);
      dx_context->HSSetShader(vs && vs->hs ? vs->hs : NULL, NULL, 0);
      dx_context->DSSetShader(vs && vs->ds ? vs->ds : NULL, NULL, 0);
      dx_context->GSSetShader(vs && vs->gs ? vs->gs : NULL, NULL, 0);
    }
    unsigned hdgBits = vs ? vs->hdgBits : 0;
    if (rs.hdgBits != hdgBits)
      rs.hdgBits = hdgBits;

    if (rs.nextVdecl == BAD_VDECL)
    {
      ContextAutoLock contextLock;
      dx_context->IASetInputLayout(NULL);
    }
    else if (vs)
    {
      ID3D11InputLayout *layout = vs->ilCache->getInputLayout(rs.nextVdecl);
      ContextAutoLock contextLock;
      dx_context->IASetInputLayout(layout); // NULL is allowed for shaders without VB input.
    }

    if (vs && (rs.hdgBits & HAS_HS))
      rs.hullTopology = vs->hsTopology;
    else
      rs.hullTopology = 0;

    rs.vsModified = false;
  }

  if (flushConsts)
  {
    uint32_t vsConstsCount = vs ? max((unsigned)rs.constants.constsRequired[STAGE_VS], (unsigned)vs->constsUsed) : 0;
    rs.constants.flush(vsConstsCount, ps ? ps->constsUsed : 0, rs.hdgBits);
  }
}

void close_shaders()
{
  g_render_state.constants.destroy();

  close_default_shaders();

  if (!resetting_device_now)
  {
    if (g_input_layouts.totalUsed() > 0)
    {
      D3D_ERROR("Unreleased vdecl objects: %d", g_input_layouts.totalUsed());
    }

    g_input_layouts.clear();
  }

  g_pixel_shaders.clear();
  g_vertex_shaders.clear();
  g_compute_shaders.clear();
  g_programs.clear();
}


// return shader bytecode
/*ID3D10Blob * create_hlsl(const char *hlsl_text, unsigned len,
    const char *entry, const char *profile)
{
  ID3D10Blob *bytecode = NULL;
  ID3D10Blob *errors = NULL;

  //deprecated
  HRESULT hr = D3DX11CompileFromMemory(
      hlsl_text, //_In_   LPCSTR pSrcData,
      strlen(hlsl_text), //  _In_   SIZE_T SrcDataLen,
      NULL, //  _In_   LPCSTR pFileName,
      NULL, //  _In_   const D3D10_SHADER_MACRO *pDefines,
      NULL, //  _In_   LPD3D10INCLUDE pInclude,
      entry, //  _In_   LPCSTR pFunctionName,
      profile, //  _In_   LPCSTR pProfile, //vs_5_0
      D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, //  _In_   UINT Flags1,
      0, //  _In_   UINT Flags2,
      NULL,//  _In_   ID3DX11ThreadPump *pPump,
      &bytecode, //  _Out_  ID3D10Blob **ppShader,
      &errors, //  _Out_  ID3D10Blob **ppErrorMsgs,
      NULL //  _Out_  HRESULT *pHResult
      );

  if (FAILED(hr))
  {
    String str;
    if (errors)
    {
      //debug("errors compiling HLSL code:\n%s", source.str());
      const char *es = (const char *)errors->GetBufferPointer(), *ep = strchr(es, '\n');
      while (es)
      {
        if (ep)
        {
          str.aprintf(256, "  %.*s\n", ep-es, es);
          es = ep+1;
          ep = strchr(es, '\n');
        }
        else
          break;
      }
      D3D_ERROR("Can't compile:\n %s",str.data());
    } else
      D3D_ERROR("Can't compile");
  }

  if (errors)
    errors->Release();
  return bytecode;
}*/


static struct
{
  LPCSTR name;
  int index;
} semantic_remap[25] = {"POSITION", 0, "BLENDWEIGHT", 0, "BLENDINDICES", 0, "NORMAL", 0, "PSIZE", 0, "COLOR", 0, "COLOR", 1,
  "TEXCOORD", 0,                                                                                                          // 7
  "TEXCOORD", 1, "TEXCOORD", 2, "TEXCOORD", 3, "TEXCOORD", 4, "TEXCOORD", 5, "TEXCOORD", 6, "TEXCOORD", 7, "POSITION", 1, // POSITIONT
                                                                                                                          // ?
  "NORMAL", 1, // 16 BINNORMAL?
  "TEXCOORD", 15, "TEXCOORD", 8, "TEXCOORD", 9, "TEXCOORD", 10, "TEXCOORD", 11, "TEXCOORD", 12, "TEXCOORD", 13, "TEXCOORD", 14};

/*
   BINORMAL[n]    Binormal    float4
   BLENDINDICES[n]    Blend indices   uint
   BLENDWEIGHT[n] Blend weights   float
   COLOR[n]   Diffuse and specular color  float4
   NORMAL[n]  Normal vector   float4
   POSITION[n]    Vertex position in object space.    float4
   POSITIONT  Transformed vertex position.    float4
   PSIZE[n]   Point size  float
   TANGENT[n] Tangent float4
   TEXCOORD[n]    Texture coordinates
   */

void InputLayout::set(size_t index, size_t sem_index,
  // LPCSTR semantic_name,
  // UINT semantic_index,
  DXGI_FORMAT format, UINT input_slot, UINT aligned_byte_offset, D3D11_INPUT_CLASSIFICATION input_slot_class,
  UINT instance_data_steprate)
{
  G_ASSERT(index < MAX_CHANNELS);
  G_ASSERT(sem_index < countof(semantic_remap));
  D3D11_INPUT_ELEMENT_DESC &e = ied[index];
  e.SemanticName = semantic_remap[sem_index].name;
  e.SemanticIndex = semantic_remap[sem_index].index;
  e.Format = format;
  e.InputSlot = input_slot;
  e.AlignedByteOffset = aligned_byte_offset;
  e.InputSlotClass = input_slot_class;
  e.InstanceDataStepRate = instance_data_steprate;
}

void InputLayout::destroy()
{
  g_input_layouts.safeReleaseEntry(handle);
  delete this;
}

void InputLayout::makeFromVdecl(const VSDTYPE *decl)
{
  // parse stream like VSDTYPE dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT3), VSD_END };

  const VSDTYPE *__restrict vt = decl;
  size_t idx = 0;
  uint32_t ofs = 0;
  uint32_t slot = 0;
  bool perInstanceData = false;

  for (; *vt != VSD_END; ++vt)
  {
    if ((*vt & VSDOP_MASK) == VSDOP_INPUT)
    {
      if (*vt & VSD_SKIPFLG)
      {
        ofs += GET_VSDSKIP(*vt) * 4;
        continue;
      }

      size_t sem = GET_VSDREG(*vt);        // VSDR_* semantic
      DXGI_FORMAT f = DXGI_FORMAT_UNKNOWN; // DXGI fmt
      uint32_t ne = 0;                     // number of elements
      uint32_t sz = 0;                     // size of entry

      switch (*vt & VSDT_MASK)
      {
        case VSDT_FLOAT1:
          f = DXGI_FORMAT_R32_FLOAT;
          ne = 1;
          sz = 4;
          break;
        case VSDT_FLOAT2:
          f = DXGI_FORMAT_R32G32_FLOAT;
          ne = 2;
          sz = 8;
          break;
        case VSDT_FLOAT3:
          f = DXGI_FORMAT_R32G32B32_FLOAT;
          ne = 3;
          sz = 12;
          break;
        case VSDT_FLOAT4:
          f = DXGI_FORMAT_R32G32B32A32_FLOAT;
          ne = 4;
          sz = 16;
          break;

        case VSDT_HALF2:
          f = DXGI_FORMAT_R16G16_FLOAT;
          ne = 2;
          sz = 4;
          break;
        case VSDT_SHORT2N:
          f = DXGI_FORMAT_R16G16_SNORM;
          ne = 2;
          sz = 4;
          break;
        case VSDT_SHORT2:
          f = DXGI_FORMAT_R16G16_SINT;
          ne = 2;
          sz = 4;
          break;
        case VSDT_USHORT2N:
          f = DXGI_FORMAT_R16G16_UNORM;
          ne = 2;
          sz = 4;
          break;

        case VSDT_HALF4:
          f = DXGI_FORMAT_R16G16B16A16_FLOAT;
          ne = 4;
          sz = 8;
          break;
        case VSDT_SHORT4N:
          f = DXGI_FORMAT_R16G16B16A16_SNORM;
          ne = 4;
          sz = 8;
          break;
        case VSDT_SHORT4:
          f = DXGI_FORMAT_R16G16B16A16_SINT;
          ne = 4;
          sz = 8;
          break;
        case VSDT_USHORT4N:
          f = DXGI_FORMAT_R16G16B16A16_UNORM;
          ne = 4;
          sz = 8;
          break;

        case VSDT_E3DCOLOR:
          f = DXGI_FORMAT_B8G8R8A8_UNORM;
          ne = 4;
          sz = 4;
          break;
        case VSDT_UBYTE4:
          f = DXGI_FORMAT_R8G8B8A8_UINT;
          ne = 4;
          sz = 4;
          break;

        case VSDT_INT1:
          f = DXGI_FORMAT_R32_SINT;
          ne = 1;
          sz = 4;
          break;
        case VSDT_INT2:
          f = DXGI_FORMAT_R32G32_SINT;
          ne = 2;
          sz = 8;
          break;
        case VSDT_INT3:
          f = DXGI_FORMAT_R32G32B32_SINT;
          ne = 3;
          sz = 12;
          break;
        case VSDT_INT4:
          f = DXGI_FORMAT_R32G32B32A32_SINT;
          ne = 4;
          sz = 16;
          break;

        case VSDT_UINT1:
          f = DXGI_FORMAT_R32_UINT;
          ne = 1;
          sz = 4;
          break;
        case VSDT_UINT2:
          f = DXGI_FORMAT_R32G32_UINT;
          ne = 2;
          sz = 8;
          break;
        case VSDT_UINT3:
          f = DXGI_FORMAT_R32G32B32_UINT;
          ne = 3;
          sz = 12;
          break;
        case VSDT_UINT4:
          f = DXGI_FORMAT_R32G32B32A32_UINT;
          ne = 4;
          sz = 16;
          break;

          //        case VSDT_DEC3N:    f=DXGI_FORMAT_R11G11B10_FLOAT;    ne = 3; sz = 4; break; //flt?
          //        case VSDT_UDEC3:    //t=D3DDECLTYPE_UDEC3;  4
        default:
          D3D_CONTRACT_ASSERT(f != DXGI_FORMAT_UNKNOWN); // -V547 "invalid vertex declaration type"
          break;
      };

      set(idx, sem, f, slot, ofs, perInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA,
        perInstanceData ? 1 : 0);
      ofs += sz;
      idx++;
    }
    else if ((*vt & VSDOP_MASK) == VSDOP_STREAM)
    {
      slot = GET_VSDSTREAM(*vt);
      ofs = 0;
      perInstanceData = *vt & VSDS_PER_INSTANCE_DATA;
    }
    else
    {
      G_ASSERTF(0, "Invalid vsd opcode 0x%08X", *vt);
    }
  };

  numElements = idx & 0xff;
}

ID3D11InputLayout *InputLayoutCache::getInputLayout(int vdecl)
{
  D3D_CONTRACT_ASSERT(vdecl != BAD_VDECL);
  InputLayout *il = g_input_layouts[vdecl].obj;

  COMProxyPtr<ID3D11InputLayout> e;
  InputLayout::Key key = InputLayout::makeKey(vdecl);
  const COMProxyPtr<ID3D11InputLayout> *ilp = find(key);
  if (ilp == NULL) // not found
  {
    {
      const uint32_t *u32 = (const uint32_t *)shaderBytecode;
      size_t u32_sz = shaderBytecodeSize;
      if (u32[0] == _MAKE4C('DX11'))
      {
        u32_sz = u32[1] * sizeof(uint32_t);
        u32 += 5;
      }

      HRESULT hr = S_OK;
      if (il->numElements > 0)
      {
        hr = dx_device->CreateInputLayout(il->ied, // D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
          il->numElements,                         // UINT NumElements,
          u32,                                     // void *pShaderBytecodeWithInputSignature,
          u32_sz,                                  // SIZE_T BytecodeLength,
          &e.obj);                                 // ID3D11InputLayout **pInputLayout
        LLLOG("inputlayout (shdr %p; n=%d)", shaderBytecode, il->numElements);
        if (FAILED(hr))
        {
          debug("BytecodeLength=%d, NumElements=%d", u32_sz, il->numElements);
          for (int elemNo = 0; elemNo < il->numElements; elemNo++)
            debug("[%d] %s %d %d", elemNo, il->ied[elemNo].SemanticName, il->ied[elemNo].SemanticIndex, il->ied[elemNo].Format);
        }
        DXERR(hr, "CreateInputLayout");
      }
      else
      {
        e.obj = NULL; // IASetInputLayout(NULL) may be more effective than empty layout for shaders with VertexID input.
      }

      if (SUCCEEDED(hr))
      {
        if (!add(key, e))
        {
          e.obj->Release();
          return NULL;
        }
        ilp = &e;
      }
    }
  }
  return ilp ? ilp->obj : NULL;
}


/*

//  last_hres = dx_device->CreateVertexShader((const void*)(shader_bin+1), size, NULL, &e.shader);
*/

/*
   void DRV3D_GL::set_fvf_vdecl(int fvf_desc, DRV3D_GL::VDecl &vdecl)
   {
   vdecl.reset();

   uint32_t  fvf = uint32_t(fvf_desc);
   uint32_t  ofs = 0;

   switch (fvf & FVF_XYZMASK)
   {
   case FVF_XYZ:     vdecl.set(channel_remap(VSDR_POS), 0, GL_FLOAT, 3, ofs, 0); ofs += 12; break;
   case FVF_XYZRHW:  vdecl.set(channel_remap(VSDR_POS), 0, GL_FLOAT, 4, ofs, 0); ofs += 16; break;
   }

   if (fvf & FVF_NORMAL)
   {
   vdecl.set(channel_remap(VSDR_NORM), 0, GL_FLOAT, 3, ofs, 0);
   ofs += 12;
   }

   if (fvf & FVF_DIFFUSE)
   {
   vdecl.set(channel_remap(VSDR_DIFF), 0, GL_BGRA, 4, ofs, 1);
   ofs += 4;
   }

   if (fvf & FVF_SPECULAR)
   {
   vdecl.set(channel_remap(VSDR_SPEC), 0, GL_BGRA, 4, ofs, 1);
   ofs += 4;
   }

   if (fvf & FVF_PSIZE)
   {
   vdecl.set(channel_remap(VSDR_PSIZE), 0, GL_FLOAT, 1, ofs, 0);
   ofs += 4;
   }

   uint32_t  tn = GET_FVF_TEXCOUNT(fvf); //0..8
   for (uint32_t i = 0; i < tn; ++i)
   {
   uint32_t  coords = GET_FVF_TEXCOORDSIZE(fvf, i);
   vdecl.set(channel_remap(VSDR_TEXC0 + i), 0, GL_FLOAT, coords, ofs, 0);
   ofs += coords * 4;
   }
   }
   */


void delete_programs_for_vs(VPROG vpr)
{
  ITERATE_OVER_OBJECT_POOL(g_programs, i)
    if (g_programs[i].vertexShader == vpr && g_programs[i].refCntX2 >= 2)
    {
      g_programs[i].refCntX2 = 0;
      g_programs[i].destroyObject();
      g_programs.releaseEntryUnsafe(i); // Note: replace to fillEntryAsInvalid(i) if don't want to re-use this index/slot anymore
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_programs);
}

void delete_programs_for_ps(FSHADER fsh)
{
  ITERATE_OVER_OBJECT_POOL(g_programs, i)
    if (g_programs[i].pixelShader == fsh && g_programs[i].refCntX2 >= 2)
    {
      g_programs[i].refCntX2 = 0;
      g_programs[i].destroyObject();
      g_programs.releaseEntryUnsafe(i); // Note: replace to fillEntryAsInvalid(i) if don't want to re-use this index/slot anymore
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_programs);
}

void delete_programs_for_vd(VDECL vd)
{
  ITERATE_OVER_OBJECT_POOL(g_programs, i)
    if (g_programs[i].vdecl == vd && g_programs[i].refCntX2 >= 2)
    {
      g_programs[i].refCntX2 = 0;
      g_programs[i].destroyObject();
      g_programs.releaseEntryUnsafe(i); // Note: replace to fillEntryAsInvalid(i) if don't want to re-use this index/slot anymore
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_programs);
}

bool set_compute_shader(SHADER_ID handle)
{
  if ((handle != BAD_SHADER_ID) && !g_compute_shaders.isIndexValid(handle))
    return false;
  RenderState &rs = g_render_state;

  if (rs.nextComputeShader == handle)
    return true;

  rs.nextComputeShader = handle;
  rs.modified = rs.csModified = true;
  return true;
}

void Program::destroyObject()
{
  // warning - these functions can delete programs recursively
  if (vertexShader == BAD_VPROG)
    return;
  pixelShader = BAD_FSHADER;
  vertexShader = BAD_VPROG;
  vdecl = BAD_VDECL;
  if (computeShader != BAD_SHADER_ID)
  {
    G_ASSERT(!g_compute_shaders.isIndexInFreeList(computeShader));
    g_compute_shaders.release(computeShader);
  }
  computeShader = BAD_SHADER_ID;
}
}; // namespace drv3d_dx11

/*

void reserve_fsh(int max_ps)
{
  if (max_ps > g_pixel_shader_list.size())
    g_pixel_shader_list.reserve(max_ps - g_pixel_shader_list.size());
}
void get_stat_fsh(bool total, int &max_ps)
{
  max_ps = total ? g_pixel_shader_list.capacity() : g_pixel_shader_list.size();
}*/


/*
FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE *p,int n)
{
  int sz;
  uint32_t *ps = (uint32_t*)parse_fsh_shader(p,n,sz);

  if ( !ps )
  {
    error("create_pixel_shader_dagor error");
    print_fsh_shader(p,n);
    return BAD_FSHADER;
  }

  return d3d::create_pixel_shader ( ps );
}
*/

FSHADER d3d::create_pixel_shader_hlsl(const char * /*hlsl_text*/, unsigned /*len*/, const char * /*entry*/, const char * /*profile*/,
  String * /*out_err*/)
{
  G_ASSERT(0);
  return BAD_FSHADER;
}

FSHADER d3d::create_pixel_shader(const uint32_t *shader_bin)
{
  return drv3d_dx11::create_pixel_shader(shader_bin + 3, *shader_bin, ((int *)shader_bin)[1] >= 0 ? shader_bin[1] + 1 : 0);
}

void d3d::delete_pixel_shader(FSHADER handle)
{
  G_ASSERT(!g_pixel_shaders.isIndexInFreeList(handle));
  g_pixel_shaders.release(handle);
  delete_programs_for_ps(handle);
}

VPROG d3d::create_vertex_shader_hlsl(const char * /*hlsl_text*/, unsigned /*len*/, const char * /*entry*/, const char * /*profile*/,
  String * /*out_err*/)
{
  G_ASSERT(0);
  return BAD_VPROG;
}

VPROG d3d::create_vertex_shader(const uint32_t *shader_bin)
{
  return drv3d_dx11::create_vertex_shader(shader_bin + 3, *shader_bin, (int)(((int *)shader_bin)[1] >= 0 ? shader_bin[1] + 1 : 0));
}

void d3d::delete_vertex_shader(VPROG handle)
{
  G_ASSERT(!g_vertex_shaders.isIndexInFreeList(handle));
  g_vertex_shaders.release(handle);
  delete_programs_for_vs(handle);
}

bool d3d::set_pixel_shader(FSHADER handle)
{
  if ((handle != BAD_FSHADER) && !g_pixel_shaders.isIndexValid(handle))
    return false;
  RenderState &rs = g_render_state;

  if (rs.nextPixelShader == handle)
    return true;

  rs.nextPixelShader = handle;
  rs.modified = rs.psModified = true;
  //  DRAWSTAT_UPD(updatePixelShader);
  return true;
}

bool d3d::set_vertex_shader(VPROG handle)
{
  RenderState &rs = g_render_state;
  if ((handle != BAD_FSHADER) && !g_vertex_shaders.isIndexValid(handle))
    return false;

  if (rs.nextVertexShader == handle)
    return true;

  rs.nextVertexShader = handle;
  rs.modified = rs.vsModified = true;

  //  DRAWSTAT_UPD(updatePixelShader);
  return true;
}

bool d3d::set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs)
{
  ConstantBuffers &cb = g_render_state.constants;
  D3D_CONTRACT_ASSERT_RETURN(stage < STAGE_MAX, false);

  // Check OOB
#if DAGOR_DBGLEVEL > 0
  if (stage == STAGE_PS)
    D3D_CONTRACT_ASSERTF(reg_base + num_regs <= MAX_PS_CONSTS, "PS: Writing %u regs with base=%u over the limit=%u", num_regs,
      reg_base, MAX_PS_CONSTS);
  else if (stage == STAGE_CS)
  {
    D3D_CONTRACT_ASSERTF(reg_base + num_regs <= g_csbin_max_size, "CS: Writing %u regs with base=%u over the hard limit=%u", num_regs,
      reg_base, g_csbin_max_size);
    const unsigned limit = max<unsigned>(DEF_CS_CONSTS, cb.constsRequired[stage]);
    D3D_CONTRACT_ASSERTF(reg_base + num_regs <= limit, "CS: Writing %u regs with base=%u over the hard limit=%u, (constsRequired=%u)",
      num_regs, reg_base, limit, cb.constsRequired[stage]);
  }
  else if (stage == STAGE_VS)
  {
    const unsigned limit = max<unsigned>(DEF_VS_CONSTS, cb.constsRequired[stage]);
    D3D_CONTRACT_ASSERTF(reg_base + num_regs <= limit, "VS: Writing %u regs with base=%u over the hard limit=%u, (constsRequired=%u)",
      num_regs, reg_base, limit, cb.constsRequired[stage]);
  }
  else
    D3D_CONTRACT_ASSERT_FAIL("Invalid stage %d", stage);
#endif

  if (reg_base < 32 && !cb.constantsModified[stage]) // only do this for first 32 constants. it is unlikely, that later constants won't
                                                     // change, and it helps speed up rendinst/landmesh
    if (memcmp(cb.consts[stage][reg_base].f, data, num_regs * 16) == 0)
      return true;

  memcpy(cb.consts[stage][reg_base].f, data, num_regs * 16);
  g_render_state.modified = true;
  cb.constantsModified[stage] = true;
  if (stage == STAGE_CS)
    cb.constantsModified[STAGE_CS_ASYNC_STATE] = true;
  return true;
}

int d3d::set_cs_constbuffer_size(int required_size)
{
  if (required_size > 0)
  {
    int size = clamp(get_bigger_pow2(required_size), DEF_CS_CONSTS, g_csbin_max_size);
    g_render_state.constants.constsRequired[STAGE_CS] = size;
    return size;
  }
  else
  {
    g_render_state.constants.constsRequired[STAGE_CS] = 0;
    return 0;
  }
}

int d3d::set_vs_constbuffer_size(int required_size)
{
  if (required_size > 0)
  {
    int id = g_render_state.constants.getVsConstBufferId(required_size);
    int size = g_vsbin_sizes[id];
    g_render_state.constants.constsRequired[STAGE_VS] = size;
    return size;
  }
  else
  {
    g_render_state.constants.constsRequired[STAGE_VS] = 0;
    return 0;
  }
}


VDECL d3d::create_vdecl(VSDTYPE *decl)
{
  D3D_CONTRACT_ASSERT(decl != NULL);
  if (decl == NULL) // runtime check
    return BAD_VDECL;

  InputLayout *il = new InputLayout();

  il->makeFromVdecl(decl);

  ObjectProxyPtr<InputLayout> e;
  e.obj = il;
  int handle = g_input_layouts.safeAllocAndSet(e);
  if (handle == BAD_HANDLE)
  {
    delete il;
    return BAD_VDECL;
  }
  il->handle = handle;
  return handle;
}

void d3d::delete_vdecl(VDECL vdecl)
{
  G_ASSERT(!g_input_layouts.isIndexInFreeList(vdecl));
  RenderState &rs = g_render_state;

  if (rs.nextVdecl == vdecl)
    rs.nextVdecl = BAD_VDECL;

  g_input_layouts.release(vdecl);
  // iterate over shaders, kill caches
  // delete_programs_for_vd(vd);
}

bool d3d::setvdecl(VDECL vdecl)
{
  RenderState &rs = g_render_state;

  if (rs.nextVdecl == vdecl)
    return true;

  rs.nextVdecl = vdecl;
  rs.modified = rs.vsModified = true;
  return true;
}

PROGRAM d3d::create_program(VPROG vpr, FSHADER fsh, VDECL vdecl, unsigned *strides, unsigned streams)
{
  if (vdecl == BAD_VDECL || vpr == BAD_VPROG)
    return BAD_PROGRAM;

  g_programs.lock();

  // reuse identical programs: till we use DX9-based notion of constants we have not benefits from separating programs
  for (int i = 0; i < g_programs.size(); i++)
    if (g_programs.isEntryUsed(i) && g_programs[i].vertexShader == vpr && g_programs[i].pixelShader == fsh &&
        g_programs[i].vdecl == vdecl)
    {
      g_programs[i].refCntX2 += 2;
      g_programs.unlock();
      return i;
    }

  int i = g_programs.alloc();
  Program &prg = g_programs[i];
  prg.vertexShader = vpr;
  prg.pixelShader = fsh;
  prg.computeShader = BAD_SHADER_ID;
  prg.vdecl = vdecl;
  prg.refCntX2 = 2;
  g_programs.unlock();
  return i;
}

PROGRAM d3d::create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides, unsigned streams)
{
  G_ASSERT(0); // Shader binaries are API dependent - remove support.
  return BAD_PROGRAM;
}

PROGRAM d3d::create_program_cs(const uint32_t *native_code, CSPreloaded)
{
  SHADER_ID compute_shader = BAD_SHADER_ID;

  if (memcmp(native_code + 3, "DXBC", 4) == 0) // HLSL binary format
    compute_shader = drv3d_dx11::create_compute_shader(native_code + 3, *native_code, DEF_CS_CONSTS);
  if (compute_shader == BAD_SHADER_ID)
    return BAD_PROGRAM;

  g_programs.lock();

  // reuse identical programs: till we use DX9-based notion of constants we have not benefits from separating programs
  for (int i = 0; i < g_programs.size(); i++)
  {
    if (g_programs.isEntryUsed(i) && g_programs[i].vertexShader == BAD_VPROG && g_programs[i].pixelShader == BAD_FSHADER &&
        g_programs[i].computeShader == compute_shader)
    {
      g_programs[i].refCntX2 += 2;
      g_programs.unlock();
      return i;
    }
  }

  int i = g_programs.alloc();
  Program &prg = g_programs[i];
  prg.vertexShader = BAD_VPROG;
  prg.pixelShader = BAD_FSHADER;
  prg.computeShader = compute_shader;
  prg.vdecl = BAD_VDECL;
  prg.refCntX2 = 2;
  g_programs.unlock();
  return i;
}

VDECL d3d::get_program_vdecl(PROGRAM sh)
{
  if (!g_programs.isIndexValid(sh))
    return BAD_VDECL;
  return g_programs[sh].vdecl;
}

bool d3d::set_program(PROGRAM sh)
{
  if (!g_programs.isIndexValid(sh))
  {
    d3d::set_vertex_shader(BAD_VPROG);
    d3d::set_pixel_shader(BAD_FSHADER);
    drv3d_dx11::set_compute_shader(BAD_SHADER_ID);
    d3d::setvdecl(BAD_VDECL);
    return true;
  }

  const Program &prg = g_programs[sh];
  if (prg.computeShader != BAD_SHADER_ID)
  {
    return set_compute_shader(prg.computeShader);
  }

  return setvdecl(prg.vdecl) & set_vertex_shader(prg.vertexShader) & set_pixel_shader(prg.pixelShader); // -V792
}

void d3d::delete_program(PROGRAM sh)
{
  //  AutoAcquireGpu gpu_lock;
  if (!g_programs.isIndexValid(sh))
    return;
  g_programs.lock();
  D3D_CONTRACT_ASSERTF(!g_programs.isIndexInFreeListUnsafe(sh),
    "Double delete of program %d. Refcount was equal to 0 on a previous delete.", sh);
  g_programs[sh].refCntX2 -= 2;
  if (g_programs[sh].refCntX2 > 0)
    return g_programs.unlock();
  G_ASSERT(g_programs[sh].refCntX2 == 0);
  g_programs.releaseEntryUnsafe(sh);
  g_programs.unlock();
}

PROGRAM d3d::get_debug_program() { return drv3d_dx11::get_debug_program(); }
