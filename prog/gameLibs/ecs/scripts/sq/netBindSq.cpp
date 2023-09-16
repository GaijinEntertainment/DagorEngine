#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel/sqModules/sqModules.h>
#include <sqrat.h>
#include <ecs/scripts/sqEntity.h>
#include <ecs/scripts/sqBindEvent.h>
#include <daECS/net/netbase.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/replay.h>

namespace ecs
{

namespace sq
{

void bind_net(SqModules *module_mgr)
{
  ///@module net
  Sqrat::Object *existingModule = module_mgr->findNativeModule("net");
  auto tbl = existingModule ? Sqrat::Table(*existingModule) : Sqrat::Table(module_mgr->getVM());

  tbl // comments to supress clang-format and allow qdox to generate doc
    .Func("is_server", is_server)
    .Func("has_network", has_network)
    .Func("get_replay_proto_version", net::get_replay_proto_version)
    .SetValue("INVALID_CONNECTION_ID", net::INVALID_CONNECTION_ID_VALUE);
#define DC(x) tbl.SetValue(#x, x);
  DANET_DEFINE_DISCONNECTION_CAUSES
#undef DC

  ecs::sq::EventsBind<
#define NET_ECS_EVENT(e, ...) e,
    NET_ECS_EVENTS
#undef NET_ECS_EVENT
      ecs::sq::term_event_t>::bindall(tbl);

  if (!existingModule)
    module_mgr->addNativeModule("net", tbl);
}

} // namespace sq

} // namespace ecs
