// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_driverCode.h>
#include <drv_log_defs.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_platform_pc.h>
#include <drvCommonConsts.h>
#include <drv/3d/dag_shader.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>

namespace d3d_multi_dx11
{
#include "d3d_api.inc.h"
}

namespace drv3d_dx11
{
VPROG create_vertex_shader_unpacked(const ShaderSource &source, const void *shader_bin, uint32_t size, uint32_t vs_consts_count,
  bool do_fatal);
FSHADER create_pixel_shader_unpacked(const ShaderSource &source, const void *shader_bin, uint32_t size, uint32_t ps_consts_count,
  int32_t ps_max_rtv, bool do_fatal);
} // namespace drv3d_dx11

#include <D3DCommon.h>
#include <D3Dcompiler.h>

struct LibraryReleaser
{
  typedef HMODULE pointer;
  void operator()(HMODULE ptr)
  {
    if (ptr)
      FreeLibrary(ptr);
  }
};

using pD3DReflect = decltype(&D3DReflect);

static eastl::unique_ptr<HMODULE, LibraryReleaser> d3dcompiler_module;
#define D3DCOMPILE_FUNC_NAME "D3DCompile"
#define D3DREFLECT_FUNC_NAME "D3DReflect"
static pD3DCompile d3d_compile = nullptr;
static pD3DReflect d3d_reflect = nullptr;
namespace drv3d_dx11
{
extern __declspec(thread) HRESULT last_hres;
}

void shutdown_hlsl_d3dcompiler_internal()
{
  d3d_compile = nullptr;
  d3dcompiler_module.reset();
}

static ID3D10Blob *compile_hlsl(const char *hlsl_text, const char *entry, const char *profile, String *out_err)
{
  auto report_error = [out_err](const char *err_str) -> ID3D10Blob * {
    D3D_ERROR("Can't compile:\n %s", err_str);
    if (out_err)
      out_err->aprintf(1024, "%s\n", err_str);
    return nullptr;
  };

  if (!d3d_compile)
  {
    d3dcompiler_module.reset(LoadLibraryA(D3DCOMPILER_DLL_A));
    if (d3dcompiler_module)
    {
      // cast to void* to avoid C4191 warning
      d3d_compile = (pD3DCompile)(void *)GetProcAddress(d3dcompiler_module.get(), D3DCOMPILE_FUNC_NAME);
      d3d_reflect = (pD3DReflect)(void *)GetProcAddress(d3dcompiler_module.get(), D3DREFLECT_FUNC_NAME);
      if (!d3d_compile || !d3d_reflect)
      {
        d3dcompiler_module.reset();
        if (!d3d_compile)
          report_error("GetProcAddress(\"" D3DCOMPILER_DLL_A "\", \"" D3DCOMPILE_FUNC_NAME "\") failed");
        if (!d3d_reflect)
          report_error("GetProcAddress(\"" D3DCOMPILER_DLL_A "\", \"" D3DREFLECT_FUNC_NAME "\") failed");
        return nullptr;
      }
    }
    else
      return report_error(D3DCOMPILER_DLL_A " load failed");
  }

  ID3D10Blob *bytecode = NULL;
  ID3D10Blob *errors = NULL;

  G_ASSERT(d3d_compile);
  HRESULT hr = d3d_compile(hlsl_text, (int)strlen(hlsl_text), NULL, NULL, NULL, entry, profile,
    D3DCOMPILE_OPTIMIZATION_LEVEL3 /*|D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY*/, NULL, &bytecode, &errors);

  drv3d_dx11::last_hres = hr;
  if (FAILED(hr))
  {
    String str;
    if (errors)
    {
      // debug("errors compiling HLSL code:\n%s", hlsl_text);
      const char *es = (const char *)errors->GetBufferPointer(), *ep = strchr(es, '\n');
      while (es)
      {
        if (ep)
        {
          str.aprintf(256, "  %.*s\n", ep - es, es);
          es = ep + 1;
          ep = strchr(es, '\n');
        }
        else
          break;
      }
      report_error(str.data());
    }
    else
      report_error("<unknown>");
    if (bytecode)
      bytecode->Release();
    bytecode = NULL;
  }

  if (errors)
    errors->Release();
  return bytecode;
}


//
// DX12: DXC runtime compilation (dxcompiler.dll)
//
#ifdef USE_MULTI_D3D_DX12

