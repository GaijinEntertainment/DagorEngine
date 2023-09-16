//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <perfMon/dag_statDrv.h>

struct ID3dDrawStatistic : DrawStatSingle
{
  void startFrame() { val[DRAWSTAT_FRAME]++; }
  void updateDrawPrim() { val[DRAWSTAT_DP]++; }
  void updateTriangles(int add_Tr) { tri += add_Tr; }
  void updateLockVIBuf() { val[DRAWSTAT_LOCKVIB]++; }
  void updateRenderTarget() { val[DRAWSTAT_RT]++; }
  void updateProgram() { val[DRAWSTAT_PS]++; }
  void updateInstances(int inst) { val[DRAWSTAT_INS] += inst; }
  void updateLogicalRenderPass() { val[DRAWSTAT_RENDERPASS_LOGICAL]++; }
};

namespace Stat3D
{

#if TIME_PROFILER_ENABLED
extern ID3dDrawStatistic g_draw_stat;

inline void startStatSingle(DrawStatSingle &dSS) { dSS = g_draw_stat; }
inline void stopStatSingle(DrawStatSingle &dSS) { drawstat_diff(dSS, g_draw_stat); }

inline void startFrame() { g_draw_stat.startFrame(); }
inline void updateDrawPrim() { g_draw_stat.updateDrawPrim(); }
inline void updateTriangles(int add_tr) { g_draw_stat.updateTriangles(add_tr); }
inline void updateLockVIBuf() { g_draw_stat.updateLockVIBuf(); }
inline void updateRenderTarget() { g_draw_stat.updateRenderTarget(); }
inline void updateProgram() { g_draw_stat.updateProgram(); }
inline void updateInstances(int inst) { g_draw_stat.updateInstances(inst); }
inline void updateLogicalRenderPass() { g_draw_stat.updateLogicalRenderPass(); }

#else

inline void startStatSingle(DrawStatSingle &) {}
inline void stopStatSingle(DrawStatSingle &) {}

inline void startFrame() {}
inline void updateDrawPrim() {}
inline void updateTriangles(int) {}
inline void updateLockVIBuf() {}
inline void updateRenderTarget() {}
inline void updateProgram() {}
inline void updateInstances(int) {}
inline void updateLogicalRenderPass() {}

#endif

}; // namespace Stat3D
