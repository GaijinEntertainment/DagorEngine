// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <syncVroms/syncVroms.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <daNet/daNetTypes.h>
#include <startup/dag_globalSettings.h>
#include <compressionUtils/vromfsCompressionImpl.h>
#include <compressionUtils/bsdiffwrap.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <statsd/statsd.h>
#include <daNet/bitStream.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>


namespace syncvroms
{
namespace external
{
// Return a hash of mounted vrom.
// The hash is calulated by syncvroms::load_vromfs_dump()
// The hash only uses vroms actual decompressed content
// in order to know that content is the same.
// This is important of vroms sync: send diffs only if it needed.
extern const VromHash &get_vrom_hash_by_name(const char *name);
} // namespace external
} // namespace syncvroms


struct BaseSyncVrom
{
  VromHash primary;
  // Additional base vrom in the build.
  // In order to have previous game build in one packege.
  VromHash secondary;

  eastl::string name;
};


struct VromfsDataRemover
{
  void operator()(VirtualRomFsData *fs)
  {
    if (fs != nullptr)
    {
      remove_vromfs(fs);
      memfree(fs, tmpmem);
    }
  }
};


static eastl::string g_root_folder;
static eastl::vector<BaseSyncVrom> g_base_sync_vroms;
static eastl::vector_map<SystemIndex, syncvroms::SyncVromsList> g_client_sync_vroms;
static const syncvroms::SyncVromsList EMPTY_SYNC_VROMS;
static eastl::unique_ptr<VirtualRomFsData, VromfsDataRemover> g_base_fs_data;
static eastl::unique_ptr<VirtualRomFsData, VromfsDataRemover> g_diff_fs_data;


const VromHash &syncvroms::get_base_vrom_hash(const char *vrom_name)
{
  const auto res = eastl::find_if(g_base_sync_vroms.cbegin(), g_base_sync_vroms.cend(),
    [vrom_name](const BaseSyncVrom &vrom) { return vrom.name == vrom_name; });
  return res != g_base_sync_vroms.cend() ? res->primary : EMPTY_VROM_HASH;
}


const char *syncvroms::get_base_vrom_name(const VromHash &vrom_hash)
{
  const auto res = eastl::find_if(g_base_sync_vroms.cbegin(), g_base_sync_vroms.cend(),
    [vrom_hash](const BaseSyncVrom &vrom) { return vrom.primary == vrom_hash; });
  return res != g_base_sync_vroms.cend() ? res->name.c_str() : nullptr;
}


void syncvroms::shutdown()
{
  g_base_fs_data.reset();
  g_diff_fs_data.reset();
}


void syncvroms::init_base_vroms()
{
  g_root_folder = dgs_get_settings()->getBlockByNameEx("syncVroms")->getStr("rootFolder", "diff");

  const eastl::string metaBlkPath{eastl::string::CtorSprintf{}, "%s/meta.blk", g_root_folder.c_str()};

  if (!dd_file_exists(metaBlkPath.c_str()))
  {
    debug("[SyncVroms]: Disabled. %s doesn't exist.", metaBlkPath.c_str());
    return;
  }

  DataBlock metaBlk;
  if (dblk::load(metaBlk, metaBlkPath.c_str(), dblk::ReadFlag::ROBUST))
  {
    const eastl::string basePath{eastl::string::CtorSprintf{}, "%s/base.vromfs.bin", g_root_folder.c_str()};
    const eastl::string diffPath{eastl::string::CtorSprintf{}, "%s/diff.vromfs.bin", g_root_folder.c_str()};

    if (dd_file_exists(basePath.c_str()))
    {
      g_base_fs_data.reset(::load_vromfs_dump(basePath.c_str(), tmpmem));
      if (g_base_fs_data)
        ::add_vromfs(g_base_fs_data.get());
    }

    if (dd_file_exists(diffPath.c_str()))
    {
      g_diff_fs_data.reset(::load_vromfs_dump(diffPath.c_str(), tmpmem));
      if (g_diff_fs_data)
        ::add_vromfs(g_diff_fs_data.get());
    }

    for (int i = 0, sz = metaBlk.blockCount(); i < sz; ++i)
    {
      const DataBlock *vromBlk = metaBlk.getBlock(i);
      const char *fullVromName = vromBlk->getBlockName();
      const char *vromExt = ::strstr(fullVromName, ".vromfs.bin");
      const size_t vromNameLength = vromExt != nullptr ? size_t(vromExt - fullVromName) : eastl::string::npos;
      BaseSyncVrom baseVrom{vromBlk->getStr("primary"), vromBlk->getStr("secondary", ""), {fullVromName, vromNameLength}};
      if (baseVrom.primary)
      {
        debug("[SyncVroms]: Base vrom: %s (%s; %s)", baseVrom.name.c_str(), baseVrom.primary.c_str(), baseVrom.secondary.c_str());
        g_base_sync_vroms.push_back(eastl::move(baseVrom));
      }
      else
        logerr("[SyncVroms]: 'primary' parameter is corrupted in %s for '%s'", metaBlkPath.c_str(), baseVrom.name.c_str());
    }
  }
  else
    logerr("[SyncVroms]: %s can't be loaded.", metaBlkPath.c_str());
}


