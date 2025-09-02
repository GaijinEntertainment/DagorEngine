// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotNet.h>
#include <dasModules/dasDataBlock.h>
#include <daScript/daScript.h>
#include <daECS/net/dasEvents.h>
#include <dasModules/dasEvent.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasSystem.h>
#include <daECS/net/component_replication_filter.h>

static char net_das[] =
#include "net.das.inl"
  ;

DAS_BASE_BIND_ENUM(net::CompReplicationFilter, CompReplicationFilter, SkipUntilFilterUpdate, SkipForConnection, SkipNow,
  ReplicateUntilFilterUpdate, ReplicateForConnection, ReplicateNow);

namespace bind_dascript
{
struct ConnectionIdAnnotation : das::ManagedValueAnnotation<net::ConnectionId>
{
  ConnectionIdAnnotation(das::ModuleLibrary &ml) : ManagedValueAnnotation(ml, "ConnectionId", " ::net::ConnectionId") {}

  virtual void walk(das::DataWalker &walker, void *data) override
  {
    if (!walker.reading)
    {
      const net::ConnectionId *t = (net::ConnectionId *)data;
      int32_t v = t->value;
      walker.Int(v);
    }
  }
};

struct IConnectionAnnotation final : das::ManagedStructureAnnotation<net::IConnection, false>
{
  IConnectionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IConnection", ml)
  {
    cppName = " ::net::IConnection";
    addProperty<DAS_BIND_MANAGED_PROP(isBlackHole)>("isBlackHole", "isBlackHole");
  }
};

struct NetObjectAnnotation final : das::ManagedStructureAnnotation<net::Object, false>
{
  NetObjectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NetObject", ml)
  {
    cppName = " ::net::Object";
    addProperty<DAS_BIND_MANAGED_PROP(getCreationOrder)>("creationOrder", "getCreationOrder");
    addProperty<DAS_BIND_MANAGED_PROP(getEid)>("eid", "getEid");
  }
};

vec4f _builtin_send_net_blobevent(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  const ecs::EntityId eid = das::cast<ecs::EntityId>::to(args[0]);
  char *data = das::cast<char *>::to(args[1]);
  char *evtTypeName = das::cast<char *>::to(args[2]);
  bind_dascript::EsContext &ctx = cast_es_context(context);

  bind_dascript::DasEvent *evt = (bind_dascript::DasEvent *)data;
  net::send_dasevent(ctx.mgr, /*bcast*/ false, eid, evt, evtTypeName, /*explicitConnections*/ eastl::nullopt,
    [call]() { return call ? call->debugInfo.describe() : eastl::string{}; });

  return v_zero();
}

vec4f _builtin_send_net_blobevent_with_recipients(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  const ecs::EntityId eid = das::cast<ecs::EntityId>::to(args[0]);
  char *data = das::cast<char *>::to(args[1]);
  char *evtTypeName = das::cast<char *>::to(args[2]);
  const das::Array &recipients = das::cast<const das::Array &>::to(args[3]);

  dag::ConstSpan<net::IConnection *> recipientsSlice{
    reinterpret_cast<net::IConnection **>(recipients.data), static_cast<intptr_t>(recipients.size)};

  bind_dascript::EsContext &ctx = cast_es_context(context);

  bind_dascript::DasEvent *evt = (bind_dascript::DasEvent *)data;
  net::send_dasevent(ctx.mgr, /*bcast*/ false, eid, evt, evtTypeName, /*explicitConnections*/ recipientsSlice,
    [call]() { return call ? call->debugInfo.describe() : eastl::string{}; });

  return v_zero();
}

vec4f _builtin_broadcast_net_blobevent(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  char *data = das::cast<char *>::to(args[0]);
  char *evtTypeName = das::cast<char *>::to(args[1]);

  bind_dascript::DasEvent *evt = (bind_dascript::DasEvent *)data;
  bind_dascript::EsContext &ctx = cast_es_context(context);

  net::send_dasevent(ctx.mgr, /*bcast*/ true, ecs::INVALID_ENTITY_ID, evt, evtTypeName, /*explicitConnections*/ eastl::nullopt,
    [call]() { return call ? call->debugInfo.describe() : das::string{}; });

  return v_zero();
}

vec4f _builtin_broadcast_net_blobevent_with_recipients(das::Context &context, das::SimNode_CallBase *call, vec4f *args)
{
  char *data = das::cast<char *>::to(args[0]);
  char *evtTypeName = das::cast<char *>::to(args[1]);
  const das::Array &recipients = das::cast<const das::Array &>::to(args[2]);

  dag::ConstSpan<net::IConnection *> recipientsSlice{
    reinterpret_cast<net::IConnection **>(recipients.data), static_cast<intptr_t>(recipients.size)};

  bind_dascript::DasEvent *evt = (bind_dascript::DasEvent *)data;
  bind_dascript::EsContext &ctx = cast_es_context(context);

  net::send_dasevent(ctx.mgr, /*bcast*/ true, ecs::INVALID_ENTITY_ID, evt, evtTypeName, /*explicitConnections*/ recipientsSlice,
    [call]() { return call ? call->debugInfo.describe() : das::string{}; });

  return v_zero();
}


struct RegisterComponentFilterFunction
{
  uint64_t mangledNameHash = 0;
  das::Context *context = nullptr;
#if DAGOR_DBGLEVEL > 0
  das::string fileName;
#endif

