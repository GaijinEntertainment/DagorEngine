#ifndef __DRV3D_DX11_SHADERS_H
#define __DRV3D_DX11_SHADERS_H
#pragma once

#include "drvCommonConsts.h"
#include <generic/dag_carray.h>

namespace drv3d_dx11
{
typedef int SHADER_ID;
static constexpr int BAD_SHADER_ID = -1;

typedef union
{
  vec4f fv;
  float f[4];
} vecflt;

static const int g_vsbin_sizes[] = {16, 32, 64, MAX_VS_CONSTS_NOBONES, MAX_VS_CONSTS_BONES, 256, 512, 1024, 2048, 4096};
static const int g_vsbin_sizes_count = sizeof(g_vsbin_sizes) / sizeof(g_vsbin_sizes[0]);
static const int g_vsbin_max_size = 4096;
extern int g_vs_bones_const;

static const int g_csbin_sizes_count = 13 - DEF_CS_CONSTS_SHIFT; // 64, 128, 256, 512, 1024, 2048, 4096
static const int g_csbin_max_size = 4096;

struct ConstantBuffers
{
  carray<carray<ID3D11Buffer *, MAX_CONST_BUFFERS>, STAGE_MAX> customBuffers;
  carray<carray<uint32_t, MAX_CONST_BUFFERS>, STAGE_MAX> buffersFirstConstants;
  carray<carray<uint32_t, MAX_CONST_BUFFERS>, STAGE_MAX> buffersNumConstants;

  carray<int, STAGE_MAX_EXT> customBuffersMaxSlotUsed;

  enum
  {
    MIN_PS_CONSTS = 16,
    PS_CONSTS_STEP = 32,
    PS_BINS = 5 // The largest bin must fit MAX_PS_CONSTS
  };
  int psCurrentBuffer;
  carray<ID3D11Buffer *, PS_BINS> psConstBuffer;
  carray<ID3D11Buffer *, g_vsbin_sizes_count> vsConstBuffer;
  int vsCurrentBuffer;

  carray<ID3D11Buffer *, g_csbin_sizes_count> csConstBuffer; // 2x multiplication
  int csCurrentBuffer;

  uint32_t vsConstsUsed, psConstsUsed, csConstsUsed;
  carray<uint32_t, STAGE_MAX> constsRequired;

  carray<bool, STAGE_MAX_EXT> constantsModified;
  carray<uint32_t, STAGE_MAX_EXT> constantsBufferChanged;
  uint8_t lastHDGBitsApplied;

  vecflt psConsts[MAX_PS_CONSTS];    // -V730_NOINIT
  vecflt vsConsts[g_vsbin_max_size]; // -V730_NOINIT
  vecflt csConsts[g_csbin_max_size]; // -V730_NOINIT
  vecflt *consts[STAGE_MAX] = {csConsts, psConsts, vsConsts};

  ConstantBuffers() :
    psCurrentBuffer(0),
    vsCurrentBuffer(0),
    csCurrentBuffer(0),
    lastHDGBitsApplied(0),
    vsConstsUsed(0),
    psConstsUsed(0),
    csConstsUsed(0)
  {
    mem_set_0(psConstBuffer);
    mem_set_0(vsConstBuffer);
    mem_set_0(csConstBuffer);
    mem_set_0(customBuffers);
    mem_set_0(buffersFirstConstants);
    mem_set_0(buffersNumConstants);
    mem_set_0(customBuffersMaxSlotUsed);
    mem_set_0(constantsBufferChanged);
    for (auto &c : constantsModified)
      c = true;
    mem_set_0(constsRequired);
  }
  void create();
  void destroy();
  void flush(uint32_t vsc, uint32_t psc, uint32_t hdg_bits);
  void flush_cs(uint32_t csc, bool async);
  int getVsConstBufferId(int required_size);
  ID3D11Buffer *createVsBuffer(int id);
};


struct InputLayout
{
  typedef uint32_t Key;

