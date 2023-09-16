//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_define_COREIMP.h>

//! returns true is game is running in DEMO mode
extern KRNLIMP bool dagor_is_demo_mode();

//! resets idle timer
extern KRNLIMP void dagor_demo_reset_idle_timer();

//! updates idle timer
extern KRNLIMP void dagor_demo_update_idle_timer();

//! stops idle timer due to non-interactive state
extern KRNLIMP void dagor_demo_idle_timer_set_IS(bool interactive);

//! returns true when we need to quit game due to idle timeout
extern KRNLIMP bool dagor_demo_check_idle_timeout();

//! executes target-specific quite code for DEMO mode (auxLauncher is used only when game is not in DEMO mode)
extern KRNLIMP void dagor_demo_final_quit(const char *auxLauncher);


//! DEBUG: sets DEMO mode and idle timeout
extern KRNLIMP void dagor_force_demo_mode(bool demo, int timout_ms);


// set it in local project under #if DEMO
extern bool dagor_demo_mode;

#include <supp/dag_undef_COREIMP.h>
