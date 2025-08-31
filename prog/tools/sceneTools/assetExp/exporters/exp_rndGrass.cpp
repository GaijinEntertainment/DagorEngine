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

static const char *TYPE = "rndGrass";
extern bool shadermeshbuilder_strip_d3dres;

BEGIN_DABUILD_PLUGIN_NAMESPACE(rndGrass)

class RndGrassExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "rndGrass exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return RndGrassGameResClassId; }
  unsigned __stdcall getGameResVersion() const override
  {
    const int base_ver = 6; // increment by 3
    return base_ver + (ShaderMeshData::preferZstdPacking ? (ShaderMeshData::allowOodlePacking ? 2 : 1) : 0);
  }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall setBuildResultsBlk(DataBlock *b) override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    int nid = a.props.getNameId("lod"), id = 0;
    const char *basePath = a.getFolderPath();
    String s, sn;

    files.clear();

    for (int i = 0; const DataBlock *blk = a.props.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        s.printf(260, "%s/%s", basePath, blk->getStr("fname", sn));
        files.push_back() = s;
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
          }
        id = saved_id;
      }
    add_shdump_deps(files);
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    ShaderMeshData::reset_channel_cvt_errors();
    AutoContext auto_ctx(a, log);
    String sn;

    DataBlock lodsBlk;
    const DataBlock &a_props = a.getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
    lodsBlk.setFrom(&a_props);
    if (!lodsBlk.getBlockByName("lod"))
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

    load_shaders_for_target(cwr.getTarget());
    setup_tex_subst(a_props);

    RenderableInstanceLodsResSrc *resSrc = new RenderableInstanceLodsResSrc;
    resSrc->log = &log;

    ::generate_extra_in_prepare_billboard_mesh = false;

    bool prevIgnoreMappingInPrepareBillboardMesh = ::ignore_mapping_in_prepare_billboard_mesh;
    ::ignore_mapping_in_prepare_billboard_mesh = true;

    bool prevGenerateQuads = ::generate_quads;
    ::generate_quads = false;

    bool prevGenerateStrips = ::generate_strips;
    ::generate_strips = false;

    RenderableInstanceLodsResSrc::optimizeForCache = true;
    RenderableInstanceLodsResSrc::setupMatSubst(*appBlkCopy.getBlockByNameEx("assets")
                                                   ->getBlockByNameEx("build")
                                                   ->getBlockByNameEx("rndGrass")
                                                   ->getBlockByNameEx("remapShaders"));

    install_fatal_handler();
    ShaderMeshData::buildForTargetCode = cwr.getTarget();
    DAGOR_TRY
    {
      if (!resSrc->build(lodsBlk))
        mark_there_were_fatals();
      if (!were_there_fatals())
        if (!resSrc->save(cwr, nullptr, log))
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

    ::ignore_mapping_in_prepare_billboard_mesh = prevIgnoreMappingInPrepareBillboardMesh;
    ::generate_quads = prevGenerateQuads;
    ::generate_strips = prevGenerateStrips;
    RenderableInstanceLodsResSrc::setupMatSubst(*appBlkCopy.getBlockByNameEx("assets")
                                                   ->getBlockByNameEx("build")
                                                   ->getBlockByNameEx("rendInst")
                                                   ->getBlockByNameEx("remapShaders"));

    delete resSrc;
    remove_fatal_handler();
    reset_tex_subst();

    int crit_cvt_err = ShaderMeshData::get_channel_cvt_critical_errors();
    if (ShaderMeshData::get_channel_cvt_errors())
      log.addMessage(ILogWriter::FATAL, "%s:  %d channel conversion errors (%d critical)", a.getName(),
        ShaderMeshData::get_channel_cvt_errors(), crit_cvt_err);

    ShaderMeshData::reset_channel_cvt_errors();
    if (crit_cvt_err)
    {
      log.addMessage(ILogWriter::FATAL, "%s: %d critical channel conversion errors", a.getName(), crit_cvt_err);
      return false;
    }
    return !were_there_fatals();
  }
};

class RndGrassRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "rndGrass refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    int nid = a.props.getNameId("lod"), id = 0;
    const char *basePath = a.getFolderPath();
    String fn, sn;

    refs.clear();

    setup_tex_subst(a.props);
    for (int i = 0; DataBlock *blk = a.props.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        fn.printf(260, "%s/%s", basePath, blk->getStr("fname", sn));
        add_dag_texture_and_proxymat_refs(fn, refs, a);
      }
  }
};

END_DABUILD_PLUGIN_NAMESPACE(rndGrass)

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(rndGrass)
IDagorAssetExporter *create_rndgrass_exporter()
{
  static RndGrassExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_rndgrass_ref_provider()
{
  static RndGrassRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
