// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <gameRes/dag_stdGameRes.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include "getSkeleton.h"

BEGIN_DABUILD_PLUGIN_NAMESPACE(animChar)

static const char *TYPE = "animChar";

class AnimCharExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "character exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return CharacterGameResClassId; }
  unsigned __stdcall getGameResVersion() const override { return 4; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    bool use_char_dep = a.props.getBool("useCharDep", false);
    if (const DataBlock *b = a.props.getBlockByName("charData"))
    {
      cwr.writeFourCC(use_char_dep ? _MAKE4C('CHR2') : _MAKE4C('CHR1'));
      write_ro_datablock(cwr, *b);
    }
    else
      cwr.writeFourCC(use_char_dep ? _MAKE4C('chr2') : _MAKE4C('chr1'));
    if (use_char_dep)
    {
      const char *root_node_nm = a.props.getStr("rootNode", "");
      float py_ofs = a.props.getReal("root_py_ofs", 0);
      if (const char *base_skel_nm = a.props.getStr("charDepBaseSkeleton", NULL))
      {
        GeomNodeTree tree;
        if (!getSkeleton(tree, a.getMgr(), base_skel_nm, log))
        {
          log.addMessage(log.ERROR, "cannot find charDepBaseSkeleton: %s", base_skel_nm);
          return false;
        }
        auto n = tree.findNodeIndex(root_node_nm);
        if (!n)
        {
          log.addMessage(log.ERROR, "cannot find rootNode \"%s\" in charDepBaseSkeleton \"%s\"", root_node_nm, base_skel_nm);
          return false;
        }
        py_ofs = tree.getNodeWposRelScalar(n).y;


        const char *skel_nm = a.props.getStr("skeleton", "");
        if (!getSkeleton(tree, a.getMgr(), skel_nm, log))
        {
          log.addMessage(log.ERROR, "cannot find skeleton: %s", skel_nm);
          return false;
        }
        n = tree.findNodeIndex(root_node_nm);
        if (!n)
        {
          log.addMessage(log.ERROR, "cannot find rootNode \"%s\" in skeleton \"%s\"", root_node_nm, skel_nm);
          return false;
        }

        py_ofs = tree.getNodeWposRelScalar(n).y - py_ofs;
      }
      cwr.writeReal(py_ofs);
      cwr.writeReal(a.props.getReal("root_px_scale", 1));
      cwr.writeReal(a.props.getReal("root_py_scale", 1));
      cwr.writeReal(a.props.getReal("root_pz_scale", 1));
      cwr.writeReal(a.props.getReal("root_s_scale", 1));
      cwr.writeDwString(root_node_nm);
    }

    String centerNode(a.props.getStr("center", ""));
    float centerBsphRad = a.props.getReal("bsphere_rad", 0);
    if (a.props.getBool("autoCenterNode", centerNode.empty() || centerBsphRad < 1e-4))
      calcCenterNode(centerNode, centerBsphRad, a.props.getStr("dynModel", ""), a.props.getStr("skeleton", ""), a.getMgr(), log);

    cwr.writeDwString(centerNode);
    cwr.writeReal(a.props.getReal("no_anim_dist", 150));
    cwr.writeReal(a.props.getReal("no_render_dist", 300));
    cwr.writeReal(centerBsphRad);
    return true;
  }

  static void calcCenterNode(String &centerNode, float &centerBsphRad, const char *dm_name, const char *skel_nm, DagorAssetMgr &m,
    ILogWriter &log)
  {
    GeomNodeTree tree;
    if (!getSkeleton(tree, m, skel_nm, log))
      return;

    static DagorAssetMgr *mgr = NULL;
    static int dynModel_atype = -2;

    if (&m != mgr)
    {
      mgr = &m;
      dynModel_atype = m.getAssetTypeId("dynModel");
    }

    if (!dm_name)
    {
      log.addMessage(ILogWriter::ERROR, "dynModel asset not specified");
      return;
    }
    DagorAsset *dm_a = m.findAsset(dm_name, dynModel_atype);
    if (!dm_a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get dynModel asset <%s>", dm_name);
      return;
    }
    processDynModel(*dm_a, tree, log, centerNode, centerBsphRad);
  }
  static void processDynModel(DagorAsset &a, const GeomNodeTree &tree, ILogWriter &log, String &centerNode, float &centerBsphRad)
  {
    String sn;
    ShaderMeshData::buildForTargetCode = _MAKE4C('PC');

    DataBlock lodsBlk;
    const DataBlock &a_props = a.getProfileTargetProps(ShaderMeshData::buildForTargetCode, nullptr);
    lodsBlk.setFrom(&a_props);

    int nid = lodsBlk.getNameId("lod"), id = 0;
    for (int i = 0; DataBlock *blk = lodsBlk.getBlock(i); i++)
      if (blk->getBlockNameId() == nid)
      {
        sn.printf(260, "%s.lod%02d.dag", a.props.getStr("lod_fn_prefix", a.getName()), id++);
        blk->setStr("scene", String(260, "%s/%s", a.getFolderPath(), blk->getStr("fname", sn)));
      }

    lodsBlk.setBool("boundsOnly", true);

    DynamicRenderableSceneLodsResSrc resSrc;
    Tab<Point3> allVerts;
    resSrc.wAllVert = &allVerts;
    resSrc.build(lodsBlk);

    dag::Index16 nearest_node(0);
    float nearest_dist2 = lengthSq(tree.getNodeWposRelScalar(nearest_node) - resSrc.bsph.c);
    float expand_dist = resSrc.bsph.r;
    for (dag::Index16 i(1), ie(tree.importantNodeCount()); i < ie; ++i)
      if (tree.getChildCount(i))
      {
        float test_dist2 = lengthSq(tree.getNodeWposRelScalar(i) - resSrc.bsph.c);
        if (nearest_dist2 > test_dist2)
        {
          nearest_dist2 = test_dist2;
          nearest_node = i;
        }
        inplace_min(expand_dist, resSrc.bsph.r - sqrtf(test_dist2));
      }

    centerNode = nearest_node.index() == 0 ? "@root" : tree.getNodeName(nearest_node);
    resSrc.bsph.c = tree.getNodeWposRelScalar(nearest_node);
    centerBsphRad = 0;
    for (int i = 0; i < allVerts.size(); i++)
      inplace_max(centerBsphRad, lengthSq(resSrc.bsph.c - allVerts[i]));
    centerBsphRad = sqrtf(centerBsphRad);
  }
};

class AnimCharRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "character refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    static int dm_atype = -2;
    static int gn_atype = -2;
    static int at_atype = -2;
    static int sg_atype = -2;
    static int po_atype = -2;
    if (dm_atype == -2)
      dm_atype = a.getMgr().getAssetTypeId("dynModel");
    if (gn_atype == -2)
      gn_atype = a.getMgr().getAssetTypeId("skeleton");
    if (at_atype == -2)
      at_atype = a.getMgr().getAssetTypeId("animTree");
    if (sg_atype == -2)
      sg_atype = a.getMgr().getAssetTypeId("stateGraph");
    if (po_atype == -2)
      po_atype = a.getMgr().getAssetTypeId("physObj");

    refs.clear();
    if (dm_atype < 0)
      return;

    const char *dm_name = a.props.getStr("dynModel", "");
    DagorAsset *dm_a = a.getMgr().findAsset(dm_name, dm_atype);
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;
      if (!dm_a)
        r.setBrokenRef(String(64, "%s:dynModel", dm_name));
      else
        r.refAsset = dm_a;
    }

    const char *gn_name = a.props.getStr("skeleton", "");
    DagorAsset *gn_a = a.getMgr().findAsset(gn_name, gn_atype);
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;
      if (!gn_a)
        r.setBrokenRef(String(64, "%s:skeleton", gn_name));
      else
        r.refAsset = gn_a;
    }

    const char *at_name = a.props.getStr("animTree", NULL);
    const char *sg_name = a.props.getStr("stateGraph", NULL);
    if (sg_name)
    {
      DagorAsset *sg_a = a.getMgr().findAsset(sg_name, sg_atype);
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;
      if (!sg_a)
        r.setBrokenRef(String(64, "%s:stateGraph", sg_name));
      else
        r.refAsset = sg_a;
    }
    else if (at_name)
    {
      DagorAsset *at_a = a.getMgr().findAsset(at_name, at_atype);
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL;
      if (!at_a)
        r.setBrokenRef(String(64, "%s:animTree", at_name));
      else
        r.refAsset = at_a;
    }
    else
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL | RFLG_OPTIONAL;
      r.refAsset = NULL;
    }

    const char *po_name = a.props.getStr("physObj", NULL);
    if (po_name)
    {
      DagorAsset *po_a = a.getMgr().findAsset(po_name, po_atype);
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL | RFLG_OPTIONAL;
      if (!po_a)
        r.setBrokenRef(String(64, "%s:physObj", po_name));
      else
        r.refAsset = po_a;
    }
    else
    {
      IDagorAssetRefProvider::Ref &r = refs.push_back();
      r.flags = RFLG_EXTERNAL | RFLG_OPTIONAL;
      r.refAsset = NULL;
    }

    if (a.props.getBool("useCharDep", false))
    {
      gn_name = a.props.getStr("charDepBaseSkeleton", NULL);
      if (gn_name)
      {
        gn_a = a.getMgr().findAsset(gn_name, gn_atype);
        {
          IDagorAssetRefProvider::Ref &r = refs.push_back();
          r.flags = 0;
          if (!gn_a)
            r.setBrokenRef(String(64, "%s:skeleton", gn_name));
          else
            r.refAsset = gn_a;
        }
      }
    }
  }
};
END_DABUILD_PLUGIN_NAMESPACE(animChar)

BEGIN_DABUILD_PLUGIN_NAMESPACE(anim)
USING_DABUILD_PLUGIN_NAMESPACE(animChar)
IDagorAssetExporter *create_animchar_exporter()
{
  static AnimCharExporter exporter;
  return &exporter;
}
IDagorAssetRefProvider *create_animchar_ref_provider()
{
  static AnimCharRefs provider;
  return &provider;
}
END_DABUILD_PLUGIN_NAMESPACE(anim)
