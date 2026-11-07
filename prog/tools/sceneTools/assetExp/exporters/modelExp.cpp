// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "modelExp.h"
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetExpCache.h>
#include <assets/assetRefs.h>
#include <assets/assetRefsCache.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetMsgPipe.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <libTools/util/makeBindump.h>
#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <libTools/shaderResBuilder/rendInstResSrc.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/shaderResBuilder/validateAlphaTest.h>
#include <image/dag_loadImage.h>
#include <shaders/dag_shaders.h>
#include <../shaders/shadersBinaryData.h>
#include <startup/dag_startupTex.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_texMetaData.h>
#include <util/dag_simpleString.h>
#include <util/dag_strUtil.h>

extern bool shadermeshbuilder_strip_d3dres;

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
namespace modelexp
{
extern SimpleString texRefNamePrefix, texRefNameSuffix;
}
using namespace modelexp;

class PrefabRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "prefab refs"; }

  const char *__stdcall getAssetType() const override { return "prefab"; }

  void __stdcall onRegister() override
  {
    G_ASSERT(!refsCache);
    refsCache = AssetRefsCache::createCache(getAssetType(), PrefabGameResClassId, appBlkCopy);
  }

  void __stdcall onUnregister() override { refsCache.reset(); }

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    // NOTE: increase ASSET_REFERENCES_CACHE_FILE_VERSION if the asset reference gathering logic changes.

    setup_tex_subst(a.props);

    Tab<SimpleString> srcFiles(tmpmem);
    srcFiles.push_back() = a.getTargetFilePath();

    if (!refsCache || !refsCache->get(a, srcFiles, {}, refs))
    {
      refs.clear();
      const bool parsedOk = add_dag_texture_and_proxymat_refs(srcFiles[0], refs, a);
      if (refsCache)
        refsCache->put(a, srcFiles, {}, refs, parsedOk);
    }

    reset_tex_subst();
  }

private:
  static constexpr unsigned PrefabGameResClassId = 0x0BD4F106u; // Prefab

  eastl::unique_ptr<AssetRefsCache> refsCache;
};


static SimpleString shaderBinFname;
struct DabuildIntStrPair
{
  unsigned code;
  SimpleString fn;
};
static Tab<DabuildIntStrPair> shaderBinFnameAlt(inimem);
static unsigned curLoadedShaderTarget = 0;

// Content hash of the dump file that load_shaders_for_target() read for curLoadedShaderTarget.
static uint8_t curLoadedShaderBinMd5Hash[AssetExportCache::HASH_SZ] = {};

// The one dump that load_shaders_for_target() reads for the target.
static const char *get_shdump_for_target(unsigned tc)
{
  for (const DabuildIntStrPair &alt : shaderBinFnameAlt)
    if (alt.code == tc)
      return alt.fn;
  return shaderBinFname;
}

void add_shdump_deps(Tab<SimpleString> &files)
{
  if (shadermeshbuilder_strip_d3dres)
    return;
  files.push_back() = shaderBinFname;
  for (int i = 0; i < shaderBinFnameAlt.size(); i++)
    files.push_back() = shaderBinFnameAlt[i].fn;
}

bool get_shdump_id_for_ref_gather(bool may_process, uint8_t out_hash[AssetExportCache::HASH_SZ],
  dag::ConstSpan<uint8_t> &out_hash_span)
{
  out_hash_span = {};
  if (!may_process || shadermeshbuilder_strip_d3dres)
    return true;

  // Hash the dump that is loaded. When none is loaded yet, hash the file that the gathering will load.
  const unsigned tc = _MAKE4C('PC');
  if (curLoadedShaderTarget == tc)
    memcpy(out_hash, curLoadedShaderBinMd5Hash, AssetExportCache::HASH_SZ);
  else if (!AssetExportCache::sharedDataGetFileHash(get_shdump_for_target(tc), out_hash))
    return false;

  out_hash_span = make_span_const(out_hash, AssetExportCache::HASH_SZ);
  return true;
}

