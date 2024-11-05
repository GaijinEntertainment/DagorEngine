// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animBlend.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomTreeMap.h>

struct AnimV20::AnimDbgCtx
{
  DataBlock blk;
  DataBlock nodemaskBlk;
  Tab<DataBlock *> nBlk;
  Tab<DataBlock *> gnBlk;
  const GeomNodeTree *gnTree;
  DataBlock *skel, *bnlActive, *pbcActive, *abnActive;
  Tab<float> abnWt;
  bool prevDumpTm;
  int ctrlNid, animNid;

  AnimDbgCtx(AnimV20::AnimationGraph *a, const GeomNodeTree *gtree, bool dump_all_nodes)
  {
    AnimBlender &blender = a->blender;
    int nodenum = blender.targetNode.nameCount();

    prevDumpTm = false;
    skel = blk.addBlock("skeleton");
    abnActive = blk.addBlock("animNodes");
    bnlActive = blk.addBlock("bnlActive");
    pbcActive = blk.addBlock("pbcActive");
    gnTree = gtree;
    if (gtree)
    {
      gnBlk.resize(gtree->nodeCount());
      mem_set_0(gnBlk);
      nBlk.resize(nodenum);
      mem_set_0(nBlk);
      for (dag::Index16 n(0), ne(gnBlk.size()); n != ne; ++n)
      {
        auto pidx = gtree->getParentNodeIdx(n);
        DataBlock *b = skel;
        if (n.index() > 0)
          b = (pidx ? gnBlk[pidx.index()] : skel)->addNewBlock("node");

        gnBlk[n.index()] = b;
        if (n.index() > 0)
          b->setStr("name", gtree->getNodeName(n));

        int node = a->getNodeId(gtree->getNodeName(n));
        if (node >= 0 && node < nodenum)
        {
          for (int j = 0; j < a->dbgNodeMask.size(); j++)
            if (a->dbgNodeMask[j].nm.getNameId(gtree->getNodeName(n)) >= 0)
              b->addStr("nodemask", a->dbgNodeMask[j].name);
          nBlk[node] = b->addBlock("blender");
        }
      }
      if (!dump_all_nodes)
        removeEmptyBlocks(skel);
    }
    else
    {
      nBlk.resize(blender.targetNode.nameCount());
      for (int i = 0; i < nBlk.size(); i++)
      {
        nBlk[i] = skel->addNewBlock("node");
        nBlk[i]->setStr("name", blender.targetNode.getName(i));
      }
    }
    for (int j = 0; j < a->dbgNodeMask.size(); j++)
    {
      DataBlock &b = *nodemaskBlk.addNewBlock(a->dbgNodeMask[j].name);
      for (int k = 0; k < a->dbgNodeMask[j].nm.nameCount(); k++)
        b.addStr((gtree && !gtree->findNodeIndex(a->dbgNodeMask[j].nm.getName(k))) ? "missing-node" : "node",
          a->dbgNodeMask[j].nm.getName(k));
    }

    blk.setInt("ctrl", 0);
    blk.setInt("anim", 0);
    ctrlNid = blk.getNameId("ctrl");
    animNid = blk.getNameId("anim");
    blk.removeParam("ctrl");
    blk.removeParam("anim");
    abnWt.resize(a->getAnimNodeCount());
  }

  bool removeEmptyBlocks(DataBlock *b)
  {
    for (int i = b->blockCount() - 1; i >= 0; i--)
      if (removeEmptyBlocks(b->getBlock(i)))
      {
        for (int j = 0; j < gnBlk.size(); j++)
          if (gnBlk[j] == b->getBlock(i))
          {
            erase_items(gnBlk, j, 1);
            break;
          }
        b->removeBlock(i);
      }

    return b->blockCount() == 0 && find_value_idx(nBlk, b) < 0;
  }

  void dumpTm(bool dump_tm)
  {
    TMatrix tm;
    if (dump_tm)
      for (int i = 0; i < gnBlk.size(); i++)
      {
        gnTree->getNodeTmScalar(dag::Index16(i), tm);
        gnBlk[i]->setTm("tm", tm);
        gnTree->getNodeWtmRelScalar(dag::Index16(i), tm);
        gnBlk[i]->setTm("wtm", tm);
      }
    else if (prevDumpTm)
      for (int i = 0; i < gnBlk.size(); i++)
      {
        gnBlk[i]->removeParam("tm");
        gnBlk[i]->removeParam("wtm");
      }
    prevDumpTm = dump_tm;
  }