  RegisterComponentFilterFunction() = default;
  explicit RegisterComponentFilterFunction(uint64_t mangled_name_hash) : mangledNameHash(mangled_name_hash) {}

  net::CompReplicationFilter operator()(ecs::EntityId eid, net::ConnectionId controlled_by, const net::IConnection *conn) const
  {
    if (!context)
      return net::CompReplicationFilter::ReplicateNow;

    const auto fn = context->fnByMangledName(mangledNameHash);
    if (!fn)
    {
      LOGERR_ONCE("Internal error: unable to find components filter function. Maybe compilation error.");
      return net::CompReplicationFilter::ReplicateNow;
    }

    vec4f args[] = {das::cast<ecs::EntityId>::from(eid), das::cast<net::ConnectionId>::from(controlled_by),
      das::cast<net::IConnection *>::from(conn)};
    vec4f res{};
    context->tryRestartAndLock();
    bind_dascript::RAIIStackwalkOnLogerr stackwalkOnLogerr(context);
    if (!context->ownStack)
    {
      das::SharedFramememStackGuard guard(*context);
      res = context->evalWithCatch(fn, args);
    }
    else
      res = context->evalWithCatch(fn, args);
    if (auto exp = context->getException())
      logerr("error: %s\n", exp);
    context->unlock();
    return net::CompReplicationFilter(das::cast<uint8_t>::to(res));
  }

  static void registerFilter(const eastl::string &name, RegisterComponentFilterFunction &filter)
  {
#if DAGOR_DBGLEVEL > 0
    debug("das: register_component_filter: add filter '%s' @%s", name.c_str(), filter.fileName.c_str());
#else
    debug("das: register_component_filter: add filter '%s'", name.c_str());
#endif
    net::register_component_filter(name.c_str(), filter);
  }
};

static das::mutex registryMutex;
static ska::flat_hash_map<eastl::string, RegisterComponentFilterFunction, ska::power_of_two_std_hash<eastl::string>> registry;

struct RegisterComponentFilterAnnotation : das::FunctionAnnotation
{
  RegisterComponentFilterAnnotation() : FunctionAnnotation("register_component_filter") {}

  bool apply(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    auto program = (*das::daScriptEnvironment::bound)->g_Program;
    if (program->thisModule->isModule)
    {
      err = "register_component_filter shouldn't be placed in the module. Please move the function to a file without module directive";
      return false;
    }
    fn->exports = true;
    return true;
  }

