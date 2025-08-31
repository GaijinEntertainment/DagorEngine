// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "amdFsr.h"

namespace amd
{

FSR *createFSR() { return nullptr; }

FSR::UpscalingMode FSR::getUpscalingMode(const DataBlock &) { return UpscalingMode::Off; }

bool FSR::isSupported() { return false; }

int FSR::getMaximumNumberOfGeneratedFrames() { return 0; }

void FSR::applyUpscaling(const UpscalingArgs &) {}
void FSR::scheduleGeneratedFrames(const FrameGenArgs &) {}

} // namespace amd