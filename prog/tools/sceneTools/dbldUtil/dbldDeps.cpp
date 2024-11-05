// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/binDumpReader.h>
#include <libTools/util/strUtil.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <scene/dag_loadLevelVer.h>
#include <rendInst/rendInstGen.h>
#include <fx/dag_commonFx.h>
#include <landMesh/lmeshManager.h>
#include <daFx/dafx.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResHooks.h>
#include <gameRes/dag_stdGameRes.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_renderScene.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texPackMgr2.h>
#include <math/dag_bounds2.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fileMd5Validate.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stdlib.h>
#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

unsigned int lightmap_quality = 0;

static FastNameMapEx refTexAll;
static FastNameMapEx refResAll;

static Tab<unsigned> texRemap;
static Tab<FastNameMap> perTexAssets;

static FastNameMap resRemap;
static Tab<Tab<TEXTUREID>> perResTextures;
static FastNameMap resVdataSzRemap;
static Tab<int> resVdataSz;

static dafx::ContextId g_dafx_ctx;
extern void dafx_sparksfx_set_context(dafx::ContextId ctx);
extern void dafx_modfx_set_context(dafx::ContextId ctx);
extern void dafx_compound_set_context(dafx::ContextId ctx);
extern void dafx_flowps2_set_context(dafx::ContextId ctx);
static void term_dafx()
{
  if (!g_dafx_ctx)
    return;

  dafx::release_all_systems(g_dafx_ctx);
  dafx::release_context(g_dafx_ctx);

  g_dafx_ctx = dafx::ContextId();
  dafx_sparksfx_set_context(g_dafx_ctx);
  dafx_modfx_set_context(g_dafx_ctx);
  dafx_compound_set_context(g_dafx_ctx);
  // dafx_flowps2_set_context(g_dafx_ctx);
}

namespace texmgr_internal
{
extern void (*hook_on_get_texture_id)(TEXTUREID id);
}
namespace matvdata
{
extern void (*hook_on_add_vdata)(ShaderMatVdata *vd, int add_vdata_sz);
}

static bool on_get_game_resource(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res)
{
  if (res_id >= 0)
  {
    String name;
    get_game_resource_name(res_id, name);
    refResAll.addNameId(name);
  }
  return false;
}
static bool on_load_game_resource_pack(int res_id, dag::Span<GameResourceFactory *> f) { return false; }
static void on_get_texture_id(TEXTUREID tid)
{
  if (dgs_fill_fatal_context)
  {
    char buf[2048];
    dgs_fill_fatal_context(buf, sizeof(buf), true);
    if (const char *outer_context_name = strrchr(buf, '\n'))
    {
      if (tid.index() >= texRemap.size())
      {
        int s = texRemap.size(), e = (tid.index() + 1 + 0x3fff) & ~0x3fff;
        append_items(texRemap, e - s);
        mem_set_ff(make_span(texRemap).subspan(s));
      }
      unsigned idx = texRemap[tid.index()];
      if (idx == ~0u)
        idx = texRemap[tid.index()] = append_items(perTexAssets, 1);
      perTexAssets[idx].addNameId(outer_context_name + 1);

      idx = resRemap.addNameId(outer_context_name + 1);
      if (idx == perResTextures.size())
        append_items(perResTextures, 1);
      perResTextures[idx].push_back(tid);
    }
  }
}
static void on_add_vdata(ShaderMatVdata *, int add_vdata_sz)
{
  if (dgs_fill_fatal_context && add_vdata_sz)
  {
    char buf[2048];
    dgs_fill_fatal_context(buf, sizeof(buf), true);
    if (const char *outer_context_name = strrchr(buf, '\n'))
    {
      int idx = resVdataSzRemap.addNameId(outer_context_name + 1);
      if (idx == resVdataSz.size())
        resVdataSz.push_back(0);
      resVdataSz[idx] += add_vdata_sz;
    }
  }
}

static bool quiet_fatal_handler(const char *msg, const char *call_stack, const char *file, int line) { return false; }

