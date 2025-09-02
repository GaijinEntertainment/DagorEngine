// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetFolder.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/fileUtils.h>
#include <libTools/pageAsg/asg_data.h>
#include <libTools/pageAsg/asg_sysHelper.h>
#include <gameRes/dag_stdGameRes.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <debug/dag_debug.h>
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

BEGIN_DABUILD_PLUGIN_NAMESPACE(stateGraph)
struct InterProcessMutex
{
  const char *mutex_name = "dagor4-animCharExp.dll";
  void *mutex = nullptr;
  InterProcessMutex()
  {
    mutex = global_mutex_create(mutex_name);
    if (!mutex)
      logerr("animCharExp.dll: cannot create mutex <%s>!\n", mutex_name);
    if (global_mutex_enter(mutex) != 0)
    {
      logerr("animCharExp.dll: cannot aquire mutex!\n");
      global_mutex_destroy(mutex, mutex_name);
      mutex = NULL;
    }
  }
  ~InterProcessMutex()
  {
    if (!mutex)
      return;
    global_mutex_leave_destroy(mutex, mutex_name);
    mutex = NULL;
  }
};

static const char *TYPE = "stateGraph";

static int a2d_atype = -2;
static int animtree_atype = -2;
static int shared_atype = -2;

static void init_atypes(DagorAssetMgr &mgr)
{
  static DagorAssetMgr *m = NULL;
  if (m != &mgr)
  {
    m = &mgr;
    a2d_atype = m->getAssetTypeId("a2d");
    shared_atype = m->getAssetTypeId("shared");
    animtree_atype = m->getAssetTypeId("animTree");
  }
}

static bool load_asg(AsgStatesGraph &asg, DagorAsset &a)
{
  const char *at_name = a.props.getStr("animTree", NULL);
  const char *sg_name = a.props.getStr("sharedSG", NULL);
  DagorAsset *at_a = at_name ? a.getMgr().findAsset(at_name, animtree_atype) : NULL;
  DagorAsset *sg_a = sg_name ? a.getMgr().findAsset(sg_name, shared_atype) : NULL;

  return asg.load(a.props, at_a ? &at_a->props : NULL, sg_a ? &sg_a->props : NULL, false);
}

class StateGraphExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "state graph exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return AnimCharGameResClassId; }
  unsigned __stdcall getGameResVersion() const override { return 6; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    init_atypes(a.getMgr());
    files.clear();

    const char *at_name = a.props.getStr("animTree", NULL);
    DagorAsset *at_a = at_name ? a.getMgr().findAsset(at_name, animtree_atype) : NULL;
    if (at_a)
      files.push_back() = at_a->getTargetFilePath();

    const char *sg_name = a.props.getStr("sharedSG", NULL);
    DagorAsset *sg_a = sg_name ? a.getMgr().findAsset(sg_name, shared_atype) : NULL;
    if (sg_a)
      files.push_back() = sg_a->getTargetFilePath();

    if (page_sys_hlp->getSgCppDestDir())
      files.push_back() = String(260, "%s/%s_statesGraph.cpp", page_sys_hlp->getSgCppDestDir(), a.getName());
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return a.props.getBool("export", true); }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    init_atypes(a.getMgr());
    AsgStatesGraph asg;

    load_asg(asg, a);
    asg.compact();

    NameMap bnl, a2d, ctrl;
    asg.getBnlsNeeded(bnl, a2d, ctrl);

    InterProcessMutex lock;
    if (!lock.mutex)
      return false;

    static const char *tempBlkName = "tmp.anim.blk";
    static const char *tempPdl = "tmp.pdl";

    asg.buildAndSave(tempBlkName, cwr.getTarget() == _MAKE4C('PC') ? tempPdl : NULL, a.getName(), a.props.getInt("debug_level", 0),
      a.props.getBool("in_editor", false));

    DataBlock tmpBlk;
    if (!tmpBlk.load(tempBlkName))
    {
      log.addMessage(ILogWriter::ERROR, "Error exporting anim graph");
      return false;
    }

    AnimObjGraphTree tree;
    tree.clear();
    tree.load(*tmpBlk.getBlockByNameEx("animGraph"));

    tmpBlk.reset();
    tree.save(tmpBlk, &a2d);

    cwr.writeFourCC(_MAKE4C('ac2'));
    write_ro_datablock(cwr, tmpBlk);

    cwr.writeDwString(a.getName()); // built-in animgraph name

    if (cwr.getTarget() == _MAKE4C('PC'))
    {
      FullFileLoadCB crd(tempPdl);
      if (!crd.fileHandle)
        return false;
      int sz = ::df_length(crd.fileHandle);
      cwr.writeInt32e(sz); // animgraph PDL size
      copy_stream_to_stream(crd, cwr.getRawWriter(), sz);
    }

    ::dd_erase(tempBlkName);
    ::dd_erase(tempPdl);
    if (page_sys_hlp->getSgCppDestDir())
      genJamFiles(a);

    return true;
  }

  void genJamFiles(DagorAsset &a_one)
  {
    static Tab<DagorAsset *> assets(tmpmem);

    dd_mkdir(page_sys_hlp->getSgCppDestDir());

    String jamfile(260, "%s/jamfile", page_sys_hlp->getSgCppDestDir());
    if (!dd_file_exist(jamfile))
    {
      FILE *fpJamF = fopen(jamfile, "wt");
      G_ASSERT(fpJamF);
      G_ANALYSIS_ASSUME(fpJamF);
      fputs("# this is template jamfile, generated only when it is absent\n"
            "# replace Root/Location/Target with appropriate strings before use\n"
            "Root    ?= . ; #requires changes\n"
            "Location = . ; #requires changes\n\n"
            "TargetType  = lib ;\n"
            "Target      = ; #requires changes\n\n"
            "include $(Root)/$(Location)/jam-list ;\n\n"
            "Sources += factory.cpp ;\n\n"
            "AddIncludes = $(Root)/prog/engine ;\n\n"
            "include $(Root)/prog/_jBuild/build.jam ;\n",
        fpJamF);
      fclose(fpJamF);
    }

    String tmp_jam_list(260, "%s/jam-list.1", page_sys_hlp->getSgCppDestDir());
    String dst_jam_list(260, "%s/jam-list", page_sys_hlp->getSgCppDestDir());
    String tmp_factory_cpp(260, "%s/factory.1.cpp", page_sys_hlp->getSgCppDestDir());
    String dst_factory_cpp(260, "%s/factory.cpp", page_sys_hlp->getSgCppDestDir());

    FILE *fpJam = fopen(tmp_jam_list, "wt");
    FILE *fpFac = fopen(tmp_factory_cpp, "wt");

    G_ASSERT(fpJam);
    G_ANALYSIS_ASSUME(fpJam);

    G_ASSERT(fpFac);
    G_ANALYSIS_ASSUME(fpFac);

    fprintf(fpJam, "Sources =\n");
    fprintf(fpFac, "#include <osApiWrappers/dag_localConv.h>\n\n"
                   "namespace AnimV20 { class GenericAnimStatesGraph; }\n\n");

    int atype = a_one.getType();
    dag::ConstSpan<int> a_idx = a_one.getMgr().getFilteredAssets(make_span_const(&atype, 1));
    G_ASSERT(a_idx.size() > 0);

    assets.clear();
    assets.reserve(a_idx.size());

    for (int i = 0; i < a_idx.size(); i++)
    {
      DagorAsset *a = &a_one.getMgr().getAsset(a_idx[i]);
      if (a->getFolderIndex() < 0)
        continue;
      const DagorAssetFolder &f = a_one.getMgr().getFolder(a->getFolderIndex());
      if ((f.flags & f.FLG_EXPORT_ASSETS) && isExportableAsset(*a))
        assets.push_back(a);
    }

    G_ASSERT(assets.size() > 0);
    for (int i = 0; i < assets.size(); i++)
    {
      fprintf(fpJam, "  %s_statesGraph.cpp\n", assets[i]->getName());
      fprintf(fpFac,
        "namespace %s_statesGraph\n{\n"
        "  AnimV20::GenericAnimStatesGraph* make_graph();\n}\n",
        assets[i]->getName());
    }

    fprintf(fpFac, "\n\nAnimV20::GenericAnimStatesGraph* make_graph_by_res_name(const char *graphname)\n{\n");

    for (int i = 0; i < assets.size(); i++)
    {
      fprintf(fpFac, "  if (dd_stricmp(graphname, \"%s\") == 0)\n", assets[i]->getName());
      fprintf(fpFac, "    return %s_statesGraph::make_graph();\n", assets[i]->getName());
    }

    fprintf(fpJam, ";\n");
    fprintf(fpFac, "\n  return NULL;\n}\n");

    fclose(fpJam);
    fclose(fpFac);

    if (!dag_file_compare(dst_jam_list, tmp_jam_list))
    {
      unlink(dst_jam_list);
      dag_copy_file(tmp_jam_list, dst_jam_list, true);
    }
    if (!dag_file_compare(dst_factory_cpp, tmp_factory_cpp))
    {
      unlink(dst_factory_cpp);
      dag_copy_file(tmp_factory_cpp, dst_factory_cpp, true);
    }
    unlink(tmp_jam_list);
    unlink(tmp_factory_cpp);

    assets.clear();
  }
};

class StateGraphRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "state graph refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    init_atypes(a.getMgr());
    refs.clear();

    AsgStatesGraph asg;
    load_asg(asg, a);
    asg.compact();

    NameMap bnl, a2d, ctrl;
    asg.getBnlsNeeded(bnl, a2d, ctrl);

    for (int i = 0; i < a2d.nameCount(); i++)
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      String a2d_name(DagorAsset::fpath2asset(a2d.getName(i)));
      DagorAsset *ra = a.getMgr().findAsset(a2d_name, a2d_atype);
      r.flags = IDagorAssetRefProvider::RFLG_EXTERNAL;
      if (!ra)
        r.setBrokenRef(String(64, "%s:a2d", a2d_name));
      else
        r.refAsset = ra;
    }

    const char *at_name = a.props.getStr("animTree", NULL);
    if (at_name)
    {
      DagorAsset *ra = a.getMgr().findAsset(at_name, animtree_atype);
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = 0;
      if (!ra)
        r.setBrokenRef(String(64, "%s:animTree", at_name));
      else
        r.refAsset = ra;
    }

    const char *sg_name = a.props.getStr("sharedSG", NULL);
    if (sg_name)
    {
      DagorAsset *ra = sg_name ? a.getMgr().findAsset(sg_name, shared_atype) : NULL;
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = 0;
      if (!ra)
        r.setBrokenRef(String(64, "%s:shared", sg_name));
      else
        r.refAsset = ra;
    }
  }
};
END_DABUILD_PLUGIN_NAMESPACE(stateGraph)

BEGIN_DABUILD_PLUGIN_NAMESPACE(anim)
USING_DABUILD_PLUGIN_NAMESPACE(stateGraph)
IDagorAssetExporter *create_stategraph_exporter()
{
  static StateGraphExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_stategraph_ref_provider()
{
  static StateGraphRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(anim)