  bool finalize(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &argTypes,
    const das::AnnotationArgumentList &, eastl::string &err) override
  {
    if (fn->arguments.size() != 3)
    {
      err += "function requires 3 arguments";
      return false;
    }
    for (const auto &arg : fn->arguments)
    {
      if (!arg->type->isConst())
        err.append_sprintf("argument %s should be constant", arg->name.c_str());
      if (arg->init)
        err.append_sprintf("argument %s shouldn't be default initialized", arg->name.c_str());
    }
    const auto &arg0 = fn->arguments[0];
    if (arg0->type->baseType != das::Type::tHandle || arg0->type->annotation->module->name != "ecs" ||
        arg0->type->annotation->name != "EntityId")
      err.append_sprintf("expected type for first argument is ecs::EntityId, got %s", arg0->type->describe().c_str());
    const auto &arg1 = fn->arguments[1];
    if (arg1->type->baseType != das::Type::tInt)
      err.append_sprintf("expected type for second argument is int (aka net::ConnectionId), got %s", arg1->type->describe().c_str());
    const auto &arg2 = fn->arguments[2];
    if (arg2->type->baseType != das::Type::tHandle || arg2->type->annotation->module->name != "net" ||
        arg2->type->annotation->name != "IConnection")
      err.append_sprintf("expected type for third argument is net::IConnection, got %s", arg2->type->describe().c_str());
    if (!fn->at.fileInfo)
      err += "Internal error: function without file information";
    if (fn->result->baseType != das::Type::tEnumeration8 || fn->result->enumType->module->name != "net" ||
        fn->result->enumType->name != "CompReplicationFilter")
      err.append_sprintf("Invalid return type, expected net::CompReplicationFilter, got %s", fn->result->describe().c_str());

    if (!err.empty())
      return false;

    const das::AnnotationArgument *nameAnnotation = argTypes.find("name", das::Type::tString);
    const eastl::string filterName = nameAnnotation ? nameAnnotation->sValue : fn->name;
    if (filterName.empty())
    {
      err.append_sprintf("Filter name can't be empty", fn->result->describe().c_str());
      return false;
    }

    das::lock_guard<das::mutex> lock(registryMutex);
    const auto &it = registry.find_as(filterName.c_str());
    if (it == registry.end())
    {
      auto func = RegisterComponentFilterFunction(fn->getMangledNameHash());
#if DAGOR_DBGLEVEL > 0
      func.fileName = fn->at.fileInfo->name;
#endif
      registry.emplace(filterName, eastl::move(func));
    }
    else
    {
#if DAGOR_DBGLEVEL > 0
      if (it->second.fileName != fn->at.fileInfo->name)
        logerr("Component filter with same name '%s' is registered in '%s'. Current file '%s'", filterName,
          it->second.fileName.c_str(), fn->at.fileInfo->name);
#endif
      it->second.context = nullptr;
      it->second.mangledNameHash = fn->getMangledNameHash();
    }
    return true;
  }

  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, eastl::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    eastl::string &err) override
  {
    err = "not a block annotation";
    return false;
  }

  using das::FunctionAnnotation::simulate;

  bool simulate(das::Context *ctx, das::SimFunction *fn) override
  {
    das::lock_guard<das::mutex> lock(registryMutex);
    for (auto &pair : registry)
    {
      if (pair.second.mangledNameHash != fn->mangledNameHash)
        continue;
      pair.second.context = ctx;
      RegisterComponentFilterFunction::registerFilter(pair.first, pair.second);
      break;
    }
    return true;
  }
};

void das_init_component_replication_filters()
{
  das::lock_guard<das::mutex> lock(registryMutex);
  for (auto &pair : registry)
    RegisterComponentFilterFunction::registerFilter(pair.first, pair.second);
}

