// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include <util/dag_compilerDefs.h>

#define D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION _HRESULT_TYPEDEF_(0x887E0003L)

namespace drv3d_dx12
{
void report_oom_info();
void set_last_error(HRESULT error);

DAGOR_NOINLINE
HRESULT debug_result(HRESULT result, const char *expr, const char *file, int line);

DAGOR_NOINLINE
HRESULT check_result(HRESULT result, const char *expr, const char *file, int line);

DAGOR_NOINLINE
HRESULT check_result_no_oom_report(HRESULT result, const char *expr, const char *file, int line);

constexpr bool is_oom_error_code(HRESULT result)
{
  return E_OUTOFMEMORY == result; //
}

constexpr bool is_gpu_crash_code(HRESULT result)
{
  return DXGI_ERROR_DEVICE_REMOVED == result ||      //
         DXGI_ERROR_DEVICE_HUNG == result ||         //
         DXGI_ERROR_DEVICE_RESET == result ||        //
         DXGI_ERROR_DRIVER_INTERNAL_ERROR == result; //
}

constexpr bool is_recoverable_error(HRESULT error)
{
  switch (error)
  {
    default: return true;
    // any device error is not recoverable
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_RESET: return false;
  }
}

constexpr const char *dxgi_error_code_to_string(HRESULT ec)
{
#define ENUM_CASE(Name) \
  case Name: return #Name
  switch (ec)
  {
    ENUM_CASE(E_FAIL); // returned by init code if a step fails in a fatal way
    ENUM_CASE(DXGI_ERROR_INVALID_CALL);
    ENUM_CASE(DXGI_ERROR_NOT_FOUND);
    ENUM_CASE(DXGI_ERROR_MORE_DATA);
    ENUM_CASE(DXGI_ERROR_UNSUPPORTED);
    ENUM_CASE(DXGI_ERROR_DEVICE_REMOVED);
    ENUM_CASE(DXGI_ERROR_DEVICE_HUNG);
    ENUM_CASE(DXGI_ERROR_DEVICE_RESET);
    ENUM_CASE(DXGI_ERROR_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_FRAME_STATISTICS_DISJOINT);
    ENUM_CASE(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE);
    ENUM_CASE(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    ENUM_CASE(DXGI_ERROR_NONEXCLUSIVE);
    ENUM_CASE(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
    ENUM_CASE(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_REMOTE_OUTOFMEMORY);
    ENUM_CASE(DXGI_ERROR_ACCESS_LOST);
    ENUM_CASE(DXGI_ERROR_WAIT_TIMEOUT);
    ENUM_CASE(DXGI_ERROR_SESSION_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE);
    ENUM_CASE(DXGI_ERROR_CANNOT_PROTECT_CONTENT);
    ENUM_CASE(DXGI_ERROR_ACCESS_DENIED);
    ENUM_CASE(DXGI_ERROR_NAME_ALREADY_EXISTS);
    ENUM_CASE(DXGI_STATUS_UNOCCLUDED);
    ENUM_CASE(DXGI_STATUS_DDA_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_MODE_CHANGE_IN_PROGRESS);
    ENUM_CASE(E_INVALIDARG);
    ENUM_CASE(E_OUTOFMEMORY);
    ENUM_CASE(E_ACCESSDENIED);
    ENUM_CASE(E_NOINTERFACE);
#if _TARGET_PC_WIN
    ENUM_CASE(D3D12_ERROR_ADAPTER_NOT_FOUND);
    ENUM_CASE(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
    ENUM_CASE(D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION);
#endif
  }
#undef ENUM_CASE

  return "";
}

DAGOR_NOINLINE
void report_resource_alloc_info_error(const D3D12_RESOURCE_DESC &desc);

DAGOR_NOINLINE
void report_agility_sdk_error(HRESULT hr);

} // namespace drv3d_dx12

#define DX12_DEBUG_RESULT(expr) \
  [result = expr] { return DAGOR_LIKELY(SUCCEEDED(result)) ? result : drv3d_dx12::debug_result(result, #expr, __FILE__, __LINE__); }()
#define DX12_DEBUG_OK(expr)   SUCCEEDED(DX12_DEBUG_RESULT(expr))
#define DX12_DEBUG_FAIL(expr) FAILED(DX12_DEBUG_RESULT(expr))

#define DX12_CHECK_RESULTF(expr, name) \
  [result = expr] { return DAGOR_LIKELY(SUCCEEDED(result)) ? result : drv3d_dx12::check_result(result, name, __FILE__, __LINE__); }()
#define DX12_CHECK_RESULT(expr) DX12_CHECK_RESULTF(expr, #expr)
#define DX12_CHECK_OK(expr)     SUCCEEDED(DX12_CHECK_RESULT(expr))
#define DX12_CHECK_FAIL(expr)   FAILED(DX12_CHECK_RESULT(expr))
#define DX12_EXIT_ON_FAIL(expr) \
  if (DX12_CHECK_FAIL(expr))    \
  {                             \
    /* no-op */                 \
  }

#define DX12_CHECK_RESULT_NO_OOM_CHECK(expr)                                                                                     \
  [result = expr] {                                                                                                              \
    return DAGOR_LIKELY(SUCCEEDED(result)) ? result : drv3d_dx12::check_result_no_oom_report(result, #expr, __FILE__, __LINE__); \
  }()
