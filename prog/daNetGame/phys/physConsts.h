// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>

#define PHYS_ENABLE_INTERPOLATION                1
#define PHYS_MIN_INTERP_DELAY_TICKS              2
// Note: too much of interpolation might cause excessive "shot around a corner" kind of issues
#define PHYS_MAX_INTERP_DELAY_TICKS              4 // Warn: should fit in 4 bits (see PhysUpdateCtx::getCurInterpDelayTicksPacked)
#define PHYS_MAX_QUANTIZED_SLOW_SPEED            6.9f
#define PHYS_MAX_QUANTIZED_FAST_SPEED            35.f
#define PHYS_DEFAULT_TICKRATE                    30
#define PHYS_DEFAULT_BOT_TICKRATE                15
#define PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC (1.5f)
#define PHYS_MIN_TICKRATE                        8
#define PHYS_MAX_TICKRATE                        128
// Note: Only last controls are important in practice (with rare exceptions for stuff like <something>History,
// where prev controls are used as well). Besides, we are using controls packet duplication in daNet thread
#define PHYS_CONTROLS_NET_REDUNDANCY             2
#define PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY   (PHYS_MAX_INTERP_DELAY_TICKS + 2)
#define PHYS_MAX_CONTROLS_TICKS_DELTA_SEC        (0.375f) // i.e. 2x of this gives maximum playable ping
#define PHYS_SEND_AUTH_STATE_PERIOD_SEC          (1.f)
#define PHYS_MAX_AAS_DELAY_K                     (1.5) // relative to PHYS_SEND_AUTH_STATE_PERIOD_SEC

enum class PhysTickRateType : uint8_t
{
  Normal,
  LowFreq,
  Last = LowFreq,
};
