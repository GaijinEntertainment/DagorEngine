// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/pageAsg/asg_data.h>
#include <libTools/pageAsg/asg_nodes.h>
#include <libTools/pageAsg/asg_scheme.h>
#include <libTools/pageAsg/asg_sysHelper.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>

//
// Anim graph object in editor
//
void AsgStatesGraph::clear()
{
  int i;

  tree.clear();
  usedCondList.clear();
  clear_and_shrink(codeForCond);
  flagList.clear();
  classList.clear();
  irqList.clear();
  clear_and_shrink(gt);
  clear_and_shrink(chan);
  clear_and_shrink(attachments);

  for (i = states.size() - 1; i >= 0; i--)
    delete states[i];
  clear_and_shrink(states);
  statePropsContainer.reset();

  rootScheme = NULL;
}
void AsgStatesGraph::clearWithDefaults()
{
  clear();

  tree.fillDefaults();

  int i;
  i = append_items(gt, 1);
  gt[i].name = "Main";
  gt[i].startNodeName = "Root";

  i = append_items(chan, 1);
  chan[i].name = "General";
  chan[i].fifoCtrl = "fifo3";
  chan[i].defMorphTime = 0.15;

  AnimGraphState *st = new (midmem) AnimGraphState(statePropsContainer.addNewBlock("props"), chan.size());
  st->name = "Root";
  st->undeletable = true;
  st->groupId.headOf = states.size();
  states.push_back(st);
}
bool AsgStatesGraph::checkValid() { return true; }

// load
bool AsgStatesGraph::load(const DataBlock &blk, const DataBlock *blk_tree, const DataBlock *blk_sg, bool forceNoStates)
{
  clear();

  const DataBlock *cb;
  int nid, i;

  if (!blk_tree)
    blk_tree = blk.getBlockByNameEx("animGraph");
  tree.load(*blk_tree);

  tree.forceNoStates = forceNoStates;
  if (forceNoStates)
  {
    debug("FORCE NO STATES");
    tree.dontUseStates = true;
  }

  nid = blk.getNameId("attach");
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cb2 = blk.getBlock(i);
      int j = append_items(attachments, 1);
      attachments[j].res = cb2->getStr("resource", "");
      attachments[j].tree = cb2->getStr("tree", "");
      attachments[j].node = cb2->getStr("bind", "");
    }


  // load Graph
  if (!blk_sg)
    blk_sg = &blk;

  cb = blk_sg->getBlockByNameEx("statesGraph");
  forcedStateMorphTime = blk_sg->getReal("forcedStateMorphTime", 0);

  // load graph threads
  nid = cb->getNameId("thread");
  for (i = 0; i < cb->blockCount(); i++)
    if (cb->getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cb2 = cb->getBlock(i);
      int j = append_items(gt, 1);
      gt[j].name = cb2->getStr("name", "thread");
      gt[j].startNodeName = cb2->getStr("root", "Root");
    }

  // load anim channels
  nid = cb->getNameId("channel");
  for (i = 0; i < cb->blockCount(); i++)
    if (cb->getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cb2 = cb->getBlock(i);
      int j = append_items(chan, 1);
      chan[j].name = cb2->getStr("name", "channel");
      chan[j].fifoCtrl = cb2->getStr("fifo", "fifo3");
      chan[j].defSuffix = cb2->getStr("defSuffix", "");
      chan[j].defNodemask = cb2->getStr("defNodemask", "");
      chan[j].defMorphTime = cb2->getReal("defMorphTime", 0.15);
    }

  // validate gt>0 && chan>0
  if (gt.size() == 0)
  {
    DEBUG_CTX("warning: graph thread count = 0");
    i = append_items(gt, 1);
    gt[i].name = "Main";
    gt[i].startNodeName = "Root";
  }
  if (chan.size() == 0)
  {
    DEBUG_CTX("warning: anim channel count = 0");
    i = append_items(chan, 1);
    chan[i].name = "General";
    chan[i].fifoCtrl = "fifo3";
    chan[i].defMorphTime = 0.15;
  }

  // create list of states
  nid = cb->getNameId("state");
  for (i = 0; i < cb->blockCount(); i++)
    if (cb->getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cbState = cb->getBlock(i);
      if (cbState->paramCount() == 0 && cbState->blockCount() == 0)
      {
        states.push_back(NULL);
        continue;
      }
      DataBlock *cbProps = statePropsContainer.addNewBlock(cbState->getBlockByNameEx("props"), "props");

      AnimGraphState *st = new (midmem) AnimGraphState(cbProps, chan.size());
      st->load(*cbState);
      states.push_back(st);
    }
  rootGroupOrigin = cb->getIPoint2("rootGroupOrigin", IPoint2(0, 0));

  // read flags/classes/irq names
  loadNamemap(*blk_sg->getBlockByNameEx("flags"), "flag", flagList);
  loadNamemap(*blk_sg->getBlockByNameEx("classes"), "class", classList);
  loadNamemap(*blk_sg->getBlockByNameEx("irqs"), "irq", irqList);

  // read conditions
  cb = blk_sg->getBlockByNameEx("conditions");
  nid = cb->getNameId("cond");
  int nid2 = cb->getNameId("__code");
  for (i = 0; i < cb->paramCount(); i++)
    if (cb->getParamNameId(i) == nid && cb->getParamType(i) == DataBlock::TYPE_STRING)
      usedCondList.addNameId(cb->getStr(i));
    else if (cb->getParamNameId(i) == nid2 && cb->getParamType(i) == DataBlock::TYPE_STRING && usedCondList.nameCount() > 0)
    {
      codeForCond.resize(usedCondList.nameCount());
      codeForCond[usedCondList.nameCount() - 1] = cb->getStr(i);
    }

  return true;
}


