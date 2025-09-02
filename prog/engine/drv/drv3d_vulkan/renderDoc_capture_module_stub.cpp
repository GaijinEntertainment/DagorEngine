// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderDoc_capture_module.h"

namespace drv3d_vulkan
{
void RenderDocCaptureModule::init() {}
void RenderDocCaptureModule::shutdown() {}
void RenderDocCaptureModule::triggerCapture(uint32_t /*count*/){};
void RenderDocCaptureModule::beginCapture() {}
void RenderDocCaptureModule::endCapture() {}
void RenderDocCaptureModule::loadRenderDocAPI() {}
const char *RenderDocCaptureModule::getLibName() { return ""; };
} // namespace drv3d_vulkan
