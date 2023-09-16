#include <libTools/pageAsg/asg_anim_tree.h>
#include <libTools/pageAsg/asg_anim_ctrl.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

void AnimObjGraphTree::load(const DataBlock &blk)
{
  DataBlock *b;
  int i, j, nid;

  sharedNodeMaskFName = blk.getStr("sharedNodeMask", "");
  sharedCtrlFName = blk.getStr("sharedAnimCtrl", "");
  sharedBnlFName = blk.getStr("sharedAnimBnl", "");

  preview = *blk.getBlockByNameEx("preview");
  initAnimState = *blk.getBlockByNameEx("initAnimState");
  stateDesc = *blk.getBlockByNameEx("stateDesc");

  // loading nodemasks
  if (sharedNodeMaskFName.length())
    loadNodemasks(DataBlock(sharedNodeMaskFName));
  else
    loadNodemasks(blk);

  // append all leaf nodes first
  if (sharedBnlFName.length())
    loadAnimBnl(DataBlock(sharedBnlFName));
  else
    loadAnimBnl(blk);

  // append other controllers
  if (sharedCtrlFName.length())
    loadAnimCtrl(DataBlock(sharedCtrlFName));
  else
    loadAnimCtrl(blk);

  // other parameters
  root = blk.getStr("root", "N/A");
  dontUseAnim = blk.getBool("dont_use_anim", false);
  dontUseStates = blk.getBool("dontUseStates", false);
  if (dontUseStates)
  {
    debug("TREE LOAD: USE NO STATES");
  }
  if (forceNoStates)
  {
    debug("TREE LOAD: FORCE NO STATES");
    dontUseStates = true;
  }
}

void AnimObjGraphTree::loadNodemasks(const DataBlock &blk)
{
  int i, nid;
  nid = blk.getNameId("nodeMask");
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      AnimObjNodeMask *nm = new (tmpmem) AnimObjNodeMask;
      nm->load(*blk.getBlock(i));
      nodemask.push_back(nm);
    }
}
void AnimObjGraphTree::loadAnimCtrl(const DataBlock &blk)
{
  int i, j;
  for (i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock *b = blk.getBlock(i);
    if (stricmp(b->getBlockName(), "AnimBlendCtrl") == 0)
      for (j = 0; j < b->blockCount(); j++)
        loadBn(*b->getBlock(j), b->getBool("use", true));
  }
}
void AnimObjGraphTree::loadAnimBnl(const DataBlock &blk)
{
  int i, j;
  for (i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock *b = blk.getBlock(i);
    if (stricmp(b->getBlockName(), "AnimBlendNodeLeaf") == 0)
    {
      String a2d_fname(b->getStr("a2d", "N/A"));
      ::dd_simplify_fname_c(a2d_fname);
      strlwr(a2d_fname);

      a2dList.addNameId(a2d_fname);

      for (j = 0; j < b->blockCount(); j++)
        loadBnl(*b->getBlock(j), a2d_fname);
    }
  }
}