// save
bool AsgStatesGraph::save(DataBlock &blk, DataBlock *blk_tree, DataBlock *blk_sg)
{
  DataBlock *cb;
  int i;

  if (!blk_tree)
    blk_tree = blk.addNewBlock("animGraph");
  tree.save(*blk_tree);

  for (i = 0; i < attachments.size(); i++)
  {
    DataBlock *cb2 = blk.addNewBlock("attach");
    cb2->setStr("resource", attachments[i].res);
    cb2->setStr("tree", attachments[i].tree);
    cb2->setStr("bind", attachments[i].node);
  }

  if (!blk_sg)
    blk_sg = &blk;

  cb = blk_sg->addNewBlock("statesGraph");
  if (forcedStateMorphTime > 0)
    blk_sg->setReal("forcedStateMorphTime", forcedStateMorphTime);
  for (i = 0; i < gt.size(); i++)
  {
    DataBlock *cb2 = cb->addNewBlock("thread");
    cb2->setStr("name", gt[i].name);
    cb2->setStr("root", gt[i].startNodeName);
  }
  for (i = 0; i < chan.size(); i++)
  {
    DataBlock *cb2 = cb->addNewBlock("channel");
    cb2->setStr("name", chan[i].name);
    cb2->setStr("fifo", chan[i].fifoCtrl);
    cb2->setStr("defSuffix", chan[i].defSuffix);
    cb2->setStr("defNodemask", chan[i].defNodemask);
    cb2->setReal("defMorphTime", chan[i].defMorphTime);
  }
  for (i = 0; i < states.size(); i++)
  {
    if (!states[i])
    {
      cb->addNewBlock("state");
      continue;
    }

    DataBlock *cbState = cb->addNewBlock("state");
    if (states[i]->props)
      cbState->addNewBlock(states[i]->props, "props");
    states[i]->save(*cbState);
  }
  cb->setIPoint2("rootGroupOrigin", rootGroupOrigin);

  saveNamemap(*blk_sg->addNewBlock("flags"), "flag", flagList);
  saveNamemap(*blk_sg->addNewBlock("classes"), "class", classList);
  saveNamemap(*blk_sg->addNewBlock("irqs"), "irq", irqList);

  cb = blk_sg->addNewBlock("conditions");
  for (i = 0; i < usedCondList.nameCount(); i++)
  {
    cb->addStr("cond", usedCondList.getName(i));
    cb->addStr("__code", codeForCond[i]);
  }

  return true;
}

