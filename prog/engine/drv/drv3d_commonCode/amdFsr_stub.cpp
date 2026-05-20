// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_amdFsr.h>

namespace amd
{
FSR::UpscalingMode FSR::getUpscalingMode(const DataBlock &) { return UpscalingMode::Off; }

bool FSR::isSupported() { return false; }

int FSR::getMaximumNumberOfGeneratedFrames() { return 0; }

} // namespace amd
