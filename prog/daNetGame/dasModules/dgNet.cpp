// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/net.h"
#include <daScript/daScript.h>
#include <daScriptModules/rapidjson/rapidjson.h>
#include <dasModules/dasEvent.h>
#include <dasModules/dasModulesCommon.h>
#include <daECS/net/component_replication_filter.h>
#include <dasModules/aotEcsEvents.h>
#include <dasModules/aotBitStream.h>

#include "net/protoVersion.h"

static char dng_net_das[] =
#include "dgNet.das.inl"
  ;

class EnumerationDisconnectionCause : public das::Enumeration
{
public:
  EnumerationDisconnectionCause() : das::Enumeration("DisconnectionCause")
  {
    external = true;
    cppName = "DisconnectionCause";
    baseType = (das::Type)das::ToBasicType<das::underlying_type<DisconnectionCause>::type>::type;
    DisconnectionCause enumArray[] = {
#define DC(x) x,
      DANET_DEFINE_DISCONNECTION_CAUSES
#undef DC
    };
    static const char *enumArrayName[] = {
#define DC(x) #x,
      DANET_DEFINE_DISCONNECTION_CAUSES
#undef DC
    };
    for (uint32_t i = 0; i < sizeof(enumArray) / sizeof(enumArray[0]); ++i)
      addI(enumArrayName[i], int64_t(enumArray[i]), das::LineInfo());
  }
};
DAS_BASE_BIND_ENUM_FACTORY(DisconnectionCause, "DisconnectionCause")
DAS_BASE_BIND_ENUM_FACTORY(DisconnectionCause_DasProxy, "DisconnectionCause")

namespace bind_dascript
{

struct LocSnapshotAnnotation final : das::ManagedStructureAnnotation<LocSnapshot, false>
{
  LocSnapshotAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("LocSnapshot", ml)
  {
    cppName = " ::LocSnapshot";
    addField<DAS_BIND_MANAGED_FIELD(quat)>("quat");
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(atTime)>("atTime");
    addField<DAS_BIND_MANAGED_FIELD(blink)>("blink");
    addField<DAS_BIND_MANAGED_FIELD(interval)>("interval");
  }
};

struct LocSnapshotsListAnnotation final : das::ManagedVectorAnnotation<LocSnapshotsList>
{
  LocSnapshotsListAnnotation(das::ModuleLibrary &ml) : ManagedVectorAnnotation("LocSnapshotsList", ml)
  {
    cppName = " ::LocSnapshotsList";
  }
};

class DngNetModule final : public das::Module
{
public:
  DngNetModule() : das::Module("DngNet")
  {
    using namespace das;

    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("rapidjson")); // for get_matching_invite_data
    addBuiltinDependency(lib, require("net"), true);
    addBuiltinDependency(lib, require("BitStream"));
    das::addConstant(*this, "NET_MAX_PLAYERS", NET_MAX_PLAYERS);
    das::addConstant(*this, "NET_PROTO_VERSION", NET_PROTO_VERSION);
    addEnumeration(das::make_smart<EnumerationDisconnectionCause>());
    addEnumeration(das::make_smart<EnumerationClientNetFlags>());
    addAnnotation(das::make_smart<LocSnapshotAnnotation>(lib));
    // addAnnotation(das::make_smart<SnapshotEntityDataAnnotation>(lib));
    das::addVectorAnnotation<LocSnapshotsList>(this, lib, das::make_smart<LocSnapshotsListAnnotation>(lib));

    addExtern<DAS_BIND_FUN(_builtin_remote_recreate_entity_from)>(*this, lib, "_builtin_remote_recreate_entity_from",
      SideEffects::modifyExternal, "bind_dascript::_builtin_remote_recreate_entity_from");
    addExtern<DAS_BIND_FUN(_builtin_remote_recreate_entity_from_block)>(*this, lib, "_builtin_remote_recreate_entity_from_block",
      SideEffects::modifyExternal, "bind_dascript::_builtin_remote_recreate_entity_from_block");
    addExtern<DAS_BIND_FUN(_builtin_remote_recreate_entity_from_block_lambda)>(*this, lib,
      "_builtin_remote_recreate_entity_from_block_lambda", SideEffects::modifyExternal,
      "bind_dascript::_builtin_remote_recreate_entity_from_block_lambda");
    addExtern<DAS_BIND_FUN(_builtin_remote_change_sub_template)>(*this, lib, "remote_change_sub_template", SideEffects::modifyExternal,
      "bind_dascript::_builtin_remote_change_sub_template")
      ->arg_init(3, make_smart<ExprConstBool>(false)); // force = false
    addExtern<DAS_BIND_FUN(_builtin_remote_change_sub_template_block)>(*this, lib, "remote_change_sub_template",
      SideEffects::modifyExternal, "bind_dascript::_builtin_remote_change_sub_template_block")
      ->arg_init(4, make_smart<ExprConstBool>(false)); // force = false
    addExtern<DAS_BIND_FUN(_builtin_remote_change_sub_template_block_lambda)>(*this, lib, "remote_change_sub_template",
      SideEffects::modifyExternal, "bind_dascript::_builtin_remote_change_sub_template_block_lambda");

