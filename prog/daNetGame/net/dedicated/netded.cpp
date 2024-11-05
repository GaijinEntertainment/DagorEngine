// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include "netded.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "main/appProfile.h"
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <daECS/net/netbase.h>
#include "net/netConsts.h"
#include <util/dag_string.h>
#include <util/dag_watchdog.h>
#include <crypto/base64.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include <ioSys/dag_dataBlock.h>
#include <daECS/net/network.h>
#include <syncVroms/syncVroms.h>
#include "net/dedicated/matching.h"
#include "net/netMsg.h"
#include <daECS/net/msgSink.h>
#include "main/version.h"
#include "main/circuit.h"
#include "net/netPrivate.h"
#include "net/protoVersion.h"
#include "net/net.h"
#include "net/userid.h"
#include <daECS/net/netEvents.h>
#include "phys/physUtils.h" // phys_get_tickrate
#include "net/dedicated.h"
#include "net/time.h"
#include "net/netEvents.h"
#include <statsd/statsd.h>
#include <game/player.h>
#include <contentUpdater/version.h>
#include "main/gameLoad.h"
#include "matching_state_data.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define EVP_MD_CTX_new  EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

ECS_REGISTER_EVENT(OnNetDedicatedCreated)
ECS_REGISTER_EVENT(OnNetDedicatedDestroyed)
ECS_REGISTER_EVENT(OnNetDedicatedPrepareServerInfo)
ECS_REGISTER_EVENT(OnNetDedicatedServerDisconnect)

#define DED_SERVER_ECS_EVENT ECS_REGISTER_EVENT
DED_SERVER_ECS_EVENTS
#undef DED_SERVER_ECS_EVENT