void AnimObjGraphTree::loadBnl(const DataBlock &blk, const char *a2d)
{
  if (stricmp(blk.getBlockName(), AnimObjBnl::typeName[AnimObjBnl::TYPE_Continuous]) == 0)
    addBnl(blk.getStr("name", "N/A"), a2d, AnimObjBnl::TYPE_Continuous, blk);

  else if (stricmp(blk.getBlockName(), AnimObjBnl::typeName[AnimObjBnl::TYPE_Single]) == 0)
    addBnl(blk.getStr("name", "N/A"), a2d, AnimObjBnl::TYPE_Single, blk);

  else if (stricmp(blk.getBlockName(), AnimObjBnl::typeName[AnimObjBnl::TYPE_Still]) == 0)
    addBnl(blk.getStr("name", "N/A"), a2d, AnimObjBnl::TYPE_Still, blk);

  else if (stricmp(blk.getBlockName(), AnimObjBnl::typeName[AnimObjBnl::TYPE_Parametric]) == 0)
    addBnl(blk.getStr("name", "N/A"), a2d, AnimObjBnl::TYPE_Parametric, blk);

  else
    debug_ctx("unknown BlenNode <%s>", blk.getBlockName());
}
void AnimObjGraphTree::loadBn(const DataBlock &blk, bool use)
{
  AnimObjCtrl *bn = NULL;

  if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Fifo]) == 0)
    bn = new (midmem) AnimObjCtrlFifo;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Fifo3]) == 0)
    bn = new (midmem) AnimObjCtrlFifo3;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Linear]) == 0)
    bn = new (midmem) AnimObjCtrlLinear;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_LinearPoly]) == 0)
    bn = new (midmem) AnimObjCtrlLinearPoly;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_RandomSwitch]) == 0)
    bn = new (midmem) AnimObjCtrlRandomSwitch;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_ParamSwitch]) == 0)
    bn = new (midmem) AnimObjCtrlParametricSwitch;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_ParamSwitchS]) == 0)
    bn = new (midmem) AnimObjCtrlParametricSwitchS;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Hub]) == 0)
    bn = new (midmem) AnimObjCtrlHub;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_DirectSync]) == 0)
    bn = new (midmem) AnimObjCtrlDirectSync;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Planar]) == 0)
    debug_ctx("not implenmented");

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Null]) == 0)
    bn = new (midmem) AnimObjCtrlNull;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Blender]) == 0)
    bn = new (midmem) AnimObjCtrlBlender;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_BIS]) == 0)
    bn = new (midmem) AnimObjCtrlBIS;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_AlignNode]) == 0)
    bn = new (midmem) AnimObjCtrlAlignNode;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_RotateNode]) == 0)
    bn = new (midmem) AnimObjCtrlRotateNode;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_MoveNode]) == 0)
    bn = new (midmem) AnimObjCtrlMoveNode;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_ParamsChange]) == 0)
    bn = new (midmem) AnimObjCtrlParamsChange;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_CondHideNode]) == 0)
    bn = new (midmem) AnimObjCtrlHideNode;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_AnimateNode]) == 0)
    bn = new (midmem) AnimObjAnimateNode;

  else if (stricmp(blk.getBlockName(), AnimObjCtrl::typeName[AnimObjCtrl::TYPE_Stub]) == 0)
  {
    bn = new (midmem) AnimObjCtrlNull;
    bn->type = AnimObjCtrl::TYPE_Stub;
  }

  else
    debug_ctx("unknown BlenNode <%s>", blk.getBlockName());

  // load comon params
  if (bn)
  {
    bn->used = use;
    bn->name = blk.getStr("name", "N/A");
    bn->load(blk);

    ctrlList.push_back(bn);
  }
}
void AnimObjGraphTree::addBnl(const char *name, const char *a2d, int type, const DataBlock &blk)
{
  AnimObjBnl *bnl = new (midmem) AnimObjBnl;

  bnl->name = name;
  bnl->a2dfname = a2d;
  bnl->type = type;

  bnl->load(blk);

  bnlList.push_back(bnl);
}

