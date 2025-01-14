// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/shaderResBuilder/rendInstResSrc.h>
#include <libTools/util/prepareBillboardMesh.h>
#include <gameRes/dag_stdGameRes.h>
#include "fatalHandler.h"
#include "modelExp.h"
#include <stdlib.h>

static const char *TYPE = "rendInst";
extern bool shadermeshbuilder_strip_d3dres;
static DataBlock *buildResultsBlk = NULL;

BEGIN_DABUILD_PLUGIN_NAMESPACE(rendInst)

class RendInstExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "rendInst exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return RendInstGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    const int ord_ver = 5;
    const int base_ver = 26 + ord_ver * 6 + (splitMatToDescBin && !shadermeshbuilder_strip_d3dres ? 3 : 0);
    return base_ver + (ShaderMeshData::preferZstdPacking ? (ShaderMeshData::allowOodlePacking ? 2 : 1) : 0);
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
    int lodNameId = lodsBlk.getNameId("lod");
    int plodNameId = lodsBlk.getNameId("plod");
    if (!lodsBlk.getBlockByName(lodNameId))
    {
      log.addMessage(ILogWriter::ERROR, "%s: no lods specified", a.getName());
      return false;
    }

    if (skipMostDetailedLOD)
    {
      int totalLods = 0;
      int mostDetailedLod = -1;
      for (int i = 0; DataBlock *blk = lodsBlk.getBlock(i); i++)
        if (blk->getBlockNameId() == lodNameId)
        {
          ++totalLods;
          // same logic as in simplified RI rendering option
          // remove most detailed lod if there is more than 2 lods available
          if (totalLods > 2)
          {
            debug("RI: skipping most detailed lod for %s", a.getName());
            lodsBlk.removeBlock(mostDetailedLod);
            break;
          }
          else if (totalLods == 1)
            mostDetailedLod = i;
        }
    }

    int lodNo = 0;
    int plodNo = 0;
    for (int i = 0; DataBlock *blk = lodsBlk.getBlock(i); i++)
    {
      if (blk->getBlockNameId() == lodNameId)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), lodNo++);
        blk->setStr("scene", String(260, "%s/%s", a.getFolderPath(), blk->getStr("fname", sn)));
      }
      if (blk->getBlockNameId() == plodNameId)
      {
        if (plodNo++ != 0)
          log.addMessage(ILogWriter::ERROR, "%s: plod can be only one!", a.getName());
        else
        {
          sn.printf(260, "%s.plod.dag", a.props.getStr("lod_fn_prefix", a.getName()));
          blk->setStr("scene", String(260, "%s/%s", a.getFolderPath(), blk->getStr("fname", sn)));
        }
      }
    }

    load_shaders_for_target(cwr.getTarget());
    setup_tex_subst(a_props);

    GenericTexSubstProcessMaterialData pm(a.getName(), get_process_mat_blk(a_props, TYPE),
      a.props.getBool("allowProxyMat", false) ? &a.getMgr() : nullptr, &log);
    RenderableInstanceLodsResSrc *resSrc = new RenderableInstanceLodsResSrc(&pm);
    resSrc->log = &log;

    const DataBlock *ri_blk = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst");

    ::generate_extra_in_prepare_billboard_mesh = ri_blk->getBool("useExtraPrepareBillboardMesh", false);

    bool prevIgnoreMappingInPrepareBillboardMesh = ::ignore_mapping_in_prepare_billboard_mesh;
    ::ignore_mapping_in_prepare_billboard_mesh = ri_blk->getBool("ignoreMappingInPrepareBillboardMesh", true);

    const DataBlock *generateBlk = ri_blk->getBlockByNameEx(mkbindump::get_target_str(cwr.getTarget()));

    bool prevGenerateQuads = ::generate_quads;
    ::generate_quads = generateBlk->getBool("generateQuads", false);

    bool prevGenerateStrips = ::generate_strips;
    ::generate_strips = generateBlk->getBool("generateStrips", false);

    RenderableInstanceLodsResSrc::optimizeForCache = a_props.getBool("optVertexCache", ri_blk->getBool("optVertexCache", true));

    RenderableInstanceLodsResSrc::deleteParameters.clear();
    if (auto shadersOptimizations = ri_blk->getBlockByName("shadersOptimizations"))
    {
      for (uint32_t b = 0; b < shadersOptimizations->blockCount(); b++)
      {
        auto block = shadersOptimizations->getBlock(b);
        if (!block || block->getBlockName() != eastl::string("deleteParametersFromLod"))
          continue;

        auto &params = RenderableInstanceLodsResSrc::deleteParameters.emplace_back();
        params.lod = block->getInt("lod", 2);
        if (auto shader_name = block->findParam("shader"); shader_name != -1)
          params.shader_name = block->getStr(shader_name);

        for (int i = block->findParam("parameters"); i != -1; i = block->findParam("parameters", i))
          params.parameters.emplace_back(block->getStr(i));
      }
    }

    if (const DataBlock *lodsToKeepBlk = ri_blk->getBlockByName("lodsToKeep"))
    {
      const char *pkname = a.getCustomPackageName(mkbindump::get_target_str(cwr.getTarget()), cwr.getProfile());
      int lodsToKeep = lodsToKeepBlk->getInt(pkname ? String(0, "package.%s", pkname).c_str() : "package.*", -1);
      lodsToKeep = lodsToKeep >= 0
                     ? lodsToKeep
                     : lodsToKeepBlk->getInt(cwr.getProfile() ? String(0, "profile.%s", cwr.getProfile()).c_str() : "profile.*", 16);
      for (int i = lodsBlk.blockCount() - 1; i >= 0; i--)
        if (lodsBlk.getBlock(i)->getBlockNameId() == lodNameId)
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
      RenderableInstanceLodsResSrc::buildResultsBlk = buildResultsBlk->addBlock(pkname ? pkname : ".")->addBlock(a.getName());
      RenderableInstanceLodsResSrc::sepMatToBuildResultsBlk = splitMatToDescBin && !ShaderMeshData::fastNoPacking;
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
    RenderableInstanceLodsResSrc::buildResultsBlk = NULL;
    RenderableInstanceLodsResSrc::sepMatToBuildResultsBlk = false;

    ::ignore_mapping_in_prepare_billboard_mesh = prevIgnoreMappingInPrepareBillboardMesh;
    ::generate_quads = prevGenerateQuads;
    ::generate_strips = prevGenerateStrips;

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
  bool skipMostDetailedLOD = false;
  RendInstExporter()
  {
    lvs.loadValidationSettings(*appBlkCopy.getBlockByNameEx("assets")
                                  ->getBlockByNameEx("build")
                                  ->getBlockByNameEx("rendInst")
                                  ->getBlockByNameEx("validateLODs"));

    splitMatToDescBin = appBlkCopy.getBlockByNameEx("assets")
                          ->getBlockByNameEx("build")
                          ->getBlockByNameEx("rendInst")
                          ->getBool("separateModelMatToDescBin", false) &&
                        appBlkCopy.getBlockByNameEx("assets")
                            ->getBlockByNameEx("build")
                            ->getBlockByNameEx("rendInst")
                            ->getStr("descListOutPath", nullptr) != nullptr;

    skipMostDetailedLOD = appBlkCopy.getBlockByNameEx("assets")
                            ->getBlockByNameEx("build")
                            ->getBlockByNameEx("rendInst")
                            ->getBool("skipMostDetailedLOD", false);
  }
};

class RendInstRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "rendInst refs"; }

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
    return tmp_refs;
  }
};

END_DABUILD_PLUGIN_NAMESPACE(rendInst)

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(rendInst)
IDagorAssetExporter *create_rendinst_exporter()
{
  static RendInstExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_rendinst_ref_provider()
{
  static RendInstRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
