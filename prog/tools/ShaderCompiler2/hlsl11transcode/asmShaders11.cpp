// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaders11.h"
#include "../const3d.h"
#include "../defer.h"

#include <D3Dcompiler.h>

#include <drv/shadersMetaData/dx11/compiled_shader_header.h>
#include <drv/shadersMetaData/dx11/register_limits.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_unicode.h>
#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <stdio.h>
#include <generic/dag_tab.h>
#include <generic/dag_align.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#define DBG_SHDR 0
#if DBG_SHDR
int g_index = 0;
#endif

G_STATIC_ASSERT(dx11::MAX_U_REGISTERS <= sizeof(dx11::SimpleHeader::uavReadMask) * 8);

static eastl::tuple<eastl::vector<uint8_t>, eastl::vector<uint8_t>> get_blob_with_header_aligned(const dx11::SimpleHeader &header,
  const void *data)
{
  eastl::vector<uint8_t> metadata(sizeof(header));
  memcpy(metadata.data(), &header, sizeof(header));

  unsigned data_size_aligned = (header.bytecodeByteSize + 3) & (~3);
  eastl::vector<uint8_t> bytecode(data_size_aligned, 0);
  memcpy(bytecode.data(), data, header.bytecodeByteSize);

  return {metadata, bytecode};
}

struct UavAccessMasks
{
  uint8_t read = 0;
  uint8_t write = 0;
};

// DXBC token values from d3d11TokenizedProgramFormat.hpp (Windows DDK), which the Windows SDK does not ship
namespace dxbc
{
constexpr uint32_t fourcc(char a, char b, char c, char d)
{
  return uint32_t(a) | (uint32_t(b) << 8) | (uint32_t(c) << 16) | (uint32_t(d) << 24);
}

enum : uint32_t
{
  FOURCC_DXBC = fourcc('D', 'X', 'B', 'C'),
  FOURCC_SHDR = fourcc('S', 'H', 'D', 'R'),
  FOURCC_SHEX = fourcc('S', 'H', 'E', 'X'),

  OPCODE_CUSTOMDATA = 53,
  OPCODE_LD_UAV_TYPED = 163,
  OPCODE_STORE_UAV_TYPED = 164,
  OPCODE_LD_RAW = 165,
  OPCODE_STORE_RAW = 166,
  OPCODE_LD_STRUCTURED = 167,
  OPCODE_STORE_STRUCTURED = 168,
  OPCODE_ATOMIC_FIRST = 169, // atomic_and
  OPCODE_IMM_ATOMIC_ALLOC = 178,
  OPCODE_IMM_ATOMIC_CONSUME = 179,
  OPCODE_ATOMIC_LAST = 189, // imm_atomic_umin
  OPCODE_LD_UAV_TYPED_FEEDBACK = 225,
  OPCODE_LD_RAW_FEEDBACK = 226,
  OPCODE_LD_STRUCTURED_FEEDBACK = 227,

  OPERAND_TYPE_IMMEDIATE32 = 4,
  OPERAND_TYPE_IMMEDIATE64 = 5,
  OPERAND_TYPE_UNORDERED_ACCESS_VIEW = 30,

  INDEX_IMMEDIATE32 = 0,
  INDEX_IMMEDIATE64 = 1,
  INDEX_RELATIVE = 2,
  INDEX_IMMEDIATE32_PLUS_RELATIVE = 3,
  INDEX_IMMEDIATE64_PLUS_RELATIVE = 4,
};

inline uint32_t opcode_type(uint32_t t) { return t & 0x7ff; }
inline uint32_t opcode_len(uint32_t t) { return (t >> 24) & 0x7f; }
inline bool is_extended(uint32_t t) { return (t & 0x80000000u) != 0; }
inline uint32_t operand_num_components(uint32_t t) { return t & 3; }
inline uint32_t operand_type(uint32_t t) { return (t >> 12) & 0xff; }
inline uint32_t operand_index_dim(uint32_t t) { return (t >> 20) & 3; }
inline uint32_t operand_index_rep(uint32_t t, uint32_t dim) { return (t >> (22 + dim * 3)) & 7; }
} // namespace dxbc

