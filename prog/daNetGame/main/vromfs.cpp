// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/vromfs.h"
#include "main/main.h"
#include "main/version.h"
#include "net/dedicated.h"
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <util/dag_baseDef.h>
#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/bitvector.h>
#include <ioSys/dag_dataBlock.h>
#include <memory/dag_framemem.h>
#include <contentUpdater/binaryCache.h>
#include <contentUpdater/fsUtils.h>
#include <syncVroms/syncVroms.h>
#include <compressionUtils/vromfsCompressionImpl.h>
#include <folders/folders.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>

struct VromfsDataRemover
{
  void operator()(VirtualRomFsData *pack)
  {
    remove_vromfs(pack);
    memfree(pack, inimem);
  }
};
struct VromfsPackRemover
{
  void operator()(VirtualRomFsPack *pack)
  {
    remove_vromfs(pack);
    close_vromfs_pack(pack, midmem);
  }
};

struct MountVromfsRec
{
  VromHash hash;
  SimpleString path;
  SimpleString mount;
  eastl::unique_ptr<VirtualRomFsData, VromfsDataRemover> vromfsData;
  eastl::unique_ptr<VirtualRomFsPack, VromfsPackRemover> vromfsPack;
  MountVromfsRec(const char *pth, const char *mnt) : path(pth), mount(mnt) {}
  MountVromfsRec(const char *pth, const char *mnt, const VromHash &_hash) : path(pth), mount(mnt), hash(_hash) {}
  bool operator<(const MountVromfsRec &rhs) const { return path < rhs.path; }
  bool operator<(const char *rhs_path) const { return path < rhs_path; }
};
static inline bool operator<(const char *fn, const MountVromfsRec &rhs) { return strcmp(fn, rhs.path) < 0; }
static eastl::vector_set<MountVromfsRec> mnt_vromfs;
static eastl::vector_set<MountVromfsRec> mnt_single_file;
static eastl::bitvector<> changed_vromfs;
static signature_checker_factory_cb vromfs_sig_check;

// Init the value in setup_update_folder_for_early_vromfs_load() call
updater::Version mounted_vroms_version;

namespace syncvroms
{
namespace external
{
const VromHash &get_vrom_hash_by_name(const char *name)
{
  using TmpString = eastl::basic_string<char, framemem_allocator>;
  const TmpString fullName{TmpString::CtorSprintf{}, "%s.vromfs.bin", name};

  for (const MountVromfsRec &rec : mnt_vromfs)
    if (fullName == dd_get_fname(rec.path.c_str()))
      return rec.hash;
  return EMPTY_VROM_HASH;
}
} // namespace external
} // namespace syncvroms
namespace updater
{
namespace external
{
uint32_t get_game_version() { return ::get_updated_game_version().value; }
int get_build_number() { return ::get_build_number(); }
} // namespace external
} // namespace updater

const char *get_vrom_path_by_name(const char *name)
{
  using TmpString = eastl::basic_string<char, framemem_allocator>;
  const TmpString fullName{TmpString::CtorSprintf{}, "%s.vromfs.bin", name};

  for (const MountVromfsRec &rec : mnt_vromfs)
    if (fullName == dd_get_fname(rec.path.c_str()))
      return rec.path.c_str();
  return nullptr;
}


VromfsCompression create_vromfs_compression(const char *path) { return {vromfs_sig_check, dd_get_fname(path)}; }


static void add_vrom(const char *fn, const char *mnt_path, bool is_pack, VirtualRomFsData *vd, const VromHash &hash)
{
  typename decltype(mnt_vromfs)::insert_return_type ins;
  ins = mnt_vromfs.emplace(fn, mnt_path, hash);
  if (ins.second)
  {
    is_pack ? ins.first->vromfsPack.reset(static_cast<VirtualRomFsPack *>(vd)) : ins.first->vromfsData.reset(vd);
    add_vromfs(vd, /*insert_first*/ false, ins.first->mount);
    debug("mounted %s <%s> to \"%s\", ver=%s, hash=%s", is_pack ? "VPACK" : "VROM", fn, mnt_path,
      updater::Version{vd->version}.to_string(), hash.c_str());
  }
  else
  {
    debug("%s <%s> already mounted, ignore", is_pack ? "VPACK" : "VROM", fn);
    if (is_pack)
      close_vromfs_pack(static_cast<VirtualRomFsPack *>(vd), midmem);
    else
      memfree(vd, inimem);
  }
}

