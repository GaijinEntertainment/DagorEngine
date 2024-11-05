// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/event.h>
#include <daECS/core/entityId.h>
#include "daECS/net/connection.h"
#include <daECS/net/netbase.h>
#include <daECS/net/object.h>
#include <daECS/net/schemelessEventSerialize.h>
#include <daECS/net/msgSink.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/scripts/netsqevent.h>
#include <dasModules/aotEcs.h>
#include <dasModules/aotNet.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <daNet/daNetPeerInterface.h>
#include <daNet/disconnectionCause.h>
#include <daNet/bitStream.h>

#include "net/netConsts.h"
#include "net/recipientFilters.h"
#include "net/reCreateEntity.h"
#include "phys/quantization.h"
#include "net/net.h"
#include "net/dedicated.h"
#include "net/locSnapshots.h"

#include "game/player.h"
#include "main/appProfile.h" // get_matching_invite_data


DAS_BIND_ENUM_CAST_98(DisconnectionCause)
DAS_BIND_ENUM_CAST_98(ClientNetFlags)
DAS_BASE_BIND_ENUM_98(ClientNetFlags,
  ClientNetFlags,
  CNF_REPLAY_RECORDING,
  CNF_REPLICATE_PHYS_ACTORS,
  CNF_CONN_UNRESPONSIVE,
  CNF_DEVELOPER,
  CNF_SPAWNED_AT_LEAST_ONCE,
  CNF_RECONNECT_FORBIDDEN)


MAKE_TYPE_FACTORY(LocSnapshot, LocSnapshot);
DAS_BIND_VECTOR(LocSnapshotsList, LocSnapshotsList, LocSnapshot, " ::LocSnapshotsList");

template <typename F>
inline ecs::EntityId do_entity_recreate_fn_with_init_and_lambda(ecs::EntityId eid,
  const char *template_name,
  const das::Lambda &lambda,
  const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context,
  das::LineInfoArg *line_info,
  F entfn)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, line_info);

  das::SimFunction **fnMnh = (das::SimFunction **)lambda.capture;
  if (DAGOR_UNLIKELY(!fnMnh))
    context->throw_error_at(line_info, "invoke null lambda");
  das::SimFunction *simFunc = *fnMnh;
  if (DAGOR_UNLIKELY(!simFunc))
    context->throw_error_at(line_info, "invoke null function");
  auto pCapture = lambda.capture;
  bind_dascript::ContextLockGuard lockg(*context);
  return entfn(eid, template_name, eastl::move(init),
    [=, lockg = eastl::move(lockg)](ecs::EntityId eid, ecs::ComponentsInitializer &cInit) {
      vec4f argI[3];
      argI[0] = das::cast<void *>::from(pCapture);
      argI[1] = das::cast<ecs::EntityId>::from(eid);
      argI[2] = das::cast<ecs::ComponentsInitializer &>::from(cInit);
      if (!context->ownStack)
      {
        das::SharedStackGuard guard(*context, bind_dascript::get_shared_stack());
        (void)context->call(simFunc, argI, 0);
      }
      else
        (void)context->call(simFunc, argI, 0);
    });
}

namespace bind_dascript
{
inline ecs::EntityId _builtin_remote_recreate_entity_from(ecs::EntityId eid, const char *template_name)
{
  return remote_recreate_entity_from(eid, template_name);
}
inline ecs::EntityId _builtin_remote_recreate_entity_from_block(ecs::EntityId eid,
  const char *template_name,
  const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);
  return remote_recreate_entity_from(eid, template_name, eastl::move(init));
}

inline ecs::EntityId _builtin_remote_recreate_entity_from_block_lambda(ecs::EntityId eid,
  const char *template_name,
  const das::Lambda &lambda,
  const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context,
  das::LineInfoArg *line_info)
{
  return do_entity_recreate_fn_with_init_and_lambda(eid, template_name, lambda, block, context, line_info,
    [](ecs::EntityId eid, const char *tname, ecs::ComponentsInitializer &&cinit, remote_recreate_entity_async_cb_t &&cb) {
      return remote_recreate_entity_from(eid, tname, eastl::move(cinit), eastl::move(cb));
    });
}

inline ecs::EntityId _builtin_remote_change_sub_template(ecs::EntityId eid, const char *rem_name, const char *add_name, bool force)
{
  return remote_change_sub_template(eid, rem_name, add_name, {}, {}, force);
}

inline ecs::EntityId _builtin_remote_change_sub_template_block(ecs::EntityId eid,
  const char *rem_name,
  const char *add_name,
  const das::TBlock<void, ecs::ComponentsInitializer> &block,
  bool force,
  das::Context *context,
  das::LineInfoArg *at)
{
  ecs::ComponentsInitializer init;
  vec4f arg = das::cast<char *>::from((char *)&init);
  context->invoke(block, &arg, nullptr, at);
  return remote_change_sub_template(eid, rem_name, add_name, eastl::move(init), {}, force);
}

inline ecs::EntityId _builtin_remote_change_sub_template_block_lambda(ecs::EntityId eid,
  const char *rem_name,
  const char *add_name,
  const das::Lambda &lambda,
  const das::TBlock<void, ecs::ComponentsInitializer> &block,
  das::Context *context,
  das::LineInfoArg *line_info)
{
  return do_entity_recreate_fn_with_init_and_lambda(eid, nullptr, lambda, block, context, line_info,
    [&](ecs::EntityId eid, const char *, ecs::ComponentsInitializer &&cinit, remote_recreate_entity_async_cb_t &&cb) {
      return remote_change_sub_template(eid, rem_name, add_name, eastl::move(cinit), eastl::move(cb));
    });
}

inline das::float3 get_hidden_pos() { return das::float3(HIDDEN_QWPOS_XYZ); }

inline void quantize_and_write_to_bitstream(const Point3 &pos, const Point4 &orientation, bool inMotion, danet::BitStream &bitstream)
{
  QuantizedWorldLocDefault qloc(pos, Quat(orientation.x, orientation.y, orientation.z, orientation.w), inMotion);
  CachedQuantizedInfoT<>::template serializeQLoc<>(bitstream, qloc, DPoint3(pos));
}

inline bool deserialize_snapshot_quantized_info(Point3 &pos, Quat &orientation, bool &in_motion, const danet::BitStream &bitstream)
{
  return CachedQuantizedInfoT<>::template deserializeQLoc<>(bitstream, pos, orientation, in_motion);
}
} // namespace bind_dascript
