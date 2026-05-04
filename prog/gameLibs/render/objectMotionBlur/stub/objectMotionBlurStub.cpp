// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/objectMotionBlur.h>
#include <drv/3d/dag_resId.h>

namespace objectmotionblur
{
bool is_enabled() { return false; }
void on_settings_changed(const MotionBlurSettings &) {}
bool is_frozen_in_pause() { return false; }
bool needs_transparent_gbuffer() { return false; }

void teardown() {}
void apply(BaseTexture *, BaseTexture *, float) {}
} // namespace objectmotionblur