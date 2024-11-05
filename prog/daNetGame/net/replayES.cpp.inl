// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "replay.h"
#include "net.h"
#include <daECS/core/entitySystem.h>
#include <daECS/net/replayEvents.h>
#include <daECS/net/connection.h>
#include <daECS/net/network.h>
#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>
#include <rapidJsonUtils/rapidJsonUtils.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstGen.h>
#include "phys/netPhys.h" // BasePhysActor::resizeSyncStates
#include "game/dasEvents.h"
#include "main/gameLoad.h"
#include "netPropsRegistry.h"
#include "protoVersion.h"
#include "time.h"
#include "netStat.h"
#include "userid.h"
#include "main/appProfile.h"
#include "main/app.h"
#include "netConsts.h"

#define MKSTEMP_SUFFIX         "XXXXXX"
#define REPLAY_TMP_PREFIX      "enl_replay_"
#define REPLAY_META_TMP_PREFIX "enl_replay_meta_"
#define REPLAY_META_JSON       "replaymeta.json"
#define REPLAY_CONN_ID         NET_MAX_PLAYERS

extern net::CNetwork &net_ctx_init_client(net::INetDriver *drv);
extern void net_ctx_set_recording_replay_filename(const char *path);
extern void set_time_internal(ITimeManager *tmgr);
extern net::CNetwork *get_net_internal();

namespace net
{
IConnection *get_replay_connection()
{
  IConnection *conn = get_client_connection(REPLAY_CONN_ID);
  return (conn && conn->isBlackHole()) ? conn : nullptr;
}
}; // namespace net

static rapidjson::Document replay_meta_info;

template <typename S>
static inline void trim_filename(S &path)
{
  for (char *b = (char *)path.c_str(), *c = b + path.length() - 1; c >= b; --c)
    if (*c == '/' || *c == '\\')
    {
      c[1] = '\0';
      path.force_size(c - b + 1);
      return;
    }
  path.clear(); // no slash found -> clear
}

void finalize_replay_record(const char *rpath)
{
  if (!strstr(rpath, REPLAY_TMP_PREFIX))
    return;
  if (dd_rename(rpath, app_profile::get().replay.record->c_str()))
  {
    if (!replay_meta_info.IsNull())
    {
      rapidjson::StringBuffer buf;
      {
#if _TARGET_PC_WIN // assume that win ded is used for dev only and write json prettified (for human readers)
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
#else
        rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
#endif
        replay_meta_info.Accept(writer); // written earlier by net_on_before_emgr_clear
      }
      eastl::fixed_string<char, 128, true, framemem_allocator> tmpmetapath = rpath;
      trim_filename(tmpmetapath);
      tmpmetapath += REPLAY_META_TMP_PREFIX MKSTEMP_SUFFIX;
      file_ptr_t fp = df_mkstemp((char *)tmpmetapath.c_str());
      if (fp)
      {
        bool ok = df_write(fp, buf.GetString(), buf.GetSize()) == buf.GetSize();
        df_close(fp);
        if (ok)
        {
          decltype(tmpmetapath) metapath = tmpmetapath;
          trim_filename(metapath);
          metapath += REPLAY_META_JSON;
          if (!dd_rename(tmpmetapath.c_str(), metapath.c_str()))
            logerr("Failed to rename tmp meta file '%s' -> '%s'", tmpmetapath.c_str(), metapath.c_str());
        }
        else
          logerr("Write %d bytes to '%s' failed", buf.GetSize(), tmpmetapath.c_str());
      }
      else
        logerr("Can't open '%s' for writing", tmpmetapath.c_str());
    }
  }
  else
    logerr("Failed to rename tmp replay file '%s' -> '%s'", rpath, app_profile::get().replay.record->c_str());
}

void gather_replay_meta_info()
{
  EventOnWriteReplayMetaInfo evt(rapidjson::Document{});
  g_entity_mgr->broadcastEventImmediate(evt);
  replay_meta_info = eastl::move(evt.get<0>());
}

