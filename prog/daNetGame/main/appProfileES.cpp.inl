// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <net/dedicated.h>
#include "appProfile.h"
#include "game/gameEvents.h"
#include "settings.h"
#include "circuit.h"
#include <dagCrypto/base64.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/error/en.h>

void replay_reset() { app_profile::getRW().replay.playFile.clear(); }

namespace app_profile
{

static ProfileSettings profile;

static void load_argv(rapidjson::Document &matchingInviteData) // load on first use
{
  profile.replay.playFile = dgs_get_argv("play", "");
  profile.replay.startTime = std::atof(dgs_get_argv("playStartTime", "0.0"));
  if (const char *record = dgs_get_argv("record"))
    profile.replay.record = eastl::string(record);

  profile.serverSessionId = dgs_get_argv("session_id", "");
  profile.cmdAuthKey = dgs_get_argv("auth_key", "");
  profile.userId = dgs_get_argv("user_id", "");
  profile.userName = dgs_get_argv("user_name", "{Local Player}");

  if (const char *inv_data = (dedicated::is_dedicated() || DAGOR_DBGLEVEL > 0) ? dgs_get_argv("invite_data") : nullptr) //-V560
  {
    auto jsonStr = base64::decode(inv_data);
    rapidjson::ParseResult res = matchingInviteData.Parse(jsonStr.data(), jsonStr.size());
    // Failure of parsing this json can't be meaningfully handled (hence assertion)
    G_VERIFYF(res, "Failed to parse invite data: %s", rapidjson::GetParseError_En(res.Code()));
  }

  profile.haveGraphicsWindow = dedicated::is_dedicated() == false;
  profile.devMode = dgs_get_argv("devMode");
  profile.inited = true;
}

void apply_settings_blk(const DataBlock &blk)
{
  const char *newProjectId = get_platformed_value(blk, "project_id", "");
  if (circuit::is_submission_env())
    newProjectId = get_platformed_value(blk, "submission_project_id", newProjectId);
  else if (circuit::is_staging_env())
    newProjectId = get_platformed_value(blk, "staging_project_id", newProjectId);
  G_ASSERT_RETURN(newProjectId != nullptr, );

  int newAppId = get_platformed_value(blk, "app_id", 0);
  if (circuit::is_submission_env())
    newAppId = get_platformed_value(blk, "submission_app_id", newAppId);
  else if (circuit::is_staging_env())
    newAppId = get_platformed_value(blk, "staging_app_id", newAppId);

  if (newAppId != profile.appId)
  {
    debug("apply settings from blk: using appid %d", newAppId);
    profile.appId = newAppId;
  }

  if (newProjectId != profile.projectId)
  {
    debug("apply settings from blk: using projectId %s", newProjectId);
    profile.projectId = newProjectId;
  }

  profile.disableRemoteNetServices = dgs_get_settings()->getBool("disableRemoteNetServices", false);
}

ProfileSettings &getRW()
{
  if (!profile.inited)
    load_argv(profile.matchingInviteData);
  return profile;
}

ProfileSettings const &get() { return getRW(); }

static void app_profile_es_event_handler(const EventUserLoggedIn &evt)
{
  getRW().userId = eastl::to_string(evt.get<0>());
  getRW().userName = evt.get<1>();
}

static void app_profile_es_event_handler(const EventUserLoggedOut &) { getRW().userId = ""; }

} // namespace app_profile
