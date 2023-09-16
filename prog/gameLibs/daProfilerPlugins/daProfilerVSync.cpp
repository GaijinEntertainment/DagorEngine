#if _TARGET_PC_WIN
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_finally.h>
#include <comPtr/comPtr.h>
#include <dxgi.h>
#include <eastl/deque.h>
#include <util/dag_globDef.h>

namespace da_profiler
{

class VsyncPlugin final : public ProfilerPlugin
{
  struct VBlankThread final : public DaThread
  {
    VBlankThread(ComPtr<IDXGIOutput> output) : DaThread{"VBlank"}, output{eastl::move(output)} {}
    void execute() override
    {
      DXGI_OUTPUT_DESC desc{};
      output->GetDesc(&desc);

      char strBuffer[sizeof(DXGI_OUTPUT_DESC::DeviceName) * 2 + 1];
      const size_t size = wcstombs(strBuffer, desc.DeviceName, sizeof(strBuffer));
      strBuffer[size] = '\0';

      TIME_PROFILE_THREAD(strBuffer);

      uint32_t color = 0;
      size_t s = min<size_t>(size, 3);
      memcpy(&color, &strBuffer[size - s], s);
      color = (get_current_thread_id() + color) | 0x40 & 0x00FFFFFF;

      bool toggle = false;
      auto line = __LINE__;
      const auto aVsyncDesc = add_thread_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, line, "VSync", color & 0x00FF00FF);
      const auto bVsyncDesc = add_thread_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, line, "VSync", color & 0x0000FFFF);

      SetThreadPriority(GetCurrentThread(), 15);

      while (!interlocked_relaxed_load(terminating))
      {
        ScopedEvent e{toggle ? aVsyncDesc : bVsyncDesc};
        toggle = !toggle;
        output->WaitForVBlank();
      }
    }
    ComPtr<IDXGIOutput> output;
  };

  eastl::deque<VBlankThread> detectors;
  volatile int enabled = 0;

public:
  void unregister() override
  {
    for (auto &detector : detectors)
      detector.terminate(true);

    detectors.clear();
    interlocked_relaxed_store(enabled, 0);
  }

  bool setEnabled(bool e) override
  {
    if (e)
    {
      if (!isEnabled())
      {
        HMODULE dxgi_dll = LoadLibraryA("dxgi.dll");
        FINALLY([=] { FreeLibrary(dxgi_dll); });

        using CreateDXGIFactoryFunc = HRESULT(WINAPI *)(REFIID, void **);
        CreateDXGIFactoryFunc pCreateDXGIFactory = nullptr;
        reinterpret_cast<FARPROC &>(pCreateDXGIFactory) = GetProcAddress(dxgi_dll, "CreateDXGIFactory");
        ComPtr<IDXGIFactory> factory;
        if (pCreateDXGIFactory && SUCCEEDED(pCreateDXGIFactory(COM_ARGS(&factory))))
        {
          for (uint32_t adapterIndex = 0;; adapterIndex++)
          {
            ComPtr<IDXGIAdapter> adapter;
            if (factory->EnumAdapters(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
              break;

            for (uint32_t outputIndex = 0;; outputIndex++)
            {
              ComPtr<IDXGIOutput> output;
              if (FAILED(adapter->EnumOutputs(outputIndex, &output)))
                break;

              detectors.emplace_back(eastl::move(output));
              detectors.back().start();
            }
          }

          interlocked_relaxed_store(enabled, detectors.size());
        }
      }
    }
    else
      unregister();

    return isEnabled();
  }

  bool isEnabled() const override { return interlocked_relaxed_load(enabled); }
};

} // namespace da_profiler

void register_vsync_plugin()
{
  static da_profiler::VsyncPlugin vsyncPlugin;
  da_profiler::register_plugin("Vsync", &vsyncPlugin);
}

#else

void register_vsync_plugin() {}

#endif