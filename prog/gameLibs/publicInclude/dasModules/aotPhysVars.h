//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/phys/physVars.h>
#include <ecs/scripts/dasEcsEntity.h>

MAKE_TYPE_FACTORY(PhysVars, PhysVars);

namespace bind_dascript
{
inline int physvars_get_var_id(const PhysVars &phys_vars, const char *name) { return phys_vars.getVarId(name); }

inline void physvars_set_var(PhysVars &phys_vars, int var_id, float val, das::Context *ctx, das::LineInfoArg *at)
{
  if (uint32_t(var_id) >= uint32_t(phys_vars.getVarsCount()))
    ctx->throw_error_at(at, "incorrect var id %d, vars count %d", var_id, phys_vars.getVarsCount());
  phys_vars.setVar(var_id, val);
}

inline void physvars_set_var_val(PhysVars &phys_vars, const char *name, float val)
{
  phys_vars.setVar(phys_vars.registerVar(name, val), val);
}

inline float physvars_get_var(const PhysVars &phys_vars, int var_id) { return phys_vars.getVar(var_id); }

inline int physvars_register_var(PhysVars &phys_vars, const char *name, float val) { return phys_vars.registerVar(name, val); }

inline int physvars_register_pull_var(PhysVars &phys_vars, const char *name, float val)
{
  return phys_vars.registerPullVar(name, val);
}

inline bool physvars_is_var_pullable(const PhysVars &phys_vars, const int var_id) { return phys_vars.isVarPullable(var_id); }

inline int physvars_get_vars_count(const PhysVars &phys_vars) { return phys_vars.getVarsCount(); }

inline const char *physvars_get_var_name(const PhysVars &phys_vars, const int var_id) { return phys_vars.getVarName(var_id); }

}; // namespace bind_dascript
