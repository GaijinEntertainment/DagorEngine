//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace d3dhang
{

using GPUHanger = void (*)();

void register_gpu_hanger(GPUHanger newHanger);
void hang_gpu_on(const char *event);
void hang_if_requested(const char *event);

} // namespace d3dhang
