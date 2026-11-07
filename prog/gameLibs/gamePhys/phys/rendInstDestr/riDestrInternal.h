// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <scene/dag_physMatIdDecl.h>
#include <rendInst/riexHandle.h>
#include <generic/dag_functionRef.h>
#include "riDestrProps.h"
#if DAGOR_DBGLEVEL > 0
#include <util/dag_convar.h>
#endif

#define RI_DESTR_DEBUG_ENABLED (DAGOR_DBGLEVEL > 0)

namespace rendinst
{
struct RendInstDesc;
struct RendInstBufferData;
extern bool enable_apex;
} // namespace rendinst

namespace destructables
{
struct DestructableCreationParams;
}

inline constexpr float MAX_RI_DESTROY_IMPULSE = 1e8f;

#define FMT_DESC_STR  "[pool:%i cell:%i index:%i offset:%i]"
#define FMT_DESC_V(X) int((X).pool), int((X).cellIdx), int((X).idx), int((X).offs)

namespace rendinstdestr
{

#if DAGOR_DBGLEVEL > 0
extern ConVarT<bool, false> ri_debug_collision;
#else
inline constexpr bool ri_debug_collision = false;
#endif

rendinst::riex_handle_t restore_rendinst(const rendinst::RendInstBufferData &buffer);
void for_each_nearby_ri(const RiNearbyGroupsProps *nearby_groups, RiNearbyGroupsProps::Actions required_actions,
  const rendinst::RendInstBufferData &owner, dag::FunctionRef<void(rendinst::riex_handle_t, RiNearbyGroupsProps::Actions) const> cb);

float get_sync_time();
void queue_ri_collision_for_debris_if_enabled(const mat44f &inst_tm, const bbox3f &local_bbox);
PhysMat::MatID get_destructable_physmat_for_ri(const rendinst::RendInstDesc &desc);

} // namespace rendinstdestr
