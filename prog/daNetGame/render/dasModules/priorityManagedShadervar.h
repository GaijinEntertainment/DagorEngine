// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once
#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <render/priorityManagedShadervar.h>

namespace bind_dascript
{
inline void priority_shadervar_set_real(int id, int prio, float value) { PriorityShadervar::set_real(id, prio, value); }
inline void priority_shadervar_set_int(int id, int prio, int value) { PriorityShadervar::set_int(id, prio, value); }
inline void priority_shadervar_set_color4(int id, int prio, Point4 value) { PriorityShadervar::set_color4(id, prio, value); }
inline void priority_shadervar_set_int4(int id, int prio, IPoint4 value) { PriorityShadervar::set_int4(id, prio, value); }
inline void priority_shadervar_clear(int id, int prio) { PriorityShadervar::clear(id, prio); }
} // namespace bind_dascript