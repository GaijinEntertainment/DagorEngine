//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast.h>
#include <dasModules/dasEvent.h>
#include <dasModules/aotEcs.h>
#include <daScript/ast/ast.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/schemelessEvent.h>
#include <daECS/core/component.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <daNet/packetPriority.h>

MAKE_TYPE_FACTORY(EventsDB, ecs::EventsDB);

typedef ecs::Object ecs_object;
typedef ecs::IntList ecs_IntList;
typedef ecs::FloatList ecs_FloatList;
typedef ecs::Point3List ecs_Point3List;
typedef ecs::Point4List ecs_Point4List;

// NB: edit same define in sqcoredaECS.cpp
#define DAS_EVENT_ECS_CONT_LIST_TYPES          \
  DECL_LIST_TYPE(ecs_IntList, "IntList")       \
  DECL_LIST_TYPE(ecs_FloatList, "FloatList")   \
  DECL_LIST_TYPE(ecs_Point3List, "Point3List") \
  DECL_LIST_TYPE(ecs_Point4List, "Point4List")

#define DAS_EVENT_ECS_CONT_TYPES       \
  DECL_LIST_TYPE(ecs_object, "Object") \
  DAS_EVENT_ECS_CONT_LIST_TYPES
// NB: add new types carefully, every new type has performance cost

namespace bind_dascript
{
struct DascriptEventDesc;

inline constexpr uint8_t NET_LIABLE_BITS = 2;

extern const uint8_t DASEVENT_NO_ROUTING;
extern const uint8_t DASEVENT_NATIVE_ROUTING;
;
uint64_t get_dasevents_generation();
uint8_t get_dasevent_routing(ecs::event_type_t type);
ecs::event_flags_t get_dasevent_cast_flags(ecs::event_type_t type);
eastl::tuple<uint8_t, PacketReliability> get_dasevent_routing_reliability(ecs::event_type_t type);
uint32_t lock_dasevent_net_version();
uint32_t get_dasevent_net_version();
void unlock_dasevent_net_version();

struct DascriptEventDesc
{
  enum class NetLiable : uint8_t
  {
    Strict, // required event, include in net protocol version
    Logerr, // not required event, just print logerr on receive
    Ignore, // not required event, will be ignored

    Count,
  };
  uint16_t version = 0;
  ecs::event_flags_t castFlags = 0; // only cast flags
  uint8_t routing = DASEVENT_NO_ROUTING;
  NetLiable netLiable = NetLiable::Strict;
  PacketReliability reliability = RELIABLE_ORDERED;