static inline dag::ConstSpan<uint8_t> *get_name_buf(dag::ConstSpan<uint8_t> &stor, const char *name)
{
  if ((name = name ? dd_get_fname(name) : nullptr) != nullptr)
  {
    stor.set((const uint8_t *)name, i_strlen(name));
    return &stor;
  }
  return nullptr;
}

VirtualRomFsData *mount_vrom(const char *name, const char *mnt_path, bool is_pack, bool is_optional, ReqVromSign req_sign)
{
  G_ASSERT_RETURN(name && *name, nullptr);

  VirtualRomFsData *newData;
  syncvroms::LoadedSyncVrom loadedVrom;
  String eff_path(get_eff_vrom_fpath(name));
  const auto signCheckCb = req_sign == ReqVromSign::Yes ? vromfs_sig_check : nullptr;
  if (is_pack)
    newData = ::open_vromfs_pack(eff_path, midmem, DF_IGNORE_MISSING);
  else
  {
    loadedVrom = syncvroms::load_vromfs_dump(eff_path.c_str(), VromfsCompression{signCheckCb, dd_get_fname(eff_path.c_str())});
    newData = loadedVrom.fsData;
  }
  if (!newData && dd_file_exists(eff_path))
    DAG_FATAL("[%s]%s read error '%s': content is inconsistent", is_optional ? "optional" : "mandatory", is_pack ? "pack" : "vrom",
      eff_path);
  if (!newData && strcmp(eff_path, name) != 0)
  {
    if (is_pack)
      newData = ::open_vromfs_pack(name, midmem, DF_IGNORE_MISSING);
    else
    {
      loadedVrom = syncvroms::load_vromfs_dump(name, VromfsCompression{signCheckCb, dd_get_fname(name)});
      newData = loadedVrom.fsData;
    }
    logwarn("removing broken %s", eff_path);
    dd_erase(eff_path);
  }
  if (!newData && dd_file_exists(name))
    DAG_FATAL("[%s]%s read error '%s': content is inconsistent", is_optional ? "optional" : "mandatory", is_pack ? "pack" : "vrom",
      name);
  if (!newData && !is_optional)
    DAG_FATAL("%s read error '%s'", is_pack ? "pack" : "vrom", name);
  if (newData)
    add_vrom(name, mnt_path, is_pack, newData, loadedVrom.hash);

  return newData;
}

void unmount_all_vroms() { decltype(mnt_vromfs)().swap(mnt_vromfs); }

template <typename F>
static void load_vrom_list_blk(const DataBlock &blk, const F &cb)
{
  int packNid = blk.getNameId("pack");
  int vromNid = blk.getNameId("vrom");
  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *vromBlk = blk.getBlock(i);
    bool isPack = vromBlk->getBlockNameId() == packNid;
    bool isVrom = vromBlk->getBlockNameId() == vromNid;
    if (!isPack && !isVrom)
      continue;
    const char *path = vromBlk->getStr("path", NULL);
    G_ASSERT_CONTINUE(path && *path);
    const char *mnt = vromBlk->getStr("mnt", "");
    cb(path, mnt, isPack, vromBlk->getBool("optional", false));
  }
}

static inline bool operator==(const VromLoadInfo &vi, const char *path) { return strcmp(vi.path, path) == 0; }
void apply_vrom_list_difference(const DataBlock &blk, dag::ConstSpan<VromLoadInfo> extra_list)
{
  Tab<VromLoadInfo> vrom_list(framemem_ptr());
  vrom_list.reserve(blk.blockCount() + extra_list.size());
  load_vrom_list_blk(blk, [&vrom_list](const char *path, const char *mount, bool is_pack, bool opt) {
    vrom_list.push_back(VromLoadInfo{path, mount, is_pack, opt});
  });
  for (auto &vi : extra_list)
    vrom_list.push_back(vi);

  // unload extra
  for (auto it = mnt_vromfs.begin(); it != mnt_vromfs.end();)
  {
    if (eastl::find(vrom_list.begin(), vrom_list.end(), it->path.c_str()) == vrom_list.end())
    {
      debug("unload VROM <%s>", it->path.c_str());
      it = mnt_vromfs.erase(it);
    }
    else
      ++it;
  }

  // load new
  for (auto &vi : vrom_list)
    if (mnt_vromfs.find_as(vi.path, eastl::less_2<MountVromfsRec, const char *>()) == mnt_vromfs.end()) // not exist already
      mount_vrom(vi.path, vi.mount, vi.isPack, vi.optional, vi.reqSign);
}

