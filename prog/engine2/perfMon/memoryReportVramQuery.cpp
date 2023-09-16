#if DAGOR_DBGLEVEL > 0

#include <perfMon/dag_memoryReport.h>

namespace memoryreport
{

VRamState last_video_ram_state{};

int32_t get_device_vram_used_kb()
{
  return last_video_ram_state.used_device_local > 0 ? last_video_ram_state.used_device_local >> 10 : -1;
}

int32_t get_shared_vram_used_kb() { return last_video_ram_state.used_shared > 0 ? last_video_ram_state.used_shared >> 10 : -1; }

} // namespace memoryreport

#endif
