// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotGamePhys.h>
#include <dasModules/aotDagorMath.h>
#include <debug/dag_log.h>
#include "dasModules/actor.h"
#include "../physKind.h"

MAKE_TYPE_FACTORY(PhysKindState, PhysKindState);
MAKE_TYPE_FACTORY(PhysKindControls, PhysKindControls);
MAKE_TYPE_FACTORY(PhysKind, PhysKind);
MAKE_TYPE_FACTORY(PhysKindActor, PhysKindActor);
MAKE_TYPE_FACTORY(PhysKindWireField, PhysKindWireField);

DAS_BIND_ENUM_CAST(PhysKindLod);
DAS_BASE_BIND_ENUM(PhysKindLod, PhysKindLod, Closest, Normal, Faraway, Min, Num)
DAS_BIND_ENUM_CAST(PhysKindWireCodec);
DAS_BASE_BIND_ENUM(PhysKindWireCodec, PhysKindWireCodec, Raw, Int, Range, Step)

namespace bind_dascript
{
template <unsigned N>
inline void *phys_kind_ext_ptr(uint8_t (&ext)[N], int size)
{
  if (DAGOR_LIKELY(unsigned(size) <= N))
    return ext;
  LOGERR_ONCE("phys kind data of %d bytes is over the %d byte limit", size, N);
  return nullptr;
}
inline void *phys_kind_state_ext(PhysKindState &state, int size) { return phys_kind_ext_ptr(state.ext, size); }
inline void *phys_kind_controls_ext(PhysKindControls &controls, int size) { return phys_kind_ext_ptr(controls.ext, size); }
inline das::int4 phys_desync_stats_to_int4(const PhysDesyncStats &stats)
{
  return das::int4(int(stats.processed), int(stats.untestable), int(stats.smallDifference), int(stats.largeDifference));
}
inline das::int4 phys_kind_desync_stats(const PhysKind &phys) { return phys_desync_stats_to_int4(phys.desyncStats); }
inline das::int4 phys_kind_owner_snapshot_stats(const PhysKind &phys)
{
  return phys_desync_stats_to_int4(phys.partialStateDesyncStats);
}
inline void phys_kind_set_snap_ext_sizes(PhysKind &phys, int closest, int normal, int faraway, int min_lod)
{
  const int sizes[] = {closest, normal, faraway, min_lod};
  for (int i = 0; i < (int)PhysKindLod::Num; ++i)
    phys.snapExtSize[i] = uint8_t(clamp(sizes[i], 0, int(PHYS_KIND_SNAP_EXT_MAX)));
}
inline bool phys_kind_set_wire_fields(
  PhysKind &phys, const das::TArray<PhysKindWireField> &state_fields, const das::TArray<PhysKindWireField> &controls_fields)
{
  return phys.setWire(make_span_const((const PhysKindWireField *)state_fields.data, state_fields.size),
    make_span_const((const PhysKindWireField *)controls_fields.data, controls_fields.size));
}
} // namespace bind_dascript