int AsgStatesGraph::compact(int prev_group_id)
{
  SmallTab<int, MidmemAlloc> map;
  int curId = 0;

  complementCodeTab();
  clear_and_resize(map, states.size());
  for (int i = 0; i < states.size(); i++)
    if (states[i])
    {
      map[i] = curId;
      curId++;
    }
    else
      map[i] = -1;

  // if no gaps, remapping is not needed
  if (curId == states.size())
    return prev_group_id;

  // remap states
  clear_and_resize(stateToNode, 0);
  Tab<AnimGraphState *> newSt(tmpmem);
  for (int i = 0; i < states.size(); i++)
    if (states[i])
    {
      states[i]->groupId.belongsTo = (states[i]->groupId.belongsTo == -1) ? -1 : map[states[i]->groupId.belongsTo];
      states[i]->groupId.headOf = map[i];
      newSt.push_back(states[i]);
    }
  states = newSt;

  // return remapped index
  if (prev_group_id == -1)
    return -1;
  return map[prev_group_id];
}

void AsgStatesGraph::complementCodeTab()
{
  while (codeForCond.size() < usedCondList.nameCount())
    codeForCond.push_back(String("false"));
}

bool AsgStatesGraph::buildAndSave(const char *blk_fname, const char *rel_pdl, const char *res_name, int dbg_level, bool in_editor)
{
  AsgStateGenParamGroup *rs = rootScheme;

  dd_mkpath(blk_fname);

  SimpleString pdl_fname(rel_pdl);

  if (!tree.dontUseAnim && !tree.dontUseStates && rel_pdl)
    dd_mkpath(pdl_fname);

  tree.compact();

  // split anim channels according to state's flags (needed for build)
  splitAnimChannels();
  if (!tree.dontUseAnim && !tree.dontUseStates)
  {
    if (rel_pdl)
      if (!exportCppCode(pdl_fname, dbg_level, rs, NULL))
      {
        restoreAnimChannels();
        page_sys_hlp->postMsg(true, "Build error", "Failed to build PDL\n%s", pdl_fname.str());
        return false;
      }
    if (!in_editor && page_sys_hlp->getSgCppDestDir())
      if (!exportCppCode(pdl_fname, dbg_level, rs, res_name))
      {
        restoreAnimChannels();
        page_sys_hlp->postMsg(true, "Build error", "Failed to generate graph CPP");
        return false;
      }
  }
  // gather BNL and A2D lists needed for graph; then restore splitted anim channels
  AnimObjExportContext ctx;
  gatherBnlsNeeded(ctx.bnls, ctx.a2d, ctx.controllers);
  restoreAnimChannels();

  if (true)
  {
    for (int i = 0; i < ctx.a2d.nameCount(); i++)
    {
      String newpath(ctx.a2d.getName(i));
      for (int j = 0; j < newpath.length(); j++)
        if (newpath[j] == '/' || newpath[j] == '\\' || newpath[j] == ':')
          newpath[j] = '_';
      ctx.a2d_repl.addNameId(newpath);
    }
    if (ctx.a2d_repl.nameCount() != ctx.a2d.nameCount())
    {
      page_sys_hlp->postMsg(true, "Build error", "Substitute a2d names");
      return false;
    }
  }

  // write generic anim tree file
  DataBlock blk, *cb;

  ctx.clones.reserve(chan.size());
  for (int i = 0; i < chan.size(); i++)
    if (chan[i].defSuffix.length() || chan[i].defNodemask.length())
    {
      int j = append_items(ctx.clones, 1);
      ctx.clones[j].suffix = chan[i].defSuffix;
      ctx.clones[j].nodemask = chan[i].defNodemask;
    }
  tree.exportTree(*blk.addNewBlock("animGraph"), ctx);

  if (!tree.dontUseAnim && !tree.dontUseStates && rel_pdl)
  {
    cb = blk.addNewBlock("pdlGraph");
    cb->setStr("pdl", pdl_fname);
  }

  if (!blk.saveToTextFile(blk_fname))
  {
    DEBUG_CTX("can't write <%s>", blk_fname);
    page_sys_hlp->postMsg(true, "Build error", "Failed to write graph BLK\n%s", blk_fname);
    return false;
  }

  return true;
}


