//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <syncVroms/vromHash.h>

#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>


struct VirtualRomFsData;
class VromfsCompression;

namespace danet
{
class BitStream;
}


namespace syncvroms
{
using Bytes = dag::Vector<char, EASTLAllocatorType, false>;

struct VromDiff
{
  VromHash baseVromHash;
  Bytes bytes;
};

struct SyncVrom
{
  VromHash hash;
  eastl::string name;

  bool operator==(const SyncVrom &rhs) const { return hash == rhs.hash; }
};

struct LoadedSyncVrom
{
  VromHash hash;
  eastl::string name;
  VirtualRomFsData *fsData;

  explicit operator bool() const { return fsData != nullptr; }
};

using SyncVromsList = eastl::fixed_vector<SyncVrom, 8, true /* bEnableOverflow */>;

bool apply_vrom_diff(const VromDiff &diff, const char *save_vrom_to, const VromfsCompression &compr);

SyncVromsList get_mounted_sync_vroms_list();

void set_client_sync_vroms_list(uint16_t system_index, SyncVromsList &&sync_vroms);
const SyncVromsList &get_client_sync_vroms_list(uint16_t system_index);

void init_base_vroms();
void shutdown();

const VromHash &get_base_vrom_hash(const char *vrom_name);
const char *get_base_vrom_name(const VromHash &vrom_hash);

LoadedSyncVrom load_vromfs_dump(const char *path, const VromfsCompression &compr);

// Returns diffs count and diffs total size in bytes
eastl::pair<int, int> write_vrom_diffs(danet::BitStream &bs, const SyncVromsList &server_sync_vroms,
  const SyncVromsList &client_sync_vroms);

void write_sync_vroms(danet::BitStream &bs, const syncvroms::SyncVromsList &sync_vroms);
bool read_sync_vroms(const danet::BitStream &bs, syncvroms::SyncVromsList &out_sync_vroms);

using OnVromDiffCB = eastl::function<void(const VromDiff &)>;
bool read_sync_vroms_diffs(const danet::BitStream &bs, OnVromDiffCB cb);
} // namespace syncvroms
