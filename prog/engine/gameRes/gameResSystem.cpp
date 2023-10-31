#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_
#endif

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_grpMgr.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_gameResHooks.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_texPackMgr2.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_vromfs.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_fastSeqRead.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_memIo.h>
#include <errno.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_hierBitArray.h>
#include <util/dag_bitArray.h>
#include <util/dag_fastIntList.h>
#include <util/dag_globDef.h>
#include <util/dag_texMetaData.h>
#include <perfMon/dag_perfTimer.h>
#include "grpData.h"
#include <stdio.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/string.h>

#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <util/dag_fastNameMapTS.h>

#if _TARGET_PC_WIN
#include <direct.h>
extern "C" __declspec(dllimport) unsigned long /*DWORD*/ __stdcall /*WINAPI*/ GetLastError();
#elif _TARGET_PC
#include <unistd.h>
#endif

#define TRACE_RES_MANAGEMENT 0

#if TRACE_RES_MANAGEMENT
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <memory/dag_memStat.h>
#define TRACE debug

#else
static inline void TRACE(const char *, ...) {}

#endif

#define LOGLEVEL_DEBUG _MAKE4C('GRSS')

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

static Bitarray resRestrictionList;
static FastIntList requiredResList;
static Tab<GameResourceFactory *> factories(inimem_ptr());
static OAHashNameMap<true> *avail_res_files = NULL;

static bool addReqResListReferences(int res_id, Bitarray &restr_list, FastIntList &req_list, int parent_cls = -1);
static void add_rendinst_implicit_ref(int res_id, Bitarray &restr_list, FastIntList &req_list);

DataBlock gameres_rendinst_desc;
DataBlock gameres_dynmodel_desc;

static WinCritSec gameres_cs;
static WinCritSec gameres_load_cs;

WinCritSec &get_gameres_main_cs() { return gameres_cs; }

static bool gameres_finer_load_enabled = false;

static bool noFactoryFatal = true;
struct SetScopeNoFactoryFatal
{
  bool prev;
  SetScopeNoFactoryFatal(bool set) : prev(noFactoryFatal) { noFactoryFatal = set; }
  ~SetScopeNoFactoryFatal() { noFactoryFatal = prev; }
};
static int now_loading_res_id = -1;
static enum { OGLE_ALWAYS, OGLE_ONE_ENABLED, OGLE_NOT_ENABLED } one_grp_load_enabled = OGLE_ALWAYS;
static bool ignoreUnavailableResources = false;
static bool loggingMissingResources = true;

bool is_ignoring_unavailable_resources() { return ignoreUnavailableResources; }

static int gameresSysVer = 2;

void set_gameres_sys_ver(int ver)
{
  G_ASSERT(ver == 2);
  gameresSysVer = ver;
}
int get_gameres_sys_ver() { return gameresSysVer; }

namespace gameresprivate
{
DataBlock *recListBlk = NULL;
const char *gameResPrefix = NULL, *ddsxTexPrefix = NULL;
int curRelFnOfs = 0;
bool gameResPatchInProgress = false;
Tab<int> patchedGameResIds;
FastNameMapTS<false> resNameMap;

void scanGameResPack(const char *filename);
void scanDdsxTexPack(const char *filename);
DataBlock *getDestBlock(const char *filename, bool grp);
} // namespace gameresprivate

void add_factory(GameResourceFactory *fac)
{
  if (!fac)
    return;

  for (int i = 0; i < factories.size(); ++i)
  {
    if (factories[i] == fac)
    {
      // debug("GameResourceFactory %s is already registered", fac->getResClassName());
      return;
    }

    if (factories[i]->getResClassId() == fac->getResClassId())
    {
      fatal("Conflicting GameResourceFactory id 0x%X for %s (%p) and %s (%p)", fac->getResClassId(), factories[i]->getResClassName(),
        factories[i], fac->getResClassName(), fac);
      return;
    }
  }

  factories.push_back(fac);
}


void remove_factory(GameResourceFactory *fac)
{
  if (!fac)
    return;

  for (int i = 0; i < factories.size(); ++i)
    if (factories[i] == fac)
    {
      erase_items(factories, i, 1);
      break;
    }
}


static GameResourceFactory *getFactoryByClassId(int id)
{
  if (!id)
    return NULL;
  for (int i = 0; i < factories.size(); ++i)
    if (factories[i]->getResClassId() == id)
      return factories[i];

  return NULL;
}

GameResourceFactory *find_gameres_factory(unsigned clsid) { return getFactoryByClassId(clsid); }