  DascriptEventDesc() = default;
  DascriptEventDesc(uint16_t version, ecs::event_flags_t cast_flags, uint8_t routing_, NetLiable net_liable,
    PacketReliability reliability_) :
    version(version), castFlags(cast_flags), routing(routing_), netLiable(net_liable), reliability(reliability_)
  {
    G_STATIC_ASSERT((1 << NET_LIABLE_BITS) >= (uint8_t)NetLiable::Count);
  }
};

DascriptEventDesc::NetLiable get_dasevent_net_liable(ecs::event_type_t type);

extern vec4f _builtin_event_dup(das::Context &, das::SimNode_CallBase *call, vec4f *args);

inline void _builtin_send_event(ecs::EntityId eid, ecs::event_type_t evt_type, const char *evt_type_name, const das::Array &arr)
{
  // debug("send event <%s|0x%x> to %d", evt_type_name, evt_type, ecs::entity_id_t(eid));
  G_UNUSED(eid);
  G_UNUSED(evt_type);
  G_UNUSED(evt_type_name);
  G_UNUSED(arr);
  // g_entity_mgr->sendEvent(eid, DasEvent(evt_type, 0, arr.data, arr.size, false));
  G_ASSERT(0);
}

inline void _builtin_broadcast_event(ecs::event_type_t evt_type, const char *evt_type_name, const das::Array &arr)
{
  G_UNUSED(evt_type);
  G_UNUSED(evt_type_name);
  G_UNUSED(arr);
  // g_entity_mgr->broadcastEvent(DasEvent(evt_type, 0, arr.data, arr.size, false));
  G_ASSERT(0);
}

template <typename F>
__forceinline void _builtin_send_blobevent2_impl(ecs::EntityManager *mgr, char *evt_data, const char *evt_type_name, const F &fn)
{
  ecs::Event *evt = (ecs::Event *)evt_data;
  G_UNUSED(evt_type_name);
  fn(*evt);
  if (EASTL_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is immediate strategy.
    mgr->getEventsDb().destroy(*evt);
}

__forceinline void _builtin_send_blobevent2(ecs::EntityManager *mgr, ecs::EntityId eid, char *evt_data, const char *evt_type_name)
{
  _builtin_send_blobevent2_impl(mgr, evt_data, evt_type_name, [eid, mgr](ecs::Event &evt) { mgr->sendEvent(eid, evt); });
}

__forceinline void _builtin_send_blobevent_immediate2(ecs::EntityManager *mgr, ecs::EntityId eid, char *evt_data,
  const char *evt_type_name)
{
  _builtin_send_blobevent2_impl(mgr, evt_data, evt_type_name, [eid, mgr](ecs::Event &evt) { mgr->sendEventImmediate(eid, evt); });
}

__forceinline void _builtin_broadcast_blobevent2(ecs::EntityManager *mgr, char *evt_data, const char *evt_type_name)
{
  _builtin_send_blobevent2_impl(mgr, evt_data, evt_type_name, [mgr](ecs::Event &evt) { mgr->broadcastEvent(evt); });
}

__forceinline void _builtin_broadcast_blobevent_immediate2(das::Context &context, char *evt_data, const char *evt_type_name)
{
  bind_dascript::EsContext &ctx = cast_es_context(context);
  _builtin_send_blobevent2_impl(ctx.mgr, evt_data, evt_type_name, [&ctx](ecs::Event &evt) { ctx.mgr->broadcastEventImmediate(evt); });
}

inline vec4f _builtin_send_blobevent(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  G_FAST_ASSERT(call->nArguments == 3);
  G_UNUSED(call);
  bind_dascript::EsContext &ctx = cast_es_context(context);
  _builtin_send_blobevent2(ctx.mgr, das::cast<ecs::EntityId>::to(args[0]), das::cast<char *>::to(args[1]),
    das::cast<char *>::to(args[2]));
  return v_zero();
}

inline vec4f _builtin_broadcast_blobevent(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  G_FAST_ASSERT(call->nArguments == 2);
  G_UNUSED(call);
  bind_dascript::EsContext &ctx = cast_es_context(context);
  _builtin_broadcast_blobevent2(ctx.mgr, das::cast<char *>::to(args[0]), das::cast<char *>::to(args[1]));
  return v_zero();
}

inline vec4f _builtin_send_blobevent_immediate(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  G_FAST_ASSERT(call->nArguments == 3);
  G_UNUSED(call);
  bind_dascript::EsContext &ctx = cast_es_context(context);
  _builtin_send_blobevent_immediate2(ctx.mgr, das::cast<ecs::EntityId>::to(args[0]), das::cast<char *>::to(args[1]),
    das::cast<char *>::to(args[2]));
  return v_zero();
}

inline vec4f _builtin_broadcast_blobevent_immediate(das::Context &ctx, das::SimNode_CallBase *call, vec4f *args)
{
  G_FAST_ASSERT(call->nArguments == 2);
  G_UNUSED(call);
  _builtin_broadcast_blobevent_immediate2(ctx, das::cast<char *>::to(args[0]), das::cast<char *>::to(args[1]));
  return v_zero();
}

inline void send_schemeless(ecs::EntityId eid, const char *evt_name, const das::TBlock<void, ecs::Object> &block, das::Context *ctx,
  das::LineInfoArg *at)
{
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  ctx->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  bind_dascript::EsContext *context = cast_es_context(ctx);
  context->mgr->sendEvent(eid, eastl::move(evt));
}

inline void broadcast_schemeless(const char *evt_name, const das::TBlock<void, ecs::Object> &block, das::Context *ctx,
  das::LineInfoArg *at)
{
  ecs::Object data;
  vec4f arg = das::cast<char *>::from((char *)&data);
  ctx->invoke(block, &arg, nullptr, at);
  const char *delimiter = strstr(evt_name, "::");
  const char *realName = delimiter ? delimiter + 2 : evt_name;
  ecs::SchemelessEvent evt(ECS_HASH_SLOW(realName).hash, eastl::move(data));
  bind_dascript::EsContext *context = cast_es_context(ctx);
  context->mgr->broadcastEvent(eastl::move(evt));
}

inline const ecs::EventsDB &get_events_db() { return g_entity_mgr->getEventsDb(); }
inline ecs::EventsDB &get_events_db_rw() { return g_entity_mgr->getEventsDbMutable(); }

template <typename T>
inline const T *ecs_addr(const T &val)
{
  return &val;
}

} // namespace bind_dascript