static const uint32_t *find_dxbc_program(const void *bytecode, size_t size, size_t &out_dwords)
{
  const uint8_t *base = (const uint8_t *)bytecode;
  if (size < 32 || *(const uint32_t *)base != dxbc::FOURCC_DXBC)
    return nullptr;
  const uint32_t chunkCount = *(const uint32_t *)(base + 28);
  if (chunkCount > (size - 32) / 4)
    return nullptr;
  for (uint32_t i = 0; i < chunkCount; ++i)
  {
    const uint32_t ofs = *(const uint32_t *)(base + 32 + i * 4);
    if (ofs > size - 8 || (ofs & 3))
      continue;
    const uint32_t fourcc = *(const uint32_t *)(base + ofs);
    const uint32_t chunkSize = *(const uint32_t *)(base + ofs + 4);
    if (fourcc != dxbc::FOURCC_SHDR && fourcc != dxbc::FOURCC_SHEX)
      continue;
    if (chunkSize > size - ofs - 8)
      return nullptr;
    // program[1] is the length in dwords, including the version token and itself
    const uint32_t *program = (const uint32_t *)(base + ofs + 8);
    if (chunkSize < 8 || program[1] < 2 || program[1] > chunkSize / 4)
      return nullptr;
    out_dwords = program[1];
    return program;
  }
  return nullptr;
}

static const uint32_t *skip_dxbc_operand(const uint32_t *op, const uint32_t *end, uint8_t &uav_mask)
{
  if (op >= end)
    return nullptr;
  const uint32_t token = *op++;
  for (bool ext = dxbc::is_extended(token); ext; ext = dxbc::is_extended(*op++))
    if (op >= end)
      return nullptr;

  const uint32_t type = dxbc::operand_type(token);
  if (type == dxbc::OPERAND_TYPE_IMMEDIATE32 || type == dxbc::OPERAND_TYPE_IMMEDIATE64)
  {
    // a 4 component immediate is 4 dwords for both types: immediate64 then holds 2 doubles, not 4
    const uint32_t numComps = dxbc::operand_num_components(token);
    const uint32_t dwords = numComps == 2 ? 4 : numComps * (type == dxbc::OPERAND_TYPE_IMMEDIATE64 ? 2 : 1);
    return dwords <= size_t(end - op) ? op + dwords : nullptr;
  }

  const uint32_t dims = dxbc::operand_index_dim(token);
  // constraint: only SM 5.0 u# (one immediate index) is a register; SM 5.1 U#[...] indexes a range id, extend for SM 5.1
  if (type == dxbc::OPERAND_TYPE_UNORDERED_ACCESS_VIEW && dims == 1 && dxbc::operand_index_rep(token, 0) == dxbc::INDEX_IMMEDIATE32 &&
      op < end && *op < uint32_t(dx11::MAX_U_REGISTERS))
    uav_mask |= 1u << *op;

  for (uint32_t d = 0; d < dims && op; ++d)
  {
    switch (dxbc::operand_index_rep(token, d))
    {
      case dxbc::INDEX_IMMEDIATE32: op += 1; break;
      case dxbc::INDEX_IMMEDIATE64: op += 2; break;
      case dxbc::INDEX_RELATIVE: op = skip_dxbc_operand(op, end, uav_mask); break;
      case dxbc::INDEX_IMMEDIATE32_PLUS_RELATIVE: op = skip_dxbc_operand(op + 1, end, uav_mask); break;
      case dxbc::INDEX_IMMEDIATE64_PLUS_RELATIVE: op = skip_dxbc_operand(op + 2, end, uav_mask); break;
      default: return nullptr;
    }
  }
  return op && op <= end ? op : nullptr;
}