// node conversion
AnimGraphState *AsgStatesGraph::getState(int state_id)
{
  if (stateToNode.size() != states.size())
    return NULL;

  if (state_id < 0 || state_id >= states.size())
    return NULL;
  return states[state_id];
}
int AsgStatesGraph::findState(const char *name)
{
  for (int i = 0; i < states.size(); i++)
    if (states[i] && strcmp(states[i]->name, name) == 0)
      return i;
  return -1;
}
int AsgStatesGraph::getNodeId(int state_id)
{
  if (stateToNode.size() != states.size())
    return -1;
  if (state_id == -1)
    return -1;
  return stateToNode[state_id];
}

const char *AsgStatesGraph::getParentGroupName(AnimGraphState *s)
{
  if (s->groupId.belongsTo == -1)
    return "";
  if (states[s->groupId.belongsTo])
    return states[s->groupId.belongsTo]->name;
  return "--deleted--";
}
void AsgStatesGraph::addLink(int src_id, int dest_id, AnimGraphBranchType cond_type, const char *condition, bool customMorph,
  real customMorphTime)
{
  if (dest_id < 0 || dest_id >= states.size() || !states[dest_id])
    return;
  addLink(src_id, states[dest_id]->name, cond_type, condition, customMorph, customMorphTime);
}
void AsgStatesGraph::addLink(int src_id, const char *dest_name, AnimGraphBranchType cond_type, const char *condition, bool customMorph,
  real customMorphTime)
{
  Tab<AnimGraphCondition *> *cond = getCond(src_id, cond_type);
  if (!cond)
    return;
  AnimGraphCondition *c = new (midmem) AnimGraphCondition;
  c->condition = condition;
  c->targetName = dest_name;
  c->customMorph = customMorph;
  c->customMorphTime = customMorphTime;
  cond->push_back(c);
}
void AsgStatesGraph::delLinks(int src_id, int dest_id, AnimGraphBranchType cond_type)
{
  Tab<AnimGraphCondition *> *cond = getCond(src_id, cond_type);
  if (!cond)
    return;

  if (dest_id < 0 || dest_id >= states.size())
    return;

  for (int i = cond->size() - 1; i >= 0; i--)
    if (strcmp(states[dest_id]->name, (*cond)[i]->targetName) == 0)
    {
      delete (*cond)[i];
      erase_items(*cond, i, 1);
    }
}
Tab<AnimGraphCondition *> *AsgStatesGraph::getCond(int state_id, AnimGraphBranchType cond_type)
{
  if (state_id < 0 || state_id >= states.size())
    return NULL;

  switch (cond_type)
  {
    case AGBT_EnterCondition: return &states[state_id]->enterCond;
    case AGBT_CheckAtEnd: return &states[state_id]->endCond;
    case AGBT_CheckAlways: return &states[state_id]->whileCond;
  }
  return NULL;
}
void AsgStatesGraph::delOneLink(int state_id, int cond_idx, AnimGraphBranchType cond_type)
{
  Tab<AnimGraphCondition *> *cond = getCond(state_id, cond_type);
  if (!cond)
    return;

  if (cond_idx < 0 || cond_idx >= cond->size())
    return;

  delete (*cond)[cond_idx];
  erase_items(*cond, cond_idx, 1);
}
void AsgStatesGraph::moveLink(int state_id, int cond_idx, AnimGraphBranchType cond_type, int dir)
{
  Tab<AnimGraphCondition *> *cond = getCond(state_id, cond_type);
  if (!cond || !dir)
    return;

  if (cond_idx < 0 || cond_idx >= cond->size())
    return;
  if ((cond_idx == 0 && dir < 0) || (cond_idx == cond->size() - 1 && dir > 0))
    return;

  int new_idx = cond_idx + ((dir > 0) ? 1 : -1);
  AnimGraphCondition *tmp = cond->at(cond_idx);
  cond->at(cond_idx) = cond->at(new_idx);
  cond->at(new_idx) = tmp;
}

int AsgStatesGraph::getConditionsCount(int src_id, int dest_id, AnimGraphBranchType cond_type)
{
  Tab<AnimGraphCondition *> *cond = getCond(src_id, cond_type);
  if (!cond)
    return 0;
  if (dest_id < 0 || dest_id >= states.size())
    return 0;

  int cnt = 0;
  for (int i = cond->size() - 1; i >= 0; i--)
    if (strcmp(states[dest_id]->name, (*cond)[i]->targetName) == 0)
      cnt++;

  return cnt;
}

