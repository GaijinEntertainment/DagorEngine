// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texMgrData.h"
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
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
    DAG_FATAL("incorrect enable_tex_mgr_mt refcount=%d", mt_enabled);

  if (!prev && mt_enabled)
  {
    debug("d3dResMgr: multi-threaded access ENABLED  (reserving %d entries)", max_tex_entry_count);
    ::create_critical_section(crit_sec, "tex_mgr");

    Tab<uint16_t> generation_to_preserve;
    if (RMGR.getAccurateIndexCount())
    {
      bool has_live_entries = false;
      for (unsigned idx = 0, ie = RMGR.getRelaxedIndexCount(); idx < ie; idx++)
        if (RMGR.getRefCount(idx) >= 0)
        {
          has_live_entries = true;
          break;
        }
      if (!has_live_entries)
      {
        generation_to_preserve.assign(RMGR.generation,
          RMGR.generation + min<unsigned>(RMGR.getRelaxedIndexCount(), max_tex_entry_count));
        interlocked_release_store(RMGR.indexCount, 0);
      }
    }

    if (!RMGR.getAccurateIndexCount())
    {
      RMGR.term();
      RMGR.init(max_tex_entry_count);
      managed_tex_map_by_name.reserve(RMGR.getMaxTotalIndexCount());
      managed_tex_map_by_idx.resize(RMGR.getMaxTotalIndexCount());
      mem_set_0(managed_tex_map_by_idx);
      if (!generation_to_preserve.empty())
      {
        mem_copy_to(generation_to_preserve, RMGR.generation);
        interlocked_release_store(RMGR.indexCount, generation_to_preserve.size());
      }
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
