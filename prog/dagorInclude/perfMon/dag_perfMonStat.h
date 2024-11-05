//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace perfmonstat
{
extern unsigned fastphys_update_cnt, fastphys_update_time_usec;
extern unsigned geomtree_calcWtm_cnt, geomtree_calcWtm_tm_cnt, geomtree_calc_time_usec;
extern unsigned animchar_act_cnt, animchar_act_time_usec;
extern unsigned animchar_blendAnim_cnt, animchar_blendAnim_tm_cnt, animchar_blendAnim_time_usec;
extern unsigned animchar_blendAnim_getprs_time_usec, animchar_blendAnim_pbc_time_usec;

extern unsigned generation_counter;

void dump_stat(bool only_if_gen_count_changed = true);
} // namespace perfmonstat
