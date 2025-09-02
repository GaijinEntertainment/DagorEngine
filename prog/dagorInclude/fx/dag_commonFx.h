//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

void register_light_fx_factory();
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

class AcesEffect;
class TMatrix;
class Point3;

enum FxErrorType
{
  FX_ERR_NONE = 0,
  FX_ERR_INVALID_TYPE = 1,
  FX_ERR_HAS_ENOUGH = 2,
};

namespace acesfx
{
struct StartFxParams;
enum class PrefetchType : int
{
  DEFAULT = 0,
  FORCE_SYNC = 1,
  FORCE_ASYNC = 2,
};

int get_type_by_name(const char *name);
void start_effect_client(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, bool is_player, bool with_sound,
  AcesEffect **locked_fx, float audio_intensity, FxErrorType *perr = nullptr);
void start_effect_client(const StartFxParams &params, AcesEffect **locked_fx, FxErrorType *perr = nullptr);
} // namespace acesfx