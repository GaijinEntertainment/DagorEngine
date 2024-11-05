// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <contentUpdater/binaryCache.h>

#include <contentUpdater/fsUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_findFiles.h>
#include <osApiWrappers/dag_directUtils.h>
#include <osApiWrappers/dag_vromfs.h>
#include <folders/folders.h>
#include <statsd/statsd.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>

// Project specific functions. Must be available during linking.
namespace updater
{
namespace external
{
// Should return current game's version as int
extern uint32_t get_game_version();

// Should build number in order to
// determine that the game is for dev or not
extern int get_build_number();
} // namespace external
} // namespace updater
namespace updater
{
void write_remote_version_file(const char *folder, Version remote_version);
}

using namespace updater;
using namespace updater::binarycache;
using namespace updater::fs;


bool updater::binarycache::set_cache_version(const char *folder, Version version)
{
  const eastl::string cacheVersionFile(eastl::string::CtorSprintf{}, "%s/cache.version", folder);
  return write_file_with_content(cacheVersionFile.c_str(), version.to_string());
}


Version updater::binarycache::get_cache_version(const char *folder)
{
  const eastl::string cacheVersionFile(eastl::string::CtorSprintf{}, "%s/cache.version", folder);
  return Version{read_file_content(cacheVersionFile.c_str()).c_str()};
}


template <typename Callable>
inline static int foreach_vrom_in_folder(const char *folder, SyncMode mode, Callable callback)
{
  Tab<SimpleString> srcVroms;
  find_files_in_folder(srcVroms, folder, ".vromfs.bin", false /* vromfs */, true /* realfs */, mode == SyncMode::RECURSIVE);

  for (const SimpleString &srcVrom : srcVroms)
    callback(srcVrom.c_str());

  return srcVroms.size();
}


int updater::binarycache::sync_vroms_in_folders(const char *src, const char *dst, SyncMode mode)
{
  const eastl::string srcFolder{normalize_path(src)};
  const eastl::string dstFolder{normalize_path(dst)};

  const int vromsCount = foreach_vrom_in_folder(srcFolder.c_str(), mode, [&](const char *src_vrom) {
    const char *relName = get_filename_relative_to_path(srcFolder.c_str(), src_vrom);

    const eastl::string dstVrom(eastl::string::CtorSprintf{}, "%s%s", dstFolder.c_str(), relName);
    if (dd_file_exists(dstVrom.c_str()))
    {
      debug("[BinaryCache]: Copy '%s' to '%s'", src_vrom, dstVrom.c_str());
      dag::copy_file(src_vrom, dstVrom.c_str());
    }
  });

  return vromsCount;
}


int updater::binarycache::sync_remote_vroms_with_cache(const char *remote_folder, const char *cache_folder, SyncMode mode)
{
  const eastl::string remoteFolder{normalize_path(remote_folder)};
  const eastl::string cacheFolder{normalize_path(cache_folder)};

  const Version remoteDoneVersion{read_remote_version_file(remoteFolder.c_str())};
  if (!remoteDoneVersion)
  {
    statsd::counter("binarycache.error", 1, {"error", "remote_version_unknown"});
    debug("[BinaryCache]: Cannot sync with remote folder '%s': version is unknown", remoteFolder.c_str());
    return 0;
  }

  const Version gameVersion{external::get_game_version()};

  if (!remoteDoneVersion.isCompatible(gameVersion))
  {
    statsd::counter("binarycache.error", 1, {"error", "incompatible_versions"});
    debug("[BinaryCache]: Remote version '%s' is not compatible with game version '%s'", remoteDoneVersion.to_string(),
      gameVersion.to_string());
    return 0;
  }

  debug("[BinaryCache]: Sync folders '%s' and '%s'", remoteFolder.c_str(), cacheFolder.c_str());

  int copiedVromsCount = 0;

  foreach_vrom_in_folder(remoteFolder.c_str(), mode, [&](const char *remote_path) {
    const char *vromName = get_filename_relative_to_path(remoteFolder, remote_path);

    const Version remoteVersion{get_vromfs_dump_version(remote_path)};
    if (!remoteVersion)
    {
      statsd::counter("binarycache.error", 1, {"error", "remote_version_unknown"});
      logerr("[BinaryCache]: Cannot read version of remote vrom '%s'", vromName);
      return;
    }

    const eastl::string cachePath(eastl::string::CtorSprintf{}, "%s%s", cacheFolder.c_str(), vromName);
    if (!dd_file_exists(cachePath.c_str()))
    {
      debug("[BinaryCache]: Use remote version of '%s'. Cache version doesn't exist.", vromName);
      ++copiedVromsCount;
      dag::copy_file(remote_path, cachePath.c_str());
      return;
    }

    const Version cacheVersion{get_vromfs_dump_version(cachePath.c_str())};
    if (!cacheVersion)
    {
      DagorStat remoteStat;
      DagorStat cacheStat;
      if (::df_stat(remote_path, &remoteStat) == 0 && ::df_stat(cachePath.c_str(), &cacheStat) == 0)
      {
        if (remoteStat.mtime > 0 && cacheStat.mtime > 0 && remoteStat.mtime > cacheStat.mtime)
        {
          debug("[BinaryCache]: Use remote version of '%s'. Cache version is older by mtime: %lld > %lld", vromName, remoteStat.mtime,
            cacheStat.mtime);
          ++copiedVromsCount;
          dag::copy_file(remote_path, cachePath.c_str());
          return;
        }
      }
      return;
    }

    if (!gameVersion.isCompatible(remoteVersion))
    {
      statsd::counter("binarycache.error", 1, {"error", "incompatible_versions"});
      logerr("[BinaryCache]: %s: Remote version '%s' is not compatible with cache version '%s' and game version '%s'", vromName,
        remoteVersion.to_string(), cacheVersion.to_string(), gameVersion.to_string());
      return;
    }

    // In order to support reverts allows to copy any remote version to the cache
    if (remoteVersion != cacheVersion)
    {
      debug("[BinaryCache]: Use remote version of '%s'. Remote version '%s' is different from cache '%s'.", vromName,
        remoteVersion.to_string(), cacheVersion.to_string());
      ++copiedVromsCount;
      dag::copy_file(remote_path, cachePath.c_str());
    }
  });

  debug("[BinaryCache]: Setting cache version to '%s'", remoteDoneVersion.to_string());

  if (!set_cache_version(cacheFolder.c_str(), remoteDoneVersion))
  {
    statsd::counter("binarycache.error", 1, {"error", "cache_version_file"});
    logerr("[BinaryCache]: Cannot write the cache version");
  }

  return copiedVromsCount;
}


