// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "d3d12_error_handling.h"
#include "d3d12_utils.h"
#include "device.h"
#include "driver.h"

#include <debug/dag_log.h>
#include <drv_log_defs.h>

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>
#include <util/dag_string.h>

#include <EASTL/iterator.h>
#include <Windows.h>


extern "C"
{
  extern UINT D3D12SDKVersion;
  extern const char *D3D12SDKPath;
}
#endif

namespace drv3d_dx12
{
namespace
{
void check_result_for_gpu_crash_and_enter_error_state([[maybe_unused]] HRESULT result)
{
#if _TARGET_PC_WIN
  if (is_gpu_crash_code(result))
    get_device().signalDeviceErrorNoDebugInfo();
#endif
}
} // namespace

HRESULT debug_result(HRESULT result, const char *expr, const char *file, int line)
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logdbg("DX12: %s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logdbg("DX12: %s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

HRESULT check_result(HRESULT result, const char *expr, const char *file, int line)
{
  if (SUCCEEDED(result))
    return result;

  if (is_oom_error_code(result))
  {
    report_oom_info();
  }

  set_last_error(result);

  check_result_for_gpu_crash_and_enter_error_state(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    D3D_ERROR("DX12: %s returned unknown return code 0x%X, %s %u", expr, result, file, line);
  }
  else
  {
    D3D_ERROR("DX12: %s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

HRESULT check_result_no_oom_report(HRESULT result, const char *expr, const char *file, int line)
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  check_result_for_gpu_crash_and_enter_error_state(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    D3D_ERROR("DX12: %s returned unknown return code 0x%X, %s %u", expr, result, file, line);
  }
  else
  {
    D3D_ERROR("DX12: %s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

void report_resource_alloc_info_error(const D3D12_RESOURCE_DESC &desc)
{
  D3D_ERROR("DX12: Error while querying resource allocation info, resource desc: %s, %u, %u x %u x "
            "%u, %u, %s, %u by %u, %u, %08X",
    to_string(desc.Dimension), desc.Alignment, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels,
    dxgi_format_name(desc.Format), desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Layout, desc.Flags);
}

void report_agility_sdk_error([[maybe_unused]] HRESULT hr)
{
#if _TARGET_PC_WIN
  if (hr != D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION)
    return;

  wchar_t exePathW[MAX_PATH] = {};
  if (::GetModuleFileNameW(nullptr, exePathW, eastl::size(exePathW)) == 0)
    return;

  char utf8buf[MAX_PATH * 3] = {};
  ::wcs_to_utf8(exePathW, utf8buf, eastl::size(utf8buf));

  const char *exeDir = ::dd_get_fname_location(utf8buf, utf8buf);

  String dllPath(0, "%s%sD3D12Core.dll", exeDir, D3D12SDKPath);
  if (!dd_file_exists(dllPath))
  {
    logdbg("DX12: %s is missing", dllPath.c_str());
    return;
  }
  dllPath.printf(0, "%s%sd3d12SDKLayers.dll", exeDir, D3D12SDKPath);
  if (!dd_file_exists(dllPath))
  {
    logdbg("DX12: %s is missing", dllPath.c_str());
    return;
  }
  logdbg("DX12: D3D12SDKVersion %d isn't available", D3D12SDKVersion);
#endif
}

} // namespace drv3d_dx12