static FastNameMap req_res_list;
static void add_resource_cb(const char *resname) { req_res_list.addNameId(resname); }

static void load_res_package(const char *folder, bool optional, const char *patch_root_folder)
{
  String mnt0(0, "%s/", folder);
  String mntP;
  if (patch_root_folder)
    mntP.printf(0, "%s/%s", patch_root_folder, mnt0);
  bool base_pkg_loaded = false;

  gameres_append_desc(gameres_rendinst_desc, mnt0 + "riDesc.bin", folder);
  gameres_append_desc(gameres_dynmodel_desc, mnt0 + "dynModelDesc.bin", folder);

  if (VirtualRomFsData *vrom = load_vromfs_dump(mnt0 + "grp_hdr.vromfs.bin", tmpmem, NULL, NULL, optional ? DF_IGNORE_MISSING : 0))
  {
    add_vromfs(vrom, true, mnt0.str());
    debug("loading res pkg: %s", mnt0);
    ::load_res_packs_from_list(mnt0 + "respacks.blk", true, true, mnt0);
    base_pkg_loaded = true;
    remove_vromfs(vrom);
    tmpmem->free(vrom);
  }
  else
    logwarn("missing res pkg: %s", mnt0);

  if (base_pkg_loaded && !mntP.empty())
  {
    gameres_patch_desc(gameres_rendinst_desc, mntP + "riDesc.bin", folder, mnt0 + "riDesc.bin");
    gameres_patch_desc(gameres_dynmodel_desc, mntP + "dynModelDesc.bin", folder, mnt0 + "dynModelDesc.bin");

    if (VirtualRomFsData *vrom = load_vromfs_dump(mntP + "grp_hdr.vromfs.bin", tmpmem, NULL, NULL, optional ? DF_IGNORE_MISSING : 0))
    {
      add_vromfs(vrom, true, mntP.str());
      debug("loading res pkg(patch): %s", folder);
      ::load_res_packs_patch(mntP + "respacks.blk", mnt0 + "grp_hdr.vromfs.bin", true, true);
      remove_vromfs(vrom);
      tmpmem->free(vrom);
    }
  }
}

struct CachedTexData
{
  ska::flat_hash_map<unsigned, int> tid2nid;
  SmallTab<TEXTUREID> nid2tid;
  SmallTab<ddsx::DDSxDataPublicHdr> texHdrByNameId;
  SmallTab<unsigned> texSizeByNameId;

  CachedTexData()
  {
    nid2tid.resize(refTexAll.nameCount());
    mem_set_0(nid2tid);
    texHdrByNameId.resize(refTexAll.nameCount());
    mem_set_0(texHdrByNameId);
    texSizeByNameId.resize(refTexAll.nameCount());
    mem_set_0(texSizeByNameId);

    iterate_names(refTexAll, [&](int nid, const char *name) {
      texSizeByNameId[nid] = get_tex_header(name, texHdrByNameId[nid]);
      if (TEXTUREID tid = get_managed_texture_id(name))
        tid2nid[unsigned(tid)] = nid, nid2tid[nid] = tid;
    });
  }
  inline unsigned getTexSize(TEXTUREID tid)
  {
    int nid = tid2nid[unsigned(tid)];
    return nid >= 0 ? texSizeByNameId[nid] : 0;
  }
  inline const char *getTexName(TEXTUREID tid) { return refTexAll.getName(tid2nid[unsigned(tid)]); }
  static int get_tex_header(const char *name, ddsx::DDSxDataPublicHdr &desc)
  {
    TEXTUREID tid = get_managed_texture_id(name);
    int tex_sz = ddsx::read_ddsx_header(name, desc, true);
    if (tex_sz < 0)
    {
      if (BaseTexture *t = acquire_managed_tex(tid))
      {
        TextureInfo ti;
        t->getinfo(ti, 0);

        desc.w = ti.w;
        desc.h = ti.h;
        desc.depth = max<int>(ti.d, ti.a);
        desc.levels = ti.mipLevels;
        if (ti.resType == RES3D_CUBETEX)
          desc.flags |= desc.FLG_CUBTEX;
        else if (ti.resType == RES3D_VOLTEX)
          desc.flags |= desc.FLG_VOLTEX;
        tex_sz = t->ressize();
      }
      release_managed_tex(tid);
    }
    return (get_managed_texture_refcount(tid) > 0) ? tex_sz : 0;
  }
};

