// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <EASTL/string.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/event.h>
#include <sqrat.h>
#include <sqstdblob.h>
#include <crypto/base64.h>
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "game/gameLauncher.h"
#include "main/appProfile.h"
#include "main/gameLoad.h"
#include "net/net.h" // net::ConnectParams
#include "net/userid.h"

namespace gamelauncher
{

static SQRESULT fill_ugm_vroms(const Sqrat::Array &sq_mods_vroms, eastl::vector<eastl::string> &mods_vroms)
{
  if (sq_mods_vroms.GetType() == OT_NULL)
    return SQ_OK;

  if (sq_mods_vroms.GetType() != OT_ARRAY)
    return SQ_ERROR;

  const int hlen = sq_mods_vroms.Length();
  mods_vroms.resize(hlen);
  for (int i = 0; i < hlen; ++i)
  {
    if (sq_mods_vroms[SQInteger(i)].GetType() != OT_STRING)
    {
      mods_vroms.clear();
      return SQ_ERROR;
    }
    mods_vroms[i] = sq_mods_vroms[SQInteger(i)].GetVar<const char *>().value;
  }
  return SQ_OK;
}

const char *fill_ugm_context_from_table(const Sqrat::Table &params, sceneload::UserGameModeContext &ugm_context)
{
  ugm_context.modId = params.GetSlotValue<eastl::string>("modId", "");
  const Sqrat::Array sqModsVroms = params.GetSlot("modsVroms");
  if (SQ_FAILED(fill_ugm_vroms(params.GetSlot("modsVroms"), ugm_context.modsVroms)))
    return "<modsVroms> should be array of string";
  if (SQ_FAILED(fill_ugm_vroms(params.GetSlot("modsPackVroms"), ugm_context.modsPackVroms)))
    return "<modsPackVroms> should be array of string";
  if (SQ_FAILED(fill_ugm_vroms(params.GetSlot("modsAddons"), ugm_context.modsAddons)))
    return "<modsAddons> should be array of string";
  return nullptr;
}

static SQRESULT launch_session(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Table> params_var(vm, 2);
  const Sqrat::Table &params = params_var.value;

  sceneload::UserGameModeContext ugmCtx{};
  const char *errorMessage = fill_ugm_context_from_table(params, ugmCtx);
  if (errorMessage != nullptr)
    return sq_throwerror(vm, errorMessage);

  Sqrat::Array host_urls = params["host_urls"];
  if (int hlen = (host_urls.GetType() == OT_ARRAY) ? host_urls.Length() : 0)
  {
    net::ConnectParams connectParams;
    for (int i = 0; i < hlen; ++i)
      connectParams.serverUrls.push_back(host_urls[SQInteger(i)].GetVar<const char *>().value);

    Sqrat::Object sessIdObj = params.GetSlot("sessionId");
    G_ASSERT(sessIdObj.GetType() == OT_INTEGER || sessIdObj.GetType() == OT_NULL || sessIdObj.GetType() == OT_STRING);
    if (sessIdObj.GetType() == OT_STRING)
      connectParams.sessionId = sessIdObj.Cast<eastl::string>();
    else
      connectParams.sessionId = eastl::to_string(sessIdObj.IsNull() ? matching::INVALID_SESSION_ID : sessIdObj.Cast<uint64_t>());
    app_profile::getRW().serverSessionId = connectParams.sessionId;

    if (Sqrat::Object authKeyObj = params.GetSlot("authKey"); authKeyObj.GetType() == OT_STRING)
      connectParams.authKey = base64::decode<dag::Vector<uint8_t>>(authKeyObj.GetVar<eastl::string_view>().value);
    else if (!app_profile::get().cmdAuthKey.empty())
      connectParams.authKey = base64::decode<dag::Vector<uint8_t>>(app_profile::get().cmdAuthKey);
    if (net::auth_key_t pltAuthKey = net::get_platform_auth_key(); !pltAuthKey.empty())
      for (unsigned i = 0; i < connectParams.authKey.size(); ++i) // To consider: use 16-byte aligned keys and v_xor
        connectParams.authKey[i] ^= pltAuthKey[i % pltAuthKey.size()];

    if (Sqrat::Object encKeyObj = params.GetSlot("encKey"); encKeyObj.GetType() == OT_STRING)
      connectParams.encryptKey = base64::decode<dag::Vector<uint8_t>>(encKeyObj.GetString());

    net::clear_cached_ids();
    g_entity_mgr->broadcastEvent(EventGameSessionStarted());
    sceneload::connect_to_session(eastl::move(connectParams), eastl::move(ugmCtx));
    return 0;
  }

  // Note: scene is still required for offline sessions
  Sqrat::Object sceneObj = params.GetSlot("scene");
  auto scene = (sceneObj.GetType() == OT_STRING) ? sceneObj.GetVar<eastl::string_view>().value : eastl::string_view{};

  net::clear_cached_ids();
  g_entity_mgr->broadcastEvent(EventGameSessionStarted());
  sceneload::switch_scene(scene, {}, eastl::move(ugmCtx));
  return 0;
}

void game_launcher_es_event_handler(const EventUserLoggedOut &) { g_entity_mgr->broadcastEvent(EventGameSessionFinished(false)); }


void bind_game_launcher(Sqrat::Table &ns) { ns.SquirrelFunc("launch_network_session", launch_session, 2, ".t"); }


} // namespace gamelauncher


static bool console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;

  int found = 0;
  CONSOLE_CHECK_NAME("game", "connect", 1, 3)
  {
    const char *serverUrl = argc > 1 ? argv[1] : "127.0.0.1";
    net::clear_cached_ids();
    g_entity_mgr->broadcastEvent(EventGameSessionStarted());
    net::ConnectParams cp;
    cp.serverUrls.push_back(serverUrl);
    sceneload::connect_to_session(eastl::move(cp));
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(console_handler);
