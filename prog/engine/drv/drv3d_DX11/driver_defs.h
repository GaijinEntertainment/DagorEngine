// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define INSIDE_DRIVER 1
#define LLLOG \
  if (0)      \
  debug

#include <d3d11.h>
#include "d3d_config.h"

#define IDXGI_SWAP_CHAIN           IDXGISwapChain
#define IDXGI_FACTORY              IDXGIFactory1
#define ID3D11_DEV                 ID3D11Device
#define ID3D11_DEV1                ID3D11Device1
#define ID3D11_DEV3                ID3D11Device3
#define ID3D11_DEVCTX              ID3D11DeviceContext
#define ID3D11_DEVCTX1             ID3D11DeviceContext1
#define D3D11_RASTERIZER_DESC_NEXT D3D11_RASTERIZER_DESC2
struct RENDERDOC_API_1_5_0;

#if DAGOR_DBGLEVEL > 0
#define CHECK_MAIN_THREAD()                                                                                                     \
  G_ASSERTF((drv3d_dx11::gpuAcquireRefCount && drv3d_dx11::gpuThreadId == GetCurrentThreadId()),                                \
    "curThread=%X != gpuThread=%d, is_main=%d acquire_ref=%d", GetCurrentThreadId(), drv3d_dx11::gpuThreadId, is_main_thread(), \
    drv3d_dx11::gpuAcquireRefCount)
#else
#define CHECK_MAIN_THREAD() ((void)0)
#endif
#define CHECK_THREAD CHECK_MAIN_THREAD()

#define MAX_PS_SAMPLERS 16
#define MAX_VS_SAMPLERS 16
#define MAX_CS_SAMPLERS 16

#define MAX_RESOURCES 32
// D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT

#define MAX_UAV           8
#define MAX_CONST_BUFFERS 9

#define MAX_VERTEX_STREAMS 4
#define PRIM_UNDEF         0

#define SAFE_RELEASE(obj) \
  if (obj != NULL)        \
  {                       \
    obj->Release();       \
    obj = NULL;           \
  }
#define COPY_KEY(name)          u.s.name = name
#define COPY_KEY_PTR(ptr, name) u.s.name = ptr->name

#define NVAPI_SLI_SYNC 0

#define PC_BACKBUFFER_DXGI_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM

#define IMMEDIATE_CB_NAMESPACE namespace drv3d_dx11

#define DEBUG_MEMORY_METRICS DAGOR_DBGLEVEL > 0

#define __DXFATAL(x, ...) DAG_FATAL(x, ##__VA_ARGS__)
#define __DXERR(x, ...)   LOGERR_CTX(x, ##__VA_ARGS__)
//-V:DXFATAL:1048
#define DXFATAL(expr, text, ...)                                                                        \
  do                                                                                                    \
  {                                                                                                     \
    HRESULT __hres = (expr);                                                                            \
    drv3d_dx11::last_hres = __hres;                                                                     \
    if (FAILED(__hres))                                                                                 \
    {                                                                                                   \
      dump_memory_if_needed(__hres);                                                                    \
      __DXFATAL("dx11 error: %s hr=0x%X " text, drv3d_dx11::dx11_error(__hres), __hres, ##__VA_ARGS__); \
    }                                                                                                   \
  } while (0)
#define DXERR(expr, text, ...)                                                                        \
  do                                                                                                  \
  {                                                                                                   \
    HRESULT __hres = (expr);                                                                          \
    drv3d_dx11::last_hres = __hres;                                                                   \
    if (FAILED(__hres))                                                                               \
    {                                                                                                 \
      dump_memory_if_needed(__hres);                                                                  \
      __DXERR("dx11 error: %s hr=0x%X " text, drv3d_dx11::dx11_error(__hres), __hres, ##__VA_ARGS__); \
    }                                                                                                 \
  } while (0)

#define RETRY_CREATE_STAGING_TEX(HR, SZ, EXPR)                                       \
  if (DAGOR_UNLIKELY(FAILED(HR)) && (HR) == E_OUTOFMEMORY)                           \
  {                                                                                  \
    if (dgs_on_out_of_memory && dgs_on_out_of_memory(tql::sizeInKb(SZ + (2 << 20)))) \
      HR = EXPR;                                                                     \
  }