#define BIND_MEMBER_IMPL2(member, dasAliasStr, sideEffects, uniqueName) \
  using bind_member_##uniqueName = DAS_CALL_MEMBER(member);             \
  das::addExtern<DAS_CALL_METHOD(bind_member_##uniqueName)>(*this, lib, dasAliasStr, sideEffects, DAS_CALL_MEMBER_CPP(member));
#define BIND_MEMBER_IMPL(member, dasAliasStr, sideEffects, uniqueName) BIND_MEMBER_IMPL2(member, dasAliasStr, sideEffects, uniqueName)
#define BIND_MEMBER(member, dasAliasStr, sideEffects)                  BIND_MEMBER_IMPL(member, dasAliasStr, sideEffects, __LINE__)

class NetModule final : public das::Module
{
public:
  NetModule() : das::Module("net")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorDataBlock")); // for get_circuit_conf
    addEnumeration(das::make_smart<EnumerationCompReplicationFilter>());
    addAnnotation(das::make_smart<ConnectionIdAnnotation>(lib));
    addAnnotation(das::make_smart<IConnectionAnnotation>(lib));
    addAnnotation(das::make_smart<NetObjectAnnotation>(lib));
    addAnnotation(das::make_smart<RegisterComponentFilterAnnotation>());
    das::addExtern<DAS_BIND_FUN(server_send_schemeless_event)>(*this, lib, "server_send_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::server_send_schemeless_event");
    das::addExtern<DAS_BIND_FUN(server_send_schemeless_event_to)>(*this, lib, "server_send_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::server_send_schemeless_event_to");
    das::addExtern<DAS_BIND_FUN(server_broadcast_schemeless_event)>(*this, lib, "server_broadcast_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::server_broadcast_schemeless_event");
    das::addExtern<DAS_BIND_FUN(server_broadcast_schemeless_event_to)>(*this, lib, "server_broadcast_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::server_broadcast_schemeless_event");

    das::addExtern<DAS_BIND_FUN(client_send_schemeless_event)>(*this, lib, "client_send_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::client_send_schemeless_event");
    das::addExtern<DAS_BIND_FUN(client_broadcast_schemeless_event)>(*this, lib, "client_broadcast_schemeless_event",
      das::SideEffects::modifyExternal, "bind_dascript::client_broadcast_schemeless_event");

    das::addInterop<_builtin_send_net_blobevent, void, ecs::EntityId, vec4f, char *>(*this, lib, "_builtin_send_net_blobevent",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::_builtin_send_net_blobevent");

    das::addInterop<_builtin_send_net_blobevent_with_recipients, void, ecs::EntityId, vec4f, char *, const das::Array &>(*this, lib,
      "_builtin_send_net_blobevent_with_recipients", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::_builtin_send_net_blobevent_with_recipients");

    das::addInterop<_builtin_broadcast_net_blobevent, void, vec4f, char *>(*this, lib, "_builtin_broadcast_net_blobevent",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::_builtin_broadcast_net_blobevent");

    das::addInterop<_builtin_broadcast_net_blobevent_with_recipients, void, vec4f, char *, const das::Array &>(*this, lib,
      "_builtin_broadcast_net_blobevent_with_recipients", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::_builtin_broadcast_net_blobevent_with_recipients");

    das::addExtern<DAS_BIND_FUN(get_client_connections)>(*this, lib, "get_client_connections", das::SideEffects::accessExternal,
      "bind_dascript::get_client_connections");

    das::addExtern<DAS_BIND_FUN(net_object_setControlledBy)>(*this, lib, "net_object_setControlledBy",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::net_object_setControlledBy");
    das::addExtern<DAS_BIND_FUN(net_object_getControlledBy)>(*this, lib, "net_object_getControlledBy", das::SideEffects::none,
      "bind_dascript::net_object_getControlledBy");
    das::addExtern<DAS_BIND_FUN(connection_getId)>(*this, lib, "connection_getId", das::SideEffects::none,
      "bind_dascript::connection_getId");
    das::addExtern<DAS_BIND_FUN(connection_setUserEid)>(*this, lib, "connection_setUserEid", das::SideEffects::modifyArgument,
      "bind_dascript::connection_setUserEid");
    das::addExtern<DAS_BIND_FUN(setObjectInScopeAlways)>(*this, lib, "setObjectInScopeAlways", das::SideEffects::modifyArgument,
      "bind_dascript::setObjectInScopeAlways");
    das::addExtern<DAS_BIND_FUN(setEntityInScopeAlways)>(*this, lib, "setEntityInScopeAlways", das::SideEffects::modifyArgument,
      "bind_dascript::setEntityInScopeAlways");
    das::addExtern<DAS_BIND_FUN(addObjectInScope)>(*this, lib, "addObjectInScope", das::SideEffects::modifyArgument,
      "bind_dascript::addObjectInScope");
    das::addExtern<DAS_BIND_FUN(clearObjectInScopeAlways)>(*this, lib, "clearObjectInScopeAlways", das::SideEffects::modifyArgument,
      "bind_dascript::clearObjectInScopeAlways");
    das::addExtern<DAS_BIND_FUN(clearObjectInScope)>(*this, lib, "clearObjectInScope", das::SideEffects::modifyArgument,
      "bind_dascript::clearObjectInScope");

    das::addExternTempRef<DAS_BIND_FUN(get_circuit_conf)>(*this, lib, "get_circuit_conf", das::SideEffects::accessExternal,
      "bind_dascript::get_circuit_conf");
    das::addExtern<DAS_BIND_FUN(net::get_session_id)>(*this, lib, "get_session_id", das::SideEffects::modifyExternal,
      "net::get_session_id");
    das::addExtern<DAS_BIND_FUN(net::get_replay_connection)>(*this, lib, "get_replay_connection", das::SideEffects::accessExternal,
      "net::get_replay_connection");
    das::addExtern<DAS_BIND_FUN(is_server)>(*this, lib, "is_server", das::SideEffects::accessExternal, "::is_server");

    BIND_MEMBER(net::IConnection::isEntityInScope, "connection_isEntityInScope", das::SideEffects::none)
    BIND_MEMBER(net::IConnection::setUserPtr, "connection_setUserPtr", das::SideEffects::modifyArgument)
    BIND_MEMBER(net::IConnection::getUserPtr, "connection_getUserPtr", das::SideEffects::none)

    das::addConstant(*this, "replicate_everywhere_filter_id", net::replicate_everywhere_filter_id);
    das::addConstant(*this, "invalid_filter_id", net::invalid_filter_id);

    das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) {
      das::lock_guard<das::mutex> lock(registryMutex);
      for (auto &pair : registry)
        if (pair.second.context == ctx)
          pair.second.context = nullptr;
    });

    compileBuiltinModule("net.das", (unsigned char *)net_das, sizeof(net_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotNet.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(NetModule, bind_dascript);
