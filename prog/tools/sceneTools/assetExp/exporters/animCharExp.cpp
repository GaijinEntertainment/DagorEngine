// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <libTools/pageAsg/asg_sysHelper.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(anim)

extern IDagorAssetExporter *create_animchar_exporter();
extern IDagorAssetExporter *create_animtree_exporter();
extern IDagorAssetExporter *create_stategraph_exporter();
extern IDagorAssetRefProvider *create_animchar_ref_provider();
extern IDagorAssetRefProvider *create_animtree_ref_provider();
extern IDagorAssetRefProvider *create_stategraph_ref_provider();

class PageSysHelper : public IPageSysHelper
{
public:
  void postMsgV(bool error, const char *caption, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    logmessage_fmt(error ? LOGLEVEL_ERR : LOGLEVEL_DEBUG, fmt, arg, anum);
  }
  const char *getDagorCdkDir() override { return dagorCdkDir; }
  const char *getSgCppDestDir() override { return sgOutDir.empty() ? NULL : sgOutDir.str(); }

public:
  SimpleString dagorCdkDir, sgOutDir;
};


class AnimCharExporterPlugin : public IDaBuildPlugin
{
public:
  AnimCharExporterPlugin()
  {
    expAnimTree = NULL;
    refAnimTree = NULL;
    expAnimChar = NULL;
    refAnimChar = NULL;
    expStateGraph = NULL;
    refStateGraph = NULL;
  }
  ~AnimCharExporterPlugin()
  {
    expAnimTree = NULL;
    refAnimTree = NULL;
    expAnimChar = NULL;
    refAnimChar = NULL;
    expStateGraph = NULL;
    refStateGraph = NULL;
  }

  bool __stdcall init(const DataBlock &appblk) override
  {
    sysHelper.dagorCdkDir = appblk.getStr("dagorCdkDir", NULL);
    const char *p =
      appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("stateGraph")->getStr("cppOutDir", NULL);
    if (p)
    {
      sysHelper.sgOutDir = String(260, "%s/%s", appblk.getStr("appDir", "."), p);
      ::dd_simplify_fname_c(sysHelper.sgOutDir);
    }

    ::page_sys_hlp = &sysHelper;
    return true;
  }
  void __stdcall destroy() override
  {
    ::page_sys_hlp = NULL;
    delete this;
  }

  int __stdcall getExpCount() override { return 3; }
  const char *__stdcall getExpType(int idx) override
  {
    switch (idx)
    {
      case 0: return "animTree";
      case 1: return "animChar";
      case 2: return "stateGraph";
      default: return NULL;
    }
  }
  IDagorAssetExporter *__stdcall getExp(int idx) override
  {
    switch (idx)
    {
      case 0:
        if (!expAnimTree)
          expAnimTree = create_animtree_exporter();
        return expAnimTree;
      case 1:
        if (!expAnimChar)
          expAnimChar = create_animchar_exporter();
        return expAnimChar;
      case 2:
        if (!expStateGraph)
          expStateGraph = create_stategraph_exporter();
        return expStateGraph;
      default: return NULL;
    }
  }

  int __stdcall getRefProvCount() override { return getExpCount(); }
  const char *__stdcall getRefProvType(int idx) override { return getExpType(idx); }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override
  {
    switch (idx)
    {
      case 0:
        if (!refAnimTree)
          refAnimTree = create_animtree_ref_provider();
        return refAnimTree;
      case 1:
        if (!refAnimChar)
          refAnimChar = create_animchar_ref_provider();
        return refAnimChar;
      case 2:
        if (!refStateGraph)
          refStateGraph = create_stategraph_ref_provider();
        return refStateGraph;
      default: return NULL;
    }
  }

protected:
  IDagorAssetExporter *expAnimTree, *expAnimChar, *expStateGraph;
  IDagorAssetRefProvider *refAnimTree, *refAnimChar, *refStateGraph;
  PageSysHelper sysHelper;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) AnimCharExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(anim)
REGISTER_DABUILD_PLUGIN(anim, nullptr)
