// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dagFileRW/nodeTreeBuilder.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/normalizeNodeTm.h>
#include <libTools/dagFileRW/geomMeshHelper.h>
#include "exp_tools.h"
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_meshBones.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_staticTab.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_direct.h>
#include "fatalHandler.h"
#include "modelExp.h"
#include <debug/dag_debug.h>
#include "getSkeleton.h"
#include <stdio.h>
#include "exp_skeleton_tools.h"

BEGIN_DABUILD_PLUGIN_NAMESPACE(skeleton)

static const char *TYPE = "skeleton";
static const char *curDagFname = NULL;
static OAHashNameMap<true> skinNodes;
static bool hier_up_animated = false;
static bool preferZstdPacking = false;
static bool allowOodlePacking = false;

class SkeletonExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "skeleton exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return GeomNodeTreeGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    static constexpr const int base_ver = 3;
    return base_ver * 3 + 1 + (!preferZstdPacking ? 0 : (allowOodlePacking ? 2 : 1));
  }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
    gatherReferencedAdditionalDags(a.props, a.getFolderPath(), a.props.getNameId("attachSubSkel"), a.props.getNameId("mergeSkel"),
      files);
  }
  void gatherReferencedAdditionalDags(const DataBlock &a_props, const char *a_dir, int nid_attach, int nid_merge,
    Tab<SimpleString> &files)
  {
    for (int i = 0; i < a_props.blockCount(); i++)
      if (a_props.getBlock(i)->getBlockNameId() == nid_attach)
      {
        const DataBlock &b2 = *a_props.getBlock(i);
        String add_skel_fn(0, "%s/%s", a_dir, b2.getStr("skel_file"));
        bool exists = false;
        simplify_fname(add_skel_fn);
        for (SimpleString &s : files)
          if (strcmp(s, add_skel_fn) == 0)
          {
            exists = true;
            break;
          }
        if (!exists)
          files.push_back() = add_skel_fn;

        gatherReferencedAdditionalDags(b2, a_dir, nid_attach, nid_merge, files);
      }
      else if (a_props.getBlock(i)->getBlockNameId() == nid_merge)
      {
        const DataBlock &b2 = *a_props.getBlock(i);
        String add_skel_fn(0, "%s/%s", a_dir, b2.getStr("skel_file"));
        bool exists = false;
        simplify_fname(add_skel_fn);
        for (SimpleString &s : files)
          if (strcmp(s, add_skel_fn) == 0)
          {
            exists = true;
            break;
          }
        if (!exists)
          files.push_back() = add_skel_fn;
      }
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    String fpath(a.getTargetFilePath());
    if (stricmp(::dd_get_fname_ext(fpath), ".dat") == 0)
    {
      log.addMessage(ILogWriter::ERROR, "Obsolete .DAT skeleton file %s", fpath.str());
      return false;
    }
    if (stricmp(::dd_get_fname_ext(fpath), ".dag") == 0)
      return exportDag(fpath, a, cwr, log);
    log.addMessage(log.ERROR, "skeleton: unknown file format %s", fpath.str());
    return false;
  }

  bool exportDag(const char *fn, DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    GeomNodeTreeBuilder nodeTree;
    bool reduce_nodes = a.props.getBool("reduceNodes", true);
    bool add_skin = a.props.getBool("addSkinNodes", false);

    int flags = LASF_NOMATS | LASF_NOMESHES | LASF_NOSPLINES | LASF_NOLIGHTS;
    if (reduce_nodes && add_skin)
      flags = LASF_NOMATS | LASF_NOSPLINES | LASF_NOLIGHTS;

    AScene sc;
    if (!load_ascene(fn, sc, flags, false) || !sc.root)
    {
      log.addMessage(ILogWriter::ERROR, "cannot load skeleton from %s", fn);
      return false;
    }
    if (!attachSubSkels(a.props, a.getFolderPath(), "", fn, sc, flags, log, a.getName()))
      return false;
    if (a.props.getBlockByName("attachSubSkel") || a.props.getBlockByName("mergeSkel"))
    {
      struct CheckNodeDupCB : public Node::NodeEnumCB
      {
        FastNameMap allNodes;
        FastNameMap dupNodes;
        int node(Node &n) override
        {
          if (!n.name || !*n.name)
            return 0;
          if (allNodes.getNameId(n.name) < 0)
            allNodes.addNameId(n.name);
          else
            dupNodes.addNameId(n.name);
          return 0;
        }
      } cb;
      sc.root->enum_nodes(cb);
      if (cb.dupNodes.nameCount())
      {
        String nlist;
        iterate_names(cb.dupNodes, [&nlist](int, const char *name) { nlist.aprintf(0, "%s ", name); });
        log.addMessage(ILogWriter::ERROR, "%s: contain %d duplicate node names after attach/merge:\n%s", a.getNameTypified(),
          cb.dupNodes.nameCount(), nlist);
        return false;
      }
    }

    if (a.props.getBool("needResetNodesTmScale", false))
      normalizeNodeTreeTm(*sc.root, TMatrix::IDENT, false);

    curDagFname = fn;
    hier_up_animated = a.props.getBool("animated_node_animates_parent", true);
    if (add_skin)
      addSkinNodes(sc.root);
    if (reduce_nodes && !a.props.getBool("all_animated_nodes", false))
      reduceNodes(sc.root);

    skinNodes.reset(false);
    curDagFname = NULL;

    combine_skeleton(a, flags, log, sc.root);
    sc.root->calc_wtm();
    auto_complete_skeleton(a, "", flags, log, sc.root);

    nodeTree.buildFromDagNodes(sc.root, a.props.getStr("unimportantNodesPrefix", NULL));

    DAGOR_TRY { nodeTree.save(cwr, preferZstdPacking, allowOodlePacking); }
    DAGOR_CATCH(const IGenSave::SaveException &)
    {
      log.addMessage(ILogWriter::ERROR, "I/O error for skeleton file %s", fn);
      return false;
    }
    return true;
  }
  bool reduceNodes(Node *n)
  {
    bool has_animated = false;

    for (int i = n->child.size() - 1; i >= 0; i--)
      if (reduceNodes(n->child[i]))
        erase_items(n->child, i, 1);
      else
        has_animated = true;

    if (!n->parent)
      return false;

    bool def_animated = (hier_up_animated && has_animated) || skinNodes.getNameId(n->name) >= 0;

    DataBlock blk;
    if (!dblk::load_text(blk, make_span_const(n->script), dblk::ReadFlag::ROBUST, curDagFname))
      logwarn("skip reading BLK from %s : node[\"%s\"].script=\n%s", curDagFname, n->name, n->script);
    if (!blk.getBool("animated_node", def_animated) && !blk.getBool("helper_node", false))
    {
      debug("removed non-animated/helper node: %s", n->name.str());
      Node *p = n->parent;
      n->parent = NULL;
      for (int i = 0; i < n->child.size(); i++)
      {
        p->child.push_back(n->child[i]);
        n->child[i]->parent = p;
        n->child[i]->tm = n->tm * n->child[i]->tm;
      }
      n->child.clear();
      delete n;
      return true;
    }
    return false;
  }

  void addSkinNodes(Node *n)
  {
    if (n->obj && n->obj->isSubOf(OCID_MESHHOLDER))
    {
      MeshHolderObj &mh = *(MeshHolderObj *)n->obj;
      if (mh.bones)
        for (int i = 0; i < mh.bones->boneNames.size(); i++)
          skinNodes.addNameId(mh.bones->boneNames[i]);
    }

    for (int i = 0; i < n->child.size(); i++)
      addSkinNodes(n->child[i]);
  }

  struct AddNodePrefixCB : public Node::NodeEnumCB
  {
    String prefix;
    int cnt = 0;
    AddNodePrefixCB(const char *p1) : prefix(p1) {}
    AddNodePrefixCB(const char *p1, const char *p2) { prefix.setStrCat(p1, p2); }

    int node(Node &n) override
    {
      if (!prefix.empty())
        n.name = String::mk_str_cat(prefix, n.name);
      cnt++;
      return 0;
    }
  };

  bool attachSubSkels(const DataBlock &a_props, const char *a_dir, const char *dest_name_prefix, const char *fn, AScene &sc, int flags,
    ILogWriter &log, const char *asset_name)
  {
    int nid_attach = a_props.getNameId("attachSubSkel");
    int nid_merge = a_props.getNameId("mergeSkel");
    for (int i = 0; i < a_props.blockCount(); i++)
      if (a_props.getBlock(i)->getBlockNameId() == nid_attach)
      {
        const DataBlock &b2 = *a_props.getBlock(i);
        String add_skel_fn(0, "%s/%s", a_dir, b2.getStr("skel_file"));
        String nm_attach_to(0, "%s%s", dest_name_prefix, b2.getStr("attach_to"));
        const char *nm_skel_node = b2.getStr("skel_node");

        AScene sc2;
        if (!load_ascene(add_skel_fn, sc2, flags, false) || !sc.root)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: cannot load skeleton (attachSubSkel) from %s", asset_name, TYPE, add_skel_fn);
          return false;
        }
        Node *dst_n = sc.root->find_node(nm_attach_to);
        Node *src_n = sc2.root->find_node(nm_skel_node);
        if (!dst_n)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: failed to find %s node (attach_to) in base skeleton %s", asset_name, TYPE,
            nm_attach_to, fn);
          return false;
        }
        if (!src_n)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: failed to find %s node (skel_node) in sub skeleton %s", asset_name, TYPE,
            nm_skel_node, add_skel_fn);
          return false;
        }

        AddNodePrefixCB add_prefix(dest_name_prefix, b2.getStr("add_prefix", ""));
        if (!add_prefix.prefix.empty())
          src_n->enum_nodes(add_prefix);

        // debug("attached skel=<%s> node <%s> to <%s> with prefix=<%d>, %d nodes added",
        //   add_skel_fn, nm_skel_node, nm_attach_to, b2.getStr("add_prefix", ""), add_prefix.cnt);
        for (Node *cn : src_n->child)
          cn->parent = dst_n;
        append_items(dst_n->child, src_n->child.size(), src_n->child.data());
        clear_and_shrink(src_n->child);
        del_it(sc2.root);

        if (!attachSubSkels(b2, a_dir, add_prefix.prefix, fn, sc, flags, log, asset_name))
          return false;
      }
      else if (a_props.getBlock(i)->getBlockNameId() == nid_merge)
      {
        const DataBlock &b2 = *a_props.getBlock(i);
        String add_skel_fn(0, "%s/%s", a_dir, b2.getStr("skel_file"));

        AScene sc2;
        if (!load_ascene(add_skel_fn, sc2, flags, false) || !sc.root)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: cannot load skeleton (attachSubSkel) from %s", asset_name, TYPE, add_skel_fn);
          return false;
        }

        const char *merge_to = b2.getStr("merge_to", nullptr);
        const char *merge_from = b2.getStr("skel_merge_from", nullptr);
        Node *dn = merge_to ? sc.root->find_node(merge_to) : sc.root;
        Node *sn = merge_from ? sc2.root->find_node(merge_from) : sc2.root;
        if (!dn || !sn)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: failed to merge skel=<%s> - merge_to(%s)=%p  skel_merge_from(%s)=%p", asset_name,
            TYPE, add_skel_fn, merge_to ? merge_to : "", dn, merge_from ? merge_from : "", sn);
          return false;
        }
        int nodes_added =
          merge_skeleton_subtree(dn, sn, b2.getStr("dest_prefix", ""), !b2.getBool("merge_only_to_existing_top_nodes", true));
        del_it(sc2.root);

        if (!nodes_added)
        {
          log.addMessage(ILogWriter::ERROR, "%s:%s: merging skel=<%s> with prefix=<%d>, no nodes added!", asset_name, TYPE,
            add_skel_fn, b2.getStr("dest_prefix", ""));
          return false;
        }
        // debug("merged skel=<%s> with prefix=<%d>, %d nodes added", add_skel_fn, b2.getStr("dest_prefix", ""), nodes_added);
      }
    return true;
  }
  int merge_skeleton_subtree(Node *dn, Node *sn, const char *dest_prefix, bool add_missing = true)
  {
    int nodes_added = 0;
    for (int i = 0; i < sn->child.size(); i++)
      if (Node *n = dn->find_node(String::mk_str_cat(dest_prefix, sn->child[i]->name)))
      {
        if (!add_missing || n->parent == dn)
          nodes_added += merge_skeleton_subtree(n, sn->child[i], dest_prefix);
        else
          logwarn("skip merging node <%s> (parent <%s>) to dest node with parent <%s>; relation broken", sn->child[i]->name, sn->name,
            dn->name);
      }
      else if (add_missing)
      {
        AddNodePrefixCB add_prefix(dest_prefix);
        sn->child[i]->enum_nodes(add_prefix);
        nodes_added += add_prefix.cnt;

        sn->child[i]->parent = dn;
        dn->child.push_back(sn->child[i]);
        erase_items(sn->child, i, 1);
        i--;
      }
      else
        logwarn("skip merging topnode <%s> due to merge_only_to_existing_top_nodes:b=yes", sn->child[i]->name);
    return nodes_added;
  }
};

END_DABUILD_PLUGIN_NAMESPACE(skeleton)

BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(skeleton)
IDagorAssetExporter *create_skeleton_exporter()
{
  static SkeletonExporter exporter;
  const DataBlock *skeletonBlk = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("skeleton");
  if (skeletonBlk->getBool("preferZSTD", false))
  {
    preferZstdPacking = true;
    debug("Skeleton prefers ZSTD");
  }
  if (skeletonBlk->getBool("allowOODLE", false))
  {
    allowOodlePacking = true;
    debug("Skeleton allows OODLE");
  }
  return &exporter;
}
END_DABUILD_PLUGIN_NAMESPACE(modelExp)
