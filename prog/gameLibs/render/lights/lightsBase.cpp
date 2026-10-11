// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsBase.h>

IDynLightsPipelineSection::~IDynLightsPipelineSection() = default;
bool IDynLightsPipelineSection::isGpu() const { return getRuntimeFeatures() != DynLightsFeatures::CpuOnly; }
