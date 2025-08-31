// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/riexSync.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_map.h>
#include <EASTL/bitvector.h>
#include <daNet/bitStream.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>
#include <util/dag_hash.h>

namespace riexsync
{
bool sync_active = false;
void (*pool_mapping_change_callback)() = nullptr;


struct NetSyncData
{
  // start syncing pools with id > `syncedPoolsOffset`
  int syncedPoolsOffset = 0;
  // syncedPoolsOffset is used as offset in remap vectors
  dag::Vector<int> serverToClientPoolRemap;
  dag::Vector<int> clientToServerPoolRemap;
  eastl::vector_map<uint32_t, int> hashToClientPoolId;
  eastl::vector_map<uint32_t, int> pendingServerPoolIds;
  eastl::bitvector<> pendingServerPoolsBitVec;
  eastl::vector_set<int> allPoolsToSync;
  eastl::vector_set<int> newPoolsToSync;

  void clear()
  {
    serverToClientPoolRemap.clear();
    clientToServerPoolRemap.clear();
    allPoolsToSync.clear();
    newPoolsToSync.clear();
    hashToClientPoolId.clear();
    pendingServerPoolIds.clear();
    pendingServerPoolsBitVec.clear();
    syncedPoolsOffset = 0;
    debug("[riex sync] all pool sync data cleared");
  }
};
static NetSyncData g_pool_sync;

#define VERBOSE_SYNC(...)           // debug("[riex sync] " __VA_ARGS__)
#define VERBOSE_SYNC_POOL_DATA(...) // debug("[riex sync] " __VA_ARGS__)


static void add_synced_ri_extra_pool_remap(int server_idx, int client_idx)
{
  server_idx -= g_pool_sync.syncedPoolsOffset;
  client_idx -= g_pool_sync.syncedPoolsOffset;
  G_ASSERT_RETURN(server_idx >= 0 && client_idx >= 0, );
  if (g_pool_sync.serverToClientPoolRemap.size() <= server_idx)
    g_pool_sync.serverToClientPoolRemap.resize(server_idx + 8, -1);
  g_pool_sync.serverToClientPoolRemap[server_idx] = client_idx;
  if (g_pool_sync.clientToServerPoolRemap.size() <= client_idx)
    g_pool_sync.clientToServerPoolRemap.resize(client_idx + 8, -1);
  g_pool_sync.clientToServerPoolRemap[client_idx] = server_idx;
  if (pool_mapping_change_callback)
    pool_mapping_change_callback();
  VERBOSE_SYNC_POOL_DATA("    synced pool %i to server pool %i", client_idx + g_pool_sync.syncedPoolsOffset,
    server_idx + g_pool_sync.syncedPoolsOffset);
}

static void add_synced_ri_extra_pool_impl(int pool_idx, const char *name)
{
  G_ASSERT_RETURN(name != nullptr, );
  const uint32_t hash = str_hash_fnv1<32>(name);
  // check if already registered
  if (const auto it = g_pool_sync.hashToClientPoolId.find(hash); it != g_pool_sync.hashToClientPoolId.end())
  {
    G_ASSERTF(it->second == pool_idx, "synced ri extra pool hash collision %i %i %s %u", pool_idx, it->second, name, hash);
    return;
  }
  VERBOSE_SYNC_POOL_DATA("add client pool %i %s %08x", pool_idx, name, hash);
  // check if was synced from server
  if (const auto it = g_pool_sync.pendingServerPoolIds.find(hash); it != g_pool_sync.pendingServerPoolIds.end())
  {
    add_synced_ri_extra_pool_remap(it->second, pool_idx);
    VERBOSE_SYNC_POOL_DATA("    remove pending server pool %i", it->second);
    g_pool_sync.pendingServerPoolsBitVec.set(it->second, false);
    g_pool_sync.pendingServerPoolIds.erase(it);
  }
  // add
  g_pool_sync.hashToClientPoolId.emplace(hash, pool_idx);
  G_VERIFY(g_pool_sync.allPoolsToSync.insert(pool_idx).second);
  g_pool_sync.newPoolsToSync.insert(pool_idx);
}

void clear_synced_ri_extra_pools() { g_pool_sync.clear(); }

void sync_all_ri_extra_pools()
{
  const int prevPoolsToSync = int(g_pool_sync.newPoolsToSync.size());
  rendinst::iterateRIExtraMap([](int id, const char *name) {
    if (id >= g_pool_sync.syncedPoolsOffset)
      add_synced_ri_extra_pool_impl(id, name);
  });
  const int newPoolsToSync = int(g_pool_sync.newPoolsToSync.size()) - prevPoolsToSync;
  if (newPoolsToSync != 0)
  {
    VERBOSE_SYNC("new pools to sync: %i", newPoolsToSync);
  }
}

// Will mark all currently existing pools as synced, and won't serialize them,
// intended to be used right after loading rendinsts pools from level binary,
// because those are guarantee to match on server and on client
//
// NOTE: Needs to be called on server and on client with same set of pools
void mark_all_loaded_pools_synced()
{
  rendinst::ScopedRIExtraReadLock rd;
  g_pool_sync.clear();
  g_pool_sync.syncedPoolsOffset = rendinst::getRIExtraMapSize();
  debug("[riex sync] set synced pool offset to current rendinst pool count %i", g_pool_sync.syncedPoolsOffset);
}

int get_base_synced_pools_count() { return g_pool_sync.syncedPoolsOffset; }

uint64_t calc_base_synced_pools_hash()
{
  uint64_t hash = 1u;
  rendinst::iterateRIExtraMap([&](int id, const char *name) {
    if (id < g_pool_sync.syncedPoolsOffset)
      hash = str_hash_fnv1<64>(name, id + hash);
  });
  return hash;
}

void sync_ri_extra_pool(int pool_id) { add_synced_ri_extra_pool_impl(pool_id, rendinst::getRIGenExtraName(pool_id)); }

int get_ri_pool_id_remap_impl(const dag::Vector<int> &remap, int pool_id, int def)
{
  if (pool_id < g_pool_sync.syncedPoolsOffset)
    return pool_id;
  pool_id -= g_pool_sync.syncedPoolsOffset;
  if (unsigned(pool_id) < remap.size())
    if (const int id = remap[pool_id]; id != -1)
      return id;
  return def;
}

int get_client_ri_pool_id(int server_pool_id)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.serverToClientPoolRemap, server_pool_id, server_pool_id);
}

