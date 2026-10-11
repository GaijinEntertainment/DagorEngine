//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

//! Starts a low priority thread that logs and removes files directly in get_log_directory(), none when it is empty, by the purgeLog{}
//! block: maxAgeDays (30), maxFileSizeMB (1024), maxCount newest kept (100), timeBudgetSec (120); 0 disables a rule. enabled defaults
//! to def_enabled, dryRun to DAGOR_DBGLEVEL > 0 (report only), logVerbose (off) lists each removed file. Start it after log handlers.
void start_purge_logs_thread(const DataBlock &cfg, bool def_enabled = true);
