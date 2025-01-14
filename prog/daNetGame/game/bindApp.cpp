// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <squirrel.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/autoBind.h>
#include <quirrel_json/rapidjson.h>
#include <quirrel_json/jsoncpp.h>
#include <sqstdblob.h>
#include <util/dag_delayedAction.h>
#include <startup/dag_globalSettings.h>
#include <dataBlockUtils/blk2sqrat/blk2sqrat.h>
#include <memory/dag_framemem.h>
#include "main/appProfile.h"
#include "main/main.h"
#include "ui/overlay.h"
#include "game/gameLauncher.h"
#include "main/localStorage.h"
#include "main/app.h"
#include "net/replay.h" // load_replay_meta_info
#include "ui/userUi.h"
#include <main/circuit.h>
#include "main/gameLoad.h"
#include "net/net.h" // net::ConnectParams
#include "net/userid.h"
#include "main/version.h"
#include <daEditorE/editorCommon/inGameEditor.h>
#include <crypto/base64.h>

int app_profile_get_app_id() { return app_profile::get().appId; }

const char *app_profile_get_session_id() { return app_profile::get().serverSessionId.c_str(); }

static void script_exit_game() { exit_game("script"); }

static eastl::string encode_uri_component(const char *text)
{
  eastl::string ret;
  if (!text || !text[0])
    return ret;

  const int tlen = ::strlen(text);
  ret.reserve(tlen * 3);

  // implements rfc-3986
  for (int idx = 0; idx < tlen; ++idx)
  {
    const unsigned char c = text[idx];
    if (::isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~'))
      ret += c;
    else if (c == ' ')
      ret += '+';
    else
      ret.append_sprintf("%%%02x", c & 0xFF);
  }
  return ret;
}

template <typename FuncStore, typename FuncLoad>
void bind_local_storage(HSQUIRRELVM vm, Sqrat::Table &ns, FuncStore store, FuncLoad load)
{
  ns //
    .Func("set_value", [store](const char *key, Sqrat::Object value) { store(key, quirrel_to_jsoncpp(value)); })
    .Func("get_value", [vm, load](const char *key) { return jsoncpp_to_quirrel(vm, load(key)); })
    /**/;
}


static SQInteger switch_scene_sq(HSQUIRRELVM vm)
{
  SQInteger nArgs = sq_gettop(vm);
  const SQChar *sceneName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &sceneName)));
  eastl::vector<eastl::string> imports;
  if (nArgs > 2 && sq_gettype(vm, 3) != OT_NULL)
  {
    Sqrat::Var<Sqrat::Array> sqImports(vm, 3);
    int len = (int)sqImports.value.Length();
    imports.reserve(len);
    for (int i = 0; i < len; ++i)
    {
      Sqrat::Object sqImport = sqImports.value.GetSlot((SQInteger)i);
      if (DAGOR_LIKELY(sqImport.GetType() == OT_STRING))
        imports.push_back(sqImport.GetVar<const char *>().value);
      else
        return sq_throwerror(vm, "<imports> should be array of strings");
    }
  }
  sceneload::UserGameModeContext ugmCtx{};
  if (nArgs > 3 && sq_gettype(vm, 4) != OT_NULL)
  {
    Sqrat::Var<Sqrat::Table> sqModInfo(vm, 4);
    const char *errorMessage = gamelauncher::fill_ugm_context_from_table(sqModInfo.value, ugmCtx);
    if (errorMessage != nullptr)
      return sq_throwerror(vm, errorMessage);
  }
  debug("SQ switch_scene(%s)", sceneName);
  sceneload::switch_scene(sceneName, eastl::move(imports), eastl::move(ugmCtx));
  return 0;
}

static SQInteger switch_scene_and_update_sq(HSQUIRRELVM vm)
{
  const SQChar *sceneName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &sceneName)));
  debug("SQ switch_scene_and_update(%s)", sceneName);
  sceneload::switch_scene_and_update(sceneName);
  return 0;
}

static SQInteger connect_to_session_sq(HSQUIRRELVM vm)
{
  SQInteger nArgs = sq_gettop(vm);
  Sqrat::Var<Sqrat::Table> params_var(vm, 2);
  const Sqrat::Table &params = params_var.value;

  sceneload::UserGameModeContext ugmCtx{};
  if (nArgs > 2 && sq_gettype(vm, 3) != OT_NULL)
  {
    Sqrat::Var<Sqrat::Table> sqModInfo(vm, 3);
    const char *errorMessage = gamelauncher::fill_ugm_context_from_table(sqModInfo.value, ugmCtx);
    if (errorMessage != nullptr)
      return sq_throwerror(vm, errorMessage);
  }

  Sqrat::Array host_urls = params["host_urls"];
  int hlen = (host_urls.GetType() == OT_ARRAY) ? host_urls.Length() : 0;
  if (!hlen)
    return sq_throwerror(vm, "no .host_urls passed in <params> to connect_to_session()");

  net::ConnectParams connectParams;
  for (int i = 0; i < hlen; ++i)
    connectParams.serverUrls.push_back(host_urls[SQInteger(i)].GetVar<const char *>().value);

  Sqrat::Object sessIdObj = params.GetSlot("sessionId");
  if (sessIdObj.GetType() == OT_STRING)
    app_profile::getRW().serverSessionId = connectParams.sessionId = sessIdObj.Cast<eastl::string>();
  else if (sessIdObj.GetType() == OT_INTEGER)
    app_profile::getRW().serverSessionId = connectParams.sessionId = eastl::to_string(sessIdObj.Cast<uint64_t>());
  else if (sessIdObj.GetType() == OT_NULL)
    app_profile::getRW().serverSessionId = connectParams.sessionId = eastl::to_string(matching::INVALID_SESSION_ID);
  else
    G_ASSERTF(0, "unsupported sessIdObj.GetType()==%d", sessIdObj.GetType());

  if (Sqrat::Object authKeyObj = params.GetSlot("authKey"); authKeyObj.GetType() == OT_STRING)
  {
    connectParams.authKey = base64::decode<dag::Vector<uint8_t>>(authKeyObj.GetVar<eastl::string_view>().value);
    if (net::auth_key_t pltAuthKey = net::get_platform_auth_key(); !pltAuthKey.empty())
      for (unsigned i = 0; i < connectParams.authKey.size(); ++i) // To consider: use 16-byte aligned keys and v_xor
        connectParams.authKey[i] ^= pltAuthKey[i % pltAuthKey.size()];
  }

  if (Sqrat::Object encKeyObj = params.GetSlot("encKey"); encKeyObj.GetType() == OT_STRING)
    connectParams.encryptKey = base64::decode<dag::Vector<uint8_t>>(encKeyObj.GetString());

  sceneload::connect_to_session(eastl::move(connectParams), eastl::move(ugmCtx));
  return 0;
}

