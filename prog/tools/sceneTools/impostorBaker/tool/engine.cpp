// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "engine.h"

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_stdGameResId.h>
#include <generic/dag_span.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_texMetaData.h>
#include <assets/assetExpCache.h>
#include <de3_dxpFactory.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <rendInst/rendInstGen.h>
#include <libTools/util/fileUtils.h>

#include <libTools/shaderResBuilder/shaderMeshData.h>

#include <assets/asset.h>
#include <assets/assetHlp.h>

#include <EditorCore/ec_interface.h>
#include <oldEditor/de_interface.h>

IDaEditor3Engine *IDaEditor3Engine::__daeditor3_global_instance = nullptr;
IEditorCoreEngine *IEditorCoreEngine::__global_instance = nullptr;
IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = nullptr;

DaEditor3Engine::DaEditor3Engine()
{
  console.startLog();
  IDaEditor3Engine::set(this);
}

DaEditor3Engine::~DaEditor3Engine()
{
  AssetExportCache::saveSharedData();
  dabuildcache::term();
  texconvcache::term();
  assetMgr.enableChangesTracker(false);
  assetMgr.clear();
  assetrefs::unload_plugins(assetMgr);

  IDaEditor3Engine::set(nullptr);
  console.endLog();
}

int DaEditor3Engine::getAssetTypeId(const char *entity_name) const { return assetMgr.getAssetTypeId(entity_name); }

dag::ConstSpan<DagorAsset *> DaEditor3Engine::getAssets() const { return assetMgr.getAssets(); }

DagorAsset *DaEditor3Engine::getAssetByName(const char *asset_name, int asset_type)
{
  return assetMgr.findAsset(asset_name, asset_type);
}

DagorAsset *DaEditor3Engine::getAssetByName(const char *asset_name, dag::ConstSpan<int> asset_types)
{
  return assetMgr.findAsset(asset_name, asset_types);
}

void DaEditor3Engine::addMessage(ILogWriter::MessageType type, const char *s)
{
  console.addMessage(type, s);
  debug("CON[%d]: %s", type, s);
}

void DaEditor3Engine::conMessageV(ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum)
{
  String s;
  s.vprintf(1024, msg, arg, anum);
  addMessage(type, s.c_str());
}

void DaEditor3Engine::conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::ERROR, fmt, arg, anum); }

void DaEditor3Engine::conWarningV(const char *fmt, const DagorSafeArg *arg, int anum)
{
  conMessageV(ILogWriter::WARNING, fmt, arg, anum);
}

void DaEditor3Engine::conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::NOTE, fmt, arg, anum); }

void DaEditor3Engine::conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum)
{
  conMessageV(ILogWriter::REMARK, fmt, arg, anum);
}