void syncvroms::set_client_sync_vroms_list(uint16_t system_index, syncvroms::SyncVromsList &&sync_vroms)
{
  const auto res = g_client_sync_vroms.insert(eastl::make_pair(system_index, eastl::move(sync_vroms)));

  // In case existance of the key overwrite the value
  // On JIP we already have the list. So, we need to overwrite it with a new one.
  if (!res.second)
    g_client_sync_vroms[system_index] = eastl::move(sync_vroms);
}


const syncvroms::SyncVromsList &syncvroms::get_client_sync_vroms_list(uint16_t system_index)
{
  const auto res = g_client_sync_vroms.find(system_index);
  if (res != g_client_sync_vroms.end())
    return res->second;
  return EMPTY_SYNC_VROMS;
}


syncvroms::SyncVromsList syncvroms::get_mounted_sync_vroms_list()
{
  SyncVromsList syncVroms;
  syncVroms.reserve(g_base_sync_vroms.size());

  for (const BaseSyncVrom &baseVrom : g_base_sync_vroms)
    if (const VromHash &hash = external::get_vrom_hash_by_name(baseVrom.name.c_str()))
      syncVroms.push_back({hash, baseVrom.name});

  return syncVroms;
}


static syncvroms::Bytes read_file_content(const char *path, int64_t &out_mtime)
{
  eastl::unique_ptr<void, DagorFileCloser> fp{df_open(path, DF_READ | DF_IGNORE_MISSING)};
  if (fp)
  {
    DagorStat st;
    if (df_fstat(fp.get(), &st) != 0)
      return {};

    out_mtime = st.mtime;

    syncvroms::Bytes buffer(size_t(df_length(fp.get())));
    if (df_read(fp.get(), buffer.data(), buffer.size()) == buffer.size())
      return buffer;
  }
  return {};
}


syncvroms::Bytes load_compressed(const char *path, const Compression &compr, int64_t &out_mtime)
{
  const syncvroms::Bytes compressedData{read_file_content(path, out_mtime)};

  const size_t compressedSize = compressedData.size();
  if (compressedSize < 1)
    return {};

  if (compressedData[0] != compr.getId())
  {
    logerr("Unknown compression id '%c'", compressedData[0]);
    return {};
  }

  const int plainSize = compr.getRequiredDecompressionBufferLength(&compressedData[1], compressedSize - 1);
  if (plainSize <= 0)
  {
    logerr("Invalid plain size %d", plainSize);
    return {};
  }

  syncvroms::Bytes decompressedData((size_t)plainSize);

  int outPlainDataSize = decompressedData.size();
  const char *plainData = compr.decompress(&compressedData[1], compressedSize - 1, decompressedData.data(), outPlainDataSize);
  if (plainData == nullptr)
  {
    logerr("Could not decompress data");
    return {};
  }

  if (plainData == decompressedData.data())
    return decompressedData;

  decompressedData.resize(outPlainDataSize);
  ::memcpy(decompressedData.begin(), plainData, outPlainDataSize);
  return decompressedData;
}