bool dumpDbldDeps(IGenLoad &crd, const DataBlock &env)
{
  const char *root_dir = env.getStr("root", ".");
  bool verbose = !dgs_execute_quiet;
  dgs_execute_quiet = true;

  debug("using root: %s", root_dir);
  if (const char *s_file = env.getStr("settings", NULL))
    dgs_load_settings_blk(false, s_file, NULL);

  char prev_dir[260];
  getcwd(prev_dir, sizeof(prev_dir));
  chdir(root_dir);

  refTexAll.reset();
  refResAll.reset();

  cpujobs::init();

  void *n = NULL;
  d3d::init_driver();
  d3d::init_video(NULL, NULL, NULL, 0, n, NULL, NULL, NULL, NULL);
  set_missing_texture_name("", false);
  set_gameres_sys_ver(env.getInt("gameResSysVer", 2));

  gamereshooks::on_get_game_resource = &on_get_game_resource;
  gamereshooks::on_load_game_resource_pack = &on_load_game_resource_pack;
  texmgr_internal::hook_on_get_texture_id = &on_get_texture_id;
  matvdata::hook_on_add_vdata = &on_add_vdata;

  texRemap.resize(32768);
  mem_set_ff(texRemap);
  perTexAssets.reserve(4096);

  for (int i = 0; i < env.blockCount(); i++)
    if (strcmp(env.getBlock(i)->getBlockName(), "vromfs") == 0)
    {
      const DataBlock &blk_vrom = *env.getBlock(i);
      Tab<char> vromfs_data(tmpmem);
      FullFileLoadCB fcrd(blk_vrom.getStr("file"));
      if (!fcrd.fileHandle || df_length(fcrd.fileHandle) < 8)
      {
        logerr("failed to load %s", blk_vrom.getStr("file"));
        continue;
      }
      vromfs_data.resize(df_length(fcrd.fileHandle));
      fcrd.read(vromfs_data.data(), data_size(vromfs_data));
      fcrd.close();

      unsigned &code = *(unsigned *)&vromfs_data[4];
      if (!dagor_target_code_be(code))
        code = _MAKE4C('PC');

      VirtualRomFsData *vrom = ::load_vromfs_dump_from_mem(vromfs_data, tmpmem);
      if (vrom)
      {
        ::add_vromfs(vrom, false, str_dup(blk_vrom.getStr("mnt", "./"), tmpmem));
        debug("mount %p to %s", vrom, blk_vrom.getStr("mnt", "./"));
      }
      else
        logerr("failed to load %s", blk_vrom.getStr("file"));
    }

  if (const char *gp_file = env.getStr("gameParams", NULL))
    dgs_load_game_params_blk(gp_file);

  startup_shaders(env.getStr("shaders", "compiledShaders/game"));
  startup_game(RESTART_ALL);

  ::register_dynmodel_gameres_factory();
  ::register_rendinst_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_character_gameres_factory();
  ::register_fast_phys_gameres_factory();
  ::register_phys_sys_gameres_factory();
  ::register_phys_obj_gameres_factory();
  ::register_ragdoll_gameres_factory();
  ::register_animchar_gameres_factory();
  ::register_a2d_gameres_factory();
  ::register_effect_gameres_factory();
  rendinst::register_land_gameres_factory();
  CollisionResource::registerFactory();
  ::register_all_common_fx_factories();
  g_dafx_ctx = dafx::create_context(dafx::Config{});

  dafx_sparksfx_set_context(g_dafx_ctx);
  dafx_modfx_set_context(g_dafx_ctx);
  dafx_compound_set_context(g_dafx_ctx);
  // dafx_flowps2_set_context( g_dafx_ctx );

  if (const DataBlock *addons = env.getBlockByName("addons"))
    for (int i = 0, nid = addons->getNameId("folder"); i < addons->paramCount(); i++)
      if (addons->getParamNameId(i) == nid)
      {
        load_res_package(String::mk_str_cat(addons->getStr(i), "/res"), true, env.getBool("allowResPatch", false) ? "patch" : nullptr);
      }
  gameres_final_optimize_desc(gameres_rendinst_desc, "riDesc");
  gameres_final_optimize_desc(gameres_dynmodel_desc, "dynModelDesc");

  for (int i = 0, nid_resList = env.getNameId("resList"); i < env.paramCount(); i++)
    if (env.getParamNameId(i) == nid_resList && env.getParamType(i) == DataBlock::TYPE_STRING)
    {
      String resDir(env.getStr(i));
      location_from_path(resDir);
      ::load_res_packs_from_list(env.getStr(i), true, true, resDir);

      String patch_res_fn(0, "patch/%s", env.getStr(i));
      String patch_dir(0, "patch/%s", resDir);
      if (env.getBool("allowResPatch", false) && dd_file_exists(patch_res_fn))
        ::load_res_packs_patch(patch_res_fn, String(0, "%s/grp_hdr.vromfs.bin", resDir), true, true);
    }

  if (crd.readInt() != _MAKE4C('DBLD'))
  {
    printf("%s: bad header\n", crd.getTargetName());
  err_exit:
    reset_game_resources();
    cpujobs::term(true);
    chdir(prev_dir);
    term_dafx();
    return false;
  }
  if (crd.readInt() != DBLD_Version)
  {
    printf("%s: unsupported version\n", crd.getTargetName());
    goto err_exit;
  }
  crd.readInt();

  FastNameMapEx refRes;
  FastNameMapEx refTex;
  LandMeshManager *lmeshMgr = NULL;
  RenderScene *scn = NULL;
  RenderScene *envi = NULL;
  Tab<TEXTUREID> texmap;
  String tmpStr;

  bool (*prev_handler)(const char *msg, const char *call_stack, const char *fn, int ln) = dgs_fatal_handler;
  dgs_fatal_handler = quiet_fatal_handler;
  rendinst::rendinstClipmapShadows = rendinst::rendinstGlobalShadows = false;
  if (env.getBlockByNameEx("fields")->getBool("RIGz", true))
    rendinst::initRIGen(/*render*/ false, 8 * 8 + 8, 80000, 0, 0, -1);
  dgs_fatal_handler = prev_handler;

  for (;;)
  {
    int tag = crd.beginTaggedBlock();

    if (tag != _MAKE4C('END'))
    {
      printf("%c%c%c%c at %08X..%08X (size=%d)\n", _DUMP4C(tag), crd.tell(), crd.tell() + crd.getBlockRest() - 1, crd.getBlockRest());
      if (!env.getBlockByNameEx("fields")->getBool(String(5, "%c%c%c%c", DUMP4C(tag)), true))
      {
        printf("  skipped %c%c%c%c\n", _DUMP4C(tag));
        crd.endBlock();
        continue;
      }
    }

    if (tag == _MAKE4C('RqRL'))
    {
      RoNameMap *rqrl_ronm = (RoNameMap *)memalloc(crd.getBlockRest(), tmpmem);
      crd.read(rqrl_ronm, crd.getBlockRest());
      rqrl_ronm->patchData(rqrl_ronm);

      for (int i = 0; i < rqrl_ronm->map.size(); i++)
        refRes.addNameId(rqrl_ronm->map[i]);
      memfree(rqrl_ronm, tmpmem);

      set_required_res_list_restriction(refRes);
      preload_all_required_res();
      reset_required_res_list_restriction();
    }
    else if (tag == _MAKE4C('DxP2'))
    {
      String str;
      int tex_count;

      crd.readString(str);
      tex_count = crd.readInt();

      texmap.resize(tex_count);
      for (int i = 0; i < tex_count; i++)
      {
        crd.readString(str);
        refTex.addNameId(TextureMetaData::decodeFileName(str));
        texmap[i] = get_managed_texture_id(str);
      }
    }
    else if (tag == _MAKE4C('lmap'))
    {
      if (crd.tell() >= 0)
      {
        lmeshMgr = new LandMeshManager();
        if (!lmeshMgr->loadDump(crd, midmem, true))
        {
          delete lmeshMgr;
          lmeshMgr = NULL;
          debug("can't load lmesh");
        }
      }
    }
    else if (tag == _MAKE4C('SCN'))
    {
      scn = new RenderScene;
      scn->loadBinary(crd, texmap, false);
    }
    else if (tag == _MAKE4C('ENVI'))
    {
      envi = new RenderScene;
      envi->loadBinary(crd, texmap, false);
    }
    else if (tag == _MAKE4C('RIGz'))
    {
      rendinst::RIGenLoadingAutoLock riGenLd;
      rendinst::loadRIGen(crd, add_resource_cb);

      set_required_res_list_restriction(req_res_list);
      preload_all_required_res();
      reset_required_res_list_restriction();

      rendinst::prepareRIGen();

      if (const char *ri_dmg = env.getStr("riDmg", NULL))
      {
        DataBlock ri_blk(ri_dmg);
        rendinst::initRiGenDebris(ri_blk, NULL);
      }
    }
    // else if (tag == _MAKE4C('wt3d')) {}

    crd.endBlock();
    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      printf("valid end at %08X\n", crd.tell());
      break;
    }
  }

  gamereshooks::on_get_game_resource = NULL;
  gamereshooks::on_load_game_resource_pack = NULL;
  texmgr_internal::hook_on_get_texture_id = NULL;
  matvdata::hook_on_add_vdata = NULL;
  for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
    refTexAll.addNameId(TextureMetaData::decodeFileName(get_managed_texture_name(i)));

  CachedTexData cachedTexData; // constructed using current refTexAll
  String tmp_dim_str;
  auto printTexDim = [&tmp_dim_str](const ddsx::DDSxDataPublicHdr &desc, unsigned tex_sz) {
    tmp_dim_str.printf(0, "[%4dx%d", desc.w, desc.h);
    if (desc.flags & desc.FLG_VOLTEX)
      tmp_dim_str.aprintf(0, "x%d", desc.depth);
    else if (desc.flags & desc.FLG_CUBTEX)
      tmp_dim_str.aprintf(0, "x6", desc.depth);
    else if (desc.depth > 1)
      tmp_dim_str.aprintf(0, "[%d]", desc.depth);
    if (desc.levels <= get_log2i(max(max(desc.w, desc.h), desc.depth)))
      tmp_dim_str.aprintf(0, ",L%d", desc.levels);
    tmp_dim_str.aprintf(0, " %*dK]", max<int>(15 - tmp_dim_str.length(), 0), tex_sz >> 10);
    return tmp_dim_str.c_str();
  };

  del_it(lmeshMgr);
  del_it(scn);
  del_it(envi);
  rendinst::clearRIGen();

  if (env.getBlockByNameEx("fields")->getBool("RIGz", true))
    rendinst::termRIGen();
  reset_game_resources();
  cpujobs::term(true);

  printf("\nreferenced %d DDSx (%d root%s):\n", refTexAll.nameCount(), refTex.nameCount(),
    verbose ? ", legend: [WxH memSz] TEX <- RES..." : "");
  int max_texname_len = 0;
  iterate_names(refTexAll, [&](int, const char *name) {
    if (int len = strlen(name))
      if (max_texname_len < len)
        max_texname_len = len;
  });
  int64_t total_texmem_used = 0;
  iterate_names(refTexAll, [&](int nid, const char *name) {
    TEXTUREID tid = cachedTexData.nid2tid[nid];
    const ddsx::DDSxDataPublicHdr &desc = cachedTexData.texHdrByNameId[nid];
    int tex_sz = cachedTexData.texSizeByNameId[nid];
    if (tex_sz > 0)
      total_texmem_used += tex_sz;
    if (!verbose)
      printf("%s %s\n", refTex.getNameId(name) >= 0 ? "*" : " ", name);
    else if (tid.index() >= texRemap.size() || texRemap[tid.index()] == ~0u)
      printf("%s %s  %s\n", refTex.getNameId(name) >= 0 ? "*" : " ", printTexDim(desc, tex_sz), name);
    else
    {
      const FastNameMap &map = perTexAssets[texRemap[tid.index()]];
      tmpStr.printf(0, "%s %s", refTex.getNameId(name) >= 0 ? "*" : " ", printTexDim(desc, tex_sz));
      tmpStr.aprintf(0, "  %-*s <-", max<int>(max_texname_len + 20 - tmpStr.length(), 1), name);
      iterate_names(map, [&tmpStr](int, const char *name2) { tmpStr.aprintf(0, " %s;", name2); });

      printf("%s\n", tmpStr.str());
    }
  });
  printf("    total %dM for %d texture(s)\n", int(total_texmem_used >> 20), refTexAll.nameCount());

  printf("\nreferenced %d resources (%d root%s):\n", refResAll.nameCount(), refRes.nameCount(),
    verbose ? ", legend: [resVB + uniqueTex] RES -> unique{TEX...}; shared{TEX...}" : "");
  int max_resname_len = 0;
  iterate_names(refResAll, [&](int, const char *name) {
    if (int len = strlen(name))
      if (max_resname_len < len)
        max_resname_len = len;
  });
  int64_t total_vbmem_used = 0;
  Tab<TEXTUREID> unique_tex, shared_tex;
  iterate_names(refResAll, [&](int, const char *name) {
    if (!verbose)
      printf("%s %s\n", refRes.getNameId(name) >= 0 ? "*" : " ", name);
    else
    {
      int res_vbsz = 0;
      {
        int idx = resVdataSzRemap.getNameId(name);
        if (idx >= 0)
          res_vbsz = resVdataSz[idx];
        total_vbmem_used += res_vbsz;
      }

      int idx = resRemap.getNameId(name);
      if (idx < 0 && !res_vbsz)
        printf("%-19s %s\n", refRes.getNameId(name) >= 0 ? "*" : " ", name);
      else if (idx < 0)
        printf("%s [%5dK+%6dK]  %s\n", refRes.getNameId(name) >= 0 ? "*" : " ", res_vbsz >> 10, 0, name);
      else
      {
        unique_tex.clear();
        shared_tex.clear();
        int64_t unique_texmem_used = 0;
        for (TEXTUREID tid : perResTextures[idx])
          if (tid.index() >= texRemap.size() || texRemap[tid.index()] == ~0u || perTexAssets[texRemap[tid.index()]].nameCount() < 2)
          {
            unique_tex.push_back(tid);
            int tex_sz = cachedTexData.getTexSize(tid);
            if (tex_sz > 0)
              unique_texmem_used += tex_sz;
          }
          else
            shared_tex.push_back(tid);

        tmpStr.printf(0, "%s [%5dK+%6dK]  %-*s -> ", refRes.getNameId(name) >= 0 ? "*" : " ", res_vbsz >> 10, unique_texmem_used >> 10,
          max_resname_len, name);
        if (unique_tex.size())
        {
          tmpStr.aprintf(0, "unique[%d]={", unique_tex.size());
          for (TEXTUREID tid : unique_tex)
            tmpStr.aprintf(0, " %s;", cachedTexData.getTexName(tid));
          tmpStr += " };";
        }
        if (shared_tex.size())
        {
          tmpStr.aprintf(0, " shared[%d]={", shared_tex.size());
          for (TEXTUREID tid : shared_tex)
            tmpStr.aprintf(0, " %s;", cachedTexData.getTexName(tid));
          tmpStr += " };";
        }

        printf("%s\n", tmpStr.str());
      }
    }
  });
  printf("    total %.1fM (VB/IB) for %d resource(s)\n", total_vbmem_used / 1024.0 / 1024.0, refResAll.nameCount());

  refRes.reset();
  iterate_names(resRemap, [&](int, const char *name) {
    if (refResAll.getNameId(name) < 0)
      refRes.addNameId(name);
  });
  iterate_names(resVdataSzRemap, [&](int, const char *name) {
    if (refResAll.getNameId(name) < 0)
      refRes.addNameId(name);
  });

  if (verbose)
  {
    total_texmem_used = total_vbmem_used = 0;
    printf("\nother %d shader meshes ([resVB + uniqueTex] MESH -> unique{TEX...}; shared{TEX...}):\n", refRes.nameCount());
    iterate_names(refRes, [&](int, const char *name) {
      int res_vbsz = 0;
      {
        int idx = resVdataSzRemap.getNameId(name);
        if (idx >= 0)
          res_vbsz = resVdataSz[idx];
        total_vbmem_used += res_vbsz;
      }

      int idx = resRemap.getNameId(name);
      if (idx < 0)
        printf(" [%5dK+%6dK]  %s\n", res_vbsz >> 10, 0, name);
      else
      {
        unique_tex.clear();
        shared_tex.clear();
        int64_t unique_texmem_used = 0;
        for (TEXTUREID tid : perResTextures[idx])
          if (tid.index() >= texRemap.size() || texRemap[tid.index()] == ~0u || perTexAssets[texRemap[tid.index()]].nameCount() < 2)
          {
            unique_tex.push_back(tid);
            int tex_sz = cachedTexData.getTexSize(tid);
            if (tex_sz > 0)
              unique_texmem_used += tex_sz;
          }
          else
            shared_tex.push_back(tid);

        tmpStr.printf(0, " [%5dK+%6dK]  %s -> ", res_vbsz >> 10, unique_texmem_used >> 10, name);
        if (unique_tex.size())
        {
          tmpStr.aprintf(0, "unique[%d]={", unique_tex.size());
          for (TEXTUREID tid : unique_tex)
            tmpStr.aprintf(0, " %s;", cachedTexData.getTexName(tid));
          tmpStr += " };";
        }
        if (shared_tex.size())
        {
          tmpStr.aprintf(0, " shared[%d]={", shared_tex.size());
          for (TEXTUREID tid : shared_tex)
            tmpStr.aprintf(0, " %s;", cachedTexData.getTexName(tid));
          tmpStr += " };";
        }

        printf("%s\n", tmpStr.str());
        total_texmem_used += unique_texmem_used;
      }
    });
    printf("    total %.1fM (VB/IB) and %dM (unique tex) for %d mesh(es)\n", total_vbmem_used / 1024.0 / 1024.0,
      int(total_texmem_used >> 20), refRes.nameCount());
  }

  printf("\nunreferenced root DDSx:\n");
  for (int i = 0; i < refTex.nameCount(); i++)
    if (refTexAll.getNameId(refTex.getName(i)) < 0)
      printf("  %s\n", refTex.getName(i));
  printf("listing done\n");

  chdir(prev_dir);
  debug("restored %s", prev_dir);
  term_dafx();
  return true;
}

namespace rendinst::gen
{
float custom_max_trace_distance = 0;
bool custom_trace_ray(const Point3 &src, const Point3 &dir, real &dist, Point3 *out_norm) { return false; }
bool custom_trace_ray_earth(const Point3 &src, const Point3 &dir, real &dist) { return false; }
bool custom_get_height(Point3 &pos, Point3 *out_norm) { return false; }
vec3f custom_update_pregen_pos_y(vec4f pos, int16_t *, float, float) { return pos; }
void custom_get_land_min_max(BBox2, float &out_min, float &out_max)
{
  out_min = 0;
  out_max = 8192;
}
} // namespace rendinst::gen

// stub it
#include <eventLog/eventLog.h>
void event_log::send_udp(const char *, const void *, uint32_t, Json::Value *) {}