// links update routines
void AsgStatesGraph::renameState(const char *old_name, const char *new_name)
{
  if (!old_name || !old_name[0])
    return;

  for (int i = 0; i < states.size(); i++)
    if (states[i])
      states[i]->renameState(old_name, new_name);
}
void AsgStatesGraph::renameAnimNode(const char *old_name, const char *new_name)
{
  if (!old_name || !old_name[0])
    return;

  for (int i = 0; i < states.size(); i++)
    if (states[i])
      for (int j = 0; j < states[i]->animNode.size(); j++)
        if (strcmp(old_name, states[i]->animNode[j]) == 0)
          states[i]->animNode[j] = new_name;
}
void AsgStatesGraph::renameA2dFile(const char *old_name, const char *new_name)
{
  for (int i = 0; i < tree.bnlList.size(); i++)
    if (strcmp(old_name, tree.bnlList[i]->a2dfname) == 0)
      tree.bnlList[i]->a2dfname = new_name;
}

void AsgStatesGraph::setWndPosition(int groupId, const IPoint2 &pos)
{
  if (groupId == -1)
    rootGroupOrigin = pos;
  else if (groupId >= 0 && groupId < states.size())
    states[groupId]->groupOrigin = pos;
}
void AsgStatesGraph::getWndPosition(int groupId, IPoint2 &pos) const
{
  if (groupId == -1)
    pos = rootGroupOrigin;
  else if (groupId >= 0 && groupId < states.size())
    pos = states[groupId]->groupOrigin;
  else
    pos = IPoint2(0, 0);
}

void AsgStatesGraph::incGtCount()
{
  if (gt.size() < 4)
    append_items(gt, 1);
}
void AsgStatesGraph::decGtCount()
{
  if (gt.size() <= 1)
    return;
  gt.pop_back();
  for (int i = 0; i < states.size(); i++)
    if (states[i])
      if (states[i]->gt >= gt.size())
        states[i]->gt = gt.size() - 1;
}
void AsgStatesGraph::incChanCount()
{
  if (chan.size() < 4)
    append_items(chan, 1);
  for (int i = 0; i < states.size(); i++)
    if (states[i])
      states[i]->animNode.resize(chan.size());
}
void AsgStatesGraph::decChanCount()
{
  if (chan.size() <= 1)
    return;
  chan.pop_back();
  for (int i = 0; i < states.size(); i++)
    if (states[i])
      states[i]->animNode.resize(chan.size());
}

