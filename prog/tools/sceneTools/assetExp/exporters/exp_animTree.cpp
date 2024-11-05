// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/iLogWriter.h>
#include <regExp/regExp.h>
#include <gameRes/dag_stdGameRes.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include "getSkeleton.h"

BEGIN_DABUILD_PLUGIN_NAMESPACE(animTree)

static int a2d_atype = -2;
static int skeleton_atype = -2;

static void init_atypes(DagorAssetMgr &mgr)
{
  static DagorAssetMgr *m = NULL;
  if (m != &mgr)
  {
    m = &mgr;
    a2d_atype = mgr.getAssetTypeId("a2d");
    skeleton_atype = mgr.getAssetTypeId("skeleton");
  }
}

static dag::ConstSpan<IDagorAssetRefProvider::Ref> enum_a2d_refs(DagorAsset &a)
{
  static Tab<IDagorAssetRefProvider::Ref> tmpRefs(tmpmem);
  init_atypes(a.getMgr());

  tmpRefs.clear();

  int nid = a.props.getNameId("AnimBlendNodeLeaf");
  for (int i = 0; i < a.props.blockCount(); i++)
    if (a.props.getBlock(i)->getBlockNameId() == nid)
    {
      String a2d_name(DagorAsset::fpath2asset(a.props.getBlock(i)->getStr("a2d", "")));
      DagorAsset *ra = a.getMgr().findAsset(a2d_name, a2d_atype);

      if (ra)
      {
        int found = -1;
        for (int j = 0; j < tmpRefs.size(); j++)
          if (tmpRefs[j].getAsset() == ra)
          {
            found = j;
            break;
          }
        if (found != -1)
          continue;
      }

      IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
      r.flags = IDagorAssetRefProvider::RFLG_EXTERNAL;
      if (!ra)
        r.setBrokenRef(String(64, "%s:a2d", a2d_name));
      else
        r.refAsset = ra;
    }
  if (const char *skel_nm = a.props.getStr("skeleton", NULL))
  {
    DagorAsset *ra = a.getMgr().findAsset(skel_nm, skeleton_atype);
    IDagorAssetRefProvider::Ref &r = tmpRefs.push_back();
    r.flags = 0;
    if (!ra)
      r.setBrokenRef(String(0, "%s:skeleton", skel_nm));
    else
      r.refAsset = ra;
  }

  return tmpRefs;
}


static const char *TYPE = "animTree";

class AnimTreeExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "anim exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return AnimCharGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const { return 4; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();

    if (!a.isVirtual())
    {
      DataBlock blk;
      struct IncludeGather : DataBlock::IFileNotify
      {
        Tab<SimpleString> &files;
        const char *srcFn;
        IncludeGather(Tab<SimpleString> &files, const char *src_fn) : files(files), srcFn(src_fn) {}
        virtual void onFileLoaded(const char *fname)
        {
          if (strcmp(fname, srcFn) != 0)
          {
            SimpleString str(fname);
            files.emplace_back(simplify_fname(str));
          }
        }
      } notify(files, a.getSrcFilePath());

      dblk::load(blk, a.getSrcFilePath(), dblk::ReadFlags(), &notify);
    }
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return a.props.getBool("export", true); }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    dag::ConstSpan<IDagorAssetRefProvider::Ref> refList = enum_a2d_refs(a);
    DataBlock blk;
    bool err = false;

    blk = a.props;
    if (const char *skel_nm = a.props.getStr("skeleton", NULL))
    {
      GeomNodeTree tree;
      if (!getSkeleton(tree, a.getMgr(), skel_nm, log))
      {
        log.addMessage(log.ERROR, "cannot find skeleton: %s", skel_nm);
        return false;
      }

      // process nodemasks
      blk.removeParam("skeleton");
      int nid_nodemask = blk.getNameId("nodeMask");
      int nid_node = blk.getNameId("node");
      int nid_nodesRE = blk.getNameId("nodesRE");
      int nid_nodesFrom = blk.getNameId("nodesFrom");
      int nid_nodesRange = blk.getNameId("nodesRange");
      FastNameMap map, nodeStop;
      String name;

      for (int i = 0; i < blk.blockCount(); i++)
        if (blk.getBlock(i)->getBlockNameId() == nid_nodemask)
        {
          DataBlock &b = *blk.getBlock(i);
          map.reset();
          name = b.getStr("name", NULL);

          for (int j = 0; j < b.paramCount(); j++)
            if (b.getParamNameId(j) == nid_node && b.getParamType(j) == b.TYPE_STRING)
            {
              const char *n = b.getStr(j);
              if (tree.findNodeIndex(n))
                map.addNameId(n);
              else
                logwarn("skipped node %s for skeleton");
            }

          for (int j = 0; j < b.paramCount(); j++)
            if (b.getParamNameId(j) == nid_nodesRE && b.getParamType(j) == b.TYPE_STRING)
            {
              RegExp inc_re;
              if (!inc_re.compile(b.getStr(j), "i"))
              {
                log.addMessage(log.ERROR, "%s:%s: bad regexp in nodemask <%s>: %s", a.getName(), a.getTypeStr(), name, b.getStr(j));
                return false;
              }
              addNodesByRE(map, tree, inc_re, NULL);
            }
          for (int j = 0; j < b.blockCount(); j++)
            if (b.getBlock(j)->getBlockNameId() == nid_nodesRE)
            {
              const char *incl_re_s = b.getBlock(j)->getStr("incl", "");
              const char *excl_re_s = b.getBlock(j)->getStr("excl", NULL);
              RegExp inc_re, exc_re;
              if (!inc_re.compile(incl_re_s, "i"))
              {
                log.addMessage(log.ERROR, "%s:%s: bad incl regexp in nodemask <%s>: %s", a.getName(), a.getTypeStr(), name, incl_re_s);
                return false;
              }
              if (excl_re_s && !exc_re.compile(excl_re_s, "i"))
              {
                log.addMessage(log.ERROR, "%s:%s: bad excl regexp in nodemask <%s>: %s", a.getName(), a.getTypeStr(), name, excl_re_s);
                return false;
              }
              addNodesByRE(map, tree, inc_re, excl_re_s ? &exc_re : NULL);
            }

          nodeStop.reset();
          for (int j = 0; j < b.paramCount(); j++)
            if (b.getParamNameId(j) == nid_nodesFrom && b.getParamType(j) == b.TYPE_STRING)
              if (!addNodesRange(map, tree, b.getStr(j), nodeStop))
              {
                log.addMessage(log.ERROR, "%s:%s: start node <%s> not found in nodemask <%s>: %s", a.getName(), a.getTypeStr(),
                  b.getStr(j), name);
                return false;
              }
          for (int j = 0; j < b.blockCount(); j++)
            if (b.getBlock(j)->getBlockNameId() == nid_nodesRange)
            {
              DataBlock &b2 = *b.getBlock(j);
              int nid_to = b2.getNameId("to");
              nodeStop.reset();
              for (int k = 0; k < b2.paramCount(); k++)
                if (b2.getParamNameId(k) == nid_to && b2.getParamType(k) == b.TYPE_STRING)
                  nodeStop.addNameId(b2.getStr(k));
              if (!addNodesRange(map, tree, b2.getStr("from", ""), nodeStop))
              {
                log.addMessage(log.ERROR, "%s:%s: start node <%s> not found in nodemask <%s>: %s", a.getName(), a.getTypeStr(),
                  b2.getStr("from", ""), name);
                return false;
              }
            }

          b.clearData();
          b.setStr("name", name);
          iterate_names_in_lexical_order(map, [&b](int, const char *name) { b.addStr("node", name); });
        }
    }
    else
    {
      int nid_nodemask = blk.getNameId("nodeMask");
      int nid_node = blk.getNameId("node");
      int nid_name = blk.getNameId("name");
      for (int i = 0; i < blk.blockCount(); i++)
        if (blk.getBlock(i)->getBlockNameId() == nid_nodemask)
        {
          DataBlock &b = *blk.getBlock(i);
          const char *name = b.getStr("name", NULL);
          for (int j = 0; j < b.paramCount(); j++)
            if (b.getParamType(j) != b.TYPE_STRING || (b.getParamNameId(j) != nid_node && b.getParamNameId(j) != nid_name))
            {
              log.addMessage(log.ERROR, "%s:%s: bad param %s (type=%d) in nodemask <%s>, skeleton not set", a.getName(),
                a.getTypeStr(), b.getParamName(j), b.getParamType(j), name);
              return false;
            }
        }
    }

    int nid = blk.getNameId("AnimBlendNodeLeaf");
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        String a2d_name(DagorAsset::fpath2asset(blk.getBlock(i)->getStr("a2d", "")));
        int a_nid = a.getMgr().getAssetNameId(a2d_name);

        int found = -1;
        if (a_nid != -1)
          for (int j = 0; j < refList.size(); j++)
            if (refList[j].getAsset() && refList[j].getAsset()->getNameId() == a_nid)
            {
              found = j;
              break;
            }

        if (found != -1)
        {
          blk.getBlock(i)->removeParam("a2d");
          blk.getBlock(i)->addInt("a2d_id", found);
        }
        else
        {
          err = true;
          log.addMessage(log.ERROR, "cannot find a2d: %s", a2d_name);
        }
      }

    if (err)
      return false;

    cwr.writeFourCC(_MAKE4C('ac2'));
    write_ro_datablock(cwr, blk);

    cwr.writeDwString(""); // built-in animgraph name

    if (cwr.getTarget() == _MAKE4C('PC'))
    {
      cwr.writeInt32e(-1); // animgraph PDL size
    }
    return true;
  }

  static void addNodesByRE(FastNameMap &map, const GeomNodeTree &tree, RegExp &inc_re, RegExp *exc_re)
  {
    for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
    {
      const char *nm = tree.getNodeName(i);
      if (inc_re.test(nm) && (!exc_re || !exc_re->test(nm)))
        map.addNameId(nm);
    }
  }
  static bool addNodesRange(FastNameMap &map, const GeomNodeTree &tree, const char *from, const FastNameMap &nodeStop)
  {
    if (tree.empty())
      return false;
    auto r = (from && *from) ? tree.findNodeIndex(from) : dag::Index16(0);
    if (!r)
      return false;
    addNodes(map, tree, r, nodeStop);
    return true;
  }
  static void addNodes(FastNameMap &map, const GeomNodeTree &tree, dag::Index16 n, const FastNameMap &nodeStop)
  {
    if (tree.getParentNodeIdx(n))
      map.addNameId(tree.getNodeName(n));
    for (int i = 0; i < tree.getChildCount(n); i++)
      if (nodeStop.getNameId(tree.getNodeName(tree.getChildNodeIdx(n, i))) < 0)
        addNodes(map, tree, tree.getChildNodeIdx(n, i), nodeStop);
  }
};

class AnimTreeRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "anim refs"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override { return enum_a2d_refs(a); }
};
END_DABUILD_PLUGIN_NAMESPACE(animTree)

BEGIN_DABUILD_PLUGIN_NAMESPACE(anim)
USING_DABUILD_PLUGIN_NAMESPACE(animTree)
IDagorAssetExporter *create_animtree_exporter()
{
  static AnimTreeExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_animtree_ref_provider()
{
  static AnimTreeRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(anim)