static uint32_t patch_vromfs_dump_version(syncvroms::Bytes &bytes, uint32_t patch_version)
{
  InPlaceMemLoadCB cb(bytes.data(), bytes.size());

  VirtualRomFsDataHdr hdr;
  if (cb.tryRead(&hdr, sizeof(hdr)) != sizeof(hdr))
    return 0;

  if (hdr.label != _MAKE4C('VRFx'))
    return 0;

  const int hrdOffset = cb.tell();

  VirtualRomFsExtHdr hdr_ext;
  if (cb.tryRead(&hdr_ext, sizeof(hdr_ext)) != sizeof(hdr_ext))
    return 0;
  if (hdr_ext.size < sizeof(hdr_ext))
    return 0;

  const uint32_t wasVersion = hdr_ext.version;
  hdr_ext.version = patch_version;

  ConstrainedMemSaveCB writeCb(bytes.data(), bytes.size());
  writeCb.setsize(bytes.size());
  writeCb.seekto(hrdOffset);
  writeCb.write(&hdr_ext, sizeof(hdr_ext));

  return wasVersion;
}


static eastl::pair<syncvroms::Bytes, VromHash> load_vrom_compressed(const char *path, const VromfsCompression &compr,
  int64_t &out_mtime)
{
  syncvroms::Bytes vromContent = load_compressed(path, compr, out_mtime);

  // Hash vrom ignoring its version.
  // So, just rebuilding vrom with version increase won't change the hash.
  // Only content matters
  const uint32_t wasVersion = patch_vromfs_dump_version(vromContent, 0);
  const VromHash vromHash{VromHash::hash(vromContent.data(), vromContent.size())};
  patch_vromfs_dump_version(vromContent, wasVersion);

  return eastl::make_pair(eastl::move(vromContent), vromHash);
}


static eastl::pair<syncvroms::Bytes, VromHash> load_vrom_compressed(const char *path)
{
  int64_t dummy;
  return load_vrom_compressed(path, {}, dummy);
}


static bool write_file_atomic(const char *path, const syncvroms::Bytes &bytes, const char *tmp_mask)
{
  char tmpPath[DAGOR_MAX_PATH];
  dd_get_fname_location(tmpPath, path);
  dd_mkdir(tmpPath);
  strncat(tmpPath, tmp_mask, sizeof(tmpPath) - strlen(tmpPath) - 1);

  {
    eastl::unique_ptr<void, DagorFileCloser> fp{df_mkstemp(tmpPath)};
    if (fp)
    {
#if _TARGET_PC_LINUX
      fchmod(fileno((FILE *)fp.get()), 0644);
#endif
      if (df_write(fp.get(), bytes.data(), bytes.size()) != bytes.size())
        return false;
    }
    else
      return false;
  }

  if (dd_rename(tmpPath, path))
    return true;

  dd_erase(tmpPath);

  return false;
}


static bool save_compressed(const char *path, const Tab<char> &bytes, const Compression &compr)
{
  const int compressedSize = compr.getRequiredCompressionBufferLength(bytes.size());
  if (compressedSize < 0)
    return false;

  syncvroms::Bytes copressed((size_t)(compressedSize + 1));
  copressed[0] = compr.getId();

  int outCompressedSize = compressedSize;
  void *writeTo = &copressed[1];
  if (compr.compress(bytes.data(), bytes.size(), writeTo, outCompressedSize) != writeTo)
    return false;

  copressed.resize(outCompressedSize + 1);

  return write_file_atomic(path, copressed, "XXXXXX");
}


syncvroms::LoadedSyncVrom syncvroms::load_vromfs_dump(const char *path, const VromfsCompression &compr)
{
  static constexpr VromfsCompression::CheckType vromfsCheck = VromfsCompression::CHECK_VERBOSE;
  static constexpr VromfsCompression::InputDataType inputData = VromfsCompression::IDT_DATA;

  int64_t mtime = -1;
  eastl::pair<Bytes, VromHash> vrom;

#if DAGOR_DBGLEVEL > 0
  static const char *devVromExt = ".dev";
  eastl::string devPath = eastl::string(path) + devVromExt;
  vrom = load_vrom_compressed(devPath.c_str(), compr, mtime);
  if (!vrom.first.empty())
  {
    path = devPath.c_str();
    logwarn("[SyncVroms] DEV-vrom exists and will be used: %s", path);
  }
#endif // DAGOR_DBGLEVEL

  if (vrom.first.empty())
    vrom = load_vrom_compressed(path, compr, mtime);

  if (!vrom.first.empty() && compr.getDataType(vrom.first.data() + 1, vrom.first.size() - 1, vromfsCheck) != inputData)
  {
    dag::ConstSpan<char> vromSlice{vrom.first.data(), (intptr_t)vrom.first.size()};
    if (syncvroms::LoadedSyncVrom loadedVrom{vrom.second, dd_get_fname(path), load_vromfs_dump_from_mem(vromSlice, defaultmem)})
    {
      loadedVrom.fsData->mtime = mtime;
      return loadedVrom;
    }
  }

  return {};
}


