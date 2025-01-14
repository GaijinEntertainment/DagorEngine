//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace gpu_mem_dumper
{
// Collects and save GPU dump files to the directory gpuDumps/<dir_name>/
void saveDump(const char *dir_name);
void init();
} // namespace gpu_mem_dumper