  enum
  {
    MAX_CHANNELS = 16,
    MAX_STREAM_STRIDES = 4,
    BAD_ATTR_MASK = 0xFFFFFFFF
  };

  D3D11_INPUT_ELEMENT_DESC ied[MAX_CHANNELS]; // -V730_NOINIT
  int handle;
  uint8_t numElements;

  InputLayout() : numElements(0), handle(BAD_HANDLE) {}

  void set(size_t index, size_t sem_index,
    // LPCSTR semantic_name,
    // UINT semantic_index,
    DXGI_FORMAT format, UINT input_slot, UINT aligned_byte_offset, D3D11_INPUT_CLASSIFICATION input_slot_class,
    UINT instance_data_steprate);

  static Key makeKey(int vdecl)
  {
    G_ASSERT(vdecl != BAD_VDECL);
    return vdecl + 1;
  }
  void makeFromVdecl(const VSDTYPE *decl);
  void destroy();
  void destroyObject() { delete this; }
};

// extern uint32_t get_vdecl_stride(VDECL id, int stream);
// vdecl -> ID3D11InputLayout* map
struct InputLayoutCache : KeyMap<COMProxyPtr<ID3D11InputLayout>, InputLayout::Key, 2> // 2 inputlayouts is absolutely enough. one for
                                                                                      // depth, one for color (usually it is one)
{
  const void *shaderBytecode;
  size_t shaderBytecodeSize;

  InputLayoutCache(const void *shader_bytecode, size_t shader_bytecode_size) :
    shaderBytecode(shader_bytecode), shaderBytecodeSize(shader_bytecode_size)
  {}

  /*
  void setShader(ID3D10Blob *blob)
  {
    shaderBytecode     = blob->GetBufferPointer();
    shaderBytecodeSize = blob->GetBufferSize();
  }
*/

  ID3D11InputLayout *getInputLayout(int vdecl);
};

struct VertexShader
{
  ID3D11VertexShader *shader;
  ID3D11HullShader *hs;
  ID3D11DomainShader *ds;
  ID3D11GeometryShader *gs;
  InputLayoutCache *ilCache;
  uint16_t constsUsed;
  uint16_t hsTopology : 8, hdgBits : 3, dataOwned : 5;

  void destroyObject()
  {
    G_ASSERT(shader || FAILED(device_is_lost));
    if (shader)
      shader->Release();
    if (hs)
      hs->Release();
    if (ds)
      ds->Release();
    if (gs)
      gs->Release();
    if (ilCache)
    {
      if (dataOwned)
      {
        memfree((void *)ilCache->shaderBytecode, midmem);
        dataOwned = 0;
      }
      delete ilCache;
    }
    shader = NULL;
    hs = NULL;
    ds = NULL;
    gs = NULL;
    ilCache = NULL;
  }
};

struct PixelShader
{
  ID3D11PixelShader *shader;
  uint32_t constsUsed;

  void destroyObject()
  {
    G_ASSERT(shader || FAILED(device_is_lost));
    if (shader)
      shader->Release();
    shader = NULL;
    constsUsed = 0;
  }
};

struct ComputeShader
{
  ID3D11ComputeShader *shader;
  uint32_t constsUsed;

  void destroyObject()
  {
    G_ASSERT(shader || FAILED(device_is_lost));
    if (shader)
      shader->Release();
    shader = NULL;
    constsUsed = 0;
  }
};

struct Program
{
  int refCntX2; // X2 in order to keep LSB 0 as indicator of used element (Pool's requirement)

  FSHADER pixelShader;
  VPROG vertexShader;
  VDECL vdecl;
  SHADER_ID computeShader;

  void destroyObject();
};

extern FSHADER g_default_copy_ps;
extern VPROG g_default_copy_vs;
extern VDECL g_default_pos_vdecl;

extern FSHADER g_default_clear_ps;
extern VPROG g_default_clear_vs;
extern VDECL g_default_clear_vdecl;
}; // namespace drv3d_dx11

#endif
