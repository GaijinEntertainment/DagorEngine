// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"

#include <EASTL/optional.h>
#include <supp/dag_comPtr.h>
#include <osApiWrappers/dag_versionQuery.h>
#include <winapi_helpers.h>


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
bool change_hdr(bool prefer_hfr, bool force_enable = false, const ComPtr<IDXGIOutput> &output = {});
void hdr_changed(const eastl::optional<HDRCapabilities> &caps);

struct SwapchainProperties;
void set_hdr_config(SwapchainProperties &sci);
void set_hfr_config(SwapchainProperties &sci);

template <size_t N>
struct StringLiteralType
{
  constexpr StringLiteralType(const char (&str)[N])
  {
    // can not use eastl::copy_n, its not constexpr
    for (size_t i = 0; i < N; ++i)
    {
      value[i] = str[i];
    }
  }
  char value[N];
};

template <StringLiteralType Name, typename PointerType>
class DynamicLibraryFunction
{
  PointerType pointer = nullptr;

public:
  constexpr DynamicLibraryFunction() = default;
  constexpr ~DynamicLibraryFunction() = default;

  constexpr DynamicLibraryFunction(const DynamicLibraryFunction &) = default;
  constexpr DynamicLibraryFunction &operator=(const DynamicLibraryFunction &) = default;

  constexpr DynamicLibraryFunction(PointerType ptr) : pointer{ptr} {}
  constexpr DynamicLibraryFunction &operator=(PointerType ptr)
  {
    pointer = ptr;
    return *this;
  }

  constexpr auto operator()(auto &&...values) { return pointer(eastl::forward<decltype(values)>(values)...); }

  constexpr bool load(auto &&loader)
  {
    loader(pointer, Name.value);
    return nullptr != pointer;
  }

  void reset() { pointer = nullptr; }

  constexpr explicit operator bool() const { return nullptr != pointer; }
  constexpr explicit operator PointerType() const { return pointer; }
  constexpr const auto &name() { return Name.value; }
};

#if _TARGET_PC_WIN
struct Direct3D12Enviroment
{
  using PFN_CREATEDXGIFACTORY2 = HRESULT(WINAPI *)(UINT Flags, REFIID riid, void **ppFactory);

  LibPointer d3d12Lib;
  LibPointer dxgiLib;
  PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
  PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
  PFN_CREATEDXGIFACTORY2 CreateDXGIFactory2 = nullptr;
  DynamicLibraryFunction<"D3D12EnableExperimentalFeatures", decltype(&::D3D12EnableExperimentalFeatures)>
    D3D12EnableExperimentalFeatures;

  FARPROC getD3DProcAddress(const char *name) const { return GetProcAddress(d3d12Lib.get(), name); }

  FARPROC getDXGIProcAddress(const char *name) const { return GetProcAddress(dxgiLib.get(), name); }

  static UINT getD3D12SdkVersion();

  bool setup();

  void teardown();

  ~Direct3D12Enviroment() { teardown(); }
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
#endif
} // namespace drv3d_dx12