#include <supp/dag_comPtr.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <dxc/dxcapi.h>
// d3d12shader.h uses DECLARE_INTERFACE/THIS macros from basetyps.h, but some headers #undef THIS.
#ifndef THIS_
#define THIS_
#endif
#ifndef THIS
#define THIS void
#endif
#include <d3d12shader.h>

namespace drv3d_dx12
{
VPROG create_vertex_shader_dxil(const void *bytecode, uint32_t size, const dxil::ShaderHeader &header);
FSHADER create_pixel_shader_dxil(const void *bytecode, uint32_t size, const dxil::ShaderHeader &header);
} // namespace drv3d_dx12

namespace
{

struct DxcState
{
  HMODULE dxcModule = nullptr;
  ComPtr<IDxcCompiler3> compiler;
  ComPtr<IDxcUtils> utils;
  bool initFailed = false;

  bool ensureLoaded()
  {
    if (compiler.Get())
      return true;
    if (initFailed)
      return false;

    dxcModule = LoadLibraryA("dxcompiler.dll");
    if (!dxcModule)
    {
      D3D_ERROR("DX12: dxcompiler.dll not found, runtime HLSL compilation unavailable");
      initFailed = true;
      return false;
    }

    // cast to void* to avoid C4191 warning (same pattern as D3DCompiler loading above)
    auto dxcCreateInstance = (DxcCreateInstanceProc)(void *)GetProcAddress(dxcModule, "DxcCreateInstance");
    if (!dxcCreateInstance)
    {
      D3D_ERROR("DX12: DxcCreateInstance not found in dxcompiler.dll");
      FreeLibrary(dxcModule);
      dxcModule = nullptr;
      initFailed = true;
      return false;
    }

    HRESULT hr = dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr))
    {
      D3D_ERROR("DX12: Failed to create IDxcCompiler3 (hr=0x%08X)", hr);
      FreeLibrary(dxcModule);
      dxcModule = nullptr;
      initFailed = true;
      return false;
    }

    hr = dxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(hr))
    {
      D3D_ERROR("DX12: Failed to create IDxcUtils (hr=0x%08X)", hr);
      compiler.Reset();
      FreeLibrary(dxcModule);
      dxcModule = nullptr;
      initFailed = true;
      return false;
    }

    // D3D12 rejects unsigned DXIL with E_INVALIDARG at CreatePipelineState.
    // DXC signs DXIL using dxil.dll — if it's missing, compilation succeeds but
    // the output is unusable. Check early to give a clear error instead of
    // per-frame CreatePipelineState failures.
    HMODULE dxilModule = LoadLibraryA("dxil.dll");
    if (!dxilModule)
    {
      D3D_ERROR("DX12: dxil.dll not found — DXIL signing unavailable, runtime HLSL compilation disabled "
                "(D3D12 rejects unsigned DXIL)");
      compiler.Reset();
      utils.Reset();
      FreeLibrary(dxcModule);
      dxcModule = nullptr;
      initFailed = true;
      return false;
    }
    FreeLibrary(dxilModule);

    return true;
  }

  ~DxcState()
  {
    compiler.Reset();
    utils.Reset();
    if (dxcModule)
      FreeLibrary(dxcModule);
    dxcModule = nullptr;
  }
};

static DxcState dxc_state;

struct DxcCompileResult
{
  ComPtr<IDxcBlob> bytecode;
  ComPtr<IDxcBlob> reflection;
};

