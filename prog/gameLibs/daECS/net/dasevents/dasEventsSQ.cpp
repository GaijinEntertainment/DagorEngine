#include <daECS/net/dasEvents.h>
#include <quirrel/sqModules/sqModules.h>
#include <memory/dag_framemem.h>
#include <daECS/net/netbase.h>
#include <daECS/core/entityManager.h>

static eastl::optional<Tab<net::IConnection *>> get_recipients_optional_arg(HSQUIRRELVM vm, int argNum)
{
  if (sq_gettop(vm) < argNum || sq_gettype(vm, argNum) == OT_NULL)
    return eastl::nullopt;

  Sqrat::Var<Sqrat::Array> recipientsVar{vm, argNum};
  eastl::optional<Tab<net::IConnection *>> recipients{framemem_ptr()};

  recipients->resize(recipientsVar.value.GetSize());
  for (int i = 0; i < recipients->size(); i++)
  {
    int connid = recipientsVar.value.GetValue<int>(i);
    (*recipients)[i] = get_client_connection(connid);
  }

  return recipients;
}

static SQRESULT sq_send_net_event(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<int32_t, bind_dascript::DasEvent *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<int32_t> eid(vm, 2);
  Sqrat::Var<bind_dascript::DasEvent *> event(vm, 3);
  eastl::optional<Tab<net::IConnection *>> recipients = get_recipients_optional_arg(vm, 4);

  eastl::optional<dag::ConstSpan<net::IConnection *>> recipientsSlice = eastl::nullopt;
  if (recipients)
    recipientsSlice = make_span_const(*recipients);

  net::send_dasevent(g_entity_mgr, /*bcast*/ false, ecs::EntityId((ecs::entity_id_t)eid.value), event.value, nullptr,
    /*explicitConnections*/ recipientsSlice, []() { return eastl::string{}; });
  return 0;
}

static SQRESULT sq_broadcast_net_event(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<bind_dascript::DasEvent *>(vm, 2))
    return SQ_ERROR;

  Sqrat::Var<bind_dascript::DasEvent *> event(vm, 2);
  eastl::optional<Tab<net::IConnection *>> recipients = get_recipients_optional_arg(vm, 3);

  eastl::optional<dag::ConstSpan<net::IConnection *>> recipientsSlice = eastl::nullopt;
  if (recipients)
    recipientsSlice = make_span_const(*recipients);

  net::send_dasevent(g_entity_mgr, /*bcast*/ true, ecs::INVALID_ENTITY_ID, event.value, nullptr,
    /*explicitConnections*/ recipientsSlice, []() { return eastl::string{}; });
  return 0;
}

void net::register_dasevents(SqModules *module_mgr)
{
  Sqrat::Object *existingModule = module_mgr->findNativeModule("dasevents");
  auto tbl = existingModule ? Sqrat::Table(*existingModule) : Sqrat::Table(module_mgr->getVM());

  tbl.SquirrelFunc("sendNetEvent", sq_send_net_event, -3, ".ixa|o")
    .SquirrelFunc("broadcastNetEvent", sq_broadcast_net_event, -2, ".xa|o");

  if (!existingModule)
    module_mgr->addNativeModule("dasevents", tbl);
}