void AnimObjBnl::load(const DataBlock &blk)
{
  const char *key;

  p0 = 0;
  p1 = 1;

  nodemask = blk.getStr("apply_node_mask", "");
  timeScaleParam = "";

  switch (type)
  {
    case AnimObjBnl::TYPE_Continuous:
      key = blk.getStr("key", name);
      keyStart = key ? blk.getStr("key_start", String(128, "%s_start", key)) : blk.getStr("key_start", NULL);
      keyEnd = key ? blk.getStr("key_end", String(128, "%s_end", key)) : blk.getStr("key_start", NULL);
      syncTime = blk.getStr("sync_time", keyStart);
      nodeMask = blk.getStr("apply_node_mask", nodeMask);
      ownTimer = blk.getBool("own_timer", false);
      if (ownTimer)
        varname = name + "::timer";
      else
        varname = blk.getStr("timer", "::GlobalTime");
      eoaIrq = blk.getBool("eoa_irq", false);
      timeScaleParam = blk.getStr("timeScaleParam", "");

      duration = blk.getReal("time", 1.0);
      moveDist = blk.getReal("moveDist", 0.0);
      rewind = blk.getBool("rewind", true);
      loadNamedRanges(labels, *blk.getBlockByNameEx("labels"));
      break;

    case AnimObjBnl::TYPE_Single:
      key = blk.getStr("key", name);
      keyStart = key ? blk.getStr("key_start", String(128, "%s_start", key)) : blk.getStr("key_start", NULL);
      keyEnd = key ? blk.getStr("key_end", String(128, "%s_end", key)) : blk.getStr("key_start", NULL);
      syncTime = blk.getStr("sync_time", keyStart);
      nodeMask = blk.getStr("apply_node_mask", nodeMask);
      ownTimer = blk.getBool("own_timer", false);
      if (ownTimer)
        varname = name + "::timer";
      else
        varname = blk.getStr("timer", "::GlobalTime");
      eoaIrq = false;
      timeScaleParam = blk.getStr("timeScaleParam", "");

      duration = blk.getReal("time", 1.0);
      moveDist = blk.getReal("moveDist", 0.0);
      loadNamedRanges(labels, *blk.getBlockByNameEx("labels"));
      break;

    case AnimObjBnl::TYPE_Still:
      key = blk.getStr("key", name);
      keyStart = key;
      keyEnd = "";
      syncTime = "";
      nodeMask = blk.getStr("apply_node_mask", nodeMask);
      varname = "";
      ownTimer = true;
      eoaIrq = false;
      duration = 0.03;
      break;

    case AnimObjBnl::TYPE_Parametric:
      key = blk.getStr("key", name);
      keyStart = key ? blk.getStr("key_start", String(128, "%s_start", key)) : blk.getStr("key_start", NULL);
      keyEnd = key ? blk.getStr("key_end", String(128, "%s_end", key)) : blk.getStr("key_start", NULL);
      syncTime = "";
      nodeMask = blk.getStr("apply_node_mask", nodeMask);
      ownTimer = true;
      varname = blk.getStr("varname", name + "::var");
      eoaIrq = false;

      duration = 0;
      p0 = blk.getReal("p_start", 0.0);
      p1 = blk.getReal("p_end", 1.0);
      mulK = blk.getReal("mulk", 1.0);
      addK = blk.getReal("addk", 0.0);
      looping = blk.getBool("looping", false);
      updOnVarChange = blk.getBool("updOnVarChange", false);
      loadNamedRanges(labels, *blk.getBlockByNameEx("labels"));
      break;

    default: fatal_x("unknow type %d", type);
  }
  disableOriginVel = blk.getBool("disable_origin_vel", false);
  additive = blk.getBool("additive", false);
  addMove.dirh = blk.getReal("addMoveDirH", 0);
  addMove.dirv = blk.getReal("addMoveDirV", 0);
  addMove.dist = blk.getReal("addMoveDist", 0);
  foreignAnimation = blk.getBool("foreignAnimation", false);

  irqBlks.clearData();
  for (int i = 0, nid = blk.getNameId("irq"); i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
      irqBlks.addNewBlock(blk.getBlock(i), "irq");
}
void AnimObjBnl::loadNamedRanges(Tab<AnimObjBnl::Label> &labels_tab, const DataBlock &blk)
{
  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock *cb = blk.getBlock(i);
    int l = append_items(labels_tab, 1);
    labels_tab[l].name = cb->getBlockName();
    labels_tab[l].keyStart = cb->getStr("start", "N/A");
    labels_tab[l].keyEnd = cb->getStr("end", "N/A");
    labels_tab[l].relStart = cb->getReal("relative_start", 0.0);
    labels_tab[l].relEnd = cb->getReal("relative_end", 1.0);
  }
}
void AnimObjBnl::loadStripes(Stripes &s, const DataBlock &blk)
{
  s.start = blk.getBool("start", true);
  s.cp.clear();
  int nid = blk.getNameId("pos");

  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_REAL)
    {
      real pos = blk.getReal(i);
      s.add(pos);
    }
}

