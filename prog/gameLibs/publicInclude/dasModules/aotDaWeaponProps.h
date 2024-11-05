//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <ecs/game/weapons/shellProps.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotProps.h>
#include <daWeapons/gunProps.h>
#include <daWeapons/shellStartProps.h>
#include <daWeapons/projectileSpread.h>
#include <daWeapons/projectileTracerProps.h>
#include <daWeapons/meleeProps.h>
#include <daWeapons/shellSpawnProps.h>
#include <ecs/game/weapons/gunES.h>
#include <daWeapons/shellEntityTemplateProps.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/dasModulesCommon.h>
#include <daScript/ast/ast_typedecl.h>

MAKE_TYPE_FACTORY(ShellPropIds, daweap::ShellPropIds);
MAKE_TYPE_FACTORY(ShellStartProps, daweap::ShellStartProps);
MAKE_TYPE_FACTORY(ProjectileSpreadProps, ProjectileSpreadProps);
MAKE_TYPE_FACTORY(ProjectileTracerProps, daweap::ProjectileTracerProps);
MAKE_TYPE_FACTORY(ShellEntityTypeProps, daweap::ShellEntityTypeProps);
MAKE_TYPE_FACTORY(MeleeProps, daweap::MeleeProps);
MAKE_TYPE_FACTORY(VolumeProps, daweap::VolumeProps);
MAKE_TYPE_FACTORY(ShellSpawnProps, daweap::ShellSpawnProps);

DAS_BIND_VECTOR(GunShellPropIds, GunShellPropIds, daweap::ShellPropIds, " ::GunShellPropIds");

namespace bind_dascript
{
inline const char *shell_entity_props_get_physTemplName(const daweap::ShellEntityTypeProps &props) { return props.physTemplName; }

inline const daweap::ShellPropIds *get_shell_props_ids_const(const ecs::ChildComponent &cmp)
{
  return cmp.getNullable<daweap::ShellPropIds>();
}
inline daweap::ShellPropIds *get_shell_props_ids(ecs::ChildComponent &cmp) { return cmp.getNullable<daweap::ShellPropIds>(); }
inline const char *das_melee_props_get_node_name(const daweap::MeleeProps &props) { return props.nodeName.str(); }

inline void set_shell_prop_ids(ecs::Object &obj, const char *name, const daweap::ShellPropIds &shell_id)
{
  obj.addMember(name, shell_id);
}
} // namespace bind_dascript