static BsdiffStatus apply_bsdiff(const syncvroms::Bytes &old, const syncvroms::Bytes &diff, Tab<char> &out_result)
{
  return apply_vromfs_bsdiff(out_result, make_span(old.data(), old.size()), make_span(diff.data(), diff.size()));
}


bool syncvroms::apply_vrom_diff(const VromDiff &diff, const char *save_vrom_to, const VromfsCompression &compr)
{
  const int startMs = get_time_msec();

  const char *vromName = get_base_vrom_name(diff.baseVromHash);
  if (vromName == nullptr)
  {
    statsd::counter("syncvroms.error", 1, {"error", "base_vrom_unknown"});
    logerr("[SyncVroms]: Cannot apply diff: base vrom name with hash %s is unknown", diff.baseVromHash.c_str());
    return false;
  }

  debug("[SyncVroms]: Try to apply diff (%d bytes) for vrom '%s' (%s)", diff.bytes.size(), vromName, diff.baseVromHash.c_str());

  const eastl::string baseBin(eastl::string::CtorSprintf{}, "%s/base/%s.bin", g_root_folder.c_str(), diff.baseVromHash.c_str());

  const eastl::pair<Bytes, VromHash> srcData = load_vrom_compressed(baseBin.c_str());
  if (srcData.first.empty())
  {
    statsd::counter("syncvroms.error", 1, {"error", "open_base_vrom"});
    logerr("[SyncVroms]: Cannot apply diff: base vrom '%s' (%s) is not found", vromName, diff.baseVromHash.c_str());
    return false;
  }

  Tab<char> resultData;
  const BsdiffStatus status = apply_bsdiff(srcData.first, diff.bytes, resultData);
  if (status != BSDIFF_OK)
  {
    statsd::counter("syncvroms.apply_bsdiff_error", 1, {"error", bsdiff_status_str(status)});
    logerr("[SyncVroms]: Cannot apply diff: apply_bsdiff has been done with '%s'", bsdiff_status_str(status));
    return false;
  }

  if (!save_compressed(save_vrom_to, resultData, compr))
  {
    statsd::counter("syncvroms.error", 1, {"error", "save_vrom_to_cache"});
    logerr("[SyncVroms]: Cannot save to '%s'", save_vrom_to);
    return false;
  }

  const int diffTimeMs = get_time_msec() - startMs;
  statsd::profile("syncvroms.apply_diff_ms", (long)diffTimeMs);

  debug("[SyncVroms]: The vrom has been saved to '%s' within %d ms", save_vrom_to, diffTimeMs);

  return true;
}


