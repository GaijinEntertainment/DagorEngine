//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_fatal.h>
#include <util/dag_string.h>

static inline void load_tex_streaming_settings(const char *app_blk, DataBlock *texStreamingBlk, bool disable = false)
{
  if (!texStreamingBlk)
    texStreamingBlk = const_cast<DataBlock *>(dgs_get_settings())->addBlock("texStreaming");

  texStreamingBlk->clearData();
  if (app_blk)
  {
    DataBlock appblk;
    if (!appblk.load(app_blk))
      fatal("failed to load %s", app_blk);
    if (const char *tsfn = appblk.getBlockByNameEx("game")->getStr("texStreamingFile", NULL))
    {
      String abs_tsfn;
      if (tsfn[0] == '*')
        abs_tsfn = tsfn + 1;
      else if (dd_get_fname(app_blk) == app_blk)
        abs_tsfn.printf(0, "./%s", tsfn);
      else
        abs_tsfn.printf(0, "%.*s%s", dd_get_fname(app_blk) - app_blk, app_blk, tsfn);
      simplify_fname(abs_tsfn);
      DataBlock blk;
      if (!blk.load(abs_tsfn))
        fatal("failed to load texStreamingFile=%s", abs_tsfn);
      if (const char *blknm = appblk.getBlockByNameEx("game")->getStr("texStreamingBlock", NULL))
      {
        if (!blk.getBlockByName(blknm))
          fatal("failed to get texStreamingBlock=%s in texStreamingFile=%s", blknm, abs_tsfn);
        texStreamingBlk->setFrom(blk.getBlockByName(blknm));
      }
      else
        texStreamingBlk->setFrom(&blk);
      // texStreamingBlk->setInt("logLevel", 1);
    }
  }
  if (disable)
    texStreamingBlk->setBool("enableStreaming", false);
}