  static DataBlock *getSubBlock(DataBlock *b, int nid, const char *nm, int idx)
  {
    for (int i = 0, ord = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
      {
        if (ord == idx)
          return b->getBlock(i);
        else
          ord++;
      }
    return b->addNewBlock(nm);
  }
  static void resizeSubBlocks(DataBlock *b, int nid, int cnt)
  {
    for (int i = 0, ord = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
      {
        if (ord < cnt)
          ord++;
        else
        {
          for (int j = b->blockCount() - 1; j >= i; j--)
            if (b->getBlock(j)->getBlockNameId() == nid)
              b->removeBlock(j);
          return;
        }
      }
  }
};

AnimV20::AnimDbgCtx *AnimV20::AnimationGraph::createDebugBlenderContext(const GeomNodeTree *gtree, bool dump_all_node)
{
  return new AnimDbgCtx(this, gtree, dump_all_node);
}
void AnimV20::AnimationGraph::destroyDebugBlenderContext(AnimV20::AnimDbgCtx *ctx) { del_it(ctx); }

const DataBlock *AnimV20::AnimationGraph::getDebugBlenderState(AnimV20::AnimDbgCtx *ctx, IPureAnimStateHolder &st, bool dump_tm)
{
  if (!ctx)
    return NULL;

  ctx->dumpTm(dump_tm);
  AnimBlender::TlsContext &tlsCtx = selectBlenderCtx();

  mem_set_0(tlsCtx.bnlWt);
  mem_set_0(tlsCtx.pbcWt);
  ctx->abnWt.resize(getAnimNodeCount());
  mem_set_0(ctx->abnWt);
  IAnimBlendNode::BlendCtx bctx(st, tlsCtx, true);
  bctx.abnWt = ctx->abnWt.data();
  root->buildBlendingList(bctx, 1.0f);

  int ord = 0;
  ctx->abnActive->setStr("root", getBlendNodeName(root));
  iterate_names(nodeNames, [&](int i, const char *name) {
    if (ctx->abnWt[i] > 0)
    {
      if (nodePtr[i]->isSubOf(AnimBlendNodeLeafCID) || nodePtr[i]->isSubOf(AnimPostBlendCtrlCID) || nodePtr[i] == root)
        return;
      DataBlock *b = AnimDbgCtx::getSubBlock(ctx->abnActive, ctx->ctrlNid, "ctrl", ord);
      b->setStr("name", name);
      b->setReal("wt", ctx->abnWt[i]);
      ord++;
    }
  });
  AnimDbgCtx::resizeSubBlocks(ctx->abnActive, ctx->ctrlNid, ord);
  ord = 0;
  for (int i = 0; i < tlsCtx.pbcWt.size(); i++)
    if (tlsCtx.pbcWt[i] > 0)
    {
      DataBlock *b = AnimDbgCtx::getSubBlock(ctx->pbcActive, ctx->ctrlNid, "ctrl", ord);
      b->setStr("name", getBlendNodeName(blender.pbCtrl[i]));
      b->setReal("wt", tlsCtx.pbcWt[i]);
      ord++;
    }
  AnimDbgCtx::resizeSubBlocks(ctx->pbcActive, ctx->ctrlNid, ord);

  int bnlNum = blender.bnl.size();
  dag::Span<DataBlock *> nBlk = make_span(ctx->nBlk);
  for (int i = 0; i < nBlk.size(); i++)
    if (nBlk[i])
      for (int j = nBlk[i]->blockCount() - 1; j >= 0; j--)
        if (nBlk[i]->getBlock(j)->getBlockNameId() == ctx->animNid)
          nBlk[i]->removeBlock(j);

  // construct blending lists
  ord = 0;
  for (int i = 0; i < bnlNum; i++)
  {
    if (!*(int *)&tlsCtx.bnlWt[i] || !blender.bnl[i] || !blender.bnl[i]->anim)
      continue;

    const char *bnl_name = getBlendNodeName(blender.bnl[i]);

    real wa_w = tlsCtx.bnlWt[i];
    float wa_pos = blender.bnl[i]->tell(st);
    int wa_key = tlsCtx.bnlCT[i];

    DataBlock *b = AnimDbgCtx::getSubBlock(ctx->bnlActive, ctx->animNid, "anim", ord);
    b->setStr("name", bnl_name);
    b->setReal("wt", tlsCtx.bnlWt[i]);
    b->setReal("pos", wa_pos);
    b->setInt("frame", wa_key / (TIME_TicksPerSec / 30));
    ord++;

    const AnimDataChan<AnimChanPoint3> &pos = blender.bnl[i]->anim->anim.pos;
    const AnimDataChan<AnimChanPoint3> &scl = blender.bnl[i]->anim->anim.scl;
    const AnimDataChan<AnimChanQuat> &rot = blender.bnl[i]->anim->anim.rot;

    const AnimBlender::ChannelMap &cm = blender.bnlChan[i];

    for (int j = 0; j < pos.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[0][j];
      if (targetChN < 0)
        continue;
      b = nBlk[targetChN];
      if (!b)
        continue;
      b = b->addBlock(bnl_name);
      b->setStr("name", bnl_name);
      b->setReal("wt", pos.nodeWt[j] * wa_w);
      b->setStr("chan", "P--");
    }
    for (int j = 0; j < rot.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[2][j];
      if (targetChN < 0)
        continue;
      b = nBlk[targetChN];
      if (!b)
        continue;
      b = b->addBlock(bnl_name);
      if (!b->paramCount())
      {
        b->setStr("name", bnl_name);
        b->setReal("wt", rot.nodeWt[j] * wa_w);
        b->setStr("chan", "-R-");
      }
      else
        b->setStr("chan", "PR-");
    }
    for (int j = 0; j < scl.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[1][j];
      if (targetChN < 0)
        continue;
      b = nBlk[targetChN];
      if (!b)
        continue;
      b = b->addBlock(bnl_name);
      if (!b->paramCount())
      {
        b->setStr("name", bnl_name);
        b->setReal("wt", scl.nodeWt[j] * wa_w);
        b->setStr("chan", "--S");
      }
      else
      {
        const char *ostr = b->getStr("chan");
        if (strcmp(ostr, "PR-") == 0)
          b->setStr("chan", "PRS");
        else if (strcmp(ostr, "P--") == 0)
          b->setStr("chan", "P-S");
        else
          b->setStr("chan", "-RS");
      }
    }
  }
  AnimDbgCtx::resizeSubBlocks(ctx->bnlActive, ctx->animNid, ord);

  for (int i = 0; i < nBlk.size(); i++)
    if (nBlk[i])
      for (int j = nBlk[i]->blockCount() - 1; j >= 0; j--)
        nBlk[i]->getBlock(j)->changeBlockName("anim");

  return &ctx->blk;
}
const DataBlock *AnimV20::AnimationGraph::getDebugNodemasks(AnimDbgCtx *ctx) { return &ctx->nodemaskBlk; }
