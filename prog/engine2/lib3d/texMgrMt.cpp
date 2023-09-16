#include "texMgrData.h"
#include <3d/dag_drv3d.h>
#include <debug/dag_debug.h>

void enable_res_mgr_mt(bool enable, int max_tex_entry_count)
{
  using namespace texmgr_internal;
  int prev = mt_enabled;

  texmgr_internal::drv3d_cmd = &d3d::driver_command;
  if (prev)
    ::enter_critical_section(crit_sec);
  if (enable)
    mt_enabled++;
  else if (mt_enabled > 0)
    mt_enabled--;
  else
    fatal("incorrect enable_tex_mgr_mt refcount=%d", mt_enabled);

  if (!prev && mt_enabled)
  {
    debug("d3dResMgr: multi-threaded access ENABLED  (reserving %d entries)", max_tex_entry_count);
    ::create_critical_section(crit_sec, "tex_mgr");
    if (!RMGR.getAccurateIndexCount())
    {
      RMGR.term();
      RMGR.init(max_tex_entry_count);
      managed_tex_map_by_name.reserve(RMGR.getMaxTotalIndexCount());
      managed_tex_map_by_idx.resize(RMGR.getMaxTotalIndexCount());
      mem_set_0(managed_tex_map_by_idx);
    }
    else if (max_tex_entry_count)
      G_ASSERTF(max_tex_entry_count <= RMGR.getMaxTotalIndexCount() && RMGR.getAccurateIndexCount() < max_tex_entry_count,
        "%s(%s, %d) while indexCount=%d and maxTotalIndexCount=%d", __FUNCTION__, enable ? "true" : "false", max_tex_entry_count,
        RMGR.getAccurateIndexCount(), RMGR.getMaxTotalIndexCount());
  }
  else if (max_tex_entry_count > RMGR.getMaxTotalIndexCount())
    logerr("d3dResMgr: cannot change reserved entries %d -> %d while in multi-threaded mode; disable it first!",
      RMGR.getMaxTotalIndexCount(), max_tex_entry_count);

  if (prev > 0)
    ::leave_critical_section(crit_sec);

  if (prev && !mt_enabled)
  {
    ::destroy_critical_section(crit_sec);
    debug("d3dResMgr: multi-threaded access disabled (used %d/%d entries)", RMGR.getAccurateIndexCount(),
      RMGR.getMaxTotalIndexCount());
  }
}
