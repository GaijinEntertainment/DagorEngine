//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//
// work cycle routines
//

//! executes one work cycle (render + act)
void dagor_work_cycle();

//! flushes any pending frame rendered from dagor_work_cycle();
//! must be used before any on-screen rendering outside of work cycle
void dagor_work_cycle_flush_pending_frame();

//! process system events
//! NOTE: it is called also from dagor_idle_cycle()
void dagor_process_sys_messages(bool input_only = false);

//! executes one idle cycle (execute regular actions, dispatch OS events and optionally issue Sleep when inactive)
//! NOTE: it is called also from dagor_work_cycle()
void dagor_idle_cycle(bool input_only = false);

//! renders current game scene and Gui
//! NOTE: it is called also from dagor_work_cycle()
void dagor_draw_scene_and_gui(bool call_before_render = true, bool draw_gui = true);

//! resets spent work time counter;
//! useful after time consuming operations (such as scene loading)
//! between two consecutive calls to dagor_work_cycle()
void dagor_reset_spent_work_time();

//! sets number of world acts per second and computes other variables
//! negative rate sets variable-act-rate mode with maximal rate being -rate
void dagor_set_game_act_rate(int rate);

//! returns act rate (world acts per second)
//! negative rate denotes  variable-act-rate mode with maximal rate being -rate
int dagor_get_game_act_rate();

//! enables or disables lowering process priority when application window is inactive.
//! true by default.
void dagor_enable_idle_priority(bool enable);

//! suppressed d3d::update() calls from engine work cycle.
//  Game is responsible for CPU<->GPU synchronization.
//! false by default.
void dagor_suppress_d3d_update(bool enable);

//
// work cycle settings; READ ONLY!
//

//! number of world acts per second; used in dagor_work_cycle()
//! this var is quasi-constant - the value is determined by game specific
//! default value =100 acts/second
//! NOTE: rate is DESIRED value, real value depends on game and system performance
extern unsigned dagor_game_act_rate;

//! 1/dagor_game_act_rate
extern float dagor_game_act_time;

//! microseconds (realtime) per one act (1000000/dagor_game_act_rate)
extern unsigned dagor_game_time_step_usec;


//
// game/real time scale (READ/WRITE)
// (default: 1.0)
//
extern float dagor_game_time_scale;

// recurrent workcycle counter
extern unsigned int dagor_workcycle_depth;

// act time is limited by 0.1s
extern bool dagor_game_act_time_limited;