static void getResClassName(unsigned class_id, String &name)
{
  GameResourceFactory *fac = getFactoryByClassId(class_id);
  if (fac)
    name = fac->getResClassName();
  else
    name.printf(32, "<classId=0x%X>", class_id);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

using gameresprivate::resNameMap;

void repack_real_game_res_id_table() { resNameMap.shrink_to_fit(); }

static int addGameResId(const char *name)
{
  if (!name || !*name)
    return NULL_GAMERES_ID;
  return ::resNameMap.addNameId(name);
}

static void getGameResName(int id, String &name)
{
  if (id == NULL_GAMERES_ID)
    name = "<null>";
  else
  {
    const char *n = ::resNameMap.getName(id);
    if (n)
      name = n;
    else if (!gamereshooks::on_get_res_name || !gamereshooks::on_get_res_name(id, name))
      name.printf(32, "<id=%d>", id);
  }
}


void get_game_resource_name(int id, String &name) { return ::getGameResName(id, name); }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


struct GameResPackInfo
{
  SimpleString fileName;
  gamerespackbin::GrpData *grData;
  int refCount;
  bool surelyLoaded;

  GameResPackInfo() : grData(NULL), refCount(0), surelyLoaded(false) {}

  bool processGrData();

  void loadPack();

  void endLoading()
  {
    if (grData)
    {
      memfree(grData, inimem);
      grData = NULL;
    }
  }

  void addRefResIds(int res_id, Tab<int> &res_ids);
};


static Tab<GameResPackInfo> packInfo(inimem_ptr());

static Tab<int> loadedPacks(tmpmem_ptr());


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//
static Tab<int> resId_to_grInfo(inimem);
static Tab<int> resId_to_grMap(inimem);
static Tab<int> resId_to_packId(inimem);

struct GameResId
{
  union
  {
    struct
    {
      int32_t classId;
      int32_t resId;
    };
    uint64_t key;
  };

  GameResId(int32_t clsid, int32_t resid) : classId(clsid), resId(resid) {}

  GameResId(uint64_t k) : key(k) {}
};

struct GameResMap
{
  GameResId id;
  int packId;

  GameResMap(int cid, int rid, int pid, int map_idx) : id(cid, rid), packId(pid)
  {
    if (rid >= resId_to_grMap.size())
    {
      int s = resId_to_grMap.size();
      resId_to_grMap.resize(rid + 1);
      mem_set_ff(make_span(resId_to_grMap).subspan(s, rid - s));
    }
    if (rid >= resId_to_packId.size())
    {
      int s = resId_to_packId.size();
      resId_to_packId.resize(rid + 1);
      mem_set_ff(make_span(resId_to_packId).subspan(s, rid - s));
    }
    resId_to_grMap[rid] = map_idx;
    resId_to_packId[rid] = pid;
  }
};


struct GameResInfo
{
  int resId, grMapIdx, packId;
  unsigned int refStartIdx, refNum;

  GameResInfo(int rid, int iid, int pid, int rbase, int rnum, int map_idx) :
    resId(rid), grMapIdx(iid), packId(pid), refStartIdx(rbase), refNum(rnum)
  {
    if (rid >= resId_to_grInfo.size())
    {
      int s = resId_to_grInfo.size();
      resId_to_grInfo.resize(rid + 1);
      mem_set_ff(make_span(resId_to_grInfo).subspan(s, rid - s));
    }
    resId_to_grInfo[rid] = map_idx;
  }
};


static Tab<GameResMap> grMap(inimem_ptr());
static Tab<GameResInfo> grInfo(inimem_ptr());
static Tab<int> grInfoRefs(inimem);

static inline GameResInfo *getGameResInfo(int res_id)
{
  if (res_id < 0 || res_id >= resId_to_grInfo.size())
    return NULL;
  int idx = resId_to_grInfo[res_id];
  return idx >= 0 ? &grInfo[idx] : NULL;
}

static int gameResHandleToId(GameResHandle handle, unsigned class_id)
{
  int id = -1;
  if (gamereshooks::resolve_res_handle && gamereshooks::resolve_res_handle(handle, class_id, id))
    return id;
  id = ::addGameResId((const char *)handle);
  if (class_id && class_id != 0xFFFFFFFFU)
  {
    GameResInfo *info = getGameResInfo(id);
    if (!info || grMap[info->grMapIdx].id.classId != class_id)
      return -1;
  }
  // debug_ctx("handle <%s> -> id=%d", (const char*)handle, id);
  return id;
}

int gamereshooks::aux_game_res_handle_to_id(GameResHandle h, unsigned cid) { return gameResHandleToId(h, cid); }

void iterate_gameres_names_by_class(unsigned class_id, const eastl::function<void(const char *)> &cb)
{
  ::resNameMap.iterate([class_id, &cb](int id, const char *name) -> void {
    GameResInfo *info = getGameResInfo(id);
    if (info && grMap[info->grMapIdx].id.classId == class_id)
      cb(name);
  });
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static int addGameRes(unsigned class_id, int res_id, int pack_id)
{
  int i = res_id < resId_to_grMap.size() ? resId_to_grMap[res_id] : -1;
  if (i < 0)
  {
    grMap.push_back(GameResMap(class_id, res_id, pack_id, grMap.size()));
    return resId_to_grMap[res_id];
  }

  G_ASSERTF(grMap[i].id.classId == class_id, "resId=%d (%s) class changes 0x%X -> 0x%X", //
    res_id, ::resNameMap.getName(res_id), grMap[i].id.classId, class_id);
  // we already know about this real-res
  if (pack_id < 0)
    return i;

  if (grMap[i].packId < 0)
  {
    grMap[i].packId = pack_id;
    resId_to_packId[res_id] = pack_id;
  }
  else if (gameresprivate::gameResPatchInProgress) // may override with patch
  {
    // String resName;
    //::getGameResName(res_id, resName);
    // debug("patch: res <%s>  taken from %s (instead of %s)",
    //   resName, packInfo[pack_id].fileName, packInfo[grMap[i].packId].fileName);

    grMap[i].packId = pack_id;
    resId_to_packId[res_id] = pack_id;
    gameresprivate::patchedGameResIds.push_back(res_id);
  }

  return i;
}


static void addGameResInfo(int res_id, unsigned class_id, int pack_id, int ref_base, int ref_num)
{
  // debug_ctx("add GR <%s> id=%d", resNameMap.getName(res_id), res_id);
  int i = res_id < resId_to_grInfo.size() ? resId_to_grInfo[res_id] : -1;
  if (i < 0)
  {
    int grMapId = addGameRes(class_id, res_id, -1);
    grInfo.push_back(GameResInfo(res_id, grMapId, pack_id, ref_base, ref_num, grInfo.size()));
    return;
  }

  if (gameresprivate::gameResPatchInProgress) // may override with patch
  {
    // String resName;
    //::getGameResName(res_id, resName);
    // debug("patch*: res <%s>  taken from %s (instead of %s)",
    //   resName, packInfo[pack_id].fileName, packInfo[grInfo[i].packId].fileName);

    grInfo[i].packId = pack_id;
    grInfo[i].refStartIdx = ref_base;
    grInfo[i].refNum = ref_num;
    return;
  }

  // check that real-res reference is the same
  GameResMap &grEntry = grMap[grInfo[i].grMapIdx];
  if (grEntry.id.classId != class_id || grEntry.id.resId != res_id)
  {
    String resName, resName2, className1, className2;

    ::getGameResName(res_id, resName);
    ::getResClassName(class_id, className1);
    ::getGameResName(grEntry.id.resId, resName2);
    ::getResClassName(grEntry.id.classId, className2);

    fatal("Invalid game resource packs:\n"
          "resource %s from %s is %s:%s\n"
          "but the same resource from %s is %s:%s",
      (char *)resName, (char *)packInfo[grInfo[i].packId].fileName, (char *)className1, (char *)resName,
      (char *)packInfo[pack_id].fileName, (char *)className2, (char *)resName2);
  }
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

void patch_res_pack_paths(const char *prefix_path)
{
  G_ASSERT(prefix_path);
  char pathBuf[DAGOR_MAX_PATH];
  for (int i = 0; i < packInfo.size(); ++i)
  {
    _snprintf(pathBuf, sizeof(pathBuf), "%s/%s", prefix_path, packInfo[i].fileName.str());
    pathBuf[sizeof(pathBuf) - 1] = 0;
    packInfo[i].fileName = pathBuf;
  }
}

void gameresprivate::scanGameResPack(const char *filename)
{
  using namespace gamerespackbin;
  //  debug_ctx("  scanning pack %s", filename);

  if (avail_res_files && avail_res_files->getNameId(filename) < 0)
  {
    logwarn("Skip missing GRP: %s", filename);
    return;
  }
#if _TARGET_ANDROID | _TARGET_IOS | _TARGET_TVOS
  if (!dd_file_exists(filename))
  {
    logwarn("Skip scanning missing GRP: %s", filename);
    return;
  }
#endif

  String cache_fname(0, "%scache.bin", filename);
  VromReadHandle dump_data = ::vromfs_get_file_data(cache_fname);
  VromReadHandle vrom_data = ::vromfs_get_file_data(filename);
  FullFileLoadCB cb(dump_data.data() && !vrom_data.data() ? cache_fname.str() : filename);
#if _TARGET_PC && DAGOR_DBGLEVEL > 0
  if (dump_data.data())
  {
    const DataBlock *settings = dgs_get_settings();
    if (!settings || settings->getBlockByNameEx("debug")->getBool("checkPacksConsistency", true))
    {
      FullFileLoadCB grp_crd(filename);
      SmallTab<char, TmpmemAlloc> grp_hdr;
      clear_and_resize(grp_hdr, dump_data.size());
      if (grp_crd.tryRead(grp_hdr.data(), grp_hdr.size()) != grp_hdr.size())
      {
        fatal("inconsistent cache for %s (short grp)", filename);
        cb.open(filename, DF_READ);
      }
      if (!mem_eq(dump_data, grp_hdr.data()))
      {
        fatal("inconsistent cache for %s (grp header differs)", filename);
        cb.open(filename, DF_READ);
      }
    }
  }
#endif

  if (!cb.fileHandle)
  {
    debug("Can't open GameResPack file %s", filename);
    return;
  }

  GrpHeader ghdr;
  GrpData *gdata = NULL;

  DAGOR_TRY
  {
    // check id
    cb.read(&ghdr, sizeof(ghdr));
    if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
    {
      debug("no GRP2 label (hdr: 0x%x 0x%x 0x%x 0x%x)", ghdr.label, ghdr.descOnlySize, ghdr.fullDataSize, ghdr.restFileSize);
#if (DAGOR_DBGLEVEL < 1) && DAGOR_FORCE_LOGS
      fatal("no GRP2 label in %s", filename);
#endif
      DAGOR_THROW(IGenLoad::LoadException("no GRP2 label", cb.tell()));
    }
    if (!dump_data.data() && ghdr.restFileSize + sizeof(ghdr) != df_length(cb.fileHandle))
    {
      debug("Corrupt file: hdr+restFileSize=%u != filesz=%d", unsigned(ghdr.restFileSize + sizeof(ghdr)), df_length(cb.fileHandle));
#if (DAGOR_DBGLEVEL < 1) && DAGOR_FORCE_LOGS
      fatal("Corrupt file %s", filename);
#endif
      DAGOR_THROW(IGenLoad::LoadException("Corrupt file: restFileSize", cb.tell()));
    }

    gdata = (GrpData *)memalloc(ghdr.fullDataSize, tmpmem);
    cb.read(gdata, ghdr.fullDataSize);
    gdata->patchDescOnly(ghdr.label);
    gdata->patchData();

    // add pack
    int packId = append_items(packInfo, 1);
    GameResPackInfo &pack = packInfo[packId];
    pack.fileName = filename;
    DataBlock *regDest = gameresprivate::getDestBlock(filename, true);

    // reserve memory for mapping new resIds
    resId_to_grInfo.reserve(resId_to_grInfo.size() + gdata->resTable.size());
    resId_to_grMap.reserve(resId_to_grMap.size() + gdata->resTable.size());
    resId_to_packId.reserve(resId_to_packId.size() + gdata->resTable.size());

    // scan real-res
    const ResEntry *rre = gdata->resTable.data();
    for (int i = gdata->resTable.size(); i > 0; rre++, i--)
      ::addGameRes(rre->classId, ::addGameResId(gdata->getName(rre->resId)), packId);

    // scan res & copy dependency info
    grInfo.reserve(gdata->resData.size());

    for (int i = 0; i < gdata->nameMap.size(); i++)
      gdata->nameMap[i] = ::addGameResId(gdata->getName(i));

    dag::ConstSpan<int> gdNameId = gdata->nameMap;
    const ResData *rd = gdata->resData.data(), *rd_end = rd + gdata->resData.size();
    for (; rd != rd_end; rd++)
    {
      int base = append_items(grInfoRefs, rd->refResIdCnt);

      for (int i = 0; i < rd->refResIdCnt; i++)
        grInfoRefs[base + i] = (rd->refResIdPtr[i] != 0xFFFF) ? gdNameId[rd->refResIdPtr[i]] : -1;

      ::addGameResInfo(gdNameId[rd->resId], rd->classId, packId, base, rd->refResIdCnt);
      if (regDest)
      {
        const char *resName = ::resNameMap.getName(gdNameId[rd->resId]);
        if (resName)
        {
          if (gameresprivate::gameResPatchInProgress)
            regDest->setInt(resName, rd->classId);
          else
            regDest->addInt(resName, rd->classId);
        }
      }
    }
    memfree(gdata, tmpmem);
  }
  DAGOR_CATCH(IGenLoad::LoadException)
  {
    debug("Error reading GameResPack file %s", filename);
#if DAGOR_DBGLEVEL <= 0
    DAGOR_RETHROW();
#endif
  }
}

void gameresprivate::scanDdsxTexPack(const char *filename)
{
  if (avail_res_files && avail_res_files->getNameId(filename) < 0)
  {
    logwarn("Skip missing DxP: %s", filename);
    return;
  }

  DataBlock *regDest = gameresprivate::getDestBlock(filename, false);
  static constexpr int BITS_PER_TEXID = 16 + 1;
  ConstSizeBitArray<BITS_PER_TEXID> *used = NULL;

  if (regDest)
  {
    used = new ConstSizeBitArray<BITS_PER_TEXID>;
    used->ctor();
    for (TEXTUREID id = first_managed_texture(0); id != BAD_TEXTUREID; id = next_managed_texture(id, 0))
      used->set(id.index());
  }

  bool ret = ddsx::load_tex_pack2(filename, NULL, gameresprivate::gameResPatchInProgress);
  if (!ret)
    logerr("cannot load texpack: '%s'", filename);

  if (ret && regDest && used)
  {
    for (TEXTUREID id = first_managed_texture(0); id != BAD_TEXTUREID; id = next_managed_texture(id, 0))
    {
      if (used->get(id.index()))
        continue;
      const char *tex_nm = get_managed_texture_name(id);
      TextureMetaData tmd;
      SimpleString nm(tmd.decode(tex_nm));
      if (nm.empty() || nm[strlen(nm) - 1] != '*')
        continue;

      nm[strlen(nm) - 1] = '\0';
      if (gameresprivate::gameResPatchInProgress)
        regDest->setInt(nm, 0);
      else
        regDest->addInt(nm, 0);
    }
  }
  if (used)
    delete used;
}

DataBlock *gameresprivate::getDestBlock(const char *filename, bool grp)
{
  if (!recListBlk)
    return NULL;

  const char *prefix = grp ? gameResPrefix : ddsxTexPrefix;
  if (!prefix)
    return NULL;

  String s(0, "%s/%s", prefix, filename + curRelFnOfs);
  dd_simplify_fname_c(s);
  s.resize(strlen(s) + 1);

  const char *ext = dd_get_fname_ext(s);
  if (ext)
  {
    if (dd_stricmp(ext, ".grp") == 0)
      erase_items(s, ext - s.data(), strlen(ext));
    else if (dd_stricmp(ext, ".bin") == 0)
    {
      erase_items(s, ext - s.data(), strlen(ext));
      ext = dd_get_fname_ext(s);
      if (ext)
        if (dd_stricmp(ext, ".dxp") == 0)
          erase_items(s, ext - s.data(), strlen(ext));
    }
  }

  DataBlock *blk = recListBlk;
  char *p = s, *p2 = strchr(p, '/');
  while (p2)
  {
    *p2 = '\0';
    blk = blk->addBlock(p);
    p = p2 + 1;
    p2 = strchr(p, '/');
  }
  return blk->addBlock(p);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


bool GameResPackInfo::processGrData()
{
  using namespace gamerespackbin;

  if (!grData)
    return false;

  fatal_context_push(fileName);
  debug_ctx("processing data from GRP %s", (char *)fileName);
  int this_packId = this - packInfo.data();

  const ResData *rd = grData->resData.data(), *rd_end = rd + grData->resData.size();
  for (; rd != rd_end; rd++)
  {
    int resId = grData->nameMap[rd->resId];
    // create res
    GameResInfo *info = ::getGameResInfo(resId);
    if (!info)
      continue;

    if (resRestrictionList.size() && !resRestrictionList.get(resId))
      continue;
    if (info->packId != this_packId)
      continue;

    int classId = grMap[info->grMapIdx].id.classId;
    GameResourceFactory *fac = ::getFactoryByClassId(classId);

    String resName;
    ::getGameResName(resId, resName);
    if (!fac)
    {
      String className;
      ::getResClassName(classId, className);

      if (noFactoryFatal)
        fatal("No factory for resource %s:%s", className.str(), resName.str());
      else
        logwarn("No factory for resource %s:%s", className.str(), resName.str());

      continue;
    }

    fatal_context_push(String(0, "%s : %s", resName, fac->getResClassName()));
    now_loading_res_id = resId;
    fac->createGameResource(resId, info->refNum ? &grInfoRefs[info->refStartIdx] : NULL, info->refNum);
    now_loading_res_id = -1;
    fatal_context_pop();
    if (gameres_finer_load_enabled)
    {
      int cnt = gameres_cs.fullUnlock();
      sleep_msec(0);
      gameres_cs.reLock(cnt);
    }
    G_ASSERTF_BREAK(grData, "grData unexpectedly became %p", grData);
  }

  debug_ctx("processed data from GRP %s", (char *)fileName);
  fatal_context_pop();

  return true;
}

void GameResPackInfo::addRefResIds(int res_id, Tab<int> &res_ids)
{
  using namespace gamerespackbin;

  if (!grData)
    return;

  dag::ConstSpan<int> gdNameId = grData->nameMap;
  const ResData *rd = grData->resData.data(), *rd_end = rd + grData->resData.size();
  for (; rd != rd_end; rd++)
  {
    int resId = gdNameId[rd->resId];
    if (resId == res_id)
    {
      int base = append_items(res_ids, rd->refResIdCnt);
      for (int i = 0; i < rd->refResIdCnt; i++)
        res_ids[base + i] = gdNameId[rd->refResIdPtr[i]];
      break;
    }
  }
}

void GameResPackInfo::loadPack()
{
  if (!refCount)
    TRACE("===+ load extraneous GRP: %s\n", fileName.str());

  int64_t reft = profile_ref_ticks();
  using namespace gamerespackbin;
  FastSeqReadCB seq_cb;
  VromReadHandle vrom_data = ::vromfs_get_file_data(fileName.str());
  InPlaceMemLoadCB vrom_cb(vrom_data.data(), data_size(vrom_data));
  IGenLoad &cb = vrom_data.data() ? static_cast<IGenLoad &>(vrom_cb) : static_cast<IGenLoad &>(seq_cb);
  int this_packId = this - packInfo.data();

  if (!vrom_data.data() && !seq_cb.open(fileName, 32 << 10))
  {
    int errn = -1;
    const char *errs = NULL, *cwd = NULL;
#if _TARGET_PC
#if _TARGET_PC_WIN
    errn = GetLastError();
#else
    errn = errno;
    errs = strerror(errn);
#endif
    char cwdbuf[256];
    cwd = getcwd(cwdbuf, sizeof(cwdbuf));
#endif
    fatal("Can't open GameResPack file '%s', error %d(%s), cwd '%s'", fileName.str(), errn, errs, cwd);
    return;
  }
  debug_ctx("loading GRP %s", (char *)fileName);
  int file_sz = vrom_data.data() ? data_size(vrom_data) : seq_cb.getSize();

#if TRACE_RES_MANAGEMENT
  size_t sys_mem = dagor_memory_stat::get_memory_allocated();
  size_t gpu_mem = d3d::driver_command(DRV3D_COMMAND_GETTEXTUREMEM, 0, 0, 0);
#endif
  static constexpr int RANGES_BUF_SZ = 64;
  FastSeqReader::Range rangesBuf[RANGES_BUF_SZ];
  FastSeqReader::Range *__restrict rb = rangesBuf, *__restrict rb_end = rb + RANGES_BUF_SZ;
  int data_size = 0;
  int waste_size = 0, waste_last_size = 0;
  int res_cnt = 0;

  DAGOR_TRY
  {
    String cache_fname(0, "%scache.bin", fileName.str());
    VromReadHandle dump_data = ::vromfs_get_file_data(cache_fname);

    if (!dump_data.data())
    {
      GrpHeader ghdr;

      // check id
      cb.read(&ghdr, sizeof(ghdr));
      if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
      {
        debug("no GRP2 label (hdr: 0x%x 0x%x 0x%x 0x%x)", ghdr.label, ghdr.descOnlySize, ghdr.fullDataSize, ghdr.restFileSize);
#if (DAGOR_DBGLEVEL < 1) && DAGOR_FORCE_LOGS
        fatal("no GRP2 label in %s", fileName.str());
#endif
        DAGOR_THROW(IGenLoad::LoadException("no GRP2 label", cb.tell()));
      }
      if (ghdr.restFileSize + sizeof(ghdr) != file_sz)
      {
        debug("Corrupt file: hdr+restFileSize=%u != filesz=%d", unsigned(ghdr.restFileSize + sizeof(ghdr)), file_sz);
#if (DAGOR_DBGLEVEL < 1) && DAGOR_FORCE_LOGS
        fatal("Corrupt file %s", fileName.str());
#endif
        DAGOR_THROW(IGenLoad::LoadException("Corrupt file: restFileSize", cb.tell()));
      }

      grData = (GrpData *)memalloc(ghdr.descOnlySize, inimem);
      cb.read(grData, ghdr.descOnlySize);
      grData->patchDescOnly(ghdr.label);
    }
    else
    {
      GrpHeader *__restrict ghdr = (GrpHeader *)dump_data.data();

      grData = (GrpData *)memalloc(ghdr->descOnlySize, inimem);
      memcpy(grData, dump_data.data() + sizeof(GrpHeader), ghdr->descOnlySize);
      grData->patchDescOnly(ghdr->label);
    }

    // register all names and renew nameMap
    for (int i = 0; i < grData->nameMap.size(); i++)
      grData->nameMap[i] = ::addGameResId(grData->getName(i));

    // create real-res
    dag::ConstSpan<int> gdNameId = grData->nameMap;
    const ResEntry *__restrict rre = grData->resTable.data(), *__restrict rre_end = rre + grData->resTable.size();

    for (; rre != rre_end; rre++)
    {
      if (rre->offset == 0)
        continue;
      int gdni = gdNameId[rre->resId];
      if (resRestrictionList.size() && ((uint32_t)gdni >= (uint32_t)resRestrictionList.size() || !resRestrictionList.get(gdni)))
        continue;
      if (resId_to_packId[gdni] != this_packId)
        continue;
      int st_p = rre->offset;
      int end_p = (rre + 1 == rre_end) ? file_sz : rre[1].offset;
      data_size += end_p - st_p;
      res_cnt++;

      if (rb == rangesBuf)
      {
        rb->start = st_p;
        rb->end = end_p;
        rb++;
        continue;
      }

      if (st_p > rb[-1].end + (32 << 10) && rb < rb_end)
      {
        rb->start = st_p;
        rb->end = end_p;
        rb++;
      }
      else
      {
        waste_size += st_p - rb[-1].end;
        if (rb == rb_end)
          waste_last_size += st_p - rb[-1].end;
        rb[-1].end = end_p;
      }
    }
    if (rb == rangesBuf)
      goto end_load;
    if (!vrom_data.data())
      seq_cb.setRangesOfInterest(make_span(rangesBuf, rb - rangesBuf));
    cb.seekto(rangesBuf[0].start);

    rre = grData->resTable.data();
    rre_end = rre + grData->resTable.size();
    for (; rre != rre_end; rre++)
    {
      if (rre->offset == 0)
        continue;

      int gdni = gdNameId[rre->resId];
      if (resRestrictionList.size() && ((uint32_t)gdni >= (uint32_t)resRestrictionList.size() || !resRestrictionList.get(gdni)))
        continue;

      // debug("read %s at %d", ::resNameMap.getName(gdNameId[rre->resId]), rre->offset);
      if (resId_to_packId[gdni] != this_packId)
        continue;
      cb.seekto(rre->offset);

      GameResourceFactory *fac = ::getFactoryByClassId(rre->classId);

      if (!fac)
      {
        String className, resName;

        ::getResClassName(rre->classId, className);
        ::getGameResName(gdNameId[rre->resId], resName);

        if (noFactoryFatal)
          fatal("No factory for game resource %s:%s", className.str(), resName.str());
        else
          logwarn("No factory for game resource %s:%s", className.str(), resName.str());

        continue;
      }

      fac->loadGameResourceData(gdNameId[rre->resId], cb);
      if (gameres_finer_load_enabled)
      {
        int cnt = gameres_cs.fullUnlock();
        sleep_msec(0);
        gameres_cs.reLock(cnt);
      }
    }

    // debug_ctx("loaded real-res from GRP %s", (char*)fileName);
  end_load:;
  }
  DAGOR_CATCH(IGenLoad::LoadException) { debug("Error reading GameResPack file %s", fileName.str()); }
  if (!vrom_data.data())
    seq_cb.close();

  // debug_ctx("loaded GRP %s", (char*)fileName);

  int t0 = profile_time_usec(reft);
#if TRACE_RES_MANAGEMENT
  TRACE("+++ loaded GRP %s, %d usec, %6.2f Mb/s, used sysmem: %zuK, dxmem: %dK\n", fileName.str(), t0,
    double(cb.getSize()) / (t0 ? t0 : 1), (dagor_memory_stat::get_memory_allocated() - sys_mem) >> 10,
    (gpu_mem - d3d::driver_command(DRV3D_COMMAND_GETTEXTUREMEM, 0, 0, 0)) >> 10);
#else
  if (vrom_data.data())
    debug("loaded GRP %s (from VROMFS), %d usec (%dK in %d res), %6.2f Mb/s", fileName.str(), t0, data_size >> 10, res_cnt,
      double(data_size) / (t0 ? t0 : 1));
  else
    debug("loaded GRP %s, %d usec (%dK of %dK range in %d areas, %d res, %dK waste load, %dK waste due to ranges), %6.2f Mb/s",
      fileName.str(), t0, data_size >> 10, (rb[-1].end - rangesBuf[0].start) >> 10, rb - rangesBuf, res_cnt, waste_size >> 10,
      waste_last_size >> 10, double(data_size) / (t0 ? t0 : 1));
#endif
  G_UNUSED(t0);

  // process loaded res-data
  processGrData();
  surelyLoaded = true;
}


static void loadGameResPack(int pack_id, int res_id)
{
  if (pack_id < 0)
    return;

  debug("loadGameResPack(%s) start", packInfo[pack_id].fileName.str());

  if (packInfo[pack_id].processGrData())
    goto end_;

  for (int i = 0; i < loadedPacks.size(); ++i)
    if (loadedPacks[i] == pack_id)
      goto end_;

  if (one_grp_load_enabled == OGLE_NOT_ENABLED)
  {
    if (!ignoreUnavailableResources)
      fatal("(!one_grp_load_enabled) loading of <%s> is not allowed, required for res=<%s>", packInfo[pack_id].fileName.str(),
        res_id < 0 ? "NULL" : ::resNameMap.getName(res_id));
    goto end_;
  }
  if (one_grp_load_enabled == OGLE_ONE_ENABLED)
    one_grp_load_enabled = OGLE_NOT_ENABLED;

  loadedPacks.push_back(pack_id);

  packInfo[pack_id].loadPack();

  debug("loadGameResPack(%s) end", packInfo[pack_id].fileName.str());

end_:;
}


static void clearLoadedPacksList()
{
  for (int i = 0; i < loadedPacks.size(); ++i)
    packInfo[loadedPacks[i]].endLoading();

  clear_and_shrink(loadedPacks);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


int validate_game_res_id(int res_id)
{
  if (gamereshooks::on_validate_game_res_id && gamereshooks::on_validate_game_res_id(res_id, res_id))
    return res_id;
  return getGameResInfo(res_id) ? res_id : NULL_GAMERES_ID;
}


int get_game_res_class_id(int res_id)
{
  if (gamereshooks::on_get_game_res_class_id)
  {
    unsigned class_id = 0;
    if (gamereshooks::on_get_game_res_class_id(res_id, class_id))
      return class_id;
  }

  GameResInfo *info = getGameResInfo(res_id);
  if (!info)
    return 0;

  return grMap[info->grMapIdx].id.classId;
}


GameResource *get_game_resource(int res_id)
{
  if (res_id == NULL_GAMERES_ID)
    return NULL;
  // debug_ctx("get game-res %d", res_id);
  if (gamereshooks::on_get_game_resource)
  {
    GameResource *gr = NULL;
    if (gamereshooks::on_get_game_resource(res_id, make_span(factories), gr))
      return gr;
  }

  GameResInfo *info = getGameResInfo(res_id);
  if (!info)
  {
    String name;
    ::getGameResName(res_id, name);
    debug("missing resource %s", name.str());
    return NULL;
  }

  int classId = grMap[info->grMapIdx].id.classId;

  GameResourceFactory *fac = ::getFactoryByClassId(classId);

  if (!fac)
  {
    if (noFactoryFatal)
      fatal("No GameResourceFactory for classId 0x%X", classId);
    else
      logwarn("No GameResourceFactory for classId 0x%X", classId);
    return NULL;
  }

  return fac->getGameResource(res_id);
}


void release_game_resource(int res_id)
{
  if (gamereshooks::on_release_game_resource && gamereshooks::on_release_game_resource(res_id, make_span(factories)))
    return;

  int classId = ::get_game_res_class_id(res_id);
  GameResourceFactory *fac = ::getFactoryByClassId(classId);

  if (!fac)
    return;

  fac->releaseGameResource(res_id);
}


struct LoadResPackEntryCnt
{
  LoadResPackEntryCnt() { interlocked_increment(count); }
  ~LoadResPackEntryCnt() { interlocked_decrement(count); }
  static int entryCount() { return interlocked_acquire_load(count); }
  static int count;
};
int LoadResPackEntryCnt::count = 0;

void load_game_resource_pack(int res_id)
{
  LoadResPackEntryCnt entry_count_check;
  G_ASSERTF_RETURN(entry_count_check.entryCount() < 64, , "%s: reentry=%d RRL.size=%d", __FUNCTION__, entry_count_check.entryCount(),
    resRestrictionList.size());
  // debug_ctx("load pack for game-res %d", res_id);
  if (gamereshooks::on_load_game_resource_pack && gamereshooks::on_load_game_resource_pack(res_id, make_span(factories)))
    return;

#if DAGOR_DBGLEVEL > 0
  int resId = ::validate_game_res_id(res_id);
  String name;
  get_game_resource_name(resId, name);

  if (now_loading_res_id != -1)
  {
    String fromName;
    get_game_resource_name(now_loading_res_id, fromName);
    debug("'%s' references '%s' that pulls GRP...", (const char *)fromName, (const char *)name);
  }
  else
  {
    debug("'%s' pulls GRP...", (const char *)name);
  }
#endif

  GameResInfo *info = getGameResInfo(res_id);
  if (!info)
    return;

  int resPackId = info->packId;

  d3d::LoadingAutoLock loadingLock;

  gameres_cs.lock();
  int gameres_cs_cnt = gameres_cs.fullUnlock() - 1;
  gameres_load_cs.lock();

  if (resRestrictionList.size() && !resRestrictionList.get(res_id))
  {
    logerr("res_id=%d <%s> is not present in res restriction list", res_id, resNameMap.getName(res_id));
    resRestrictionList.set(res_id); // for the case when we ignore next fatal in fatal handler
    resRestrictionList.set(grMap[info->grMapIdx].id.resId);
    clearLoadedPacksList();
  }

  static bool inLoading = false;

  bool isFirst = !inLoading;
  inLoading = true;

  ::loadGameResPack(resPackId, res_id);

  if (isFirst)
  {
    inLoading = false;
    clearLoadedPacksList();
  }

  gameres_load_cs.unlock();
  if (gameres_cs_cnt)
    gameres_cs.reLock(gameres_cs_cnt);
}

bool is_game_resource_pack_loaded(const char *fname)
{
  for (int i = 0; i < packInfo.size(); i++)
  {
    if (dd_stricmp(dd_get_fname(packInfo[i].fileName), fname) == 0)
    {
      return packInfo[i].surelyLoaded;
    }
  }
  fatal("is_game_resource_pack_loaded(%s): pack not found", fname);
  return false;
}

void load_game_resource_pack_by_name(const char *fname, bool only_one_gameres_pack /*= false*/)
{
  Tab<int> grp_id(tmpmem);
  bool name_found = false;

  for (int i = 0; i < packInfo.size(); i++)
  {
    if (dd_stricmp(dd_get_fname(packInfo[i].fileName), fname) == 0)
    {
      name_found = true;
      if (packInfo[i].surelyLoaded)
        continue;
      debug(" + %s", packInfo[i].fileName.str());
      grp_id.push_back(i);
    }
  }
  if (!name_found)
  {
    debug("no such GRPs: %s", fname);
    TRACE("no such GRPs: %s\n", fname);
  }

  if (!grp_id.size())
    return;

  debug("load_game_resource_pack_by_name(%s)", fname);
  TRACE("load_game_resource_pack_by_name(%s)\n", fname);
  if (only_one_gameres_pack)
    ::enable_one_gameres_pack_loading(true);
  for (int i = 0; i < grInfo.size(); i++)
    for (int j = grp_id.size() - 1; j >= 0; j--)
      if (grInfo[i].packId == grp_id[j])
      {
        WinAutoLock lock(gameres_cs);
        debug("preload <%s>", ::resNameMap.getName(grInfo[i].resId));
        release_game_resource(get_game_resource(grInfo[i].resId));
        if (packInfo[grp_id[j]].surelyLoaded)
          erase_items(grp_id, j, 1);
        break;
      }
  if (only_one_gameres_pack)
    ::enable_one_gameres_pack_loading(false, false);
  TRACE("load_game_resource_pack_by_name  finished\n");
}

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void set_no_gameres_factory_fatal(bool no_factory_fatal) { noFactoryFatal = no_factory_fatal; }

GameResource *get_game_resource(GameResHandle handle)
{
  WinAutoLock lock(gameres_cs);
  return ::get_game_resource(::gameResHandleToId(handle, 0));
}

GameResource *get_game_resource(GameResHandle handle, bool no_factory_fatal)
{
  WinAutoLock lock(gameres_cs);
#ifdef NO_3D_GFX
  SetScopeNoFactoryFatal nffGuard(false);
#else
  SetScopeNoFactoryFatal nffGuard(no_factory_fatal);
#endif
  return ::get_game_resource(::gameResHandleToId(handle, 0));
}


void game_resource_add_ref(GameResource *resource)
{
  if (!resource || intptr_t(resource) == -1)
    return;

  if (gameresSysVer == 2 && is_managed_texture_id_valid(D3DRESID(unsigned(uintptr_t(resource))), false))
  {
    TEXTUREID tid = (TEXTUREID)(uintptr_t)resource;
    const char *tex_name = ::get_managed_texture_name(tid);
    const char *asterisk = tex_name ? strchr(tex_name, '*') : NULL;
    if (asterisk && (asterisk[1] == 0 || asterisk[1] == '?'))
    {
      acquire_managed_tex(tid);
      return;
    }
    else
      debug("strange game_resource_add_ref(%p), name=%s", resource, tex_name);
  }


  WinAutoLock lock(gameres_cs);
  for (int i = 0; i < factories.size(); ++i)
    if (factories[i]->addRefGameResource(resource))
      return;
#if DAGOR_DBGLEVEL > 0
  fatal("res %p is not recognized by factories in game_resource_add_ref()", resource);
#endif
}


void release_gameres_or_texres(GameResource *resource)
{
  if (!resource || intptr_t(resource) == -1)
    return;

  if (gameresSysVer == 2 && is_managed_texture_id_valid(D3DRESID(unsigned(uintptr_t(resource))), false))
  {
    TEXTUREID tid = (TEXTUREID)(uintptr_t)resource;
    const char *tex_name = ::get_managed_texture_name(tid);
    const char *asterisk = tex_name ? strchr(tex_name, '*') : NULL;
    if (asterisk && (asterisk[1] == 0 || asterisk[1] == '?'))
    {
      int ref_c = ::get_managed_texture_refcount(tid);
      if (ref_c > 0)
        release_managed_tex(tid);
      else
        debug("trying to release_game_resource(%p), tex=%s, refCount=%d", resource, tex_name, ref_c);
      return;
    }
    else
      debug("strange release_game_resource(%p), name=%s", resource, tex_name);
  }


  WinAutoLock lock(gameres_cs);
  for (int i = 0; i < factories.size(); ++i)
    if (factories[i]->releaseGameResource(resource))
      return;
#if DAGOR_DBGLEVEL > 0
  fatal("res %p is not recognized by factories in release_gameres_or_texres()", resource);
#endif
}

void release_game_resource(GameResource *resource)
{
  if (!resource)
    return;
  WinAutoLock lock(gameres_cs);
  for (int i = 0; i < factories.size(); ++i)
    if (factories[i]->releaseGameResource(resource))
      return;
#if DAGOR_DBGLEVEL > 0
  fatal("res %p is not recognized by factories in release_game_resource()", resource);
#endif
}

void release_game_resource(GameResHandle handle)
{
  WinAutoLock lock(gameres_cs);
  if (gamereshooks::on_release_game_res2 && gamereshooks::on_release_game_res2(handle, make_span(factories)))
    return;

  ::release_game_resource(::gameResHandleToId(handle, 0));
}


static bool free_unused_game_resources(bool forced, bool once = false)
{
  WinAutoLock lock_load(gameres_load_cs);
  WinAutoLock lock(gameres_cs);
  bool needRepeat = false;
#if TRACE_RES_MANAGEMENT
  bool was_released = false;
  size_t sys_mem = dagor_memory_stat::get_memory_allocated();
  size_t gpu_mem = d3d::driver_command(DRV3D_COMMAND_GETTEXTUREMEM, 0, 0, 0);
#endif

  // check first whether unloadable packs present
  for (int i = 0; i < packInfo.size(); i++)
    if (packInfo[i].refCount < 1)
    {
      packInfo[i].surelyLoaded = false;
      needRepeat = true;
    }

#if _TARGET_PC_WIN
  // mainly to support tools (when assetBuildCache is used in the absence of resources from GRP
  if (gamereshooks::on_load_game_resource_pack)
    needRepeat = true;
#endif

  while (needRepeat)
  {
    needRepeat = false;
    for (int i = factories.size() - 1; i >= 0; --i)
      if (factories[i]->freeUnusedResources(forced, once))
      {
        needRepeat = true;
        if (once)
          break;
      }
#if TRACE_RES_MANAGEMENT
    if (needRepeat)
      was_released = true;
#endif
    if (once)
      break;
  }

#if TRACE_RES_MANAGEMENT
  if (was_released)
    TRACE("free_unused_game_resources, freed sysmem: %zuK, dxmem: %uzK\n", (sys_mem - dagor_memory_stat::get_memory_allocated()) >> 10,
      (d3d::driver_command(DRV3D_COMMAND_GETTEXTUREMEM, 0, 0, 0) - gpu_mem) >> 10);
  else
    TRACE("free_unused_game_resources, void run\n");
#endif
  return needRepeat;
}

void dump_game_resources_refcount(unsigned cls)
{
  for (int i = 0; i < factories.size(); i++)
    if (!cls || cls == factories[i]->getResClassId())
      factories[i]->dumpResourcesRefCount();
}

void free_unused_game_resources() { free_unused_game_resources(false); }
void free_unused_game_resources_mt()
{
  debug("GRP unload - start");
  unsigned int startFrame = ::dagor_frame_no();
  int64_t startTime = profile_ref_ticks();

  while (free_unused_game_resources(false, true))
  {
    // repeat
    sleep_msec(30);
  }

  int elapsed = profile_time_usec(startTime) / 1000;
  if (elapsed > 0) // unloaded this time
  {
    int frames = ::dagor_frame_no() - startFrame + 1;
    debug("GRP unloaded: %i ms, %i frame%s (%i ms per frame)", elapsed, frames, frames > 1 ? "s" : "",
      int(elapsed / float(frames) + 0.5f));
    G_UNUSED(frames);
  }
  else
    debug("GRP unload - end");
}

TEXTUREID get_tex_gameres(const char *resname, bool add_ref_tex)
{
  if (gameresSysVer == 2)
  {
    char tmp[512];
    _snprintf(tmp, 511, "%s*", resname);
    tmp[511] = 0;

    TEXTUREID tid = ::get_managed_texture_id(tmp);
    if (tid == BAD_TEXTUREID)
    {
      if (gamereshooks::on_get_game_res_class_id && gamereshooks::on_get_game_resource)
      {
        int res_id = ::gameResHandleToId(GAMERES_HANDLE_FROM_STRING(resname), 0);
        unsigned class_id = 0;
        GameResource *gr = NULL;
        if (gamereshooks::on_get_game_res_class_id(res_id, class_id))
          if (class_id == 0 && gamereshooks::on_get_game_resource(res_id, make_span(factories), gr))
            return (TEXTUREID)(uintptr_t)gr;
      }
      return tid;
    }
    if (add_ref_tex)
      acquire_managed_tex(tid);
    return tid;
  }

  return BAD_TEXTUREID;
}

GameResource *get_game_resource_ex(GameResHandle handle, unsigned type_id)
{
  WinAutoLock lock(gameres_cs);
  SetScopeNoFactoryFatal nffGuard(false);

  int res_id = ::gameResHandleToId(handle, type_id);
  if (gamereshooks::on_get_game_resource)
  {
    GameResource *gr = NULL;
    if (gamereshooks::on_get_game_resource(res_id, make_span(factories), gr))
      return gr;
  }

  GameResInfo *info = getGameResInfo(res_id);
  if (!info || grMap[info->grMapIdx].id.classId != type_id)
    return NULL;

  GameResourceFactory *fac = ::getFactoryByClassId(type_id);
  return fac ? fac->getGameResource(res_id) : NULL;
}

GameResource *get_one_game_resource_ex(GameResHandle handle, unsigned type_id)
{
  WinAutoLock lock(gameres_cs);

  if (is_game_resource_loaded(handle, type_id))
    return get_game_resource_ex(handle, type_id);

  int res_id = ::gameResHandleToId(handle, type_id);
  if (res_id < 0)
    return NULL;

  lock.unlockFinal();

  WinAutoLock lock_load(gameres_load_cs);
  if (res_id >= resRestrictionList.size())
    resRestrictionList.resize(resNameMap.nameCount());
  if (res_id < resRestrictionList.size())
    resRestrictionList.set(res_id);
  addReqResListReferences(res_id, resRestrictionList, requiredResList);

  GameResource *res = get_game_resource_ex(handle, type_id);

  if (res_id < resRestrictionList.size())
    resRestrictionList.reset(res_id); // don't keep reference in 'resRestrictionList' to requested resource, or it won't be freed (no
                                      // side effects)

  return res;
}

bool is_game_resource_loaded_nolock(GameResHandle handle, unsigned type_id)
{
  int res_id = ::gameResHandleToId(handle, type_id);
  GameResInfo *info = getGameResInfo(res_id);
  if (!info)
    return false;
  if (type_id != NULL_GAMERES_CLASS_ID)
  {
    if (grMap[info->grMapIdx].id.classId != type_id)
      return false;
    GameResourceFactory *fac = ::getFactoryByClassId(type_id);
    return fac && fac->isResLoaded(res_id);
  }
  for (GameResourceFactory *fac : factories)
    if (fac->isResLoaded(res_id))
      return true;
  return false;
}

bool is_game_resource_loaded(GameResHandle handle, unsigned type_id)
{
  WinAutoLock lock(gameres_cs);
  return is_game_resource_loaded_nolock(handle, type_id);
}

bool is_game_resource_loaded_trylock(GameResHandle handle, unsigned type_id)
{
  if (!gameres_cs.tryLock())
    return false;
  bool ret = is_game_resource_loaded_nolock(handle, type_id);
  gameres_cs.unlock();
  return ret;
}

bool is_game_resource_valid(GameResource *res)
{
  WinAutoLock lock(gameres_cs);
  for (int i = 0; i < factories.size(); i++)
    if (factories[i]->checkResPtr(res))
      return true;
  return false;
}


void load_game_resource_pack(GameResHandle handle) { ::load_game_resource_pack(::gameResHandleToId(handle, 0)); }


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//
void set_gameres_scan_recorder(DataBlock *rootBlk, const char *grp_pref, const char *tex_pref)
{
  gameresprivate::gameResPrefix = grp_pref;
  gameresprivate::ddsxTexPrefix = tex_pref;
  gameresprivate::recListBlk = rootBlk;
}

void scan_for_game_resources(const char *path, bool scan_subdirs, bool scan_dxp, bool allow_override, bool scan_vromfs)
{
  Tab<SimpleString> list;
  int grp_num = 0, dxp_num = 0;
  int64_t reft = profile_ref_ticks();

  auto remove_duplicates = [](auto &list) {
    eastl::sort(list.begin(), list.end());
    const intptr_t removeFrom = eastl::distance(list.begin(), eastl::unique(list.begin(), list.end()));
    const intptr_t removeCount = list.size() - removeFrom;
    if (removeCount > 0)
      erase_items(list, removeFrom, removeCount);
  };


  //  debug_ctx("scanning for GRP in %s", path);
  gameresprivate::curRelFnOfs = i_strlen(path);
  gameresprivate::gameResPatchInProgress = allow_override;

  // scan gameres packs
  find_files_in_folder(list, path, ".grp", scan_vromfs, true, scan_subdirs);
  remove_duplicates(list);
  for (auto &_fn : list)
  {
    SimpleString fn(df_get_abs_fname(_fn));
    dd_simplify_fname_c(fn);
    if (!fn.empty())
      gameresprivate::scanGameResPack(fn), grp_num++;
  }

  // scan texture packs
  if (scan_dxp)
  {
    list.clear();
    find_files_in_folder(list, path, ".dxp.bin", scan_vromfs, true, scan_subdirs);
    remove_duplicates(list);
    for (auto &_fn : list)
    {
      if (strstr(_fn, "-hq.dxp.bin")) // delay -hq to next pass
        continue;
      SimpleString fn(df_get_abs_fname(_fn));
      dd_simplify_fname_c(fn);
      if (!fn.empty())
        gameresprivate::scanDdsxTexPack(fn), dxp_num++;
      _fn = nullptr;
    }
    for (auto &_fn : list) // process -hq in second pass
      if (!_fn.empty())
      {
        SimpleString fn(df_get_abs_fname(_fn));
        dd_simplify_fname_c(fn);
        if (!fn.empty())
          gameresprivate::scanDdsxTexPack(fn), dxp_num++;
      }
  }
  clear_and_shrink(list);

  gameresprivate::gameResPatchInProgress = false;
  gameresprivate::curRelFnOfs = 0;
  debug("scan_for_game_resources(%s) processed %d GRPs and %d DxPs for %.4f sec", path, grp_num, dxp_num,
    profile_time_usec(reft) / 1e6);
  G_UNUSED(reft);
#if !(_TARGET_PC && !_TARGET_STATIC_LIB)
  if (grp_num)
    repack_real_game_res_id_table();
#endif

  if (scan_dxp)
    ddsx::dump_registered_tex_distribution();
}

void reset_game_resources()
{
  WinAutoLock lock_load(gameres_load_cs);
  WinAutoLock lock(gameres_cs);
  for (int i = 0; i < packInfo.size(); i++)
    packInfo[i].refCount = 0;
  free_unused_game_resources();

  // reset in reverse order, assuming that basic factories go first
  for (int i = factories.size() - 1; i >= 0; i--)
  {
    factories[i]->reset();
    free_unused_game_resources();
  }
  clear_and_shrink(packInfo);
  clear_and_shrink(loadedPacks);
  clear_and_shrink(grMap);
  clear_and_shrink(grInfo);
  clear_and_shrink(grInfoRefs);
  resId_to_grInfo.clear();
  resId_to_grMap.clear();
  resId_to_packId.clear();

  resNameMap.reset();

  resRestrictionList.clear();
  requiredResList.reset(true);
  one_grp_load_enabled = OGLE_ALWAYS;
}


//=================================================================================
void get_loaded_game_resource_names(Tab<String> &names)
{
  for (int i = 0; i < grInfo.size(); i++)
  {
    String name;
    getGameResName(grInfo[i].resId, name);
    names.push_back(name);
  }
}


//=================================================================================
unsigned get_resource_type_id(const char *res)
{
  int id = gameResHandleToId(GAMERES_HANDLE_FROM_STRING(res), 0xFFFFFFFFU);
  return id == -1 ? 0 : get_game_res_class_id(id);
}


//=================================================================================
const char *get_resource_type_name(unsigned id)
{
  for (int i = 0; i < factories.size(); ++i)
    if (factories[i]->getResClassId() == id)
      return factories[i]->getResClassName();

  return NULL;
}

int get_game_resource_pack_id_by_resource(GameResHandle handle)
{
  int res_id = ::gameResHandleToId(handle, 0);
  if (res_id < 0)
    return -1;

  for (int i = 0; i < grInfo.size(); ++i)
    if (grInfo[i].resId == res_id)
      return grInfo[i].packId;
  return -1;
}
int get_game_resource_pack_id_by_name(const char *fname)
{
  for (int i = 0; i < packInfo.size(); ++i)
    if (dd_stricmp(dd_get_fname(packInfo[i].fileName), fname) == 0)
      return i;
  debug_ctx("GRP not found: <%s>", fname);
  return -1;
}
const char *get_game_resource_pack_fname(int res_pack_id)
{
  if (res_pack_id < 0 || res_pack_id >= packInfo.size())
    return NULL;
  return packInfo[res_pack_id].fileName;
}


void addref_game_resource_pack(int res_pack_id)
{
  if (res_pack_id < 0)
    return;
  TRACE("addRef, pack <%s>  refCount=%d->%d\n", packInfo[res_pack_id].fileName.str(), packInfo[res_pack_id].refCount,
    packInfo[res_pack_id].refCount + 1);
  packInfo[res_pack_id].refCount++;
}
void delref_game_resource_pack(int res_pack_id)
{
  if (res_pack_id < 0)
    return;
  G_ASSERT(packInfo[res_pack_id].refCount > 0);
  TRACE("delRef, pack <%s>  refCount=%d->%d\n", packInfo[res_pack_id].fileName.str(), packInfo[res_pack_id].refCount,
    packInfo[res_pack_id].refCount - 1);
  packInfo[res_pack_id].refCount--;
}
int get_refcount_game_resource_pack(int res_pack_id)
{
  if (res_pack_id < 0)
    return 0;
  return packInfo[res_pack_id].refCount;
}

int get_refcount_game_resource_pack_by_resid(int res_id)
{
  if (res_id < resRestrictionList.size() && resRestrictionList.get(res_id))
    return 1;

  if (res_id < 0 || res_id >= resId_to_packId.size())
    return 0;

  int i = resId_to_packId[res_id];
  G_ASSERT(res_id == grMap[resId_to_grMap[res_id]].id.resId);
  G_ASSERT(i == grMap[resId_to_grMap[res_id]].packId);

  if (i < 0)
    return 0;

  return packInfo[i].refCount;
}

void get_used_gameres_packs(dag::ConstSpan<const char *> needed_res, bool check_ref, Tab<String> &out_grp_filenames)
{
  Tab<int> needed_resid(tmpmem);
  OAHashNameMap<true> grp_names;

  needed_resid.reserve(needed_res.size() * 3);
  for (int i = 0; i < needed_res.size(); i++)
  {
    int id = resNameMap.getNameId(needed_res[i]);
    if (id >= 0)
      needed_resid.push_back(id);
  }

  while (needed_resid.size())
    for (int i = needed_resid.size() - 1; i >= 0; i--)
    {
      int rid = needed_resid[i];
      GameResInfo *gri = getGameResInfo(rid);
      if (!gri)
        continue;
      grp_names.addNameId(packInfo[gri->packId].fileName);
      erase_items(needed_resid, i, 1);

      if (check_ref)
      {
        loadGameResPack(gri->packId, gri->resId);
        packInfo[gri->packId].addRefResIds(rid, needed_resid);
      }
    }

  out_grp_filenames.resize(grp_names.nameCount());
  for (int i = 0; i < grp_names.nameCount(); i++)
    out_grp_filenames[i] = grp_names.getName(i);
}

void gamerespackbin::GrpData::patchDescOnly(uint32_t hdr_label)
{
  void *base = dumpBase();
  nameMap.patch(base);
  resTable.patch(base);
  resData.patch(base);
  if (hdr_label == _MAKE4C('GRP2'))
    ResData::cvtRecInplaceVer2to3(resData.data(), resData.size());
}
void gamerespackbin::GrpData::patchData()
{
  void *base = dumpBase();
  ResData *rd = resData.data(), *rd_end = rd + resData.size();
  for (; rd != rd_end; rd++)
    rd->refResIdPtr.patchNonNull(base);
}

bool enable_gameres_finer_load(bool en)
{
  WinAutoLock lock(gameres_cs);
  bool prev = gameres_finer_load_enabled;
  gameres_finer_load_enabled = en;
  return prev;
}

void enable_one_gameres_pack_loading(bool en, bool check /*= true*/)
{
  G_UNUSED(check);
  if (en)
  {
    G_ASSERT(one_grp_load_enabled == OGLE_NOT_ENABLED || !check); // not OGLE_ALWAYS
    one_grp_load_enabled = OGLE_ONE_ENABLED;
  }
  else
  {
    G_ASSERT(one_grp_load_enabled != OGLE_NOT_ENABLED || !check);
    one_grp_load_enabled = OGLE_NOT_ENABLED;
  }
}
void debug_enable_free_gameres_pack_loading()
{
  G_ASSERT(one_grp_load_enabled == OGLE_NOT_ENABLED);
  one_grp_load_enabled = OGLE_ALWAYS;
}
void debug_disable_free_gameres_pack_loading()
{
  G_ASSERT(one_grp_load_enabled == OGLE_ALWAYS);
  one_grp_load_enabled = OGLE_NOT_ENABLED;
}
void ignore_unavailable_resources(bool ignore) { ignoreUnavailableResources = ignore; }

void logging_missing_resources(bool logging) { loggingMissingResources = logging; }

static bool addReqResListReferences(int res_id, Bitarray &restr_list, FastIntList &req_list, int parent_cls)
{
  if (res_id >= restr_list.size())
  {
    Tab<int> refs;
    if (gamereshooks::get_res_refs && gamereshooks::get_res_refs(res_id, refs))
    {
      for (int i = 0; i < refs.size(); i++)
      {
        int rrid = refs[i];
        if (rrid == -1)
          continue;

        if (rrid < restr_list.size())
        {
          if (restr_list.get(rrid))
            continue;
          restr_list.set(rrid);
        }

        if (!addReqResListReferences(rrid, restr_list, req_list, parent_cls))
        {
          restr_list.reset(rrid);
          return false;
        }
      }
    }
    return true;
  }

  GameResInfo *info = ::getGameResInfo(res_id);
  String tmpStr;

  if (!info)
  {
    int unused_resolved_res_id = 0;
    if (gamereshooks::on_validate_game_res_id && gamereshooks::on_validate_game_res_id(res_id, unused_resolved_res_id))
    {
      unsigned cls = 0;
      if (gamereshooks::on_get_game_res_class_id && gamereshooks::on_get_game_res_class_id(res_id, cls))
        if (cls == RendInstGameResClassId)
          add_rendinst_implicit_ref(res_id, restr_list, req_list);
      restr_list.set(res_id);
      G_UNUSED(unused_resolved_res_id);
      return true;
    }
    if (get_gameres_sys_ver() == 2 && parent_cls == EffectGameResClassId)
    {
      ::get_game_resource_name(res_id, tmpStr);
      tmpStr += "*";
      if (::get_managed_texture_id(tmpStr) != BAD_TEXTUREID)
        return true;
    }
    logwarn("undefined res %s", resNameMap.getName(res_id));
    return false;
  }

  if (grMap[info->grMapIdx].id.classId == RendInstGameResClassId && gameres_rendinst_desc.blockCount())
    add_rendinst_implicit_ref(res_id, restr_list, req_list);

  parent_cls = grMap[info->grMapIdx].id.classId;
  for (int i = 0; i < info->refNum; i++)
  {
    int rrid = grInfoRefs[info->refStartIdx + i];
    if (rrid == -1)
      continue;

    G_ASSERT(rrid < restr_list.size());
    if (restr_list.get(rrid))
      continue;

    if (get_gameres_sys_ver() == 2 && (unsigned)grMap[info->grMapIdx].id.classId == EffectGameResClassId)
    {
      // for effects we need hack, to check res as texture first
      tmpStr.printf(64, "%s*", resNameMap.getName(rrid));
      TEXTUREID texid = ::get_managed_texture_id(tmpStr);
      if (texid != BAD_TEXTUREID)
      {
        restr_list.set(rrid);
        // debug("found tex ref <%s> for req res <%s>",
        //   resNameMap.getName(rrid), resNameMap.getName(res_id));
        continue;
      }
    }

    restr_list.set(rrid);
    if (!addReqResListReferences(rrid, restr_list, req_list, parent_cls))
    {
      restr_list.reset(rrid);
      debug("missing ref <%s> for req res <%s>", resNameMap.getName(rrid), resNameMap.getName(res_id));
      return false;
    }
  }
  int rid = grMap[info->grMapIdx].id.resId;
  if (rid >= 0)
  {
    G_ASSERT(rid < restr_list.size());
    restr_list.set(rid);
  }
  return true;
}

static void add_rendinst_implicit_ref(int res_id, Bitarray &restr_list, FastIntList &req_list)
{
  if (!gameres_rendinst_desc.blockCount())
    return;
  if (const DataBlock *b = gameres_rendinst_desc.getBlockByName(resNameMap.getName(res_id)))
  {
    static const char *ref_nm[] = {"refRendInst", "refPhysObj", "refColl", "refApex"};
    for (int i = 0; i < sizeof(ref_nm) / sizeof(ref_nm[0]); i++)
      if (const char *nm = b->getStr(ref_nm[i], NULL))
      {
        int rrid = resNameMap.getNameId(nm);
        if (rrid >= 0 && rrid < restr_list.size())
          if (!restr_list.get(rrid))
          {
            restr_list.set(rrid);
            req_list.addInt(rrid);
            addReqResListReferences(rrid, restr_list, req_list);
          }
      }
  }
}

static Bitarray restr_list_temp;
static FastIntList req_list_temp;

template <typename GetListElement>
inline bool set_required_res_list_restriction(GetListElement list, int count, ReqResListOpt opt)
{
  bool ret = true;
  WinAutoLock lock(gameres_cs);

  restr_list_temp.resize(resNameMap.nameCount());
  restr_list_temp.reset();
  req_list_temp.reset();

  for (int i = 0; i < count; i++)
  {
    const char *name = list(i);
    int res_id = resNameMap.getNameId(name);
    if (res_id < 0 && gamereshooks::resolve_res_handle)
      gamereshooks::resolve_res_handle(GAMERES_HANDLE_FROM_STRING(name), 0xFFFFFFFFu, res_id);
    if (res_id < 0)
    {
      if (loggingMissingResources)
        logwarn_ctx("missing req res <%s>", name);
      ret = false;
    }
    else
    {
      if (res_id < restr_list_temp.size())
        restr_list_temp.set(res_id);
      if (!addReqResListReferences(res_id, restr_list_temp, req_list_temp))
      {
        if (res_id < restr_list_temp.size())
          restr_list_temp.reset(res_id);
        if (loggingMissingResources)
          logwarn_ctx("one or more missing refs for req res <%s>", name);
        ret = false;
      }
      else
        req_list_temp.addInt(res_id);
    }
  }
  lock.unlock();

  WinAutoLock lock_load(gameres_load_cs);
  lock.lock();
  resRestrictionList = restr_list_temp;
  requiredResList = req_list_temp;
  if (opt == RRL_setAndPreload || opt == RRL_setAndPreloadAndReset)
  {
    preload_all_required_res();
    if (opt == RRL_setAndPreloadAndReset)
      reset_required_res_list_restriction();
  }
  return ret;
}

bool set_required_res_list_restriction(const FastNameMap &list, ReqResListOpt opt)
{
  return set_required_res_list_restriction([&list](int i) { return list.getName(i); }, list.nameCount(), opt);
}

bool set_required_res_list_restriction(const eastl::string *begin, const eastl::string *end, ReqResListOpt opt, size_t str_stride)
{
  if (str_stride == 0)
    str_stride = sizeof(eastl::string);
  size_t sz = (ptrdiff_t(end) - ptrdiff_t(begin)) / str_stride;
  return set_required_res_list_restriction(
    [begin, str_stride](int i) { return reinterpret_cast<const eastl::string *>((char *)begin + (str_stride * i))->c_str(); }, (int)sz,
    opt);
}

void reset_required_res_list_restriction()
{
  WinAutoLock lock(gameres_load_cs);
  resRestrictionList.clear();
  requiredResList.reset();
}

bool preload_all_required_res()
{
  WinAutoLock lock_load(gameres_load_cs);
  Tab<int> l;
  l = requiredResList.getList();

  bool ok = true;

  for (int i = 0; i < l.size(); i++)
  {
    GameResource *r = get_game_resource(l[i]);
    if (r)
      release_game_resource(r);
    else
    {
      ok = false;
      logwarn_ctx("cannot preload res: <%s>", resNameMap.getName(l[i]));
    }
  }

  return ok;
}

bool release_all_not_required_res()
{
  free_unused_game_resources();
  return true; //== cannot detect hanging resources for now, just return OK
}

void reset_gameres_available_files_list() { del_it(avail_res_files); }
void add_gameres_available_files_list(const char *root_dir)
{
  if (!avail_res_files)
    avail_res_files = new OAHashNameMap<true>;

  Tab<SimpleString> list(tmpmem);
  find_files_in_folder(list, root_dir, ".grp", false, true, true);
  find_files_in_folder(list, root_dir, ".dxp.bin", false, true, true);
  for (int i = 0; i < list.size(); i++)
    avail_res_files->addNameId(list[i]);
  debug("scanned %s: %d files added (%d total in list)", root_dir, list.size(), avail_res_files->nameCount());
}
