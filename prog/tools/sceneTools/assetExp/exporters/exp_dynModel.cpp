// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <libTools/util/prepareBillboardMesh.h>
#include <gameRes/dag_stdGameRes.h>
#include "fatalHandler.h"
#include "modelExp.h"
#include <stdlib.h>


static const char *TYPE = "dynModel";
static DataBlock *buildResultsBlk = NULL;
extern bool dynmodel_exp_empty_bone_names_allowed;

#define DECL_BUILD_PROP(TYPE, VAR) \
  extern TYPE VAR;                 \
  static TYPE def_##VAR = VAR

DECL_BUILD_PROP(int, dynmodel_max_bones_count);
DECL_BUILD_PROP(int, dynmodel_max_vpr_const_count);
DECL_BUILD_PROP(bool, dynmodel_use_direct_bones_array);
DECL_BUILD_PROP(bool, dynmodel_use_direct_bones_array_combined_lods);
DECL_BUILD_PROP(bool, dynmodel_optimize_direct_bones);

#undef DECL_BUILD_PROP

extern bool shadermeshbuilder_strip_d3dres;

BEGIN_DABUILD_PLUGIN_NAMESPACE(dynModel)

class DynModelExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "dynModel exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return DynModelGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    const int ord_ver = 1;
    const int base_ver = 55 + ord_ver * 10 + (splitMatToDescBin && !shadermeshbuilder_strip_d3dres ? 5 : 0);
    if (ShaderMeshData::preferZstdPacking)
      return base_ver + (shadermeshbuilder_strip_d3dres ? 3 : (ShaderMeshData::allowOodlePacking ? 4 : 2));
    return base_ver + (shadermeshbuilder_strip_d3dres ? 1 : 0);
  }

  virtual void __stdcall onRegister() { buildResultsBlk = NULL; }
  virtual void __stdcall onUnregister() { buildResultsBlk = NULL; }

  virtual void __stdcall setBuildResultsBlk(DataBlock *b) { buildResultsBlk = b; }

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    int nid = a.props.getNameId("lod"), id = 0;
    const char *basePath = a.getFolderPath();
    String s, sn;
    bool allow_proxymat = a.props.getBool("allowProxyMat", false);

    files.clear();

    for (int i = 0; const DataBlock *blk = a.props.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        s.printf(260, "%s/%s", basePath, blk->getStr("fname", sn));
        files.push_back() = s;
        if (allow_proxymat)
          add_proxymat_dep_files(s, files, a.getMgr());
      }
      else
      {
        int saved_id = id;
        id = 0;
        for (int j = 0; const DataBlock *cblk = blk->getBlock(j); j++)
          if (cblk->getBlockNameId() == nid)
          {
            sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
            s.printf(260, "%s/%s", basePath, cblk->getStr("fname", sn));
            files.push_back() = s;
            if (allow_proxymat)
              add_proxymat_dep_files(s, files, a.getMgr());
          }
        id = saved_id;
      }
    add_shdump_deps(files);
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall buildAssetFast(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    ShaderMeshData::fastNoPacking = true;
    bool ret = exportAsset(a, cwr, log);
    ShaderMeshData::fastNoPacking = false;
    return ret;
  }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    ShaderMeshData::reset_channel_cvt_errors();
    AutoContext auto_ctx(a, log);
    String sn;

    DataBlock lodsBlk;
    const DataBlock &a_props = a.getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
    lodsBlk.setFrom(&a_props);
    if (!lodsBlk.blockExists("lod"))
    {
      log.addMessage(ILogWriter::ERROR, "%s: no lods specified", a.getName());
      return false;
    }

    int nid = lodsBlk.getNameId("lod"), id = 0;
    for (int i = 0; DataBlock *blk = lodsBlk.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        blk->setStr("scene", String(260, "%s/%s", a.getFolderPath(), blk->getStr("fname", sn)));
      }

    lodsBlk.setInt("addHdrResv", 0);

    load_shaders_for_target(cwr.getTarget());
    setup_tex_subst(a_props);

    GenericTexSubstProcessMaterialData pm(a.getName(), get_process_mat_blk(a_props, TYPE),
      a.props.getBool("allowProxyMat", false) ? &a.getMgr() : nullptr, &log);
    DynamicRenderableSceneLodsResSrc *resSrc = new DynamicRenderableSceneLodsResSrc(&pm);
    resSrc->log = &log;
    const DataBlock *dm_blk = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("dynModel");

    ::generate_extra_in_prepare_billboard_mesh = dm_blk->getBool("useExtraPrepareBillboardMesh", false);

    bool prevIgnoreMappingInPrepareBillboardMesh = ::ignore_mapping_in_prepare_billboard_mesh;
    ::ignore_mapping_in_prepare_billboard_mesh = dm_blk->getBool("ignoreMappingInPrepareBillboardMesh", true);

    bool prevEmptyBoneNamesAllowed = ::dynmodel_exp_empty_bone_names_allowed;
    ::dynmodel_exp_empty_bone_names_allowed = dm_blk->getBool("emptyBoneNamesAllowed", false);

