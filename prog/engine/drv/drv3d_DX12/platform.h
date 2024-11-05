// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <supp/dag_comPtr.h>
#include <debug/dag_log.h>
#include <winapi_helpers.h>

#include <EASTL/optional.h>

#include "driver.h"


namespace drv3d_dx12
{
struct HDRCapabilities
{
  float minLuminance;
  float maxLuminance;
  float maxFullFrameLuminance;
};

eastl::optional<HDRCapabilities> get_hdr_caps(const ComPtr<IDXGIOutput> &output);
bool is_hdr_available(const ComPtr<IDXGIOutput> &output = {});
bool change_hdr(bool force_enable = false, const ComPtr<IDXGIOutput> &output = {});
void hdr_changed(const eastl::optional<HDRCapabilities> &caps);

struct SwapchainProperties;
void set_hdr_config(SwapchainProperties &sci);

#if _TARGET_PC_WIN
struct Direct3D12Enviroment
{
  using PFN_CREATEDXGIFACTORY2 = HRESULT(WINAPI *)(UINT Flags, REFIID riid, void **ppFactory);

  LibPointer d3d12Lib;
  LibPointer dxgiLib;
  PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
  PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
  PFN_CREATEDXGIFACTORY2 CreateDXGIFactory2 = nullptr;

  FARPROC getD3DProcAddress(const char *name) const { return GetProcAddress(d3d12Lib.get(), name); }

  FARPROC getDXGIProcAddress(const char *name) const { return GetProcAddress(dxgiLib.get(), name); }

  bool setup()
  {
    d3d12Lib.reset(LoadLibraryA("d3d12.dll"));
    if (!d3d12Lib)
    {
      logdbg("DX12: Direct3D12Enviroment::setup: Unable to load d3d12.dll");
      teardown();
      return false;
    }

    dxgiLib.reset(LoadLibraryA("dxgi.dll"));
    if (!dxgiLib)
    {
      logdbg("DX12: Direct3D12Enviroment::setup: Unable to load dxgi.dll");
      teardown();
      return false;
    }

    reinterpret_cast<FARPROC &>(D3D12CreateDevice) = //
      getD3DProcAddress("D3D12CreateDevice");
    if (!D3D12CreateDevice)
    {
      logdbg("DX12: Direct3D12Enviroment::setup: d3d12.dll does not exports D3D12CreateDevice");
      return false;
    }

    reinterpret_cast<FARPROC &>(D3D12SerializeRootSignature) = //
      getD3DProcAddress("D3D12SerializeRootSignature");
    if (!D3D12SerializeRootSignature)
    {
      logdbg("DX12: Direct3D12Enviroment::setup: d3d12.dll does not exports "
             "D3D12SerializeRootSignature");
      return false;
    }

    reinterpret_cast<FARPROC &>(CreateDXGIFactory2) = //
      getDXGIProcAddress("CreateDXGIFactory2");
    if (!CreateDXGIFactory2)
    {
      logdbg("DX12: Direct3D12Enviroment::setup: dxgi.dll does not export CreateDXGIFactory2");
      return false;
    }

    return true;
  }

  void teardown()
  {
    CreateDXGIFactory2 = nullptr;
    D3D12SerializeRootSignature = nullptr;
    D3D12CreateDevice = nullptr;
    dxgiLib.reset();
    d3d12Lib.reset();
  }
};

union DriverVersion
{
  struct
  {
    uint16_t buildNumber;  // < Vendor specific minor
    uint16_t minorVersion; // < Vendor specific major
    uint16_t majorVersion; // < Unknown, may be related to target windows Version / protocol?, all WDDM 2.7 drivers have this at 20 or
                           // 21
    uint16_t productVersion; // < WDDM version x 10 -> 27 driver for WDDM 2.7
  };
  uint64_t raw;

  static DriverVersion fromRegistryValue(uint64_t value)
  {
    DriverVersion result;
    result.raw = value;

    return result;
  }
};

inline bool operator==(const DriverVersion &l, const DriverVersion &r) { return l.raw == r.raw; }

inline bool operator!=(const DriverVersion &l, const DriverVersion &r) { return l.raw != r.raw; }

inline bool operator<(const DriverVersion &l, const DriverVersion &r) { return l.raw < r.raw; }

inline bool operator>(const DriverVersion &l, const DriverVersion &r) { return l.raw > r.raw; }

// NVIDIAs public driver version XXX.XX can be deduced from driver DLL versions.
struct DriverVersionNVIDIA
{
  uint16_t majorVersion;
  uint16_t minorVersion;

  static DriverVersionNVIDIA fromDriverVersion(DriverVersion version)
  {
    // very easy to derive from DLL version
    DriverVersionNVIDIA result;
    result.majorVersion = (version.minorVersion % 10) * 100 + version.buildNumber / 100;
    result.minorVersion = version.buildNumber % 100;
    return result;
  }
};

// AMDs public driver version YY.MM.RR can _NOT_ be deduced from driver DLL versions.
// YY is year of publishing, MM is month of publishing and RR is the published index of YY.MM
// so 20.12.01 -> First (01) driver released in December (12) of 2020 (20).
// Their drivers have a increasing minorVersion and buildNumber.

#if !_TARGET_64BIT
bool is_wow64();
#endif
// returns raw integer value, can be turned into actual driver dll version with 'DriverVersion'
DriverVersion get_driver_version_from_registry(LUID adapter_luid);

D3D12_DRED_ENABLEMENT get_application_DRED_enablement_from_registry();

DXGI_GPU_PREFERENCE get_gpu_preference_from_registry();
#endif
} // namespace drv3d_dx12