// Translate SM 5.0 profiles to SM 6.0 for DXC (DXC requires SM 6.0+)
static eastl::wstring translate_profile_for_dxc(const char *profile)
{
  char buf[32];
  strncpy(buf, profile, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char *first_under = strchr(buf, '_');
  if (first_under)
  {
    char *second_under = strchr(first_under + 1, '_');
    if (second_under)
    {
      int major = atoi(first_under + 1);
      if (major < 6)
      {
        first_under[1] = '6';
        second_under[1] = '0';
        second_under[2] = '\0';
      }
    }
  }

  eastl::wstring result;
  result.reserve(strlen(buf));
  for (const char *p = buf; *p; ++p)
    result.push_back(static_cast<wchar_t>(*p));
  return result;
}

static DxcCompileResult compile_hlsl_dxc(const char *hlsl_text, const char *entry, const char *profile, String *out_err)
{
  DxcCompileResult result = {};

  if (!dxc_state.ensureLoaded())
  {
    if (out_err)
      out_err->aprintf(256, "DXC compiler not available\n");
    return result;
  }

  eastl::wstring wideProfile = translate_profile_for_dxc(profile);

  eastl::wstring wideEntry;
  for (const char *p = entry; *p; ++p)
    wideEntry.push_back(static_cast<wchar_t>(*p));

  LPCWSTR args[] = {
    L"-T",
    wideProfile.c_str(),
    L"-E",
    wideEntry.c_str(),
    L"-O3",
    L"-HV",
    L"2018",
    L"-Qstrip_reflect",
  };

  DxcBuffer srcBuf = {hlsl_text, strlen(hlsl_text), DXC_CP_ACP};

  ComPtr<IDxcResult> compileResult;
  HRESULT hr = dxc_state.compiler->Compile(&srcBuf, args, countof(args), nullptr, IID_PPV_ARGS(&compileResult));

  ComPtr<IDxcBlobUtf8> errors;
  if (compileResult.Get())
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

  if (errors.Get() && errors->GetStringLength() > 0)
  {
    const char *errStr = errors->GetStringPointer();
    D3D_ERROR("DX12 DXC: %s", errStr);
    if (out_err)
      out_err->aprintf(1024, "%s\n", errStr);
  }

  HRESULT status = E_FAIL;
  if (compileResult.Get())
    compileResult->GetStatus(&status);

  if (FAILED(hr) || FAILED(status))
  {
    if (out_err && (!errors.Get() || errors->GetStringLength() == 0))
      out_err->aprintf(256, "DXC compilation failed (hr=0x%08X, status=0x%08X)\n", hr, status);
    return result;
  }

  compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.bytecode), nullptr);
  compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&result.reflection), nullptr);

  if (!result.bytecode.Get() || !result.reflection.Get())
  {
    D3D_ERROR("DX12 DXC: compilation succeeded but missing output (bytecode=%p, reflection=%p)", result.bytecode.Get(),
      result.reflection.Get());
    if (out_err)
      out_err->aprintf(256, "DXC compilation produced no %s output\n", !result.bytecode.Get() ? "bytecode" : "reflection");
  }

  return result;
}

