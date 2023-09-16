#include <perfMon/dag_perfMonStat.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

unsigned perfmonstat::fastphys_update_cnt = 0, perfmonstat::fastphys_update_time_usec = 0;
unsigned perfmonstat::geomtree_calcWtm_cnt = 0, perfmonstat::geomtree_calcWtm_tm_cnt = 0, perfmonstat::geomtree_calc_time_usec = 0;
unsigned perfmonstat::animchar_act_cnt = 0, perfmonstat::animchar_act_time_usec = 0;
unsigned perfmonstat::animchar_blendAnim_cnt = 0, perfmonstat::animchar_blendAnim_tm_cnt = 0,
         perfmonstat::animchar_blendAnim_time_usec = 0;
unsigned perfmonstat::animchar_blendAnim_getprs_time_usec = 0, perfmonstat::animchar_blendAnim_pbc_time_usec = 0;

unsigned perfmonstat::generation_counter = 0;
static unsigned gen_frame_no = 0;

void perfmonstat::dump_stat(bool only_if_gen_count_changed)
{
  static unsigned dumped_gen_cnt = 0;
  if (only_if_gen_count_changed && dumped_gen_cnt == perfmonstat::generation_counter)
    return;

  debug("perfmon statistics (%d frames):", dagor_frame_no() - gen_frame_no);

  if (geomtree_calcWtm_cnt)
    debug("  nodeTree::calcWtm %5u usec for %u calls(%4.1f usec/calc, %.4f usec/tm)", geomtree_calc_time_usec, geomtree_calcWtm_cnt,
      (double)geomtree_calc_time_usec / geomtree_calcWtm_cnt,
      geomtree_calcWtm_tm_cnt ? (double)geomtree_calc_time_usec / geomtree_calcWtm_tm_cnt : -1.0f);

  if (fastphys_update_cnt)
    debug("  fastPhys::update  %5u usec for %u acts (%4.1f usec/call)", fastphys_update_time_usec, fastphys_update_cnt,
      (double)fastphys_update_time_usec / fastphys_update_cnt);

  if (animchar_act_cnt)
    debug("  animchar::act     %5u usec for %u acts (%4.1f usec/call) (includes anim blend)", animchar_act_time_usec, animchar_act_cnt,
      (double)animchar_act_time_usec / animchar_act_cnt);

  if (animchar_blendAnim_cnt)
    debug("  animchar::blend   %5u usec for %u acts (%4.1f usec/call, %.4f usec/tm), PRScopy=%.1f%% PBC=%.1f%%",
      animchar_blendAnim_time_usec, animchar_blendAnim_cnt, (double)animchar_blendAnim_time_usec / animchar_blendAnim_cnt,
      animchar_blendAnim_tm_cnt ? (double)animchar_blendAnim_time_usec / animchar_blendAnim_tm_cnt : -1.0f,
      animchar_blendAnim_time_usec ? perfmonstat::animchar_blendAnim_getprs_time_usec * 100.0 / animchar_blendAnim_time_usec : 0,
      animchar_blendAnim_time_usec ? perfmonstat::animchar_blendAnim_pbc_time_usec * 100.0 / animchar_blendAnim_time_usec : 0);

  dumped_gen_cnt = perfmonstat::generation_counter;
  gen_frame_no = dagor_frame_no();
}
