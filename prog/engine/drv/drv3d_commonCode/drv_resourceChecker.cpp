// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0

#include "drv_log_defs.h"

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#include <drv/3d/dag_resourceChecker.h>

uint32_t ResourceChecker::uploaded_framemem = 0;
uint32_t ResourceChecker::uploaded_total = 0;
uint32_t ResourceChecker::uploaded_framemem_limit = 0;
uint32_t ResourceChecker::uploaded_total_limit = 0;

void ResourceChecker::init()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  // 0 means no limit
  uploaded_framemem_limit = blk_video.getInt("frameLimitBufferFramememKB", 2 << 10) << 10;
  uploaded_total_limit = blk_video.getInt("frameLimitBufferTotalKB", 0) << 10;

  interlocked_exchange(uploaded_total, 0u);
  interlocked_exchange(uploaded_framemem, 0u);
}

void ResourceChecker::report()
{
  uint32_t total_size = interlocked_exchange(uploaded_total, 0u);
  uint32_t framemem_size = interlocked_exchange(uploaded_framemem, 0u);

  if (uploaded_total_limit && total_size > uploaded_total_limit)
    D3D_ERROR("[Buffer] total uploaded size per frame was %d while limit is %d", total_size, uploaded_total_limit);
  if (uploaded_framemem_limit && framemem_size > uploaded_framemem_limit)
    D3D_ERROR("[Buffer] framemem uploaded size per frame was %d while limit is %d", framemem_size, uploaded_framemem_limit);
}

#endif