void AsgStatesGraph::splitAnimChannels()
{
  if (chan.size() < 2)
    return;

  for (int i = 0; i < states.size(); i++)
    if (states[i])
      if (states[i]->useAnimForAllChannel && states[i]->splitAnim)
      {
        String anim(states[i]->animNode[0]);
        if (tree.findBnl(anim))
          for (int j = 0; j < chan.size(); j++)
            states[i]->animNode[j] = anim + chan[j].defSuffix;
        else
          for (int j = 1; j < chan.size(); j++)
            states[i]->animNode[j] = "";
      }
}
void AsgStatesGraph::restoreAnimChannels()
{
  if (chan.size() < 2)
    return;

  for (int i = 0; i < states.size(); i++)
    if (states[i])
      if (states[i]->useAnimForAllChannel && states[i]->splitAnim)
      {
        String anim(states[i]->animNode[0]);

        remove_trailing_string(anim, chan[0].defSuffix);
        states[i]->animNode[0] = anim;
        for (int j = 1; j < chan.size(); j++)
          states[i]->animNode[j] = "";
      }
}
void AsgStatesGraph::getBnlsNeeded(NameMap &bnls, NameMap &a2d, NameMap &ctrls)
{
  splitAnimChannels();
  gatherBnlsNeeded(bnls, a2d, ctrls);
  restoreAnimChannels();
}
void AsgStatesGraph::gatherBnlsNeeded(NameMap &bnls, NameMap &a2d, NameMap &ctrls)
{
  // debug("gatherBnlsNeeded, tree.dontUseStates = %d, tree.forceNoStates=%d",
  //   tree.dontUseStates,tree.forceNoStates);
  if (tree.dontUseStates || tree.forceNoStates)
  {
    if (tree.dontUseAnim)
      return;

    for (int i = 0; i < tree.bnlList.size(); i++)
    {
      bnls.addNameId(tree.bnlList[i]->name);
      a2d.addNameId(tree.bnlList[i]->a2dfname);
      // debug("added bnl %s and a2d %s",
      //   tree.bnlList[i]->name.str(), tree.bnlList[i]->a2dfname.str());
    }
    return;
  }

  for (int i = 0; i < states.size(); i++)
    if (states[i])
      for (int j = 0; j < chan.size(); j++)
        if (states[i]->animNode[j].length() > 0)
        {
          String anim(states[i]->animNode[j]);
          AnimObjBnl *bnl;
          AnimObjCtrl *ctrl;

          bnl = tree.findBnl(anim);
          if (bnl)
          {
            bnls.addNameId(anim);
            a2d.addNameId(bnl->a2dfname);
            continue;
          }

          ctrl = tree.findCtrl(anim);
          if (ctrl)
          {
            ctrl->addNeededBnls(bnls, a2d, tree, "");
            ctrls.addNameId(anim);
            continue;
          }

          for (int k = 0; k < chan.size(); k++)
          {
            remove_trailing_string(anim, chan[k].defSuffix);
            bnl = tree.findBnl(anim);
            if (bnl)
            {
              bnls.addNameId(states[i]->animNode[j]);
              a2d.addNameId(bnl->a2dfname);
              break;
            }
            ctrl = tree.findCtrl(anim);
            if (ctrl)
            {
              ctrl->addNeededBnls(bnls, a2d, tree, chan[k].defSuffix);
              ctrls.addNameId(anim + chan[k].defSuffix);
              break;
            }
          }
        }
  for (int i = 0; i < tree.ctrlList.size(); i++)
    tree.ctrlList[i]->addNeededBnls(bnls, a2d, tree, "");
}


//
// Graph state
//
AnimGraphState::AnimGraphState(DataBlock *_props, int chan_num) :
  enterCond(midmem), endCond(midmem), whileCond(midmem), animNode(midmem), actions(midmem)
{
  props = _props;
  undeletable = false;
  locked = false;
  useAnimForAllChannel = true;
  splitAnim = false;
  resumeAnim = false;
  animBasedActionTiming = false;

  groupId.belongsTo = -1;
  groupId.headOf = -1;

  gt = 0;
  memset(&nodePos, 0, sizeof(nodePos));
  animNode.resize(chan_num);
  memset(&addVel, 0, sizeof(addVel));
}