extern net::INetworkObserver *get_net_observer();
namespace dedicated
{

static uint16_t binded_net_port = 0;
uint16_t get_binded_port() { return binded_net_port; }

bool is_dedicated() { return true; }

const char *get_host_url(eastl::string &str, int &it)
{
  const char *listen = dgs_get_argv("listen", it);
  if (!listen)
    return nullptr;
  if (const char *pcol = strchr(listen, ':')) // port already specified
    return listen;
  // append binded port
  int bport = get_binded_port();
  G_FAST_ASSERT(bport != 0);
  str = listen;
  str.append_sprintf(":%d", bport);
  return str.c_str();
}

static inline uint16_t try_parse_port(const char *str) // return 0 on error
{
  uint16_t ret = 0;
  if (const char *pcol = strrchr(str, ':'))
  {
    char *pend = NULL;
    long port = strtol(pcol + 1, &pend, 0);
    if (pcol[1] && !*pend && port == uint16_t(port))
      ret = (uint16_t)port;
  }
  return ret;
}

net::INetDriver *create_net_driver()
{
  const char *listen = dgs_get_argv("listen");
  net::INetDriver *drv = nullptr;
  if (const char *udp_sock = dgs_get_argv("udp_sock")) // if udp_sock is passed than port should be passed as well
  {
    binded_net_port = listen ? try_parse_port(listen) : 0;
    if (!binded_net_port)
    {
      DAG_FATAL("failed to parse bind port from listen url '%s'", listen);
      return nullptr;
    }
    char *eptr = NULL;
    long sockfp = strtol(udp_sock, &eptr, 0);
    if (*eptr == '\0' && sockfp > 0)
    {
      SocketDescriptor sdss;
      sdss.type = SocketDescriptor::SOCKET;
      sdss.socket = sockfp;
      drv = net::create_net_driver_listen(sdss, NET_MAX_PLAYERS);
    }
    else
      DAG_FATAL("failed to parse udp sock '%s'", udp_sock);
  }
  else
    drv = net::create_net_driver_listen(listen ? listen : "", NET_MAX_PLAYERS, &binded_net_port);
  if (!drv)
  {
    DAG_FATAL("failed to create dedicated net driver on '%s', port %d is not available?", listen, binded_net_port);
    return nullptr;
  }
  debug("Inited dedicated net driver on port %d", binded_net_port);
  return drv;
}

enum class VerifyAuthResult
{
  Ok,
  EmptyRoomSecret,
  EmptyPeerKey,
  GenAuthKeyFailed,
  InvalidPeerKeyLength,
  PeerKeyAuthFailed,
  UnathorizedPlatform
};
static VerifyAuthResult verify_peer_auth(
  const eastl::string &roomSecret, eastl::string_view uid_str, Tab<uint8_t> &peer_key, const char *platf)
{
  if (peer_key.empty())
    return VerifyAuthResult::EmptyPeerKey;
  net::auth_key_t *platform_key = nullptr;
  if (!net::get_auth_key_for_platform(platf, platform_key))
    return VerifyAuthResult::UnathorizedPlatform;
  else if (platform_key && !platform_key->empty())
    for (int i = 0; i < peer_key.size(); ++i)
      peer_key[i] ^= (*platform_key)[i % platform_key->size()];
  if (roomSecret.empty())
    return VerifyAuthResult::EmptyRoomSecret;

  unsigned char hmacBuf[EVP_MAX_MD_SIZE];
  unsigned int hmacLen = EVP_MAX_MD_SIZE;

  HMAC(EVP_sha256(), (const void *)roomSecret.c_str(), roomSecret.size(), (const unsigned char *)uid_str.data(), uid_str.size(),
    hmacBuf, &hmacLen);
  if (hmacLen != peer_key.size())
    return VerifyAuthResult::InvalidPeerKeyLength;
  return (memcmp(peer_key.data(), hmacBuf, hmacLen) == 0) ? VerifyAuthResult::Ok : VerifyAuthResult::PeerKeyAuthFailed;
}

using EventOnClientConnectedPtr = eastl::unique_ptr<EventOnClientConnected>;
using EventOnClientConnectedMap = eastl::vector_map<net::ConnectionId, EventOnClientConnectedPtr>;
static EventOnClientConnectedMap delayed_connection_event_map;

static void send_event_after_sync_vroms_done(net::ConnectionId conn_id, EventOnClientConnected &&evt)
{
  auto res = delayed_connection_event_map.insert(conn_id);
  res.first->second.reset(new EventOnClientConnected(eastl::move(evt)));
}


static eastl::vector_map<net::ConnectionId, int> apply_diffs_timestamp_map;


static void send_delayed_connection_event(net::ConnectionId conn_id)
{
  auto res = delayed_connection_event_map.find(conn_id);
  if (res != delayed_connection_event_map.end())
  {
    g_entity_mgr->broadcastEventImmediate(eastl::move(*res->second));
    res->second.reset();
  }
}


static void on_sync_vroms_done_msg(const net::IMessage *msgraw)
{
  debug("[SyncVroms]: The client #%d has applied vrom diffs successfully", msgraw->connection->getId());

  auto msg = msgraw->cast<SyncVromsDone>();
  G_ASSERT(msg);

  auto it = apply_diffs_timestamp_map.find(msgraw->connection->getId());
  if (it != apply_diffs_timestamp_map.end())
  {
    const int applySentAtMs = it->second;
    const int syncTimeMs = get_time_msec() - applySentAtMs;
    if (syncTimeMs > 0)
      statsd::profile("syncvroms.done_ms", (long)syncTimeMs);
  }

  uint32_t &connFlagsRW = msgraw->connection->getConnFlagsRW();
  if (!(connFlagsRW & net::CF_PENDING))
  {
    debug("[SyncVroms]: The connection is excpected to by in peding state.");
    return;
  }

  connFlagsRW &= ~net::CF_PENDING; // not pending anymore

  send_delayed_connection_event(msgraw->connection->getId());

  flush_new_connection(*msgraw->connection);
}


static bool check_client_vroms_and_send_diffs(const ClientInfo *msg)
{
  if (circuit::get_conf()->getBool("syncVromsDisabled", false))
    return true;

#if DAGOR_DBGLEVEL > 0
  if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("syncVromsDisabled", false))
    return true;
#endif

  syncvroms::SyncVromsList clientSyncVroms;
  syncvroms::read_sync_vroms(msg->get<8>(), clientSyncVroms);

  // In order to debug sync feature we might want to
  // always send diff to the client on dev or staging circuits.
  if (circuit::get_conf()->getBool("alwaysSendVromDiffs", false))
    clientSyncVroms.clear();

#if DAGOR_DBGLEVEL > 0
  if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("alwaysSendVromDiffs", false))
    clientSyncVroms.clear();
#endif

  int diffsCount = 0;
  int totalDiffSize = 0;
  ApplySyncVromsDiffs applyDiffsMsg;
  eastl::tie(diffsCount, totalDiffSize) =
    syncvroms::write_vrom_diffs(applyDiffsMsg.get<0>(), syncvroms::get_mounted_sync_vroms_list(), clientSyncVroms);

  if (diffsCount == 0)
    return true;

  auto insIt = apply_diffs_timestamp_map.insert(msg->connection->getId());
  insIt.first->second = get_time_msec();

  debug("[SyncVroms]: Send %d diffs of size %d bytes to client #%d", diffsCount, totalDiffSize, (int)msg->connection->getId());

  statsd::gauge_inc("syncvroms.diffs_sent_size_b", totalDiffSize);
  statsd::counter("syncvroms.diffs_sent", diffsCount);

  applyDiffsMsg.connection = msg->connection;
  send_net_msg(net::get_msg_sink(), eastl::move(applyDiffsMsg));

  return false;
}

