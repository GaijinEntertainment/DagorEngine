// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// @NOTE: once MAX_TEMP_REGS reaches 512 it will definitely cause stack checks for stcode execution => then we should use alloca
static constexpr int MAX_TEMP_REGS = 256;

// If runtime sees a dump with > soft limit, it logerrs
static constexpr int SOFT_SHADER_VARIANT_LIMIT = 32000;
// If compiler is about to generate a dump with > hard limit shaders, it errors and does not write it.
static constexpr int HARD_SHADER_VARIANT_LIMIT = 33000;