Tab<uint8_t> gen_replay_meta_data()
{
  Tab<uint8_t> data(framemem_ptr());

  rapidjson::StringBuffer buf;
  {
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    replay_meta_info.Accept(writer); // written earlier by net_on_before_emgr_clear/gather_replay_meta_info
  }

  data.reserve(sizeof(net::BaseReplayFooterData) + buf.GetSize());
  data.resize(sizeof(net::BaseReplayFooterData));
  // To consider: do we need some encryption/scrambling json plaintext here?
  append_items(data, buf.GetSize(), (uint8_t *)buf.GetString());

  return data;
}

void clear_replay_meta_info() { replay_meta_info = {}; }

static void save_replay_key_frame(danet::BitStream &bs, int cur_time)
{
  TIME_PROFILE(save_replay_key_frame);
  if (!net::get_replay_connection())
  {
    DAG_FATAL("Can't save replay key frame without replay connection");
    return;
  }

  danet::BitStream tmpBs(framemem_ptr());
  ((net::Connection *)net::get_replay_connection())->writeReplayKeyFrame(bs, tmpBs);
  propsreg::serialize_net_registry(bs);
  rendinstdestr::serialize_destr_data(bs);
  g_entity_mgr->broadcastEvent(EventKeyFrameSaved(cur_time));
}

static void replay_restore_key_frame(const danet::BitStream &bs)
{
  TIME_PROFILE(restore_key_frame);
  if (!get_net_internal())
  {
    DAG_FATAL("Can't restore replay key frame without replay network");
    return;
  }

  if (!get_net_internal()->readReplayKeyFrame(bs))
    DAG_FATAL("Failed to restore replay key frame!");

  propsreg::deserialize_net_registry(bs);
  rendinst::clearRIGenDestrData();
  rendinstdestr::deserialize_destr_data(bs, 0);
}

void server_create_replay_record()
{
  eastl::optional<eastl::string> &recordOpt = app_profile::getRW().replay.record;
  const char *record = recordOpt ? recordOpt->c_str() : nullptr;
  if (!record)
    if (auto pModeInfo = jsonutils::get_ptr<rapidjson::Value::ConstObject>(app_profile::get().matchingInviteData, "mode_info"))
      if (jsonutils::get_or(*pModeInfo, "writeReplay", false))
        record = "";
  if (!record)
    return;
  eastl::fixed_string<char, 128, true, framemem_allocator> path;
  if (!*record) // auto gen replay path
  {
    path.sprintf("%sreplay.erpl", get_log_directory());
    recordOpt = eastl::string(path.begin(), path.end());
    record = recordOpt->c_str();
  }
  else
    path = record;
  trim_filename(path);
  path += REPLAY_TMP_PREFIX MKSTEMP_SUFFIX;
  net::Connection *conn = net::create_replay_connection(REPLAY_CONN_ID, (char *)path.c_str(), NET_PROTO_VERSION, &gen_replay_meta_data,
    net::scope_query_cb_t(), &save_replay_key_frame);
  if (conn)
  {
    auto netw = get_net_internal();
    G_ASSERT(netw && netw->isServer());
    netw->addConnection(conn, REPLAY_CONN_ID);
    BasePhysActor::resizeSyncStates(REPLAY_CONN_ID);
    net_ctx_set_recording_replay_filename(path.c_str());
    debug("Started replay record to '%s' ('%s')", path.c_str(), record);
  }
  else
  {
    logerr("Failed to start replay record to '%s' ('%s')", path.c_str(), record);
    recordOpt.reset(); // not recording anymore
  }
}

const rapidjson::Document &get_currently_playing_replay_meta_info() { return replay_meta_info; }

static bool load_replay_footer_info(dag::ConstSpan<uint8_t> rpl_footer_data, rapidjson::Document &rpl_meta_info)
{
  if (rpl_footer_data.empty())
    return false;
  auto footer = (const net::BaseReplayFooterData *)rpl_footer_data.data();
  const uint8_t *json_txt = nullptr;
  if (footer->magic == net::BaseReplayFooterData::MAGIC)
    json_txt = rpl_footer_data.data() + footer->footerSizeOf;
  if (size_t jlen = json_txt ? (rpl_footer_data.size() - (json_txt - rpl_footer_data.data())) : 0)
  {
    rapidjson::ParseResult res = rpl_meta_info.Parse((const char *)json_txt, jlen);
    if (!res)
      return false;
  }
  return true;
}

