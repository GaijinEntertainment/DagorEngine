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
