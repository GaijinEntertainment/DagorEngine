// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

void init_statsd_common(const char *circuit_name);
void init_event_log(const char *circuit_name);
void setup_eventlog_log_cb();
void telemetry_shutdown();
void send_first_run_event();
