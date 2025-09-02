// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>

class SqModules;

namespace ui
{
namespace xray_ui_order
{
bool should_render_xray_before_ui();
void bind_script(SqModules *moduleMgr);
} // namespace xray_ui_order
} // namespace ui
