// Copyright (C) Gaijin Games KFT.  All rights reserved.

#undef _WIN32_WINNT

#include "dxgi_utils.h"

#if _TARGET_PC_WIN

#include <util/dag_string.h>
#include <dxgi.h>

#include <EASTL/vector.h>
#include <cmath>

static constexpr size_t outputNameSize = 64;
static_assert(outputNameSize >= sizeof(DXGI_OUTPUT_DESC::DeviceName), "sizeof outputName is too small!");

// maybe optimize it (not important)
String get_monitor_name_from_output(IDXGIOutput *pOutput)
{
  DXGI_OUTPUT_DESC outDesc;
  pOutput->GetDesc(&outDesc);
  char outputName[outputNameSize];
  wcstombs(outputName, outDesc.DeviceName, outputNameSize);
  return String(outputName);
}

ComPtr<IDXGIOutput> get_output_monitor_by_name(IDXGIFactory1 *factory, const char *monitorName)
{
  G_ASSERT(monitorName);

  for (uint32_t adapterIndex = 0;; adapterIndex++)
  {
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(adapterIndex, &adapter)))
      break;

    for (uint32_t outputIndex = 0;; outputIndex++)
    {
      ComPtr<IDXGIOutput> output;
      if (FAILED(adapter->EnumOutputs(outputIndex, &output)))
        break;

      DXGI_OUTPUT_DESC outDesc;
      output->GetDesc(&outDesc);
      char name[outputNameSize];
      wcstombs(name, outDesc.DeviceName, outputNameSize);

      if (strcmp(monitorName, name) == 0)
        return output;
    }
  }

  return nullptr;
}

namespace
{
template <bool DefaultOnly>
ComPtr<IDXGIOutput> get_output_monitor_by_name_or_default_(IDXGIFactory1 *factory, const char *monitorName)
{
  ComPtr<IDXGIOutput> defaultOutput;
  ComPtr<IDXGIOutput> firstFoundOutput;
  for (uint32_t adapterIndex = 0;; adapterIndex++)
  {
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(factory->EnumAdapters1(adapterIndex, &adapter)))
      break;

    for (uint32_t outputIndex = 0;; outputIndex++)
    {
      ComPtr<IDXGIOutput> output;
      if (FAILED(adapter->EnumOutputs(outputIndex, &output)))
        break;

      DXGI_OUTPUT_DESC outDesc;
      output->GetDesc(&outDesc);

      if (!DefaultOnly)
      {
        char name[outputNameSize];
        wcstombs(name, outDesc.DeviceName, outputNameSize);

        if (monitorName && strcmp(monitorName, name) == 0)
          return output;
      }

      // According to Raymond Chen in https://devblogs.microsoft.com/oldnewthing/20070809-00/?p=25643
      // the primary monitor is which coordinates is (0,0)
      if (outDesc.DesktopCoordinates.left == 0 && outDesc.DesktopCoordinates.top == 0)
      {
        defaultOutput = eastl::move(output);
        if (DefaultOnly)
          return defaultOutput;
      }
      else if (!firstFoundOutput)
        firstFoundOutput = eastl::move(output);
    }
  }

  if (defaultOutput)
    return defaultOutput;

  return firstFoundOutput;
}

} // namespace

ComPtr<IDXGIOutput> get_output_monitor_by_name_or_default(IDXGIFactory1 *factory, const char *monitorName)
{
  return get_output_monitor_by_name_or_default_<false>(factory, monitorName);
}

ComPtr<IDXGIOutput> get_default_monitor(IDXGIFactory1 *factory)
{
  return get_output_monitor_by_name_or_default_<true>(factory, nullptr);
}

bool resolutions_have_same_ratio(eastl::pair<uint32_t, uint32_t> l_res, eastl::pair<uint32_t, uint32_t> r_res)
{
  // minimum half difference of the most common aspect ratios (3:2, 4:3, 5:4, 16:9, 16:10, 21:9)
  constexpr double ratio_eps = 0.04;

  double l_ratio = double(l_res.first) / double(l_res.second);
  double r_ratio = double(r_res.first) / double(r_res.second);

  return abs(l_ratio - r_ratio) < ratio_eps;
}

