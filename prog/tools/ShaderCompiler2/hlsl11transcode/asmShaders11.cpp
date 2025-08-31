// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../fast_isalnum.h"
#include "asmShaders11.h"

#include <D3Dcompiler.h>

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_unicode.h>
#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

#define DBG_SHDR 0
#if DBG_SHDR
int g_index = 0;
#endif

eastl::vector<uint8_t> get_blob_with_header_aligned(void *data, unsigned int data_size, void *header, unsigned int header_size)
{
  unsigned data_size_aligned = (data_size + 3) & (~3);
  int size = data_size_aligned + header_size + 4;
  eastl::vector<uint8_t> result(size);
  memcpy(result.data(), &data_size, 4);
  memcpy(result.data() + 4, header, header_size);
  memcpy(result.data() + header_size + 4, data, data_size);
  memset(result.data() + data_size + header_size + 4, 0, data_size_aligned - data_size);
  return result;
}

CompileResult compileShaderDX11(const char *shaderName, const char *source, const char **args, const char *profile, const char *entry,
  bool need_disasm, DebugLevel hlsl_debug_level, bool skip_validation, bool embed_source, unsigned flags, int max_constants_no)
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
  G_UNUSED(shaderName);

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
  if (embed_source)
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

  if (profile[0] == 'c')
  {
    ID3D11ShaderReflection *reflector = nullptr;
    hr = D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_PPV_ARGS(&reflector));
    G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the compute shader");
    reflector->GetThreadGroupSize(&result.computeShaderInfo.threadGroupSizeX, &result.computeShaderInfo.threadGroupSizeY,
      &result.computeShaderInfo.threadGroupSizeZ);
    reflector->Release();
  }

  if (need_disasm)
  {
    ID3D10Blob *disasm = nullptr;
    hr = D3DDisassemble((DWORD *)bytecode->GetBufferPointer(), bytecode->GetBufferSize(), 0, NULL, &disasm);
    if (disasm)
    {
      result.disassembly.assign((const char *)disasm->GetBufferPointer(),
        (const char *)disasm->GetBufferPointer() + disasm->GetBufferSize());
      disasm->Release();
    }
  }
  const char *bytecodeMsg = (const char *)bytecode->GetBufferPointer();
  size_t bytecodeSz = bytecode->GetBufferSize();
  uint32_t strip_reflect_flags = D3DCOMPILER_STRIP_REFLECTION_DATA;

  ID3DBlob *bytecodeStripped = NULL;
  if (hlsl_debug_level == DebugLevel::NONE)
    D3DStripShader(bytecodeMsg, bytecodeSz,
      strip_reflect_flags | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
      &bytecodeStripped);
  if (bytecodeStripped)
  {
    // debug("%s %s: %d -> %d", entry, profile, bytecodeSz, bytecodeStripped->GetBufferSize());
    bytecodeMsg = (const char *)bytecodeStripped->GetBufferPointer();
    bytecodeSz = bytecodeStripped->GetBufferSize();
  }

  // size_t extsize32 = 1 + (bytecodeSz + sizeof(uint32_t)-1) / sizeof(uint32_t);
  // uint32_t *data = new uint32_t [extsize32];
  // memset(data, 0x00, extsize32 * sizeof(uint32_t)); //clean required bytes
  // memcpy(data+1, bytecodeMsg, bytecodeSz);
  //*data = bytecodeSz;
  int header[2] = {max_constants_no, -1};
  if (strncmp(profile, "hs", 2) == 0)
  {
    ID3D10Blob *hs_disasm = nullptr;
    int topology = 0;
    if (D3DDisassemble((DWORD *)bytecode->GetBufferPointer(), bytecode->GetBufferSize(), 0, NULL, &hs_disasm) == S_OK)
    {
      const char *p = strstr((const char *)hs_disasm->GetBufferPointer(), "// Tessellation Domain");
      if (p)
        p = strchr(p, '\n');
      if (p)
        p = strchr(p + 1, '\n');
      if (p)
        p = strstr(p + 1, "// ");
      if (p)
      {
        p += 3;
        const char *p2 = p;
        while (fast_isalnum(*p2))
          p2++;
        const char *p3 = p2;
        while (!isdigit(*p3) && *p3 != '\n' && *p3 != '\0')
          p3++;

        if (strncmp(p, "Tri", 3) == 0 || strncmp(p, "Quad", 4) == 0 || strncmp(p, "Isoline", 7) == 0)
          if (int n = atoi(p3))
            topology = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + n - 1;
        if (!topology)
          logerr("HS: failed to parse <%.*s> <%.*s>", p2 - p, p, strchr(p3, '\n') - p3, p3);
        else
          debug("HS: parsed topology=%d from  <%.*s> <%.*s>", topology, p2 - p, p, strchr(p3, '\n') - p3, p3);
      }
      hs_disasm->Release();
    }
    if (!topology)
      logwarn("HS: topology undefined, using D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST");

    header[0] |= topology << 24;
  }
  result.bytecode = get_blob_with_header_aligned((void *)bytecodeMsg, (int)bytecodeSz, header, (int)sizeof(header));

  if (bytecode)
    bytecode->Release();
  if (bytecodeStripped)
    bytecodeStripped->Release();

  return result;
}