void AnimObjCtrlFifo::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", "N/A");
}
void AnimObjCtrlLinear::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");

  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", "N/A");

        int l = append_items(list, 1);
        list[l].node = nm;
        list[l].start = cblk->getReal("start", 0.0);
        list[l].end = cblk->getReal("end", 1.0);
      }
}
void AnimObjCtrlLinearPoly::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");

  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", "N/A");

        int l = append_items(list, 1);
        list[l].node = nm;
        list[l].p0 = cblk->getReal("val", 0.0);
      }
}
void AnimObjCtrlRandomSwitch::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");

  const DataBlock *cblk = blk.getBlockByName("weight");
  int i;

  if (cblk)
  {
    for (i = 0; i < cblk->paramCount(); i++)
    {
      if (cblk->getParamType(i) != DataBlock::TYPE_REAL)
        continue;

      const char *nm = cblk->getName(cblk->getParamNameId(i));
      int l = append_items(list, 1);
      list[l].node = nm;
      list[l].wt = cblk->getReal(i);
      list[l].maxRepeat = 1;
    }

    // patch max repeat times
    cblk = blk.getBlockByName("maxrepeat");
    if (cblk)
      for (i = list.size() - 1; i >= 0; i--)
      {
        const char *nm = list[i].node;
        list[i].maxRepeat = cblk->getInt(nm, list[i].maxRepeat);
      }
  }
}
void AnimObjCtrlParametricSwitch::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");
  morphTime = blk.getReal("morphTime", 0.15);

  const DataBlock *cblk = blk.getBlockByName("nodes");
  int i;

  if (cblk)
  {
    for (i = 0; i < cblk->blockCount(); i++)
    {
      const DataBlock *nblk = cblk->getBlock(i);
      const char *nm = nblk->getStr("name", "");
      int l = append_items(list, 1);
      list[l].node = nm;
      list[l].r0 = nblk->getReal("rangeFrom", 0);
      list[l].r1 = nblk->getReal("rangeTo", 0);
    }
  }
}
void AnimObjCtrlHub::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", "N/A");

        int l = append_items(list, 1);
        list[l].node = nm;
        list[l].w = cblk->getReal("weight", 1.0);
        list[l].enabled = cblk->getBool("enabled", true);
      }
  _const = blk.getBool("const", false);
}
void AnimObjCtrlDirectSync::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");

  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", "N/A");

        int l = append_items(list, 1);
        list[l].node = nm;
        list[l].linkedTo = cblk->getStr("linked_to", "N/A");
        list[l].dc = cblk->getReal("offset", 0);
        list[l].scale = cblk->getReal("scale", 1.0);
        list[l].clamp0 = cblk->getReal("clamp0", 0);
        list[l].clamp1 = cblk->getReal("clamp1", 1.0);
      }
}
void AnimObjCtrlBlender::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");
  node1 = blk.getStr("node1", "");
  node2 = blk.getStr("node2", "");
  duration = blk.getReal("duration", 1.0);
  morph = blk.getReal("morph", 1.0);
}
void AnimObjCtrlBIS::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  varname = blk.getStr("varname", name + "::var");
  maskAnd = blk.getInt("maskAnd", 0);
  maskEq = blk.getInt("maskEq", 0);
  node1 = blk.getStr("node1", "");
  morph1 = blk.getReal("morph1", 1.0);
  node2 = blk.getStr("node2", "");
  morph2 = blk.getReal("morph2", 1.0);
  fifo = blk.getStr("fifo", "");
}

void AnimObjCtrlAlignNode::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  target = blk.getStr("targetNode", "");
  src = blk.getStr("srcNode", "");
  rot_euler = blk.getPoint3("rot_euler", Point3(0, 0, 0));
  useLocal = blk.getBool("useLocal", false);
  scale = blk.getPoint3("scale", Point3(1, 1, 1));
}

void AnimObjCtrlRotateNode::load(const DataBlock &blk)
{
  AnimObjCtrl::load(blk);
  int nid = blk.getNameId("targetNode");
  defWt = blk.getReal("targetNodeWt", 1.0f);
  for (int j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = target.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", target.size()), defWt);
    }

  kMul = blk.getReal("kMul", 1.0);
  kAdd = blk.getReal("kAdd", 0.0);
  kCourseAdd = blk.getReal("kCourseAdd", 0.0);
  paramName = blk.getStr("param", "n/a");
  axisCourse = blk.getStr("axis_course", "");
  rotAxis = blk.getPoint3("rotAxis", Point3(0, 1, 0));
  dirAxis = blk.getPoint3("dirAxis", Point3(0, 0, 1));
  leftTm = blk.getBool("leftTm", true);
}


void AnimObjNodeMask::load(const DataBlock &blk)
{
  int nid2 = blk.getNameId("node"), j;

  name = blk.getStr("name", "");
  for (j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid2)
      nm.addNameId(blk.getStr(j));
}
