// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>

// Writes value into slot of the daeditor_plot_debug singleton's signals list.
// In daEditor Plot windows, "Use debug signal" binds the signal's slot to an entry,
// letting you graph arbitrary debug values from anywhere in game code or daScript.

#if DAGOR_DBGLEVEL > 0
void daeditor_plot_debug_signal(float value, int slot = 0);
float daeditor_plot_debug_get_signal(int slot = 0);
#else
inline void daeditor_plot_debug_signal(float /*value*/, int /*slot*/ = 0) {}
inline float daeditor_plot_debug_get_signal(int /*slot*/ = 0) { return 0.0f; }
#endif
