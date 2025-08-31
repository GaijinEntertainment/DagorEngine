// Copyright (C) Gaijin Games KFT.  All rights reserved.

// raytrace features implementation
#include "vulkan_api.h"

#include <rayTracingStub.inc.cpp>

#define DEF_DESCRIPTOR_RAYTRACE

const bool isRaytraceAcclerationStructure(VkDescriptorType) { return false; }
