// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

namespace amd
{

FSR *createFSR() { return nullptr; }

FSR::UpscalingMode FSR::getUpscalingMode(const DataBlock &) { return UpscalingMode::Off; }

bool FSR::isSupported() { return false; }

void FSR::applyUpscaling(const UpscalingArgs &) {}

} // namespace amd