void updater::binarycache::early_init(Version build_version, const eastl::string &cache_folder)
{
  const Version cacheVersion = get_cache_version(cache_folder.c_str());

  const bool isDisabled =
    ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("disableBinaryCache", external::get_build_number() <= 0);

  debug("[BinaryCache]: Early init, cache folder: '%s', cache version: '%s', build version: %s, is disabled: %d", cache_folder.c_str(),
    cacheVersion.to_string(), build_version.to_string(), isDisabled);

  if (!cacheVersion || isDisabled)
  {
    debug("[BinaryCache]: Cache with version '%s' is invalid. Purging the cache.", cacheVersion.to_string());
  }
  else if (!cacheVersion.isCompatible(build_version))
  {
    debug("[BinaryCache]: Cache version %s is incompatible with build version %s", cacheVersion.to_string(),
      build_version.to_string());
  }
  else if (cacheVersion < build_version)
  {
    debug("[BinaryCache]: Cache version %s is outdated related to build's version %s", cacheVersion.to_string(),
      build_version.to_string());
  }
  else
  {
    debug("[BinaryCache]: Cache with version '%s' is valid and compatible with build's version '%s'", cacheVersion.to_string(),
      build_version.to_string());
    return;
  }

  if (!cache_folder.empty() && dd_dir_exists(cache_folder.c_str()))
  {
    const bool res = dag::remove_dirtree(cache_folder.c_str());
    debug("[BinaryCache]: Cache purged. Remove dir result: '%d'", res);
  }
}


static bool validate_cache_integrity(const eastl::string &remote_folder, const eastl::string &cache_folder)
{
  debug("[BinaryCache]: Validating vroms in the cache:");

  bool isOk = true;
  foreach_vrom_in_folder(cache_folder.c_str(), SyncMode::RECURSIVE, [&](const char *path) {
    const eastl::string vromPath{fs::simplify_path(path)};

    if (::strncmp(vromPath.c_str(), remote_folder.c_str(), (int)remote_folder.length()) == 0)
      return;

    eastl::unique_ptr<VirtualRomFsData, tmpmemDeleter> vromFs{load_vromfs_dump(vromPath.c_str(), tmpmem)};

    if (vromFs == nullptr)
    {
      debug("  * %s: Fail", get_filename_relative_to_path(cache_folder.c_str(), path));
      const char *vromName = dd_get_fname(vromPath.c_str());
      statsd::counter("binarycache.error", 1, {{"error", "corrupted_vrom"}, {"vrom", vromName}});

      isOk = false;
    }
    else
    {
      const Version ver{vromFs->version};
      debug("  * %s (ver: %s): OK", get_filename_relative_to_path(cache_folder.c_str(), path), ver.to_string());
    }
  });

  return isOk;
}


void updater::binarycache::init(Version build_version, const eastl::string &remote_folder, const eastl::string &cache_folder)
{
  const int startMs = get_time_msec();

  const Version cacheVersion = get_cache_version(cache_folder.c_str());
  const Version remoteVersion = read_remote_version_file(remote_folder.c_str());

  if (cacheVersion && build_version > cacheVersion)
  {
    const String gameFolder{folders::get_game_dir()};
    if (!gameFolder.empty())
    {
      debug("[BinaryCache]: Cache version %s is older than build version %s", cacheVersion.to_string(), build_version.to_string());

      // Copy from game's folder to remote (the destination folder for the embedded updater)
      sync_vroms_in_folders(gameFolder, remote_folder.c_str(), SyncMode::RECURSIVE);

      // Copy from game's folder to the cache
      sync_vroms_in_folders(gameFolder, cache_folder.c_str(), SyncMode::RECURSIVE);

      set_cache_version(cache_folder.c_str(), build_version);
      write_remote_version_file(remote_folder.c_str(), build_version);
    }
  }
  else if (remoteVersion)
  {
    const int updatedVromsCount = sync_remote_vroms_with_cache(remote_folder.c_str(), cache_folder.c_str(), SyncMode::RECURSIVE);
    debug("[BinaryCache]: Updater's early init has been done with %d updated vroms", updatedVromsCount);
  }

  if (!validate_cache_integrity(remote_folder, cache_folder))
  {
    debug("[BinaryCache]: Some vroms are corrupted in the cache. Purging the cache.");
    dag::remove_dirtree(cache_folder.c_str());
  }

  const int initTimeMs = get_time_msec() - startMs;
  statsd::profile("binarycache.early_init_ms", (long)initTimeMs);
}


void updater::binarycache::purge(const eastl::string &cache_folder)
{
  if (!cache_folder.empty() && dd_dir_exists(cache_folder.c_str()))
    dag::remove_dirtree(cache_folder.c_str());
}
