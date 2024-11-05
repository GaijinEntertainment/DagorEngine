//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

void PartialDxtRender(Texture *rt, Texture *rtn, int linesPerPart, int picWidth, int picHeight, int numMips, bool dxt5, bool dxt5n,
  void (*renderFunc)(int lineNo, int linesCount, int totalLines, void *user, const Point3 &view_pos), void *user,
  const Point3 &view_pos, bool gamma_mips = true, bool update_game_screen = false);
