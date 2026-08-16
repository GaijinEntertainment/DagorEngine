//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Registers native module "ecs.computed" with two factories:
//   mkEcsComputed({comps=["a","b"], comps_rq=[...], comps_no=[...],
//                  comps_filter=[...], filter=null,
//                  defVal=null, tags=null, name=null})
//   mkEcsComputedEidMap({...}) - same params except defVal
//
// The result is an frp Computed whose value mirrors ECS data. An ECS change
// copies the raw values into a cache at event time; the script value is built
// from that cache during the graph's pull, and only while something consumes it.
// mkEcsComputed mirrors a single entity: one component in comps gives the bare
// value, several give a {comp_name: value} table, defVal is used when nothing
// matches. Selecting one entity is the caller's job: several matches are not an
// error, an arbitrary one wins; the mirror keeps its entity while it still
// matches and adopts another one when it is lost.
// mkEcsComputedEidMap mirrors every match instead, as a table keyed by the
// integer eid. The values keep the same shape: the bare value for one
// component, a {comp_name: value} table for several.
//
// A component is written as "name", ["name"], ["name", TYPE] or
// ["name", TYPE, default]. With a default it becomes optional, and entities
// that do not have it mirror that default. The type is baked into the query
// when it is built and never resolves later, so leave it out only for
// components that something registers already; write ecs.TYPE_* for the rest.
// Only types with a script value can be mirrored: a boxed engine type (an
// animchar, a phys actor) logs an error and always reads as the default.
// All components are tracked, the filter ones included, so declare them
// _tracked in the template. Without that the value only refreshes when an
// entity is created, recreated or destroyed.
//
// Mirrors can be created only during the es-loading phase, like ecs queries and
// entity systems.
//
// Threading: the mirroring-system events run on the entity manager owner thread,
// and daECS defers them out of constrained MT mode. A pull reads only the cache,
// never ECS data, so it may run off the owner thread (daNetGame pulls from a
// worker job) at any point where such events can not dispatch.
//
// from "ecs.computed" import mkEcsComputed, mkEcsComputedEidMap
//
// let heroHp = mkEcsComputed({comps=["hitpoints__hp"], comps_rq=["watchedByPlr"], defVal=0})
// let heroState = mkEcsComputed({comps=["hitpoints__hp", "hitpoints__maxHp"], comps_rq=["watchedByPlr"]})
// let playerNames = mkEcsComputedEidMap({comps=["name"], comps_rq=["player"], comps_no=["playerIsBot"]})
// let woundedNames = mkEcsComputedEidMap({comps=["name"], comps_rq=["player"],
//                                         comps_filter=[["hitpoints__hp", ecs.TYPE_FLOAT]],
//                                         filter="lt(hitpoints__hp, 30.0)"})

class SqModules;
typedef struct SQVM *HSQUIRRELVM;

namespace ecscomputed
{

void bind_module(SqModules *module_mgr, const char *default_es_tags = nullptr);

// Mandatory before the VM is closed: unregisters the VM's mirroring systems
// and releases their script references, which no later cleanup can do safely.
void shutdown_vm(HSQUIRRELVM vm);

// Unregisters mirroring systems whose script handle has been collected.
int sweep();

void get_stats(unsigned &out_events, unsigned &out_recalcs, unsigned &out_copied_rows);
void reset_stats();

} // namespace ecscomputed