RAIIVrom::RAIIVrom(const char *vname, bool ignore_missing, IMemAlloc *alloc, bool insert_fist, char *mnt, bool req_sign) :
  mem(alloc ? alloc : tmpmem), vrom(nullptr)
{
  dag::ConstSpan<uint8_t> name_stor;
  vrom = load_vromfs_dump(get_eff_vrom_fpath(vname), mem, req_sign ? vromfs_sig_check : nullptr,
    get_name_buf(name_stor, req_sign ? vname : nullptr), ignore_missing ? DF_IGNORE_MISSING : 0);
  if (!vrom)
  {
    String eff_path(get_eff_vrom_fpath(vname));
    if (strcmp(eff_path, vname) != 0)
    {
      if (dd_file_exists(eff_path))
        DAG_FATAL("%s read error '%s': content is inconsistent", "vrom", eff_path);
      vrom = load_vromfs_dump(vname, mem, req_sign ? vromfs_sig_check : nullptr, get_name_buf(name_stor, req_sign ? vname : nullptr),
        ignore_missing ? DF_IGNORE_MISSING : 0);
      logwarn("removing broken %s", eff_path);
      dd_erase(eff_path);
    }
  }
  if (vrom)
    add_vromfs(vrom, insert_fist, mnt);
  else if (dd_file_exists(vname))
    DAG_FATAL("%s read error '%s': content is inconsistent", "vrom", vname);
}

RAIIVrom::~RAIIVrom()
{
  if (vrom)
  {
    remove_vromfs(vrom);
    mem->free(vrom);
  }
}


updater::Version get_game_build_version()
{
  const updater::Version gameVersion{
    get_vromfs_dump_version(String(0, "content/%s/%s-game.vromfs.bin", get_game_name(), get_game_name()))};
  const updater::Version exeVersion{get_exe_version32()};
  return max(gameVersion, exeVersion);
}


updater::Version get_updated_game_version() { return max(get_game_build_version(), mounted_vroms_version); }


bool check_vromfs_version_newer(updater::Version base_ver, updater::Version upd_ver)
{
#if DAGOR_DBGLEVEL > 0
  if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("forceOnlineBinariesAlways", false))
    return true;
#endif

  return base_ver.isCompatible(upd_ver) && upd_ver > base_ver;
}

String get_eff_vrom_fpath(const char *name)
{
#if DAGOR_DBGLEVEL > 0
  // In order to reproduce case when the server and the client has different vromfs
  // This is an often case for the production
  // Run the server with --force_vromfs_folder server_vroms_folder and the server
  // Will use set of vromfs from given folder.
  if (const char *forceVromfsFolder = dgs_get_argv("force_vromfs_folder", nullptr))
  {
    String ded_vrom(0, "%s/%s", forceVromfsFolder, name);
    if (dd_file_exists(ded_vrom))
    {
      debug("vromfs.sel: %s", ded_vrom);
      return ded_vrom;
    }
  }
#endif

  const eastl::string &cacheFolder = updater::binarycache::get_cache_folder();
  if (cacheFolder.empty())
    return String(name);

  String upd_vrom(0, "%s/%s", cacheFolder.c_str(), name);
  if (!dd_file_exists(upd_vrom))
    return String(name);

  const updater::Version base_ver{get_vromfs_dump_version(name)};
  const updater::Version upd_ver{get_vromfs_dump_version(upd_vrom)};
  bool use_newer = check_vromfs_version_newer(base_ver, upd_ver);
  debug("vromfs.sel: %s baseVer=%s updVer=%s, using %s", name, base_ver.to_string(), upd_ver.to_string(), use_newer ? "UPD" : "base");

  return use_newer ? upd_vrom : String(name);
}

