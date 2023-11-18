#pragma once

#include <debug/dag_log.h>

#include "driver.h"
#include "d3d12_utils.h"


namespace drv3d_dx12
{

#define D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION _HRESULT_TYPEDEF_(0x887E0003L)

void report_oom_info();
void set_last_error(HRESULT error);
HRESULT get_last_error_code();
inline const char *dxgi_error_code_to_string(HRESULT ec)
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
#if _TARGET_PC_WIN
    ENUM_CASE(D3D12_ERROR_ADAPTER_NOT_FOUND);
    ENUM_CASE(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
    ENUM_CASE(D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION);
#endif
  }
#undef ENUM_CASE

  return "";
}

inline HRESULT dx12_check_result_no_oom_report(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

inline bool is_oom_error_code(HRESULT result) { return E_OUTOFMEMORY == result; }

inline HRESULT dx12_check_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  if (is_oom_error_code(result))
  {
    report_oom_info();
  }

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

inline bool is_recoverable_error(HRESULT error)
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

inline HRESULT dx12_debug_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    debug("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    debug("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

#define DX12_DEBUG_RESULT(r) drv3d_dx12::dx12_debug_result(r, #r, __FILE__, __LINE__)
#define DX12_DEBUG_OK(r)     SUCCEEDED(DX12_DEBUG_RESULT(r))
#define DX12_DEBUG_FAIL(r)   FAILED(DX12_DEBUG_RESULT(r))

#define DX12_CHECK_RESULT(r) drv3d_dx12::dx12_check_result(r, #r, __FILE__, __LINE__)
#define DX12_CHECK_OK(r)     SUCCEEDED(DX12_CHECK_RESULT(r))
#define DX12_CHECK_FAIL(r)   FAILED(DX12_CHECK_RESULT(r))
#define DX12_EXIT_ON_FAIL(r) \
  if (DX12_CHECK_FAIL(r))    \
  {                          \
    /* no-op */              \
  }

#define DX12_CHECK_RESULT_NO_OOM_CHECK(r) drv3d_dx12::dx12_check_result_no_oom_report(r, #r, __FILE__, __LINE__)

inline void report_resource_alloc_info_error(const D3D12_RESOURCE_DESC &desc)
{
  logerr("DX12: Error while querying resource allocation info, resource desc: %s, %u, %u x %u x "
         "%u, %u, %s, %u by %u, %u, %08X",
    to_string(desc.Dimension), desc.Alignment, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels,
    dxgi_format_name(desc.Format), desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Layout, desc.Flags);
}

} // namespace drv3d_dx12
