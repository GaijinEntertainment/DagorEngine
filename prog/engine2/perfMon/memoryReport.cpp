#if DAGOR_DBGLEVEL > 0

#include <perfMon/dag_memoryReport.h>

#include "memoryReportPrivate.h"

#include <ioSys/dag_dataBlock.h>
#include <memory/dag_memStat.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_strUtil.h>
#include <workCycle/wcPrivHooks.h>
#include <debug/dag_debug.h>

#include <EASTL/vector_set.h>

namespace memoryreport
{

size_t last_memory_allocated = 0;
extern VRamState last_video_ram_state;

eastl::vector_set<IBackend *> backends;

BackendList *BackendList::head = nullptr;

bool report_enabled{false};
bool vram_stat_enabled{false};

TVRamStateCallback report_vram_state_callback;


void on_frame_end()
{
  VRamState video_ram_state{};
  if (vram_stat_enabled && report_vram_state_callback)
  {
    report_vram_state_callback(video_ram_state);
  }
  last_video_ram_state = video_ram_state;

  if (report_enabled)
  {
    Entry entry;

    entry.memory_allocated = dagor_memory_stat::get_memory_allocated(true);
    // it's unsafe if we allocate more than 2^63 bytes of RAM
    entry.allocation_diff = int64_t(entry.memory_allocated) - int64_t(last_memory_allocated);
    entry.used_device_local_vram = video_ram_state.used_device_local;
    entry.used_shared_vram = video_ram_state.used_shared;

    last_memory_allocated = entry.memory_allocated;

    for (IBackend *backend : backends)
    {
      backend->onReport(entry);
    }
  }
}

void register_backend(IBackend *const backend)
{
  backends.insert(backend);

  if (report_enabled)
  {
    backend->onStart();
  }
}

void update_hook()
{
  if (report_enabled || (vram_stat_enabled && report_vram_state_callback))
  {
    dwc_hook_memory_report = on_frame_end;
  }
  else
  {
    dwc_hook_memory_report = nullptr;
    last_memory_allocated = 0;
    last_video_ram_state = VRamState{};
  }
}

void start_report()
{
  if (backends.empty() || report_enabled)
  {
    return;
  }

  last_memory_allocated = 0;

  for (IBackend *backend : backends)
  {
    backend->onStart();
  }

  report_enabled = true;
  update_hook();
}

void stop_report()
{
  if (!report_enabled)
  {
    return;
  }

  report_enabled = false;
  update_hook();

  for (IBackend *backend : backends)
  {
    backend->onStop();
  }
}

void init()
{
  vram_stat_enabled = ::dgs_get_settings()->getBlockByNameEx("video")->getBool("vram_stat", false);
  const char *backend_ids = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("memory_report", nullptr);

  if (backend_ids)
  {
    auto on_backend_id = [](const char *str, uint32_t n) {
      auto it = BackendList::head;
      while (it)
      {
        if (strncmp(it->name.c_str(), str, n) == 0)
        {
          register_backend(it->backend);
          return true;
        }
        it = it->next;
      }

      eastl::string backend_name(str, n);
      logwarn("Requested memory report backend not found: '%s'", backend_name.c_str());

      return true;
    };

    str_split(backend_ids, ',', on_backend_id);
  }

  if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("enableMemoryReport", true))
    update_hook();
}

void register_vram_state_callback(const TVRamStateCallback &callback) { report_vram_state_callback = callback; }

} // namespace memoryreport

PULL_MEMORYREPORT_BACKEND(log)

#endif