void AnimGraphState::load(const DataBlock &blk)
{
  const DataBlock *cb;
  int nid, i, j;

  name = blk.getStr("name", name);
  undeletable = blk.getBool("undeletable", undeletable);
  locked = blk.getBool("locked", locked);
  gt = blk.getInt("gt", 0);
  addVel.dirh = blk.getReal("addVelDirH", 0);
  addVel.dirv = blk.getReal("addVelDirV", 0);
  addVel.vel = blk.getReal("addVelAbs", 0);
  useAnimForAllChannel = blk.getBool("useAnimForAllChannel", true);
  splitAnim = blk.getBool("splitAnim", false);
  resumeAnim = blk.getBool("resumeAnim", false);
  animBasedActionTiming = blk.getBool("animBasedActionTiming", false);

  cb = blk.getBlockByNameEx("animNodeName");
  nid = cb->getNameId("name");
  for (i = 0, j = 0; j < animNode.size() && i < cb->paramCount(); i++)
    if (cb->getParamNameId(i) == nid && cb->getParamType(i) == DataBlock::TYPE_STRING)
    {
      animNode[j] = cb->getStr(i);
      j++;
    }

  // validate anim channels read
  if (j == 0 && animNode.size() > 0)
  {
    DEBUG_CTX("warning: no animNode found!");
    animNode[0] = blk.getStr("animNodeName", "");
  }

  loadConditions(enterCond, *blk.getBlockByNameEx("enterConditions"));
  loadConditions(endCond, *blk.getBlockByNameEx("endConditions"));
  loadConditions(whileCond, *blk.getBlockByNameEx("whileConditions"));

  // read class names
  cb = blk.getBlockByNameEx("classes");
  nid = cb->getNameId("class");
  for (i = 0; i < cb->paramCount(); i++)
    if (cb->getParamNameId(i) == nid && cb->getParamType(i) == DataBlock::TYPE_STRING)
      classMarks.addNameId(cb->getStr(i));

  cb = blk.getBlockByNameEx("edit");
  groupId.belongsTo = cb->getInt("groupId_belongsTo", -1);
  groupId.headOf = cb->getInt("groupId_headOf", -1);
  nodePos.belongsTo = cb->getIPoint2("nodePos_belongsTo", IPoint2(0, 0));
  nodePos.headOf = cb->getIPoint2("nodePos_headOf", IPoint2(0, 0));
  groupOrigin = cb->getIPoint2("groupOrigin", IPoint2(0, 0));

  // read actions
  cb = blk.getBlockByNameEx("actions");
  nid = cb->getNameId("action");
  for (i = 0; i < cb->blockCount(); i++)
    if (cb->getBlock(i)->getBlockNameId() == nid)
    {
      AnimGraphTimedAction *a = new (midmem) AnimGraphTimedAction;
      a->load(*cb->getBlock(i));
      actions.push_back(a);
    }
}
void AnimGraphState::save(DataBlock &blk) const
{
  DataBlock *cb;

  blk.setStr("name", name);
  blk.setBool("undeletable", undeletable);
  blk.setBool("locked", locked);
  blk.setInt("gt", gt);
  blk.setReal("addVelDirH", addVel.dirh);
  blk.setReal("addVelDirV", addVel.dirv);
  blk.setReal("addVelAbs", addVel.vel);
  blk.setBool("useAnimForAllChannel", useAnimForAllChannel);
  blk.setBool("splitAnim", splitAnim);
  blk.setBool("resumeAnim", resumeAnim);
  if (animBasedActionTiming)
    blk.setBool("animBasedActionTiming", animBasedActionTiming);

  cb = blk.addNewBlock("animNodeName");
  for (int i = 0; i < animNode.size(); i++)
    cb->addStr("name", animNode[i]);

  saveConditions(enterCond, *blk.addNewBlock("enterConditions"));
  saveConditions(endCond, *blk.addNewBlock("endConditions"));
  saveConditions(whileCond, *blk.addNewBlock("whileConditions"));

  cb = blk.addNewBlock("classes");
  for (int i = 0; i < classMarks.nameCount(); i++)
    cb->addStr("class", classMarks.getName(i));

  cb = blk.addNewBlock("edit");
  cb->setInt("groupId_belongsTo", groupId.belongsTo);
  cb->setInt("groupId_headOf", groupId.headOf);
  cb->setIPoint2("nodePos_belongsTo", nodePos.belongsTo);
  cb->setIPoint2("nodePos_headOf", nodePos.headOf);
  cb->setIPoint2("groupOrigin", groupOrigin);

  cb = blk.addNewBlock("actions");
  for (int i = 0; i < actions.size(); i++)
    actions[i]->save(*cb->addNewBlock("action"));
}

void AnimGraphState::clear()
{
  int i;

  for (i = 0; i < enterCond.size(); i++)
    delete enterCond[i];
  clear_and_shrink(enterCond);

  for (i = 0; i < endCond.size(); i++)
    delete endCond[i];
  clear_and_shrink(endCond);

  for (i = 0; i < whileCond.size(); i++)
    delete whileCond[i];
  clear_and_shrink(whileCond);

  for (int i = 0; i < actions.size(); i++)
    delete actions[i];
  clear_and_shrink(actions);

  name = "";
  clear_and_shrink(animNode);
}
void AnimGraphState::loadConditions(Tab<AnimGraphCondition *> &cond, const DataBlock &blk)
{
  int nid = blk.getNameId("cond"), i;
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cbCond = blk.getBlock(i);
      AnimGraphCondition *c = new (midmem) AnimGraphCondition;
      c->load(*cbCond);
      cond.push_back(c);
    }
}
void AnimGraphState::saveConditions(const Tab<AnimGraphCondition *> &cond, DataBlock &blk) const
{
  for (int i = 0; i < cond.size(); i++)
  {
    DataBlock *cbCond = blk.addNewBlock("cond");
    cond[i]->save(*cbCond);
  }
}