static bool scan_dxbc_uav_access(const void *bytecode, size_t size, UavAccessMasks &masks)
{
  size_t programDwords = 0;
  const uint32_t *program = find_dxbc_program(bytecode, size, programDwords);
  if (!program)
    return false;

  const uint32_t *const programEnd = program + programDwords;
  for (const uint32_t *ins = program + 2; ins < programEnd;)
  {
    const uint32_t opcodeToken = *ins;
    const uint32_t opcode = dxbc::opcode_type(opcodeToken);
    // customdata (icb, debug data) keeps its length in the next dword instead of the opcode token
    uint32_t len = opcode == dxbc::OPCODE_CUSTOMDATA ? (ins + 1 < programEnd ? ins[1] : 0) : dxbc::opcode_len(opcodeToken);
    if (len == 0 || len > size_t(programEnd - ins))
      return false;
    const uint32_t *insEnd = ins + len;

    bool reads = false, writes = false;
    switch (opcode)
    {
      case dxbc::OPCODE_LD_UAV_TYPED:
      case dxbc::OPCODE_LD_RAW:
      case dxbc::OPCODE_LD_STRUCTURED:
      case dxbc::OPCODE_LD_UAV_TYPED_FEEDBACK:
      case dxbc::OPCODE_LD_RAW_FEEDBACK:
      case dxbc::OPCODE_LD_STRUCTURED_FEEDBACK: reads = true; break;
      case dxbc::OPCODE_STORE_UAV_TYPED:
      case dxbc::OPCODE_STORE_RAW:
      case dxbc::OPCODE_STORE_STRUCTURED: writes = true; break;
      // constraint: imm_atomic_alloc and imm_atomic_consume touch the hidden UAV counter only, not the UAV data
      case dxbc::OPCODE_IMM_ATOMIC_ALLOC:
      case dxbc::OPCODE_IMM_ATOMIC_CONSUME: break;
      default: reads = writes = opcode >= dxbc::OPCODE_ATOMIC_FIRST && opcode <= dxbc::OPCODE_ATOMIC_LAST;
    }

    if (reads || writes)
    {
      const uint32_t *op = ins + 1;
      for (bool ext = dxbc::is_extended(opcodeToken); ext; ext = dxbc::is_extended(*op++))
        if (op >= insEnd)
          return false;
      uint8_t operandMask = 0;
      while (op && op < insEnd)
        op = skip_dxbc_operand(op, insEnd, operandMask);
      if (!op)
        return false;
      if (reads)
        masks.read |= operandMask;
      if (writes)
        masks.write |= operandMask;
    }
    ins = insEnd;
  }
  return true;
}