static bool fill_shader_header_from_reflection(dxil::ShaderHeader &header, dxil::ShaderStage stage, const ComPtr<IDxcBlob> &reflBlob)
{
  memset(&header, 0, sizeof(header));
  header.shaderType = static_cast<uint8_t>(stage);

  DxcBuffer reflBuf = {reflBlob->GetBufferPointer(), reflBlob->GetBufferSize(), DXC_CP_ACP};
  ComPtr<ID3D12ShaderReflection> reflector;
  HRESULT hr = dxc_state.utils->CreateReflection(&reflBuf, IID_PPV_ARGS(&reflector));
  if (FAILED(hr))
  {
    D3D_ERROR("DX12: CreateReflection failed (hr=0x%08X)", hr);
    return false;
  }

  D3D12_SHADER_DESC desc = {};
  reflector->GetDesc(&desc);

  // Device requirements
  uint64_t reqFlags = reflector->GetRequiresFlags();
  header.deviceRequirement.shaderModel = static_cast<uint8_t>(desc.Version);
  header.deviceRequirement.shaderFeatureFlagsLow = static_cast<uint32_t>(reqFlags);
  header.deviceRequirement.shaderFeatureFlagsHigh = static_cast<uint32_t>(reqFlags >> 32);
  header.deviceRequirement.extensionVendor = dxil::ExtensionVendor::NoExtensionsUsed;
  header.deviceRequirement.vendorExtensionMask = 0;

  header.inputPrimitive = static_cast<uint16_t>(desc.InputPrimitive);

  // Extract $Globals CBV size -> maxConstantCount
  for (UINT cbi = 0; cbi < desc.ConstantBuffers; ++cbi)
  {
    auto *cbInfo = reflector->GetConstantBufferByIndex(cbi);
    if (!cbInfo)
      continue;
    D3D12_SHADER_BUFFER_DESC cbDesc = {};
    cbInfo->GetDesc(&cbDesc);
    if (strcmp(cbDesc.Name, "$Globals") == 0)
    {
      header.maxConstantCount = (cbDesc.Size + 15) / 16;
      break;
    }
  }

  // Resource bindings (space 0 only, no bindless/extensions)
  for (UINT bri = 0; bri < desc.BoundResources; ++bri)
  {
    D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
    reflector->GetResourceBindingDesc(bri, &bindDesc);

    if (bindDesc.Space != 0 || bindDesc.BindCount == 0)
      continue;

    switch (bindDesc.Type)
    {
      case D3D_SIT_CBUFFER:
        if (bindDesc.BindPoint < dxil::MAX_B_REGISTERS)
          header.resourceUsageTable.bRegisterUseMask |= 1u << bindDesc.BindPoint;
        break;

      case D3D_SIT_SAMPLER:
        if (bindDesc.BindPoint < dxil::MAX_T_REGISTERS)
        {
          header.resourceUsageTable.sRegisterUseMask |= 1u << bindDesc.BindPoint;
          if (bindDesc.uFlags & D3D_SIF_COMPARISON_SAMPLER)
            header.sRegisterCompareUseMask |= 1u << bindDesc.BindPoint;
        }
        break;

      case D3D_SIT_TEXTURE:
      case D3D_SIT_STRUCTURED:
      case D3D_SIT_BYTEADDRESS:
      case D3D_SIT_RTACCELERATIONSTRUCTURE:
        for (UINT i = 0; i < bindDesc.BindCount && (bindDesc.BindPoint + i) < dxil::MAX_T_REGISTERS; ++i)
        {
          header.resourceUsageTable.tRegisterUseMask |= 1u << (bindDesc.BindPoint + i);
          header.tRegisterTypes[bindDesc.BindPoint + i] = static_cast<uint8_t>(bindDesc.Type | (bindDesc.Dimension << 4));
        }
        break;

      case D3D_SIT_UAV_RWTYPED:
      case D3D_SIT_UAV_RWSTRUCTURED:
      case D3D_SIT_UAV_RWBYTEADDRESS:
      case D3D_SIT_UAV_APPEND_STRUCTURED:
      case D3D_SIT_UAV_CONSUME_STRUCTURED:
      case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        for (UINT i = 0; i < bindDesc.BindCount && (bindDesc.BindPoint + i) < dxil::MAX_U_REGISTERS; ++i)
        {
          header.resourceUsageTable.uRegisterUseMask |= 1u << (bindDesc.BindPoint + i);
          header.uRegisterTypes[bindDesc.BindPoint + i] = static_cast<uint8_t>(bindDesc.Type | (bindDesc.Dimension << 4));
        }
        break;

      default: break;
    }
  }

  // VS: input semantics
  if (dxil::ShaderStage::VERTEX == stage)
  {
    for (UINT ipi = 0; ipi < desc.InputParameters; ++ipi)
    {
      D3D12_SIGNATURE_PARAMETER_DESC inputInfo = {};
      reflector->GetInputParameterDesc(ipi, &inputInfo);
      if (inputInfo.SystemValueType == D3D_NAME_UNDEFINED)
      {
        uint32_t id = dxil::getIndexFromSementicAndSemanticIndex(inputInfo.SemanticName, inputInfo.SemanticIndex);
        if (id < 32)
          header.inOutSemanticMask |= 1u << id;
      }
    }
  }

  // PS: output render target mask
  if (dxil::ShaderStage::PIXEL == stage)
  {
    for (UINT opi = 0; opi < desc.OutputParameters; ++opi)
    {
      D3D12_SIGNATURE_PARAMETER_DESC outputInfo = {};
      reflector->GetOutputParameterDesc(opi, &outputInfo);
      if (outputInfo.SystemValueType == D3D_NAME_TARGET)
        header.inOutSemanticMask |= uint32_t((~outputInfo.ReadWriteMask) & 0xF) << (outputInfo.SemanticIndex * 4);
    }
  }

  return true;
}

static VPROG create_vertex_shader_dxc(const char *hlsl_text, const char *entry, const char *profile, String *out_err)
{
  auto compiled = compile_hlsl_dxc(hlsl_text, entry, profile, out_err);
  if (!compiled.bytecode.Get() || !compiled.reflection.Get())
    return BAD_VPROG;

  dxil::ShaderHeader header;
  if (!fill_shader_header_from_reflection(header, dxil::ShaderStage::VERTEX, compiled.reflection))
    return BAD_VPROG;

  return drv3d_dx12::create_vertex_shader_dxil(compiled.bytecode->GetBufferPointer(), (uint32_t)compiled.bytecode->GetBufferSize(),
    header);
}

static FSHADER create_pixel_shader_dxc(const char *hlsl_text, const char *entry, const char *profile, String *out_err)
{
  auto compiled = compile_hlsl_dxc(hlsl_text, entry, profile, out_err);
  if (!compiled.bytecode.Get() || !compiled.reflection.Get())
    return BAD_FSHADER;

  dxil::ShaderHeader header;
  if (!fill_shader_header_from_reflection(header, dxil::ShaderStage::PIXEL, compiled.reflection))
    return BAD_FSHADER;

  return drv3d_dx12::create_pixel_shader_dxil(compiled.bytecode->GetBufferPointer(), (uint32_t)compiled.bytecode->GetBufferSize(),
    header);
}

} // anonymous namespace
#endif // USE_MULTI_D3D_DX12