void AnimGraphState::renameState(const char *old_name, const char *new_name)
{
  int i;

  for (i = 0; i < enterCond.size(); i++)
    if (strcmp(enterCond[i]->targetName, old_name) == 0)
      enterCond[i]->targetName = new_name;

  for (i = 0; i < endCond.size(); i++)
    if (strcmp(endCond[i]->targetName, old_name) == 0)
      endCond[i]->targetName = new_name;

  for (i = 0; i < whileCond.size(); i++)
    if (strcmp(whileCond[i]->targetName, old_name) == 0)
      whileCond[i]->targetName = new_name;
}

void AnimGraphState::addConditions(NameMap &list)
{
  int i;

  for (i = 0; i < enterCond.size(); i++)
    list.addNameId(enterCond[i]->condition);
  for (i = 0; i < endCond.size(); i++)
    list.addNameId(endCond[i]->condition);
  for (i = 0; i < whileCond.size(); i++)
    list.addNameId(whileCond[i]->condition);
}


//
// Branch condition
//
AnimGraphCondition::AnimGraphCondition()
{
  customMorph = false;
  customMorphTime = 0.15f;
}

void AnimGraphCondition::load(const DataBlock &blk)
{
  targetName = blk.getStr("target", targetName);
  condition = blk.getStr("condition", condition);
  customMorph = blk.getBool("customMorph", false);
  customMorphTime = blk.getReal("customMorphTime", 0.15f);
}
void AnimGraphCondition::save(DataBlock &blk) const
{
  blk.setStr("target", targetName);
  blk.setStr("condition", condition);
  blk.setBool("customMorph", customMorph);
  blk.setReal("customMorphTime", customMorphTime);
}


//
// Timed actions
//
AnimGraphTimedAction::AnimGraphTimedAction() : actions(midmem)
{
  performAt = AGTA_AT_Start;
  relativeTime = true;
  cyclic = false;
  preciseTime = 0.0;
}
AnimGraphTimedAction::~AnimGraphTimedAction()
{
  for (int i = 0; i < actions.size(); i++)
    delete actions[i];
  clear_and_shrink(actions);
}

void AnimGraphTimedAction::load(const DataBlock &blk)
{
  performAt = blk.getInt("performAt", AGTA_AT_Start);
  relativeTime = blk.getBool("relativeTime", false);
  cyclic = blk.getBool("cyclic", false);
  preciseTime = blk.getReal("preciseTime", 0);

  int nid = blk.getNameId("action"), i;
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *cb = blk.getBlock(i);
      Action *a = new (midmem) Action;
      a->type = cb->getInt("type", Action::AGA_Nop);
      a->subject = cb->getStr("subject", "");
      a->expression = cb->getStr("expr", "");
      a->useParams = cb->getBool("useParams", false);
      a->p1 = cb->getStr("p1", "");
      actions.push_back(a);
    }
}
void AnimGraphTimedAction::save(DataBlock &blk) const
{
  blk.setInt("performAt", performAt);
  blk.setBool("relativeTime", relativeTime);
  blk.setBool("cyclic", cyclic);
  blk.setReal("preciseTime", preciseTime);

  for (int i = 0; i < actions.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock("action");
    cb->setInt("type", actions[i]->type);
    cb->setStr("subject", actions[i]->subject);
    cb->setStr("expr", actions[i]->expression);
    if (actions[i]->type == Action::AGA_GenIrq)
    {
      cb->setBool("useParams", actions[i]->useParams);
      cb->setStr("p1", actions[i]->p1);
    }
  }
}


//
// util
//
void AsgStatesGraph::loadNamemap(const DataBlock &blk, const char *varname, NameMap &nm)
{
  int nid = blk.getNameId(varname), i;
  for (i = 0; i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == DataBlock::TYPE_STRING)
      nm.addNameId(blk.getStr(i));
}
void AsgStatesGraph::saveNamemap(DataBlock &blk, const char *varname, const NameMap &nm)
{
  for (int i = 0; i < nm.nameCount(); i++)
    blk.addStr(varname, nm.getName(i));
}

IPageSysHelper *page_sys_hlp = NULL;
