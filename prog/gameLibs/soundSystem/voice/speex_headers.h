// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define OUTSIDE_SPEEX
#define RANDOM_PREFIX op4s_speex
#include <speex/speex_echo.h>
#include <speex/speex_resampler.h>

#define speex_preprocess_state_init    op4s_speex_preprocess_state_init
#define speex_preprocess_state_destroy op4s_speex_preprocess_state_destroy
#define speex_preprocess_ctl           op4s_speex_preprocess_ctl
#define speex_preprocess_run           op4s_speex_preprocess_run
#include <speex/speex_preprocess.h>

#define jitter_buffer_init                  op4s_jitter_buffer_init
#define jitter_buffer_reset                 op4s_jitter_buffer_reset
#define jitter_buffer_destroy               op4s_jitter_buffer_destroy
#define jitter_buffer_put                   op4s_jitter_buffer_put
#define jitter_buffer_get                   op4s_jitter_buffer_get
#define jitter_buffer_get_pointer_timestamp op4s_jitter_buffer_get_pointer_timestamp
#define jitter_buffer_tick                  op4s_jitter_buffer_tick
#define jitter_buffer_update_delay          op4s_jitter_buffer_update_delay
#include <speex/speex_jitter.h>