void load_shaders_for_target(unsigned tc)
{
  static const size_t suffix_len = strlen(".psXX.shdump.bin");
  if (tc == curLoadedShaderTarget)
    return;
  if (curLoadedShaderTarget)
    ::unload_shaders_bindump(true);

  curLoadedShaderTarget = tc;
  const char *fn = get_shdump_for_target(tc);

  if (!AssetExportCache::sharedDataGetFileHash(fn, curLoadedShaderBinMd5Hash))
    memset(curLoadedShaderBinMd5Hash, 0, sizeof(curLoadedShaderBinMd5Hash));

  d3d::shadermodel::Version ver = d3d::smAny;
  if (strstr(fn, ".ps66.shdump"))
    ver = 6.6_sm;
  else if (strstr(fn, ".ps60.shdump"))
    ver = 6.0_sm;
  else if (strstr(fn, ".ps50.shdump"))
    ver = 5.0_sm;
  else if (strstr(fn, ".ps40.shdump"))
    ver = 4.0_sm;
  else if (strstr(fn, ".ps30.shdump"))
    ver = 3.0_sm;
  if (::load_shaders_bindump(String(0, "%.*s", strlen(fn) - suffix_len, fn), ver, true))
    debug("loaded dabuild-specific shader dump (%c%c%c%c): %s", _DUMP4C(tc), fn);
  else
    DAG_FATAL("failed to load shaders: %s", fn);

  const DataBlock &build_blk = *appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build");
  if (build_blk.getBool("preferZSTD", false))
  {
    ShaderMeshData::preferZstdPacking = true;
    ShaderMeshData::zstdMaxWindowLog = build_blk.getInt("zstdMaxWindowLog", 0);
    ShaderMeshData::zstdCompressionLevel = build_blk.getInt("zstdCompressionLevel", 18);
    debug("ShaderMesh prefers ZSTD (compressionLev=%d %s)", ShaderMeshData::zstdCompressionLevel,
      ShaderMeshData::zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << ShaderMeshData::zstdMaxWindowLog).c_str() : "defaultWindow");
  }
  if (build_blk.getBool("allowOODLE", false))
  {
    ShaderMeshData::allowOodlePacking = true;
    debug("ShaderMesh allows OODLE");
  }
  if (build_blk.getBool("preferZLIB", false))
  {
    ShaderMeshData::forceZlibPacking = true;
    if (!ShaderMeshData::preferZstdPacking) //-V1051
      debug("ShaderMesh prefers ZLIB");
  }
}

String validate_texture_types(const char *_tex_name, const char *class_name, int slot, DagorAsset &a)
{
  load_shaders_for_target(_MAKE4C('PC'));

  auto *sh_class = shBinDumpEx(false).findShaderClass(class_name);

  if (!sh_class || sh_class->staticTextureTypeBySlot.empty())
    return {};

  if (slot >= sh_class->staticTextureTypeBySlot.size())
    return {};

  auto texture_type = sh_class->staticTextureTypeBySlot[slot];
  if (texture_type == SHVT_TEX_UNKNOWN)
    return {};
  bool is_valid_texture_type = true;

  String tmp_stor;
  _tex_name = TextureMetaData::decodeFileName(_tex_name, &tmp_stor);
  String tex_name(0, "%s%s%s", texRefNamePrefix, DagorAsset::fpath2asset(_tex_name), texRefNameSuffix);

  DagorAsset *tex_a = a.getMgr().findAsset(tex_name, a.getMgr().getTexAssetTypeId());
  if (!tex_a)
    return {};

  const char *tex_type_str = tex_a->props.getStr("texType", nullptr);
  if (!tex_type_str || strcmp(tex_type_str, "tex2D") == 0)
    is_valid_texture_type = texture_type == ShaderVarTextureType::SHVT_TEX_2D;
  else if (strcmp(tex_type_str, "cube") == 0)
    is_valid_texture_type = texture_type == ShaderVarTextureType::SHVT_TEX_CUBE;
  else if (strcmp(tex_type_str, "tex3D") == 0)
    is_valid_texture_type = texture_type == ShaderVarTextureType::SHVT_TEX_3D;

  if (!is_valid_texture_type)
  {
    const char *tex_type_in_shader = "unknown";
    switch (texture_type)
    {
      case SHVT_TEX_2D: tex_type_in_shader = "tex2D"; break;
      case SHVT_TEX_3D: tex_type_in_shader = "tex3D"; break;
      case SHVT_TEX_CUBE: tex_type_in_shader = "cube"; break;
      case SHVT_TEX_2D_ARRAY: tex_type_in_shader = "tex2DArray"; break;
      case SHVT_TEX_CUBE_ARRAY: tex_type_in_shader = "cubeArray"; break;
      default: return {}; // some bad/uninited type in slot
    }
    return String(0, "static tex type '%s' in slot #%d does not match type '%s' (declared in '%s' shader): texture %s",
      tex_type_str ? tex_type_str : "tex2D", slot, tex_type_in_shader, class_name, tex_name);
  }
  return {};
}