static int remount_all_vroms_impl(const eastl::bitvector<> *remount_filter)
{
  const eastl::string &cacheFolder = updater::binarycache::get_cache_folder();
  if (cacheFolder.empty())
    return 0;

  const updater::Version cacheVersion = updater::binarycache::get_cache_version();
  if (!cacheVersion || mounted_vroms_version == cacheVersion)
  {
    debug("Skip remount all vroms with version: %s", cacheVersion.to_string());
    return 0;
  }

  mounted_vroms_version = cacheVersion;
  debug("Remount all vroms with version: %s", mounted_vroms_version.to_string());

  struct VromRemountRequest
  {
    String upd_vrom;
    updater::Version upd_ver;
    updater::Version vrom_ver;
    VirtualRomFsData *vromPtr;
    size_t vromIdx;
    size_t mountIdx;
    bool isPack;
  };

  eastl::vector<VromRemountRequest> vromsToRemount;
  uint32_t remountsCount = 0;
  iterate_vroms([&](VirtualRomFsData *vrom, size_t idx) {
    for (size_t i = 0; i < mnt_vromfs.size(); ++i)
    {
      if (remount_filter && !remount_filter->test(i, false /* defaultValue */))
        continue;

      MountVromfsRec &mnt = mnt_vromfs[i];
      if (vrom == mnt.vromfsData.get())
      {
        String upd_vrom(0, "%s/%s", cacheFolder.c_str(), mnt.path);
        updater::Version upd_ver{get_vromfs_dump_version(upd_vrom)};
        updater::Version vrom_ver{vrom->version};

        // Remount to any vrom from the cache in case the version is different
        // In order to support revert we should always prefer remote's version of a vrom
        if (dd_file_exists(upd_vrom) && vrom_ver != upd_ver)
        {
          ++remountsCount;
          VromRemountRequest req{upd_vrom, upd_ver, vrom_ver, vrom, idx, i, false};
          vromsToRemount.push_back(req);
        }
        break;
      }
      else if (vrom == mnt.vromfsPack.get())
      {
        String upd_vrom(0, "%s/%s", cacheFolder.c_str(), mnt.path);
        updater::Version upd_ver{get_vromfs_dump_version(upd_vrom)};
        updater::Version vrom_ver{vrom->version};
        if (dd_file_exists(upd_vrom) && check_vromfs_version_newer(vrom_ver, upd_ver))
        {
          VromRemountRequest req{upd_vrom, upd_ver, vrom_ver, vrom, idx, i, true};
          vromsToRemount.push_back(req);
        }
        break;
      }
    }
    return true; // continue
  });

  for (const VromRemountRequest &req : vromsToRemount)
  {
    MountVromfsRec &mnt = mnt_vromfs[req.mountIdx];
    if (!req.isPack)
    {
      syncvroms::LoadedSyncVrom loadedVrom{syncvroms::load_vromfs_dump(req.upd_vrom, create_vromfs_compression(req.upd_vrom.c_str()))};
      debug("remount vrom(%s), ver=%s -> %s, hash= '%s'", req.upd_vrom, req.vrom_ver.to_string(), req.upd_ver.to_string(),
        loadedVrom.hash.c_str());
      G_VERIFY(replace_vromfs(req.vromIdx, loadedVrom.fsData) == req.vromPtr);
      memfree(mnt.vromfsData.release(), inimem);
      mnt.vromfsData.reset(loadedVrom.fsData);
      mnt.hash = loadedVrom.hash;
    }
    else
    {
      debug("remount vpack(%s), ver=%s -> %s", req.upd_vrom, req.vrom_ver.to_string(), req.upd_ver.to_string());
      VirtualRomFsPack *new_vrom = ::open_vromfs_pack(req.upd_vrom, midmem, DF_IGNORE_MISSING);
      G_VERIFY(replace_vromfs(req.vromIdx, new_vrom) == req.vromPtr);
      close_vromfs_pack(mnt.vromfsPack.release(), midmem);
      mnt.vromfsPack.reset(new_vrom);
    }
  }

  return remountsCount;
}


void reset_vroms_changed_state()
{
  changed_vromfs.resize(mnt_vromfs.size());
  eastl::fill(changed_vromfs.begin(), changed_vromfs.end(), false);
}


void mark_vrom_as_changed_by_name(const char *name)
{
  using TmpString = eastl::basic_string<char, framemem_allocator>;
  const TmpString fullName{TmpString::CtorSprintf{}, "%s.vromfs.bin", name};

  const eastl_size_t vromsCount = mnt_vromfs.size();
  if (changed_vromfs.size() < vromsCount)
    changed_vromfs.resize(vromsCount, false);

  for (int i = 0; i < vromsCount; ++i)
    if (fullName == dd_get_fname(mnt_vromfs[i].path.c_str()))
      return changed_vromfs.set(i, true);
}


