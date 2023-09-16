#include <fx/dag_commonFx.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_texMgrTags.h>
#include <3d/dag_tex3d.h>
#include <startup/dag_globalSettings.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>

static Tab<TEXTUREID> active_fx_tex;
static int fx_gen = 0;

void update_and_prefetch_fx_textures_used()
{
  if (!is_managed_textures_streaming_load_on_demand())
    return;

  if (fx_gen != interlocked_relaxed_load(textag_get_info(TEXTAG_FX).loadGeneration))
  {
    fx_gen = interlocked_acquire_load(textag_get_info(TEXTAG_FX).loadGeneration);

    Tab<TEXTUREID> tex;
    textag_get_list(TEXTAG_FX, tex, true);
    // debug("textag(FX) count=%d tex=%d gen=%d", textag_get_info(TEXTAG_FX).texCount, tex.size(), fx_gen);
    for (TEXTUREID tid : tex)
    {
      if (find_value_idx(active_fx_tex, tid) >= 0)
        continue;
      BaseTexture *t = acquire_managed_tex(tid);
      if (get_managed_res_cur_tql(tid) != TQL_stub && get_managed_res_lfu(tid) + 30 > dagor_frame_no())
      {
        // debug("  add fx[#%d] %s, active_fx_tex=%d, lfu=%d (cur=%d)",
        //   tid, get_managed_texture_name(tid), active_fx_tex.size(), t->getLFU(), dagor_frame_no());
        active_fx_tex.push_back(tid);
      }
      release_managed_tex(tid);
    }
  }

  if (!active_fx_tex.size())
    return;

  prefetch_managed_textures(active_fx_tex);
  mark_managed_textures_important(active_fx_tex, 1);
  for (TEXTUREID tid : active_fx_tex)
    mark_managed_tex_lfu(tid);
}

void reset_fx_textures_used()
{
  if (active_fx_tex.size())
    debug("%s(): active_fx_tex=%d", __FUNCTION__, active_fx_tex.size());
  active_fx_tex.clear();
}