VPROG d3d::create_vertex_shader_hlsl(const char *hlsl_text, unsigned /*len*/, const char *entry, const char *profile, String *out_err)
{
  if (d3d::get_driver_code().is(d3d::dx11))
  {
    int max_const = MAX_PS_CONSTS;
    if (const char *p = strchr(profile, ':'))
    {
      max_const = atoi(profile);
      profile = p + 1;
    }

    ID3D10Blob *shader = compile_hlsl(hlsl_text, entry, profile, out_err);
    VPROG vpr = BAD_VPROG;
    if (shader)
    {
      vpr = drv3d_dx11::create_vertex_shader_unpacked({}, shader->GetBufferPointer(), shader->GetBufferSize(), max_const, false);
      shader->Release();
    }
    else
      D3D_ERROR("VS compile failed: %s, %s", entry, profile);
    return vpr;
  }
#ifdef USE_MULTI_D3D_DX12
  else if (d3d::get_driver_code().is(d3d::dx12))
  {
    if (const char *p = strchr(profile, ':'))
      profile = p + 1;
    return create_vertex_shader_dxc(hlsl_text, entry, profile, out_err);
  }
#endif
  else if (d3d::get_driver_code().is(d3d::stub))
    return 1;
  return BAD_VPROG;
}

FSHADER d3d::create_pixel_shader_hlsl(const char *hlsl_text, unsigned /*len*/, const char *entry, const char *profile, String *out_err)
{
  if (d3d::get_driver_code().is(d3d::dx11))
  {
    int max_const = 64;
    if (const char *p = strchr(profile, ':'))
    {
      max_const = atoi(profile);
      profile = p + 1;
    }

    ID3D10Blob *shader = compile_hlsl(hlsl_text, entry, profile, out_err);
    FSHADER fsh = BAD_FSHADER;
    if (shader)
    {
      int32_t maxRtv = -1;
      {
        ID3D11ShaderReflection *reflector = nullptr;
        HRESULT hr = d3d_reflect(shader->GetBufferPointer(), shader->GetBufferSize(), IID_PPV_ARGS(&reflector));
        G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the pixel shader");

        D3D11_SHADER_DESC desc;
        hr = reflector->GetDesc(&desc);
        G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the pixel shader desc");

        for (uint32_t i = 0; i < desc.OutputParameters; ++i)
        {
          D3D11_SIGNATURE_PARAMETER_DESC out;
          reflector->GetOutputParameterDesc(i, &out);
          if (strcmp(out.SemanticName, "SV_Target") == 0)
            maxRtv = max(maxRtv, int(out.SemanticIndex));
        }
      }

      fsh =
        drv3d_dx11::create_pixel_shader_unpacked({}, shader->GetBufferPointer(), shader->GetBufferSize(), max_const, maxRtv, false);
      shader->Release();
    }
    else
      D3D_ERROR("PS compile failed: %s, %s", entry, profile);
    return fsh;
  }
#ifdef USE_MULTI_D3D_DX12
  else if (d3d::get_driver_code().is(d3d::dx12))
  {
    if (const char *p = strchr(profile, ':'))
      profile = p + 1;
    return create_pixel_shader_dxc(hlsl_text, entry, profile, out_err);
  }
#endif
  if (d3d::get_driver_code().is(d3d::stub))
    return 1;
  return BAD_FSHADER;
}

bool d3d::compile_compute_shader_hlsl(const char *hlsl_text, unsigned /*len*/, const char *entry, const char *profile,
  Tab<uint8_t> &metadata, Tab<uint32_t> &shader_bin, String &out_err)
{
  int max_const = 64;
  if (const char *p = strchr(profile, ':'))
  {
    max_const = atoi(profile);
    profile = p + 1;
  }

  ID3D10Blob *shader = compile_hlsl(hlsl_text, entry, profile, &out_err);
  if (shader)
  {
    metadata.resize(12, 0);
    *(uint32_t *)&metadata[0] = shader->GetBufferSize();
    shader_bin.assign((shader->GetBufferSize() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
    memcpy(shader_bin.data(), shader->GetBufferPointer(), shader->GetBufferSize());
    shader->Release();
    return true;
  }
  else
    D3D_ERROR("CS compile failed: %s, %s", entry, profile);
  return false;
}
