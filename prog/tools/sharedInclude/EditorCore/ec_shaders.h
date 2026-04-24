// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>

namespace tools3d
{

bool get_snapshot_path(const DataBlock &blk, const char *app_dir, String &out_path);
String get_shaders_path(const DataBlock &blk, const char *app_dir, bool use_dng);

} // namespace tools3d