static uint16_t get_replay_proto_version()
{
  // we allow proto mismatch here by default so the game loads all of the replays
  // they should be shown in UI as invalid so they can be deleted from in game menus
  return dgs_get_settings()->getBlockByNameEx("replay")->getBool("allowProtoMismatch", true) ? 0 : NET_PROTO_VERSION;
}

bool load_replay_meta_info(const char *path, const eastl::function<void(const rapidjson::Document &)> &meta_cb)
{
  rapidjson::Document rplMetaInfo;
  bool json_loaded = false;
  return net::load_replay_footer_data(path, get_replay_proto_version(),
           [&](Tab<uint8_t> const &rpl_footer_data, const uint32_t replay_version) {
             json_loaded = load_replay_footer_info(rpl_footer_data, rplMetaInfo);
             if (json_loaded)
             {
               rplMetaInfo.AddMember("replay_version", replay_version, rplMetaInfo.GetAllocator());
               meta_cb(rplMetaInfo);
             }
           }) &&
         json_loaded;
}

void net_replay_rewind()
{
  const auto &replay = app_profile::get().replay;
  if (replay.startTime > 0 && get_net_internal() && !replay.playFile.empty())
    net::replay_rewind(get_net_internal()->getDriver(), replay.startTime * 1000);
}

bool try_create_replay_playback()
{
  app_profile::ProfileSettings &profile = app_profile::getRW();
  auto &play = profile.replay.playFile;
  if (play.empty())
    return false;

  Tab<uint8_t> replayFooterData;
  net::INetDriver *drv =
    net::create_replay_net_driver(play.c_str(), get_replay_proto_version(), &replayFooterData, replay_restore_key_frame);
  if (!drv)
  {
    logerr("Can't open replay file '%s'", play.c_str());
    play.clear();
    return false;
  }

  load_replay_footer_info(replayFooterData, replay_meta_info);

  auto &cnet = net_ctx_init_client(drv);
  cnet.addConnection(net::create_stub_connection(), 0);

  if (profile.replay.startTime > 0)
    set_timespeed(0);
  set_time_internal(create_replay_time(profile.replay.startTime));

  net::event::init_client();
  netstat::init();

  return true;
}

void replay_play(const char *path, float start_time, sceneload::UserGameModeContext &&ugmCtx, const char *scene)
{
  auto &profile = app_profile::getRW();
  profile.replay.playFile = path;
  profile.replay.startTime = start_time;
  net::clear_cached_ids();
  g_entity_mgr->broadcastEvent(EventGameSessionStarted());
  sceneload::switch_scene(scene, {}, eastl::move(ugmCtx));
}

static bool replay_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  // Disable warning Expression 'found > 0' is always false.
  // Why: because macro CONSOLE_CHECK_NAME_EX first check a found > 0, and then try execute cmd
  CONSOLE_CHECK_NAME_EX("replay", "play", 2, 3, "play given replay", "<replay_file> [replay_start_time]") //-V547
  {
    replay_play(argv[1], argc > 2 ? std::atof(argv[2]) : 0.f, {});
  }
  CONSOLE_CHECK_NAME_EX("replay", "key_frame", 1, 1, "save key frame to erpl", "")
  {
    if (!is_server())
      logerr("Must be executed on server");
    else
      g_entity_mgr->broadcastEvent(RequestSaveKeyFrame());
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(replay_console_handler);

ECS_ON_EVENT(RequestSaveKeyFrame)
static void replay_save_key_frame_es(const ecs::Event &)
{
  if (auto conn = net::get_replay_connection())
    net::replay_save_keyframe(conn, get_time_mgr().getAsyncMillis());
  else
    logerr("Tried to save a replay key frame but replay connection was null");
}
