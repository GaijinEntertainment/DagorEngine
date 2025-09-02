// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// @NOTE: once MAX_TEMP_REGS reaches 512 it will definitely cause stack checks for stcode execution => then we should use alloca
static constexpr int MAX_TEMP_REGS = 256;