    das::addExtern<DAS_BIND_FUN(get_hidden_pos)>(*this, lib, "get_hidden_pos", das::SideEffects::none,
      "bind_dascript::get_hidden_pos");

    das::addExtern<DAS_BIND_FUN(net::update_component_filter_event)>(*this, lib, "update_component_filter_event",
      das::SideEffects::modifyExternal, "::net::update_component_filter_event");
    das::addExtern<DAS_BIND_FUN(net::find_component_filter)>(*this, lib, "find_component_filter", das::SideEffects::none,
      "::net::find_component_filter");
    das::addExtern<DAS_BIND_FUN(::is_true_net_server)>(*this, lib, "is_true_net_server", das::SideEffects::accessExternal,
      "::is_true_net_server");
    das::addExtern<DAS_BIND_FUN(dedicated::is_dedicated)>(*this, lib, "is_dedicated", das::SideEffects::accessExternal,
      "dedicated::is_dedicated");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_dasevent_net_version)>(*this, lib, "get_dasevent_net_version",
      das::SideEffects::accessExternal, "bind_dascript::get_dasevent_net_version");
    das::addExtern<DAS_BIND_FUN(::has_network)>(*this, lib, "has_network", das::SideEffects::accessExternal, "::has_network");
    das::addExtern<DAS_BIND_FUN(::get_server_conn)>(*this, lib, "get_server_conn", das::SideEffects::accessExternal,
      "::get_server_conn");
    das::addExtern<DAS_BIND_FUN(::get_client_connection)>(*this, lib, "get_client_connection", das::SideEffects::accessExternal,
      "::get_client_connection");
    das::addExtern<DAS_BIND_FUN(::net_disconnect)>(*this, lib, "net_disconnect", das::SideEffects::modifyArgument, "::net_disconnect");
    das::addExtern<DAS_BIND_FUN(get_matching_invite_data), das::SimNode_ExtFuncCallRef>(*this, lib, "get_matching_invite_data",
      das::SideEffects::accessExternal, "::get_matching_invite_data");
    das::addExtern<DAS_BIND_FUN(bind_dascript::quantize_and_write_to_bitstream)>(*this, lib, "quantize_and_write_to_bitstream",
      das::SideEffects::modifyArgument, "bind_dascript::quantize_and_write_to_bitstream");
    das::addExtern<DAS_BIND_FUN(bind_dascript::deserialize_snapshot_quantized_info)>(*this, lib, "deserialize_snapshot_quantized_info",
      das::SideEffects::modifyArgument, "bind_dascript::deserialize_snapshot_quantized_info");
    das::addExtern<DAS_BIND_FUN(::send_transform_snapshots_event)>(*this, lib, "send_TransformSnapshots_event",
      das::SideEffects::modifyArgumentAndExternal, "::send_transform_snapshots_event");
    das::addExtern<DAS_BIND_FUN(::send_reliable_transform_snapshots_event)>(*this, lib, "send_TransformSnapshotReliable_event",
      das::SideEffects::modifyArgumentAndExternal, "::send_reliable_transform_snapshots_event");

    das::addUsing<::LocSnapshot>(*this, lib, "::LocSnapshot");

    compileBuiltinModule("dgNet.das", (unsigned char *)dng_net_das, sizeof(dng_net_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"net/reCreateEntity.h\"\n";
    tw << "#include \"dasModules/net.h\"\n";
    tw << "#include <dasModules/aotEcsEvents.h>\n";
    tw << "#include <daECS/net/component_replication_filter.h>\n";
    tw << "#include <dasModules/aotBitStream.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngNetModule, bind_dascript);
