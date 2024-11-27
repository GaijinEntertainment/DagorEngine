// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <amdFsr.h>
#include <amdFsr3Helpers.h>

#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_resetDevice.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_dynLib.h>
#include <memory/dag_framemem.h>

#include <ffx_api/dx12/ffx_api_dx12.hpp>
#include <ffx_api/ffx_upscale.hpp>


namespace amd
{

static inline FfxApiResource ffx_get_resource(void *texture, uint32_t state)
{
  FfxApiResource apiRes = {};
  if (!texture)
    return apiRes;

  apiRes = ffxApiGetResourceDX12((ID3D12Resource *)texture, state);
  return apiRes;
}

class FSRD3D12Win;

FSRD3D12Win *fSRD3D12Win = nullptr; // With this only one FSR instance is allowed. When this will became a problem, please fix it

// On windows, we use FSR3.1
class FSRD3D12Win : public FSR
{
public:
  FSRD3D12Win()
  {
    logdbg("[AMDFSR] Creating upscaling FSRD3D12Win...");

    fsrModule.reset(os_dll_load("amd_fidelityfx_dx12.dll"));

    if (fsrModule)
    {
      createContext = (PfnFfxCreateContext)os_dll_get_symbol(fsrModule.get(), "ffxCreateContext");
      destroyContext = (PfnFfxDestroyContext)os_dll_get_symbol(fsrModule.get(), "ffxDestroyContext");
      configure = (PfnFfxConfigure)os_dll_get_symbol(fsrModule.get(), "ffxConfigure");
      query = (PfnFfxQuery)os_dll_get_symbol(fsrModule.get(), "ffxQuery");
      dispatch = (PfnFfxDispatch)os_dll_get_symbol(fsrModule.get(), "ffxDispatch");
    }

    if (isLoaded())
    {
      uint64_t versionCount = 0;
      ffx::QueryDescGetVersions versionsDesc{};
      versionsDesc.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
      versionsDesc.device = d3d::get_device();
      versionsDesc.outputCount = &versionCount;
      if (query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK)
      {
        dag::Vector<uint64_t, framemem_allocator> versionIds;
        dag::Vector<const char *, framemem_allocator> versionNames;
        versionIds.resize(versionCount);
        versionNames.resize(versionCount);
        versionsDesc.versionIds = versionIds.data();
        versionsDesc.versionNames = versionNames.data();

        if (versionCount > 0 && query(nullptr, &versionsDesc.header) == FFX_API_RETURN_OK)
        {
          for (int verIx = 0; verIx < versionCount; ++verIx)
            logdbg("[AMDFSR] provider %llu - %s", versionIds[verIx], versionNames[verIx]);

          // The first returned provider is FSR3 upscaling
          upscalingVersionString = versionNames.front();
        }
      }

      logdbg("[AMDFSR] FSRD3D12Win created. Upscaling use %s.", getVersionString().data());
    }
    else
    {
      logdbg("[AMDFSR] Failed to create FSRD3D12Win.");
    }

    G_ASSERT(!fSRD3D12Win);
    fSRD3D12Win = this;
  }

  ~FSRD3D12Win() { fSRD3D12Win = nullptr; }

  bool initUpscaling(const ContextArgs &args)
  {
    logdbg("[AMDFSR] Initializing upscaling...");

    if (!isLoaded())
    {
      logdbg("[AMDFSR] Library not loaded. Failed.");
      return false;
    }

    ffx::CreateBackendDX12Desc backendDesc{};
    backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backendDesc.device = static_cast<ID3D12Device *>(d3d::get_device());

    ffx::CreateContextDescUpscale desc = convert(args, backendDesc.header);

    auto status = createContext(&upscalingContext, &desc.header, nullptr);
    if (status != FFX_API_RETURN_OK)
    {
      logerr("[AMDFSR] Failed to create FSR context: %s", get_error_string(status));
      return false;
    }

    contextArgs = args;

    logdbg("[AMDFSR] Upscaling initialized.");

    return true;
  }

  void teardownUpscaling() override
  {
    if (!upscalingContext)
      return;

    destroyContext(&upscalingContext, nullptr);
    upscalingContext = nullptr;
    contextArgs.mode = UpscalingMode::Off;
  }

  Point2 getNextJitter(uint32_t render_width, uint32_t output_width)
  {
    return get_next_jitter(render_width, output_width, jitterIndex, upscalingContext, query);
  }

  bool doApplyUpscaling(const UpscalingPlatformArgs &args, void *command_list) const override
  {
    return apply_upscaling(args, command_list, upscalingContext, dispatch, ffx_get_resource);
  }

  IPoint2 getRenderingResolution(UpscalingMode mode, const IPoint2 &target_resolution) const override
  {
    return get_rendering_resolution(mode, target_resolution, upscalingContext, query);
  }

  UpscalingMode getUpscalingMode() const override { return contextArgs.mode; }

  bool isLoaded() const override { return createContext && destroyContext && configure && query && dispatch; }

  bool isUpscalingSupported() const override { return isLoaded() && d3d::get_driver_desc().shaderModel >= 6.2_sm; }

  bool isFrameGenerationSupported() const override { return false; }

  String getVersionString() const override { return upscalingVersionString; }

  void recover() { initUpscaling(contextArgs); }

private:
  eastl::unique_ptr<void, DagorDllCloser> fsrModule;

  PfnFfxCreateContext createContext = nullptr;
  PfnFfxDestroyContext destroyContext = nullptr;
  PfnFfxConfigure configure = nullptr;
  PfnFfxQuery query = nullptr;
  PfnFfxDispatch dispatch = nullptr;

  ffxContext upscalingContext = nullptr;

  ContextArgs contextArgs = {};

  uint32_t jitterIndex = 0;

  String upscalingVersionString = String(8, "");
};

FSR *createD3D12Win() { return new FSRD3D12Win; }

} // namespace amd

static void fsr3_before_reset(bool full_reset)
{
  if (full_reset && amd::fSRD3D12Win)
    amd::fSRD3D12Win->teardownUpscaling();
}

static void fsr3_after_reset(bool full_reset)
{
  if (full_reset && amd::fSRD3D12Win)
    amd::fSRD3D12Win->recover();
}

REGISTER_D3D_BEFORE_RESET_FUNC(fsr3_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(fsr3_after_reset);
