//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>

inline bool is_streaming_log_enabled()
{
  static const bool enabled = dgs_get_settings() && dgs_get_settings()->getBlockByNameEx("debug")->getBool("logStreaming", false);
  return enabled;
}

#define STREAMING_LOG(fmt, ...)                                     \
  do                                                                \
  {                                                                 \
    if (is_streaming_log_enabled())                                 \
      logdbg("[STRM] fr=%u " fmt, dagor_frame_no(), ##__VA_ARGS__); \
  } while (0)
