//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>
struct WatchedFolderMonitorData;

KRNLIMP bool could_be_changed_folder(WatchedFolderMonitorData *); // return true if folder contents could changed during last
                                                                  // could_be_changed_folder call
KRNLIMP WatchedFolderMonitorData *add_folder_monitor(const char *folder_name, int sleep_interval_msec); // return handle

// destroys handle returned by add_folder_monitor. returns false if Thread is still active, after attempts*sleep_interval_msec
// if attempts = 0 - infinite wait amount of time
// nullptr is allowed, will return true
KRNLIMP bool destroy_folder_monitor(WatchedFolderMonitorData *, int attempts);

#include <supp/dag_undef_KRNLIMP.h>
