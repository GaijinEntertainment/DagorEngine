//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

bool init_single_process_checker(const char *mutex_name); // false - Another instance of game is already started
void close_single_process_checker();

enum class ExistingInstanceAction
{
  Continue,    // no other instance, or user chose to wait / run both
  ExitProcess, // user chose to close this instance, or release-build collision
};

// On Windows, detects whether another instance already holds the named
// mutex.
//
// On collision in DAGOR_DBGLEVEL > 0 builds, shows a 3-button modal:
//   Yes    -- close this instance         -> returns ExitProcess
//   No     -- wait for previous to exit   -> blocks, then returns Continue
//   Cancel -- run both                    -> returns Continue
//
// On collision in DAGOR_DBGLEVEL < 1 builds, returns ExitProcess without UI;
// the caller is expected to surface its own "already running" message and
// exit. On non-PC-Win platforms, always returns Continue.
//
// Pairs with close_single_process_checker() at shutdown.
ExistingInstanceAction check_or_prompt_existing_instance(const char *mutex_name);