eastl::optional<eastl::pair<uint32_t, uint32_t>> get_recommended_resolution(IDXGIOutput *dxgi_output)
{
  // Based on the example:
  // https://docs.microsoft.com/en-us/windows-hardware/drivers/display/displaying-app-on-portrait-device

  // Returns true if this is an integrated display panel e.g. the screen attached to tablets or
  // laptops.
  auto IsInternalVideoOutput = [](const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType) -> bool {
    switch (VideoOutputTechnologyType)
    {
      case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
      case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
      case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED: return TRUE;

      default: return FALSE;
    }
  };

  // Note: Since an hmon can represent multiple monitors while in clone, this function as written
  // will return
  //  the value for the internal monitor if one exists, and otherwise the highest clone-path
  //  priority.
  auto GetPathInfo = [&IsInternalVideoOutput](PCWSTR pszDeviceName, DISPLAYCONFIG_PATH_INFO *pPathInfo) -> HRESULT {
    HRESULT hr = S_OK;
    UINT32 NumPathArrayElements = 0;
    UINT32 NumModeInfoArrayElements = 0;

    eastl::vector<DISPLAYCONFIG_PATH_INFO> PathInfoArray;
    eastl::vector<DISPLAYCONFIG_MODE_INFO> ModeInfoArray;

    do
    {
      PathInfoArray.clear();
      ModeInfoArray.clear();

      hr = HRESULT_FROM_WIN32(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, &NumModeInfoArrayElements));
      if (FAILED(hr))
      {
        break;
      }

      PathInfoArray.resize(NumPathArrayElements);
      ModeInfoArray.resize(NumModeInfoArrayElements);

      hr = HRESULT_FROM_WIN32(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, PathInfoArray.data(),
        &NumModeInfoArrayElements, ModeInfoArray.data(), nullptr));
    } while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (PathInfoArray.empty())
    {
      hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
      INT DesiredPathIdx = -1;

      // Loop through all sources until the one which matches the 'monitor' is found.
      for (UINT PathIdx = 0; PathIdx < NumPathArrayElements; ++PathIdx)
      {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
        SourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        SourceName.header.size = sizeof(SourceName);
        SourceName.header.adapterId = PathInfoArray[PathIdx].sourceInfo.adapterId;
        SourceName.header.id = PathInfoArray[PathIdx].sourceInfo.id;

        hr = HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&SourceName.header));
        if (SUCCEEDED(hr))
        {
          if (wcscmp(pszDeviceName, SourceName.viewGdiDeviceName) == 0)
          {
            // Found the source which matches this hmonitor. The paths are given in path-priority
            // order so the first found is the most desired, unless we later find an internal.
            if (DesiredPathIdx == -1 || IsInternalVideoOutput(PathInfoArray[PathIdx].targetInfo.outputTechnology))
            {
              DesiredPathIdx = PathIdx;
            }
          }
        }
      }

      if (DesiredPathIdx != -1)
      {
        *pPathInfo = PathInfoArray[DesiredPathIdx];
      }
      else
      {
        hr = E_INVALIDARG;
      }
    }

    return hr;
  };

  DXGI_OUTPUT_DESC outDesc;
  if (FAILED(dxgi_output->GetDesc(&outDesc)))
  {
    return {};
  }

  // For DEBUG
  // returns the actual desktop resolution as recommended resolution
#if 0
  return eastl::pair<uint32_t, uint32_t>(
    outDesc.DesktopCoordinates.right - outDesc.DesktopCoordinates.left,
    outDesc.DesktopCoordinates.bottom - outDesc.DesktopCoordinates.top);
#endif

  DISPLAYCONFIG_PATH_INFO PathInfo = {};
  if (FAILED(GetPathInfo(outDesc.DeviceName, &PathInfo)))
  {
    return {};
  }

  DISPLAYCONFIG_TARGET_PREFERRED_MODE PreferredMode = {};
  PreferredMode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
  PreferredMode.header.size = sizeof(PreferredMode);
  PreferredMode.header.adapterId = PathInfo.targetInfo.adapterId;
  PreferredMode.header.id = PathInfo.targetInfo.id;

  HRESULT hr = HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&PreferredMode.header));
  if (FAILED(hr))
  {
    return {};
  }

  return eastl::pair<uint32_t, uint32_t>(PreferredMode.width, PreferredMode.height);
}

#endif
