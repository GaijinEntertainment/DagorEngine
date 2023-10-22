//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

void register_anim_planes_fx_factory();
void register_compound_ps_fx_factory();
void register_flow_ps_2_fx_factory();
void register_light_fx_factory();
void register_dafx_factory();
void register_dafx_sparks_factory();
void register_dafx_modfx_factory();
void register_dafx_compound_factory();


void register_all_common_fx_factories();


// helpers to improve FX textures usage with texture streaming
// this code detects usage (loading) of TEXTAG_FX-marked textures, adds them to internal list and calls prefetch on them
// internal list should be reset on session end (to avoid prefetching textures that not needed for the next session)

//! updates internal list of used FX tex and call prefetch on them (should be called from update-on-frame)
void update_and_prefetch_fx_textures_used();
//! resets internal list of used FX tex (should be called on game session end)
void reset_fx_textures_used();