int get_client_ri_pool_id(int server_pool_id, int def)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.serverToClientPoolRemap, server_pool_id, def);
}

int get_server_ri_pool_id(int client_pool_id)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.clientToServerPoolRemap, client_pool_id, client_pool_id);
}

int get_server_ri_pool_id(int client_pool_id, int def)
{
  return get_ri_pool_id_remap_impl(g_pool_sync.clientToServerPoolRemap, client_pool_id, def);
}

bool is_server_ri_pool_sync_pending(int server_pool_id) { return g_pool_sync.pendingServerPoolsBitVec.test(server_pool_id, false); }

bool serialize_synced_ri_extra_pools(danet::BitStream &bs, bool full, bool skip_if_no_data, bool mark_all_synced)
{
  dag::ConstSpan<int> pools = full ? make_span_const(g_pool_sync.allPoolsToSync) : make_span_const(g_pool_sync.newPoolsToSync);
  if (skip_if_no_data && pools.empty())
    return false;
  bs.Write(full);
  bool writeIndices = false;
  for (int i = 0; i < pools.size(); i++)
    writeIndices |= i != pools[i];
  bs.Write(writeIndices);
  bs.AlignWriteToByteBoundary();
  bs.WriteCompressed(int(pools.size()));
  int lastPoolIdx = -1;
  VERBOSE_SYNC("serializing riex pools full:%i, count:%i, offset:%i", int(full), pools.size(), g_pool_sync.syncedPoolsOffset);
  for (const int pool : pools)
  {
    if (pool < g_pool_sync.syncedPoolsOffset)
      continue;
    const char *name = rendinst::getRIGenExtraName(pool);
    const uint32_t hash = str_hash_fnv1<32>(name);
    bs.Write(hash);
    G_ASSERT(lastPoolIdx < pool);
    if (writeIndices)
      bs.WriteCompressed(int(pool - (lastPoolIdx + 1)));
    lastPoolIdx = pool;
  }
  if (mark_all_synced)
    g_pool_sync.newPoolsToSync.clear();
  return true;
}

bool deserialize_synced_ri_extra_pools(const danet::BitStream &bs)
{
  bool ok = true;
  bool isFullData = false;
  bool hasIndices = false;
  int count = 0;
  ok &= bs.Read(isFullData);
  ok &= bs.Read(hasIndices);
  bs.AlignReadToByteBoundary();
  ok &= bs.ReadCompressed(count);
  if (ok && isFullData)
  {
    g_pool_sync.serverToClientPoolRemap.clear();
    g_pool_sync.clientToServerPoolRemap.clear();
    g_pool_sync.pendingServerPoolIds.clear();
    g_pool_sync.pendingServerPoolsBitVec.clear();
  }
  VERBOSE_SYNC("deserializing riex pools full:%i, count:%i, offset:%i", int(isFullData), count, g_pool_sync.syncedPoolsOffset);
  int lastPoolIdx = -1;
  for (int i = 0; i < count; i++)
  {
    uint32_t hash = 0;
    ok &= bs.Read(hash);
    int delta = 0;
    if (hasIndices)
      ok &= bs.ReadCompressed(delta);
    if (!ok)
      break;
    const int pool = lastPoolIdx + 1 + delta;
    lastPoolIdx = pool;
    VERBOSE_SYNC_POOL_DATA("  server pool %i %08x", pool, hash);
    if (pool < g_pool_sync.syncedPoolsOffset)
      continue;
    if (const auto it = g_pool_sync.hashToClientPoolId.find(hash); it != g_pool_sync.hashToClientPoolId.end())
      add_synced_ri_extra_pool_remap(pool, it->second);
    else
    {
      g_pool_sync.pendingServerPoolIds.emplace(hash, pool);
      g_pool_sync.pendingServerPoolsBitVec.set(pool, true);
      VERBOSE_SYNC_POOL_DATA("    add pending server pool %i", pool);
    }
  }
  return ok;
}

} // namespace riexsync