#define READ_BUILD_PROP(VAR, NM, TYPE) \
  ::VAR = dm_blk->getBlockByNameEx(mkbindump::get_target_str(cwr.getTarget()))->get##TYPE(NM, dm_blk->get##TYPE(NM, ::def_##VAR))

    READ_BUILD_PROP(dynmodel_max_bones_count, "maxBonesCount", Int);
    READ_BUILD_PROP(dynmodel_max_vpr_const_count, "maxVPRConst", Int);
    READ_BUILD_PROP(dynmodel_use_direct_bones_array, "useDirectBones", Bool);
    READ_BUILD_PROP(dynmodel_use_direct_bones_array_combined_lods, "useDirectBonesCombinedLods", Bool);
    READ_BUILD_PROP(dynmodel_optimize_direct_bones, "optimizeDirectBones", Bool);

    DynamicRenderableSceneLodsResSrc::optimizeForCache = a_props.getBool("optVertexCache", dm_blk->getBool("optVertexCache", true));
    DynamicRenderableSceneLodsResSrc::limitBonePerVertex =
      a_props.getInt("limitBonePerVertex", dm_blk->getInt("limitBonePerVertex", -1));
    DynamicRenderableSceneLodsResSrc::setBonePerVertex = a_props.getInt("setBonePerVertex", dm_blk->getInt("setBonePerVertex", -1));
    dynmodel_use_direct_bones_array_combined_lods =
      a_props.getBool("useDirectBonesCombinedLods", dynmodel_use_direct_bones_array_combined_lods);