// cipher_key = sha256(string_concat(room_secret, user-id))
static auto build_peer_cipher_key(const eastl::string &roomSecret, eastl::string_view uid_str, uint8_t buf[EVP_MAX_MD_SIZE])
{
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
  EVP_DigestUpdate(ctx, roomSecret.c_str(), roomSecret.size());
  EVP_DigestUpdate(ctx, uid_str.data(), uid_str.size());
  unsigned int len = EVP_MAX_MD_SIZE;
  EVP_DigestFinal_ex(ctx, buf, &len);
  EVP_MD_CTX_free(ctx);
  G_ASSERT(len <= EVP_MAX_MD_SIZE);
  return dag::ConstSpan<uint8_t>(buf, len);
}


net::msg_handler_t get_sync_vroms_done_msg_handler() { return &on_sync_vroms_done_msg; }

struct DedicatedNetObserver final : public net::INetworkObserver
{
#if _DEBUG_TAB_ || EA_ASAN_ENABLED || defined(__SANITIZE_ADDRESS__)
  net::ServerFlags serverFlags = net::ServerFlags::Dev;
#else
  net::ServerFlags serverFlags = net::ServerFlags::None;
#endif
  DedicatedNetObserver()
  {
    g_entity_mgr->broadcastEventImmediate(OnNetDedicatedCreated{&serverFlags});

    if (dedicated::is_dynamic_tickrate_enabled())
      serverFlags |= net::ServerFlags::DynTick;

#if NET_PEER_AUTH
    if (!dgs_get_argv("nonetenc"))
#else
    if (dgs_get_argv("forcenetenc"))
#endif
      serverFlags |= net::ServerFlags::Encryption;
  }

  ~DedicatedNetObserver() { g_entity_mgr->broadcastEventImmediate(OnNetDedicatedDestroyed{&serverFlags}); }

