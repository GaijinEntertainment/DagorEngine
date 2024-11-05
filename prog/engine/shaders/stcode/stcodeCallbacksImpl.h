// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_stcode.h>
#include "../constSetter.h"

#include <shaders/stcode/callbackTable.h>

namespace stcode::cpp
{

extern const cpp::CallbackTable cb_table_impl;

extern OnBeforeResourceUsedCb on_before_resource_used_cb;
extern const char *last_resource_name;
extern int req_tex_level;

extern shaders_internal::ConstSetter *custom_const_setter;

} // namespace stcode::cpp