#undef READ_BUILD_PROP

    if (const DataBlock *lodsToKeepBlk = dm_blk->getBlockByName("lodsToKeep"))
    {
      const char *pkname = a.getCustomPackageName(mkbindump::get_target_str(cwr.getTarget()), cwr.getProfile());
      int lodsToKeep = lodsToKeepBlk->getInt(pkname ? String(0, "package.%s", pkname).c_str() : "package.*", -1);
      lodsToKeep = lodsToKeep >= 0
                     ? lodsToKeep
                     : lodsToKeepBlk->getInt(cwr.getProfile() ? String(0, "profile.%s", cwr.getProfile()).c_str() : "profile.*", 16);
      for (int i = lodsBlk.blockCount() - 1; i >= 0; i--)
        if (lodsBlk.getBlock(i)->getBlockNameId() == nid)
        {
          if (lodsToKeep > 0)
            lodsToKeep--;
          else
            lodsBlk.removeBlock(i);
        }
    }

    if (buildResultsBlk)
    {
      const char *pkname = a.getCustomPackageName(mkbindump::get_target_str(cwr.getTarget()), cwr.getProfile());
      DynamicRenderableSceneLodsResSrc::buildResultsBlk = buildResultsBlk->addBlock(pkname ? pkname : ".")->addBlock(a.getName());
      DynamicRenderableSceneLodsResSrc::sepMatToBuildResultsBlk = splitMatToDescBin && !ShaderMeshData::fastNoPacking;
    }

    install_fatal_handler();
    ShaderMeshData::buildForTargetCode = cwr.getTarget();
    DAGOR_TRY
    {
      if (!resSrc->build(lodsBlk))
        mark_there_were_fatals();
      if (!were_there_fatals())
        if (!resSrc->save(cwr, &lvs, log))
          mark_there_were_fatals();
    }
    DAGOR_CATCH(int code)
    {
#if DAGOR_EXCEPTIONS_ENABLED
      if (code == 777)
        log.addMessage(ILogWriter::ERROR, "error during export:\n%s", fatalMessages[0].str());
#endif
    }
    ShaderMeshData::buildForTargetCode = _MAKE4C('PC');
    DynamicRenderableSceneLodsResSrc::buildResultsBlk = nullptr;
    DynamicRenderableSceneLodsResSrc::sepMatToBuildResultsBlk = false;

    ::ignore_mapping_in_prepare_billboard_mesh = prevIgnoreMappingInPrepareBillboardMesh;
    ::dynmodel_exp_empty_bone_names_allowed = prevEmptyBoneNamesAllowed;
    dynmodel_use_direct_bones_array_combined_lods = false;

    delete resSrc;
    remove_fatal_handler();
    reset_tex_subst();

    int crit_cvt_err = ShaderMeshData::get_channel_cvt_critical_errors();
    if (ShaderMeshData::get_channel_cvt_errors())
      debug("%s:  %d channel conversion errors (%d critical)", a.getName(), ShaderMeshData::get_channel_cvt_errors(), crit_cvt_err);

    ShaderMeshData::reset_channel_cvt_errors();
    if (crit_cvt_err)
    {
      log.addMessage(ILogWriter::FATAL, "%s: %d critical channel conversion errors", a.getName(), crit_cvt_err);
      return false;
    }
    return !were_there_fatals();
  }

  LodValidationSettings lvs;
  bool splitMatToDescBin = false;
  DynModelExporter()
  {
    lvs.loadValidationSettings(*appBlkCopy.getBlockByNameEx("assets")
                                  ->getBlockByNameEx("build")
                                  ->getBlockByNameEx("dynModel")
                                  ->getBlockByNameEx("validateLODs"));

    splitMatToDescBin = appBlkCopy.getBlockByNameEx("assets")
                          ->getBlockByNameEx("build")
                          ->getBlockByNameEx("dynModel")
                          ->getBool("separateModelMatToDescBin", false) &&
                        appBlkCopy.getBlockByNameEx("assets")
                            ->getBlockByNameEx("build")
                            ->getBlockByNameEx("dynModel")
                            ->getStr("descListOutPath", nullptr) != nullptr;
  }
};

class DynModelRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "dynModel refs"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override
  {
    int nid = a.props.getNameId("lod"), id = 0;
    const char *basePath = a.getFolderPath();
    String fn, sn;

    tmp_refs.clear();

    setup_tex_subst(a.props);
    GenericTexSubstProcessMaterialData pm(a.getName(), get_process_mat_blk(a.props, TYPE),
      a.props.getBool("allowProxyMat", false) ? &a.getMgr() : nullptr);
    for (int i = 0; DataBlock *blk = a.props.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        fn.printf(260, "%s/%s", basePath, blk->getStr("fname", sn));
        add_dag_texture_and_proxymat_refs(fn, tmp_refs, a, pm.mayProcess() ? &pm : nullptr);
      }
    reset_tex_subst();
    return tmp_refs;
  }
};

END_DABUILD_PLUGIN_NAMESPACE(dynModel)

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(dynModel)
IDagorAssetExporter *create_dynmodel_exporter()
{
  static DynModelExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_dynmodel_ref_provider()
{
  static DynModelRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