  static void onClientInfoMsg(const net::IMessage *msgraw)
  {
    G_FAST_ASSERT(msgraw->connection);
    net::IConnection &conn = *msgraw->connection;
    auto msg = msgraw->cast<ClientInfo>();
    G_ASSERT(msg);
    uint32_t &connFlags = conn.getConnFlagsRW();
    if (DAGOR_UNLIKELY(!(connFlags & net::CF_PENDING)))
    {
      logwarn("ClientInfo for not pending connection %d", (int)conn.getId());
      return;
    }
    if (DAGOR_UNLIKELY(msg->get<0>() != NET_PROTO_VERSION))
    {
      logwarn("Invalid client #%d net proto 0x%x (vs 0x%x)", (int)conn.getId(), msg->get<0>(), NET_PROTO_VERSION);
      conn.disconnect();
      return;
    }

    matching::UserId userId = msg->get<1>();
    eastl::string userName = dedicated_matching::get_player_name(userId);
    int64_t groupId = dedicated_matching::get_player_group(userId);
    int64_t origGroupId = dedicated_matching::get_player_original_group(userId);
    int mteam = dedicated_matching::get_player_team(userId); // team assigned by matching
    if (mteam != TEAM_UNASSIGNED)
      mteam += FIRST_GAME_TEAM;
    if (userName.empty())
    {
      logwarn("matching name for client #%d userid=%lld is absent, use user provided instead", (int)conn.getId(), (long long)userId);
      userName = msg->get<2>();
    }
    const auto serverFlags = static_cast<DedicatedNetObserver *>(get_net_observer())->serverFlags;
    uint16_t clientFlags = msg->get<3>();
    const eastl::string &pltf = msg->get<5>();
    const eastl::string &platformUid = msg->get<6>();
    const eastl::string userIdStr(eastl::string::CtorSprintf{}, "%llu", userId);
    const eastl::string &roomSecret = dedicated_matching::get_room_secret();
    {
      Tab<uint8_t> &peerAuthKey = const_cast<Tab<uint8_t> &>(msg->get<4>());
      VerifyAuthResult authErr = verify_peer_auth(roomSecret, userIdStr, peerAuthKey, pltf.c_str());
      if (authErr != VerifyAuthResult::Ok)
      {
        using fmemstring = eastl::basic_string<char, framemem_allocator>;
        logwarn("Peer auth for conn #%d userid %lld pltfuid '%s' username '%s' pltf '%s' auth_key[%d]=%s failed with %d!",
          (int)conn.getId(), (long long)userId, platformUid, userName.c_str(), pltf.c_str(), peerAuthKey.size(),
          base64::encode<fmemstring>(peerAuthKey.data(), peerAuthKey.size()).c_str(), (int)authErr);
#if NET_PEER_AUTH
        if (!dgs_get_argv("nopeerauth"))
#else
        if (dgs_get_argv("forcepeerauth"))
#endif
        {
          statsd::counter("peer_auth_failed", 1, {"error", eastl::to_string((int)authErr).c_str()});
          conn.disconnect();
          return;
        }
      }
      if (bool(serverFlags & net::ServerFlags::Encryption))
      {
        uint8_t tmp[EVP_MAX_MD_SIZE];
        auto encKey = build_peer_cipher_key(roomSecret, userIdStr, tmp);
        G_ASSERT(encKey.size() >= net::MIN_ENCRYPTION_KEY_LENGTH);
        conn.setEncryptionKey(encKey, net::EncryptionKeyBits::Encryption | net::EncryptionKeyBits::Decryption);
        conn.allowReceivePlaintext(false);
      }
    }
    updater::Version ver{msg->get<7>()};
    debug("Client #%d connected with user{name,id}=<%s>/%lld, groupId=%lld, origGroupId=%lld, mteam=%d, flags=0x%x, ver=%s, pltf=%s",
      (int)conn.getId(), userName.c_str(), (long long)userId, (long long)groupId, (long long)origGroupId, mteam, (int)msg->get<3>(),
      ver.to_string(), pltf.c_str());

    // Try to sync vroms during clien't connection
    const bool vromsAreTheSame = check_client_vroms_and_send_diffs(msg);

    if (vromsAreTheSame)
      connFlags &= ~net::CF_PENDING; // not pending anymore

    {
      ServerInfo srvInfoMsg((uint16_t)serverFlags, (uint8_t)phys_get_tickrate(), (uint8_t)phys_get_bot_tickrate(), get_exe_version32(),
        sceneload::get_current_game().sceneName, sceneload::get_current_game().levelBlkPath, Tab<uint8_t>{});

      g_entity_mgr->broadcastEventImmediate(
        OnNetDedicatedPrepareServerInfo(srvInfoMsg, serverFlags, connFlags, clientFlags, userId, userName, pltf, conn));
      srvInfoMsg.connection = &conn;
      send_net_msg(net::get_msg_sink(), eastl::move(srvInfoMsg));
    }

    int app_id = dedicated_matching::get_player_app_id(userId);

    {
      EventOnClientConnected evt(conn.getId(), userId, eastl::move(userName), groupId, origGroupId, msg->get<3>(), platformUid,
        eastl::string(pltf), mteam, app_id);
      if (vromsAreTheSame)
        g_entity_mgr->broadcastEventImmediate(eastl::move(evt));
      else
        send_event_after_sync_vroms_done(conn.getId(), eastl::move(evt));
    }

    if (!(connFlags & net::CF_PENDING)) // i.e. not disconnected in event handlers
      flush_new_connection(conn);
  }

  void onConnect(net::Connection &conn) override
  {
    G_VERIFY(conn.setEntityInScopeAlways(net::get_msg_sink()));
    if (!conn.isBlackHole()) // network connection
    {
      debug("Client #%d connected, wait for identity message", (int)conn.getId());
      conn.getConnFlagsRW() = net::CF_PENDING;
    }
    else // replay connection
    {
      G_ASSERT(get_time_mgr().getMillis() == 0); // make sure that all replay connections has same timeline
      flush_new_connection(conn);
    }
  }

  void onDisconnect(net::Connection *conn, DisconnectionCause cause) override
  {
    G_FAST_ASSERT(conn != nullptr); // on server conn can't be null here
    debug("Client #%d disconnected with %s", (int)conn->getId(), describe_disconnection_cause(cause));
    if (cause == DC_CONNECTION_LOST)
      statsd::counter("users_lost", 1);
    g_entity_mgr->broadcastEventImmediate(OnNetDedicatedServerDisconnect(*conn));
    // send immediately because connection might get destroyed after this
    g_entity_mgr->broadcastEventImmediate(EventOnClientDisconnected(conn->getId(), cause));
  }
};

net::msg_handler_t get_clientinfo_msg_handler() { return &DedicatedNetObserver::onClientInfoMsg; }

net::INetworkObserver *create_server_net_observer(void *buf, size_t bufsz)
{
  G_ASSERT(sizeof(DedicatedNetObserver) <= bufsz);
  G_UNUSED(bufsz);
  return new (buf, _NEW_INPLACE) DedicatedNetObserver;
}

} // namespace dedicated