static void reload_ui_scripts(bool hard_reload)
{
  bool hadOverlayScene = overlay_ui::gui_scene() != nullptr;
  if (hadOverlayScene && hard_reload)
    overlay_ui::shutdown_ui(false);

  // USER UI VM
  user_ui::reload_user_ui_script(hard_reload);

  if (hadOverlayScene && hard_reload)
    overlay_ui::init_ui();
}

static void reload_overlay_ui_scripts(bool hard_reload) { overlay_ui::reload_scripts(hard_reload); }

static void sq_reload_ui_scripts()
{
  ::delayed_call([]() { reload_ui_scripts(false); });
}
static void sq_reload_overlay_ui_scripts()
{
  ::delayed_call([]() { reload_overlay_ui_scripts(false); });
}

static SQInteger get_circuit_conf(HSQUIRRELVM vm)
{
  sq_pushobject(vm, blk_to_sqrat(vm, *circuit::get_conf()));
  return 1;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_app, "app", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);

  tbl //
    .Func("get_game_name", get_game_name)
    .Func("get_current_scene", []() { return sceneload::get_current_game().sceneName.c_str(); })
    .Func("get_app_id", app_profile_get_app_id)
    .Func("get_matching_invite_data", [vm]() { return rapidjson_to_quirrel(vm, app_profile::get().matchingInviteData); })
    // client only feature for apply matching info in offline mode
    .Func("set_matching_invite_data",
      [vm](const Sqrat::Object &obj) {
        Sqrat::Object ret(vm);
        app_profile::getRW().matchingInviteData = quirrel_to_rapidjson(obj);
        return ret;
      })
    .Func("load_replay_meta_info",
      [vm](const char *path) {
        Sqrat::Object ret(vm);
        ::load_replay_meta_info(path, [&](const rapidjson::Document &jmeta) { ret = rapidjson_to_quirrel(vm, jmeta); });
        return ret;
      })
    .Func("replay_play",
      [](const char *path, float start_time, const Sqrat::Table &mod_info, const char *scene) {
        sceneload::UserGameModeContext ugmCtx{};
        if (mod_info.GetType() != OT_NULL)
          if (const char *errorMessage = gamelauncher::fill_ugm_context_from_table(mod_info, ugmCtx))
          {
            logerr("Failed to start replace '%s'", errorMessage);
            return;
          }
        replay_play(path, start_time, eastl::move(ugmCtx), scene);
      })
    .Func("replay_get_play_file", []() { return app_profile::get().replay.playFile.c_str(); })
    .Func("exit_game", script_exit_game)
    .Func("is_app_terminated", dng_is_app_terminated)
    .Func("is_user_game_mod", sceneload::is_user_game_mod)
    .SquirrelFunc("get_circuit_conf", get_circuit_conf, 1)
    .SetValue("circuit_name", circuit::get_name().data())
    .Func("get_circuit", []() { return circuit::get_name().data(); })
    .Func("get_build_number", get_build_number)
    .Func("get_exe_version", get_exe_version_str)
    .Func("get_session_id", []() { return app_profile_get_session_id(); })
    .Func("set_fps_limit", set_fps_limit)
    .Func("set_timespeed", set_timespeed)
    .Func("get_timespeed", get_timespeed)
    .Func("encode_uri_component", encode_uri_component)
    .SquirrelFunc("switch_scene", switch_scene_sq, -2, ".s a|o t|o")              // (scene_name:string, [imports:array<string>|null],
                                                                                  //   [ugmCtx:table|null])
    .SquirrelFunc("switch_scene_and_update", switch_scene_and_update_sq, 2, ".s") // (scene_name:string)
    .SquirrelFunc("connect_to_session", connect_to_session_sq, -2, ".t t|o")      // (conn_params:table, [ugmCtx:table|null])
    .Func("get_dir", get_dir)
    .Func("reload_ui_scripts", sq_reload_ui_scripts)
    .Func("reload_overlay_ui_scripts", sq_reload_overlay_ui_scripts)
    /**/;

  gamelauncher::bind_game_launcher(tbl);

  Sqrat::Table localStorage(vm);
  tbl.Bind("local_storage", localStorage);
  Sqrat::Table lsProfile(vm), lsHidden(vm);
  bind_local_storage(vm, lsHidden, local_storage::hidden::store_json, local_storage::hidden::load_json);
  localStorage.Bind("profile", lsProfile);
  localStorage.Bind("hidden", lsHidden);
  return tbl;
}
