//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// base gui startup: font loading + StdRenderer setup
void startup_gui_base(const char *fonts_blk_filename);

// special callbacks to control glyph rasterization (called from StdGuiRender::update_internals_per_act);
// missing callbacks implicitly allow rasterization;
// installed callbacks may prevent unexpected stalls (e.g. during battles, etc.)
extern bool (*dgs_gui_may_rasterize_glyphs_cb)();
extern bool (*dgs_gui_may_load_ttf_for_rasterization_cb)();
