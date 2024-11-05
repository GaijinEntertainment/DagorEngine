// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <propsRegistry/propsRegistry.h>
#include "net/netPropsRegistry.h"

namespace bind_dascript
{
inline int register_net_props(const char *blk_name, const char *class_name)
{
  return propsreg::register_net_props(blk_name ? blk_name : "", class_name ? class_name : "");
}
} // namespace bind_dascript