eastl::pair<int, int> syncvroms::write_vrom_diffs(danet::BitStream &bs, const SyncVromsList &server_sync_vroms,
  const SyncVromsList &client_sync_vroms)
{
  const uint32_t diffsCountOffset = bs.GetWriteOffset();
  uint8_t diffsCount = 0;
  bs.Write(diffsCount);

  int32_t totalDiffSize = 0;
  for (const syncvroms::SyncVrom &serverSyncVrom : server_sync_vroms)
  {
    // Skip client vroms with the same hashes on the server
    const auto clientSyncVromIt = eastl::find(client_sync_vroms.cbegin(), client_sync_vroms.cend(), serverSyncVrom);
    if (clientSyncVromIt != client_sync_vroms.cend())
      continue;

    const VromHash &baseVromHash = syncvroms::get_base_vrom_hash(serverSyncVrom.name.c_str());

    const eastl::string diffBin(eastl::string::CtorSprintf{}, "%s/diff/%s.bin", g_root_folder.c_str(), baseVromHash.c_str());
    eastl::unique_ptr<void, DagorFileCloser> diffHandle(df_open(diffBin.c_str(), DF_READ | DF_IGNORE_MISSING));
    if (!diffHandle)
    {
      statsd::counter("syncvroms.error", 1, {"error", "open_diff"});
      logerr("[SyncVroms]: Cannot open diff file '%s'", diffBin.c_str());
      continue;
    }

    const int32_t diffSize = df_length(diffHandle.get());
    if (diffSize <= 0)
    {
      statsd::counter("syncvroms.error", 1, {"error", "diff_zero_size"});
      logerr("[SyncVroms]: Cannot get diff file '%s' size", diffBin.c_str());
      continue;
    }

    ++diffsCount;
    totalDiffSize += diffSize;

    bs.Write(diffSize);
    bs.Write(baseVromHash.c_str(), baseVromHash.length());

    bs.reserveBits(BYTES_TO_BITS(diffSize));
    if (df_read(diffHandle.get(), bs.GetData() + BITS_TO_BYTES(bs.GetWriteOffset()), diffSize) != diffSize)
    {
      statsd::counter("syncvroms.error", 1, {"error", "read_diff"});
      logerr("Cannot read diff file '%s'", diffBin.c_str());
      continue;
    }

    bs.SetWriteOffset(bs.GetWriteOffset() + BYTES_TO_BITS(diffSize));
  }

  bs.WriteAt(diffsCount, diffsCountOffset);

  return eastl::make_pair(diffsCount, totalDiffSize);
}


static void write_sync_vrom(danet::BitStream &bs, const syncvroms::SyncVrom &vrom)
{
  bs.Write(vrom.name);
  bs.Write(vrom.hash.c_str(), vrom.hash.length());
}


static bool read_sync_vrom(const danet::BitStream &bs, syncvroms::SyncVrom &out_vrom)
{
  return bs.Read(out_vrom.name) && bs.Read(out_vrom.hash.data(), out_vrom.hash.length());
}


void syncvroms::write_sync_vroms(danet::BitStream &bs, const syncvroms::SyncVromsList &sync_vroms)
{
  debug("[SyncVroms] Send vroms:");

  bs.Write((uint8_t)sync_vroms.size());
  for (const syncvroms::SyncVrom &vrom : sync_vroms)
  {
    if (!vrom.hash)
      logerr("Cannot get hash from vrom '%s'. The vrom possibly doesn't exist.", vrom.name);

    debug("  %s = %s", vrom.name, vrom.hash.c_str());
    write_sync_vrom(bs, vrom);
  }
}


bool syncvroms::read_sync_vroms(const danet::BitStream &bs, syncvroms::SyncVromsList &out_sync_vroms)
{
  uint8_t syncVromsCount = 0;
  bs.Read(syncVromsCount);

  debug("[SyncVroms] Received vroms:");

  bool isOk = true;
  out_sync_vroms.resize(syncVromsCount);
  for (int i = 0; i < syncVromsCount; ++i)
  {
    isOk &= read_sync_vrom(bs, out_sync_vroms[i]);
    debug("  %s = %s", out_sync_vroms[i].name, out_sync_vroms[i].hash.c_str());
  }

  return isOk;
}


bool syncvroms::read_sync_vroms_diffs(const danet::BitStream &bs, syncvroms::OnVromDiffCB cb)
{
  bool isOk = true;

  uint8_t diffsCount = 0;
  isOk &= bs.Read(diffsCount);

  for (int i = 0; isOk && i < diffsCount; ++i)
  {
    syncvroms::VromDiff vromDiff;
    int32_t size = 0;
    isOk &= bs.Read(size);
    isOk &= bs.Read(vromDiff.baseVromHash.data(), vromDiff.baseVromHash.length());

    vromDiff.bytes.resize(size);
    isOk &= bs.ReadBits((uint8_t *)vromDiff.bytes.data(), BYTES_TO_BITS(vromDiff.bytes.size()));

    if (isOk)
      cb(vromDiff);
  }

  if (!isOk || diffsCount == 0)
    statsd::counter("syncvroms.error", 1, {"error", "corrupted_diffs_message"});

  return isOk;
}