int remount_all_vroms() { return remount_all_vroms_impl(nullptr /* remount_filter */); }


int remount_changed_vroms()
{
  const int remountsCount = remount_all_vroms_impl(&changed_vromfs);
  reset_vroms_changed_state();
  return remountsCount;
}


void install_vromfs_signature_check(signature_checker_factory_cb sig) { vromfs_sig_check = sig; }

static bool is_valid_ugm_vromfs(dag::ConstSpan<char> data)
{
  if (data_size(data) < sizeof(VirtualRomFsDataHdr))
    return false;

  const VirtualRomFsDataHdr *hdr = (const VirtualRomFsDataHdr *)data.data();
  return hdr->label == _MAKE4C('VRFs') || hdr->label == _MAKE4C('VRFx');
}

bool register_data_as_single_file_in_virtual_vrom(dag::ConstSpan<char> data, const char *mount_dir, const char *rel_fname)
{
  // load as vrom dump or register data as single file in virtual vrom
  VirtualRomFsData *vrom = is_valid_ugm_vromfs(data)
                             ? load_vromfs_dump_from_mem(data, inimem)
                             : VirtualRomFsSingleFile::make_mem_data(inimem, (void *)data.data(), data.size(), rel_fname, true);

  if (!vrom)
  {
    logerr("failed to create virtual vrom to expose data=(%p,%d) as file %s in %s dir", data.data(), data.size(), rel_fname,
      mount_dir);
    return false;
  }
  typename decltype(mnt_single_file)::insert_return_type ins =
    mnt_single_file.emplace(String(0, "%s/%s", mount_dir, rel_fname).str(), String(0, "%s/", mount_dir).str());

  if (ins.second)
  {
    ins.first->vromfsData.reset(vrom);
    add_vromfs(vrom, /*insert_first*/ false, ins.first->mount);
    debug("registered blob=(%p,%d) as virtual file \"%s/%s\"", data.data(), data.size(), mount_dir, rel_fname);
    return true;
  }

  logwarn("single file %s/%s already mounted, ignore", mount_dir, rel_fname);
  memfree(vrom, inimem);
  return false;
}
bool unregister_data_as_single_file_in_virtual_vrom(const char *mount_dir, const char *rel_fname)
{
  String fn(0, "%s/%s", mount_dir, rel_fname);
  for (MountVromfsRec &r : mnt_single_file)
    if (strcmp(r.path, fn) == 0)
    {
      debug("unregistered virtual file \"%s\"", r.path);
      mnt_single_file.erase(r);
      return true;
    }
  return false;
}
void unregister_all_virtual_vrom()
{
  if (mnt_single_file.size())
    debug("%s: removing %d virtual files", __FUNCTION__, mnt_single_file.size());
  decltype(mnt_single_file)().swap(mnt_single_file);
}

const eastl::string &updater::binarycache::get_cache_folder()
{
  static eastl::unique_ptr<eastl::string> cacheFolder;

  if (cacheFolder)
    return *cacheFolder;

  cacheFolder.reset(new eastl::string{
    eastl::string::CtorSprintf{}, "%s%cupdate-%s", folders::get_downloads_dir().c_str(), PATH_DELIM, ::get_game_name()});
  *cacheFolder = fs::normalize_path(cacheFolder->c_str());

  debug("Init cache folder: %s", cacheFolder->c_str());

  return *cacheFolder;
}


const eastl::string &updater::binarycache::get_remote_folder()
{
  static eastl::unique_ptr<eastl::string> remoteFolder;

  if (remoteFolder)
    return *remoteFolder;

  const eastl::string &cacheFolder{binarycache::get_cache_folder()};
  remoteFolder.reset(new eastl::string{eastl::string::CtorSprintf{}, "%s%c_remote", cacheFolder.c_str(), PATH_DELIM});
  *remoteFolder = fs::normalize_path(remoteFolder->c_str());

  debug("Init remote folder: %s", remoteFolder->c_str());

  return *remoteFolder;
}


updater::Version updater::binarycache::get_cache_version() { return get_cache_version(binarycache::get_cache_folder().c_str()); }