bool DaEditor3Engine::initAssetBase(const char *app_dir)
{
  char start_dir[260] = "";
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));

  String fname(260, "%sapplication.blk", app_dir);
  DataBlock appblk;

  assetMgr.clear();
  assetMgr.setMsgPipe(&msgPipe);

  if (!appblk.load(fname))
  {
    debug("cannot load %s", fname.str());
    return false;
  }
  appblk.setStr("appDir", app_dir);
  DataBlock::setRootIncludeResolver(app_dir);

  // load asset base
  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  int base_nid = blk.getNameId("base");

  assetMgr.setupAllowedTypes(*blk.getBlockByNameEx("types"), blk.getBlockByName("export"));
  for (int i = 0; srcAssetsScanAllowed && i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == base_nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
    {
      fname.printf(260, "%s%s", app_dir, blk.getStr(i));
      assetMgr.loadAssetsBase(fname, "global");
    }
  if (!minimizeDabuildUsage) // prefer real texture assets to gameres loaded from *.dxp.bin
  {
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    for (int i = 0; i < assets.size(); i++)
      if (assets[i]->getType() == assetMgr.getTexAssetTypeId())
      {
        TEXTUREID tid = get_managed_texture_id(String(0, "%s*", assets[i]->getName()));
        if (tid != BAD_TEXTUREID)
        {
          console.addMessage(ILogWriter::NOTE, "using texture %s (tid=%d) from asset, not from *.DxP.bin", assets[i]->getName(), tid);
          evict_managed_tex_id(tid);
        }
      }
  }

  if (blk.getStr("fmodEvents", nullptr))
    assetMgr.mountFmodEvents(blk.getStr("fmodEvents", nullptr));

  if (false)
  {
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    debug("%d assets", assets.size());

    for (int i = 0; i < assets.size(); i++)
    {
      const DagorAsset &a = *assets[i];
      debug("  [%d] <%s> name=%s nspace=%s(%d) isVirtual=%d folder=%d srcPath=%s", i, assetMgr.getAssetTypeName(a.getType()),
        a.getName(), a.getNameSpace(), a.isGloballyUnique(), a.isVirtual(), a.getFolderIndex(), a.getSrcFilePath());
    }
  }

  // add texture assets to texmgr
  if (::get_gameres_sys_ver() == 2)
  {
    int atype = assetMgr.getTexAssetTypeId();
    dag::ConstSpan<DagorAsset *> assets = assetMgr.getAssets();
    int hqtex_ns = assetMgr.getAssetNameSpaceId("texHQ");
    int tqtex_ns = assetMgr.getAssetNameSpaceId("texTQ");
    for (int i = 0; i < assets.size(); i++)
      if (assets[i]->getType() == atype && assets[i]->getNameSpaceId() != hqtex_ns && assets[i]->getNameSpaceId() != tqtex_ns)
      {
        TextureMetaData tmd;
        tmd.read(assets[i]->props, "PC");
        SimpleString nm(tmd.encode(String(128, "%s*", assets[i]->getName())));
        if (get_managed_texture_id(nm) == BAD_TEXTUREID)
          add_managed_texture(nm);
      }
  }
  assetlocalprops::init(app_dir, "develop/.asset-local");
  AssetExportCache::createSharedData(assetlocalprops::makePath("assets-hash.bin"));

  if (dabuildUsageAllowed)
    dabuildUsageAllowed = appblk.getBool("dagored_use_dabuild", true);
  if (dabuildUsageAllowed)
  {
    if (!minimizeDabuildUsage)
    {
      G_ASSERT(dabuildcache::init(start_dir, &console));
      G_ASSERT(dabuildcache::bind_with_mgr(assetMgr, appblk, app_dir) >= 0);
    }
    if (texconvcache::init(assetMgr, appblk, start_dir, false, true))
    {
      addMessage(ILogWriter::NOTE, "texture conversion cache inited");
      int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", start_dir));
      debug("loaded %d DDSx export plugin(s)", pc);
    }
    if (assetrefs::load_plugins(assetMgr, appblk, start_dir, !minimizeDabuildUsage))
      addMessage(ILogWriter::NOTE, "asset refs plugins inited");
  }
  const DataBlock &projDefBlk = *appblk.getBlockByNameEx("projectDefaults");
  if (bool zstd = projDefBlk.getBool("preferZSTD", false))
  {
    ShaderMeshData::preferZstdPacking = zstd;
    ShaderMeshData::zstdMaxWindowLog = projDefBlk.getInt("zstdMaxWindowLog", 0);
    ShaderMeshData::zstdCompressionLevel = projDefBlk.getInt("zstdCompressionLevel", 18);
    debug("ShaderMesh prefers ZSTD (compressionLev=%d %s)", ShaderMeshData::zstdCompressionLevel,
      ShaderMeshData::zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << ShaderMeshData::zstdMaxWindowLog).c_str() : "defaultWindow");
  }
  if (bool oodle = projDefBlk.getBool("allowOODLE", false))
  {
    ShaderMeshData::allowOodlePacking = oodle;
    debug("ShaderMesh allows OODLE");
  }
  if (bool zlib = projDefBlk.getBool("preferZLIB", false))
  {
    ShaderMeshData::forceZlibPacking = zlib;
    if (!ShaderMeshData::preferZstdPacking)
      debug("ShaderMesh prefers ZLIB");
  }

  const DataBlock &b = *appblk.getBlockByNameEx("tex_shader_globvar");
  for (int i = 0; i < b.paramCount(); i++)
    if (b.getParamType(i) == DataBlock::TYPE_STRING)
    {
      int gvid = get_shader_glob_var_id(b.getParamName(i), true);
      if (gvid < 0)
      {
        logwarn("cannot set tex <%s> to var <%s> - shader globvar is missing", b.getStr(i), b.getParamName(i));
        console.addMessage(ILogWriter::WARNING, "cannot set tex <%s> to var <%s> - shader globvar is missing", b.getStr(i),
          b.getParamName(i));
      }
      else
      {
        TEXTUREID tid = get_managed_texture_id(b.getStr(i));
        if (tid == BAD_TEXTUREID)
        {
          logwarn("cannot set tex <%s> to var <%s> - texture is missing", b.getStr(i), b.getParamName(i));
          console.addMessage(ILogWriter::WARNING, "cannot set tex <%s> to var <%s> - texture is missing", b.getStr(i),
            b.getParamName(i));
        }
        else
        {
          ShaderGlobal::set_texture_fast(gvid, tid);
          debug("set tex <%s> to globvar <%s>", b.getStr(i), b.getParamName(i));
          console.addMessage(ILogWriter::NOTE, "set tex <%s> to globvar <%s>", b.getStr(i), b.getParamName(i));
        }
      }
    }
  return true;
}

DagorAssetMgr &DaEditor3Engine::getAssetManager() { return assetMgr; }

const DagorAssetMgr &DaEditor3Engine::getAssetManager() const { return assetMgr; }
