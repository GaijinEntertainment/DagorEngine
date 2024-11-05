// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <streaming/dag_streamingCtrl.h>
#include <streaming/dag_streamingMgr.h>
#include <debug/dag_debug3d.h>

void StreamingSceneController::renderDbg()
{
  begin_draw_cached_debug_lines(true, true);
  for (int i = actionSph.size() - 1; i >= 0; i--)
  {
    E3DCOLOR col;

    if (!mgr.isBinDumpValid(actionSph[i].bindumpId))
      col = E3DCOLOR(0, 160, 160);
    else if (mgr.isBinDumpLoaded(actionSph[i].bindumpId))
      col = E3DCOLOR(255, 160, 160);
    else
      col = E3DCOLOR(200, 200, 200);

    draw_cached_debug_xz_circle(actionSph[i].center, actionSph[i].rad, col);
  }

  end_draw_cached_debug_lines();
}