class ModelExporterPlugin : public IDaBuildPlugin, public ITextureNameResolver
{
public:
  ModelExporterPlugin()
  {
    riExp = dmExp = skExp = rgExp = NULL;
    riRef = dmRef = rgRef = NULL;
  }
  ~ModelExporterPlugin()
  {
    AlphaTestValidation::shutdown(atest_validation_ctx);
    atest_validation_ctx = nullptr;
    riExp = dmExp = skExp = rgExp = NULL;
    riRef = dmRef = rgRef = NULL;
  }

  bool __stdcall init(const DataBlock &appblk) override
  {
    const char *sh_file = appblk.getStr("shadersAbs", "compiledShaders/tools");
    const char *ver_suffix = "ps50.shdump.bin";
    shaderBinFname = NULL;
    String full_sh_file;
    full_sh_file.printf(260, "%s.exp.%s", sh_file, ver_suffix);
    if (!dd_file_exist(full_sh_file))
      full_sh_file.printf(260, "%s.exp.%s", sh_file, ver_suffix = "ps40.shdump.bin");
    if (!dd_file_exist(full_sh_file))
      full_sh_file.printf(260, "%s.exp.%s", sh_file, ver_suffix = "ps30.shdump.bin");

    if (dd_file_exist(full_sh_file))
    {
      static unsigned codes[] = {
        _MAKE4C('PC'),
        _MAKE4C('iOS'),
        _MAKE4C('and'),
        _MAKE4C('PS4'),
      };

      shaderBinFname = String(260, "%s.exp.%s", sh_file, ver_suffix);
      debug("using dabuild-specific shader dump:        %s", shaderBinFname.str());
      for (int i = 0; i < countof(codes); i++)
      {
        uint64_t tc_storage = 0;
        String fn(0, "%s.%s.exp.%s", sh_file, mkbindump::get_target_str(codes[i], tc_storage), ver_suffix);
        if (dd_file_exist(fn))
        {
          DabuildIntStrPair &pair = shaderBinFnameAlt.push_back();
          pair.code = codes[i];
          pair.fn = fn;
          debug("using dabuild-specific shader dump (%c%c%c%c): %s", _DUMP4C(pair.code), pair.fn);
        }
      }
    }
    else
    {
      full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps50.shdump.bin");
      if (!dd_file_exist(full_sh_file))
        full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps40.shdump.bin");
      if (!dd_file_exist(full_sh_file))
        full_sh_file.printf(260, "%s.%s", sh_file, ver_suffix = "ps30.shdump.bin");
      if (dd_file_exist(full_sh_file))
        shaderBinFname = full_sh_file;
    }
    if (shaderBinFname.empty())
    {
      logerr("modelExp-dev.dll: dabuild plugin not loaded due to missing shaders <%s>", sh_file);
      return false;
    }

    appBlkCopy.setFrom(&appblk, appblk.resolveFilename());
    set_global_tex_name_resolver(this);
    ::shadermeshbuilder_strip_d3dres = appblk.getBool("strip_d3dres", false);

    register_tga_tex_load_factory();
    register_tiff_tex_load_factory();
    register_psd_tex_load_factory();
    register_jpeg_tex_load_factory();
    register_png_tex_load_factory();
    register_avif_tex_load_factory();

    const DataBlock *dm_blk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("dynModel");
    const DataBlock *ri_blk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst");
    if (const DataBlock *remap_blk = dm_blk->getBlockByName("remapShaders"))
      DynamicRenderableSceneLodsResSrc::setupMatSubst(*remap_blk);
    if (const DataBlock *remap_blk = ri_blk->getBlockByName("remapShaders"))
      RenderableInstanceLodsResSrc::setupMatSubst(*remap_blk);
    if (const DataBlock *b = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByName("validateAlphaTest"))
    {
      atest_validation_ctx = AlphaTestValidation::init(
        *b, []() -> DagorAssetMgr * { return cur_asset ? &cur_asset->getMgr() : nullptr; },
        [](const char *path, IMemAlloc *mem) { return load_image(path, mem); });
    }
    set_missing_texture_usage(false);
    set_missing_texture_name("", false);
    GlobalVertexDataConnector::allowVertexMerge = true;
    GlobalVertexDataConnector::allowBaseVertex = true;
    const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
    if (build_blk.getBool("preferZSTD", false))
    {
      ShaderMeshData::preferZstdPacking = true;
      ShaderMeshData::zstdMaxWindowLog = build_blk.getInt("zstdMaxWindowLog", 0);
      ShaderMeshData::zstdCompressionLevel = build_blk.getInt("zstdCompressionLevel", 18);
      debug("ShaderMesh prefers ZSTD (compressionLev=%d %s)", ShaderMeshData::zstdCompressionLevel,
        ShaderMeshData::zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << ShaderMeshData::zstdMaxWindowLog).c_str() : "defaultWindow");
    }
    if (build_blk.getBool("allowOODLE", false))
    {
      ShaderMeshData::allowOodlePacking = true;
      debug("ShaderMesh allows OODLE");
    }
    if (first_managed_d3dres() == BAD_TEXTUREID)
      enable_tex_mgr_mt(true, appblk.getInt("texMgrReserveTIDcount", 128 << 10));
    return true;
  }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 4; }
  const char *__stdcall getExpType(int idx) override
  {
    switch (idx)
    {
      case 0: return "rendInst";
      case 1: return "dynModel";
      case 2: return "skeleton";
      case 3: return "rndGrass";
      default: return NULL;
    }
  }
  IDagorAssetExporter *__stdcall getExp(int idx) override
  {
    switch (idx)
    {
      case 0:
        if (!riExp)
          riExp = create_rendinst_exporter();
        return riExp;
      case 1:
        if (!dmExp)
          dmExp = create_dynmodel_exporter();
        return dmExp;
      case 2:
        if (!skExp)
          skExp = create_skeleton_exporter();
        return skExp;
      case 3:
        if (!rgExp)
          rgExp = create_rndgrass_exporter();
        return rgExp;
      default: return NULL;
    }
  }

  int __stdcall getRefProvCount() override { return 4; }
  const char *__stdcall getRefProvType(int idx) override
  {
    switch (idx)
    {
      case 0: return "rendInst";
      case 1: return "dynModel";
      case 2: return "prefab";
      case 3: return "rndGrass";
      default: return NULL;
    }
  }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override
  {
    switch (idx)
    {
      case 0:
        if (!riRef)
          riRef = create_rendinst_ref_provider();
        return riRef;
      case 1:
        if (!dmRef)
          dmRef = create_dynmodel_ref_provider();
        return dmRef;
      case 2: return &pfRefs;
      case 3:
        if (!rgRef)
          rgRef = create_rndgrass_ref_provider();
        return rgRef;

      default: return NULL;
    }
  }

  bool resolveTextureName(const char *src_name, String &out_str) override
  {
    if (!cur_asset)
      return false;

    if (!src_name || !*src_name)
    {
      cur_log->addMessage(cur_log->WARNING, "empty texname in %s", getCurrentDagName());
      return false;
    }

    const char *q_prefix = src_name[0] == '<' ? "<" : (src_name[0] == '>' ? ">" : "");
    DagorAssetMgr &mgr = cur_asset->getMgr();
    String tmp_stor;
    out_str.printf(64, "%s%s%s", texRefNamePrefix.str(), DagorAsset::fpath2asset(TextureMetaData::decodeFileName(src_name, &tmp_stor)),
      texRefNameSuffix.str());
    DagorAsset *tex_a = mgr.findAsset(out_str, mgr.getTexAssetTypeId());

    if (tex_a)
    {
      out_str.printf(64, "%s%s*", q_prefix, tex_a->getName());
      dd_strlwr(out_str);
    }
    else
    {
      cur_log->addMessage(cur_log->ERROR, "tex %s for %s not found (src tex is %s)", out_str.str(), getCurrentDagName(), src_name);
      return false;
    }
    return true;
  }

protected:
  IDagorAssetExporter *riExp, *dmExp, *skExp, *rgExp;
  IDagorAssetRefProvider *riRef, *dmRef, *rgRef;
  PrefabRefs pfRefs;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) ModelExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
REGISTER_DABUILD_PLUGIN(modelExp, nullptr)
