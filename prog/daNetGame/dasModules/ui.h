// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <ui/uiShared.h>

namespace bind_dascript
{
inline void get_ui_view_tm(TMatrix &tm) { tm = uishared::view_tm; }
inline void get_ui_view_itm(TMatrix &tm) { tm = uishared::view_itm; }
} // namespace bind_dascript