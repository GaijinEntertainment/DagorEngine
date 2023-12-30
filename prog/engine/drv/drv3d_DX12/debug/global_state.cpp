#include "device.h"

using namespace drv3d_dx12;

void debug::GlobalState::setup(const DataBlock *settings, Direct3D12Enviroment &d3d_env)
{
  config.applyDefaults();
  config.applySettings(settings);

  gpuCapture.setup(config, d3d_env);
  gpuPostmortem.setup(config, d3d_env);

  if (!config.anyValidation())
  {
    logdbg("DX12: Debugging is turned off");
    return;
  }

  PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
  reinterpret_cast<FARPROC &>(DXGIGetDebugInterface1) = d3d_env.getDXGIProcAddress("DXGIGetDebugInterface1");

  if (DXGIGetDebugInterface1)
  {
    logdbg("DX12: DXGIGetDebugInterface1 for IDXGIDebug...");
    HRESULT hr = DXGIGetDebugInterface1(0, COM_ARGS(&dxgiDebug));
    logdbg("DX12: DXGIGetDebugInterface1 returns 0x%08x", hr);
  }

  PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
  reinterpret_cast<FARPROC &>(D3D12GetDebugInterface) = d3d_env.getD3DProcAddress("D3D12GetDebugInterface");

  if (D3D12GetDebugInterface)
  {
    if (config.anyValidation())
    {
      logdbg("DX12: D3D12GetDebugInterface validation layer");

      ComPtr<ID3D12Debug> debug0;
      if (SUCCEEDED(D3D12GetDebugInterface(COM_ARGS(&debug0))))
      {
        config.enableCPUValidation = true;
        logdbg("DX12: Validation layer enabled");
        // debug layer always implies cpu validation
        debug0->EnableDebugLayer();
        ComPtr<ID3D12Debug1> debug1;
        if (SUCCEEDED(debug0.As(&debug1)))
        {
          logdbg("DX12: Synchronized queues enabled");
          // Always try to enable queue sync, this allows for better state tracking of resources,
          // otherwise false errors can be generated as some dependencies are unavailable and some
          // state is assumed.
          debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
          // NOTE: GPU based validation can induce timeout driver resets
          // so don't be surprised if that happens if this is on
          if (config.enableGPUValidation)
          {
            logdbg("DX12: GPU based validation enabled");
            logdbg("DX12: NONE: GPU based validation may induce TDRs, you might need to adjust "
                   "timeout value for video drivers");

            debug1->SetEnableGPUBasedValidation(TRUE);
          }
        }
        ComPtr<ID3D12Debug2> debug2;
        if (SUCCEEDED(debug0.As(&debug2)))
        {
          // only flag available is to turn _off_ state tracking, but we want it on, so none
          // TODO may add feature flag for this?
          debug2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
        }
      }
      else
      {
        G_ASSERTF(false, "DX12: User requested debug layer to be enabled, but it seems not to be installed.");
        logdbg("DX12: No validation layer available");
        config.enableCPUValidation = false;
        config.enableGPUValidation = false;
      }
    }
  }
  else
  {
    logdbg("DX12: d3d12.dll does not export D3D12GetDebugInterface");
    config.enableCPUValidation = false;
    config.enableGPUValidation = false;
  }
}

void debug::GlobalState::teardown()
{
  if (dxgiDebug)
  {
    // DXGI_DEBUG_RLO_IGNORE_INTERNAL - we don't care about refcounts we can't do anything about, we are only interested in dangling
    // ref counts caused by our sloppiness
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
      DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
  }

  gpuCapture.teardown();
}