CompileResult compileShaderDX11(const char *shaderName, const char *source, const char **args, const char *profile, const char *entry,
  bool need_disasm, DebugLevel hlsl_debug_level, bool skip_validation, DebugParts debug_parts, unsigned flags, int implicit_cbuf_size)
{
  CompileResult result;
  ID3D10Blob *bytecode = nullptr;
  ID3D10Blob *errors = nullptr;
#if DBG_SHDR
  g_index++;
  char name[30];
  sprintf(name, "s%d.sh", g_index);
  debug("=s %d", g_index);
#endif

  D3D_SHADER_MACRO *pMacros = NULL;

  unsigned int d3dCompileFlags = 0;
  if (skip_validation)
    d3dCompileFlags |= D3DCOMPILE_SKIP_VALIDATION;
  if (hlsl_debug_level == DebugLevel::FULL_DEBUG_INFO)
    d3dCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  else if (hlsl_debug_level == DebugLevel::BASIC)
    d3dCompileFlags |= D3DCOMPILE_DEBUG;
  else
    d3dCompileFlags |= flags; // not sure why flags is only applied in this case, but this is consistent with the original former
  if (debug_parts == DebugParts::EMBED_SOURCE)
    d3dCompileFlags |= D3DCOMPILE_DEBUG;

  HRESULT hr = D3DCompile(source, //_In_   LPCSTR pSrcData,
    strlen(source),               //  _In_   SIZE_T SrcDataLen,
    NULL,                         //  _In_   LPCSTR pFileName,
    pMacros,                      //  _In_   const D3D10_SHADER_MACRO *pDefines,
    NULL,                         //  _In_   LPD3D10INCLUDE pInclude,
    entry,                        //  _In_   LPCSTR pFunctionName,
    profile,                      //  _In_   LPCSTR pProfile, //vs_5_0
    d3dCompileFlags,              //  _In_   UINT Flags1,
    0,                            //  _In_   UINT Flags2
    &bytecode,                    //  _Out_  ID3D10Blob **ppShader,
    &errors                       //  _Out_  ID3D10Blob **ppErrorMsgs,
  );
#if DBG_SHDR
  name[0] = 'f';
  debug("=f %d", g_index);
#endif
  if (hr != S_OK)
  {
    if (errors)
    {
      result.errors.assign((const char *)errors->GetBufferPointer(),
        (const char *)errors->GetBufferPointer() + errors->GetBufferSize());
      errors->Release();
    }
    return result;
  }

  if (errors)
    errors->Release(); // should not be needed, however, for avoidance of mem leak if compiler returns warnings in errors

  DEFER([&] {
    if (bytecode)
      bytecode->Release();
  });

  int maxRtv = -1;
  int implicitCbufRegCountFromReflection = 0;
  uint32_t reflectedUavMask = 0;
  int hsTopology = 0;

  {
    ID3D11ShaderReflection *reflector = nullptr;
    hr = D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_PPV_ARGS(&reflector));
    G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the %s shader", profile[0] == 'c' ? "compute" : "pixel");
    if (profile[0] == 'c')
    {
      reflector->GetThreadGroupSize(&result.computeShaderInfo.threadGroupSizeX, &result.computeShaderInfo.threadGroupSizeY,
        &result.computeShaderInfo.threadGroupSizeZ);
    }

    D3D11_SHADER_DESC desc;
    hr = reflector->GetDesc(&desc);
    G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the shader desc");
    for (UINT bri = 0; bri < desc.BoundResources; ++bri)
    {
      D3D11_SHADER_INPUT_BIND_DESC bindDesc{};
      hr = reflector->GetResourceBindingDesc(bri, &bindDesc);
      G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the binding desc");

      switch (bindDesc.Type)
      {
        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS:
        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        {
          const UINT regEnd = min<UINT>(bindDesc.BindPoint + bindDesc.BindCount, dx11::MAX_U_REGISTERS);
          for (UINT reg = bindDesc.BindPoint; reg < regEnd; ++reg)
            reflectedUavMask |= 1u << reg;
          break;
        }
        default: break;
      }

      if (bindDesc.Type == D3D_SIT_CBUFFER && bindDesc.BindPoint == 0)
      {
        D3D11_SHADER_BUFFER_DESC cbufDesc;
        ID3D11ShaderReflectionConstantBuffer *cbuf = reflector->GetConstantBufferByName(bindDesc.Name);
        G_ASSERTF(cbuf, "Unable to reflect the constbuffer");
        hr = cbuf->GetDesc(&cbufDesc);
        G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the constbuffer desc");
        implicitCbufRegCountFromReflection = dag::divide_align_up(cbufDesc.Size, REGISTER_BYTE_SIZE);
      }
    }
    if (profile[0] == 'p')
    {
      for (uint32_t i = 0; i < desc.OutputParameters; ++i)
      {
        D3D11_SIGNATURE_PARAMETER_DESC out;
        reflector->GetOutputParameterDesc(i, &out);
        if (out.SystemValueType == D3D_NAME_TARGET)
        {
          maxRtv = max(maxRtv, int(out.SemanticIndex));
          result.psOutputInfo.colorTargetMask |= 1u << out.SemanticIndex;
        }
        else if (out.SystemValueType == D3D_NAME_DEPTH || out.SystemValueType == D3D_NAME_DEPTH_GREATER_EQUAL ||
                 out.SystemValueType == D3D_NAME_DEPTH_LESS_EQUAL)
          result.psOutputInfo.writesDepth = true;
      }
      result.psOutputInfo.valid = true;
    }
    if (strncmp(profile, "hs", 2) == 0)
    {
      // the IA patch topology must match the HS input patch size
      const bool knownDomain = desc.TessellatorDomain == D3D_TESSELLATOR_DOMAIN_ISOLINE ||
                               desc.TessellatorDomain == D3D_TESSELLATOR_DOMAIN_TRI ||
                               desc.TessellatorDomain == D3D_TESSELLATOR_DOMAIN_QUAD;
      const bool patchInput =
        desc.InputPrimitive >= D3D_PRIMITIVE_1_CONTROL_POINT_PATCH && desc.InputPrimitive <= D3D_PRIMITIVE_32_CONTROL_POINT_PATCH;
      if (knownDomain && patchInput)
        hsTopology = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (desc.InputPrimitive - D3D_PRIMITIVE_1_CONTROL_POINT_PATCH);
      else
      {
        logerr("HS: unexpected reflection, tessellator domain %d, input primitive %d", desc.TessellatorDomain, desc.InputPrimitive);
        logwarn("HS: topology undefined, draws keep their own primitive type");
      }
    }

    reflector->Release();
  }

  if (implicit_cbuf_size < implicitCbufRegCountFromReflection)
  {
    result.errors.sprintf("b0 constbuffer size %d regs in reflection exceeds dshl-allocated size of %d regs. If you use "
                          "hlsl-hardcoded register arrays, specify sentinel registers.",
      implicitCbufRegCountFromReflection, implicit_cbuf_size);
    return result;
  }

  if (need_disasm)
  {
    ID3D10Blob *disasm = nullptr;
    hr = D3DDisassemble((DWORD *)bytecode->GetBufferPointer(), bytecode->GetBufferSize(), 0, NULL, &disasm);
    if (disasm)
    {
      const char *text = (const char *)disasm->GetBufferPointer();
      result.disassembly.assign(text, text + disasm->GetBufferSize());
      disasm->Release();
    }
  }

  UavAccessMasks uavAccess;
  if (reflectedUavMask)
  {
    if (scan_dxbc_uav_access(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), uavAccess))
    {
      uavAccess.read &= reflectedUavMask;
      uavAccess.write &= reflectedUavMask;
    }
    else
    {
      uavAccess = {};
      logwarn("DX11: DXBC parse failed for %s, its UAV access masks stay empty", shaderName);
    }
  }

  const char *bytecodeMsg = (const char *)bytecode->GetBufferPointer();
  size_t bytecodeSz = bytecode->GetBufferSize();
  uint32_t strip_reflect_flags = D3DCOMPILER_STRIP_REFLECTION_DATA;

  ID3DBlob *bytecodeStripped = NULL;
  if (hlsl_debug_level == DebugLevel::NONE && debug_parts == DebugParts::STRIP)
    D3DStripShader(bytecodeMsg, bytecodeSz,
      strip_reflect_flags | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
      &bytecodeStripped);
  if (bytecodeStripped)
  {
    bytecodeMsg = (const char *)bytecodeStripped->GetBufferPointer();
    bytecodeSz = bytecodeStripped->GetBufferSize();
  }

  DEFER([&] {
    if (bytecodeStripped)
      bytecodeStripped->Release();
  });

  dx11::SimpleHeader header{};
  header.bytecodeByteSize = (uint32_t)bytecodeSz;
  // @TODO: here we store the old-style max-used-register, so as not to change format. Deprecate this altogether!
  header.maxConstantRegUsed = max(implicit_cbuf_size - 1, 0);
  header.maxRtvUsed = (int8_t)maxRtv;
  dx11::set_uav_masks(header, uavAccess.read, uavAccess.write);
  header.maxConstantRegUsed |= hsTopology << dx11::HS_TOPOLOGY_SHIFT;
  eastl::tie(result.metadata, result.bytecode) = get_blob_with_header_aligned(header, bytecodeMsg);

  return result;
}
