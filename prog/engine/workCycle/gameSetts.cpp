// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <workCycle/dag_wcHooks.h>
#include <workCycle/wcPrivHooks.h>
#include "workCyclePriv.h"

bool dgs_limit_fps = false;
bool dgs_draw_fps = false;
bool dgs_immovable_window = false;
bool dgs_dont_use_cpu_in_background = false;
bool dgs_higher_active_app_priority = false;

bool dgctrl_need_screen_shot = false;
bool dgctrl_need_print_screen_shot = false;
int dgctrl_print_screen_shot_multiplier = 3;

IGeneralGuiManager *dagor_gui_manager = NULL;

bool workcycle_internal::window_initing = true;
bool workcycle_internal::enable_idle_priority = true;
bool workcycle_internal::is_window_in_thread = false;
int workcycle_internal::minVariableRateActTimeUsec = 0;
int workcycle_internal::lastFrameTime = -1;
int workcycle_internal::fixedActPerfFrame = 0, workcycle_internal::curFrameActs = 0;

void (*dwc_hook_before_frame)() = NULL;
void (*dwc_hook_after_frame)() = NULL;
void (*dwc_hook_ts_before_frame)() = NULL;
void (*dwc_hook_fps_log)(int) = NULL;
void (*dwc_hook_memory_report)() = NULL;
bool (*dwc_can_draw_next_frame)(int frame, int usec_to_next_act) = NULL;

bool dwc_alloc_perform_delayed_actions = true;
bool dwc_alloc_perform_delayed_actions_in_internal_winloop = true;

float visibility_range_multiplier = 1.f;
