// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>

namespace d3d_multi_dx11
{
#include "d3d_api.inc.h"
}

namespace drv3d_dx11
{
VPROG create_vertex_shader_unpacked(const void *shader_bin, uint32_t size, uint32_t vs_consts_count, bool do_fatal);
FSHADER create_pixel_shader_unpacked(const void *shader_bin, uint32_t size, uint32_t ps_consts_count, bool do_fatal);
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
static eastl::unique_ptr<HMODULE, LibraryReleaser> d3dcompiler_module;
#define D3DCOMPILE_FUNC_NAME "D3DCompile"
static pD3DCompile d3d_compile = nullptr;
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
    logerr("Can't compile:\n %s", err_str);
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
      if (!d3d_compile)
      {
        d3dcompiler_module.reset();
        return report_error("GetProcAddress(\"" D3DCOMPILER_DLL_A "\", \"" D3DCOMPILE_FUNC_NAME "\") failed");
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

VPROG d3d::create_vertex_shader_hlsl(const char *hlsl_text, unsigned /*len*/, const char *entry, const char *profile, String *out_err)
{
  if (d3d::get_driver_code().is(d3d::dx11))
  {
    int max_const = 128;
    if (const char *p = strchr(profile, ':'))
    {
      max_const = atoi(profile);
      profile = p + 1;
    }

    ID3D10Blob *shader = compile_hlsl(hlsl_text, entry, profile, out_err);
    VPROG vpr = BAD_VPROG;
    if (shader)
    {
      vpr = drv3d_dx11::create_vertex_shader_unpacked(shader->GetBufferPointer(), shader->GetBufferSize(), max_const, false);
      shader->Release();
    }
    else
      logerr("VS compile failed: %s, %s", entry, profile);
    return vpr;
  }
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
      fsh = drv3d_dx11::create_pixel_shader_unpacked(shader->GetBufferPointer(), shader->GetBufferSize(), max_const, false);
      shader->Release();
    }
    else
      logerr("PS compile failed: %s, %s", entry, profile);
    return fsh;
  }
  if (d3d::get_driver_code().is(d3d::stub))
    return 1;
  return BAD_FSHADER;
}

bool d3d::compile_compute_shader_hlsl(const char *hlsl_text, unsigned /*len*/, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
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
    shader_bin.assign((shader->GetBufferSize() + sizeof(uint32_t) - 1) / sizeof(uint32_t) + 3, 0);
    shader_bin[0] = shader->GetBufferSize();
    memcpy(shader_bin.data() + 3, shader->GetBufferPointer(), shader->GetBufferSize());
    shader->Release();
    return true;
  }
  else
    logerr("CS compile failed: %s, %s", entry, profile);
  return false;
}
