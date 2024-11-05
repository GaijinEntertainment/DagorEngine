// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/pageAsg/asg_anim_tree.h>
#include <libTools/pageAsg/asg_anim_ctrl.h>
#include <debug/dag_debug.h>

//
// AnimObjGraphTree
//
void AnimObjGraphTree::clear()
{
  int i;
  a2dList.clear();
  clear_all_ptr_items_and_shrink(nodemask);
  clear_all_ptr_items_and_shrink(bnlList);
  clear_all_ptr_items_and_shrink(ctrlList);

  clear_and_shrink(root);
  clear_and_shrink(sharedNodeMaskFName);
  clear_and_shrink(sharedCtrlFName);
  clear_and_shrink(sharedBnlFName);
}
void AnimObjGraphTree::fillDefaults()
{
  AnimObjCtrlFifo3 *bn = new (midmem) AnimObjCtrlFifo3;
  bn->name = "fifo3";
  bn->varname = "queue";

  ctrlList.push_back(bn);
  root = "fifo3";
  dontUseAnim = false;
  dontUseStates = false;
}

bool AnimObjGraphTree::animNodeExists(const char *name)
{
  int i;
  for (i = 0; i < bnlList.size(); i++)
    if (stricmp(bnlList[i]->name, name) == 0)
      return true;
  for (i = 0; i < ctrlList.size(); i++)
    if (stricmp(ctrlList[i]->name, name) == 0 && ctrlList[i]->type != AnimObjCtrl::TYPE_Stub)
      return true;
  return false;
}
AnimObjBnl *AnimObjGraphTree::findBnl(const char *name)
{
  for (int i = 0; i < bnlList.size(); i++)
    if (stricmp(bnlList[i]->name, name) == 0)
      return bnlList[i];
  return NULL;
}
AnimObjCtrl *AnimObjGraphTree::findCtrl(const char *name)
{
  for (int i = 0; i < ctrlList.size(); i++)
    if (stricmp(ctrlList[i]->name, name) == 0)
      return ctrlList[i];
  return NULL;
}


//
// AnimObjBnl
//
void AnimObjBnl::fillDefaults()
{
  type = TYPE_Continuous;
  eoaIrq = true;
  ownTimer = true;
  duration = 1.0;
  moveDist = 0;
  disableOriginVel = false;
  p0 = 0;
  p1 = 1;
  mulK = 1.0;
  addK = 0.0;
  additive = false;
  rewind = true;
  addMove.dirh = 0;
  addMove.dirv = 0;
  addMove.dist = 0;
  updOnVarChange = false;
  foreignAnimation = false;
  looping = false;
}

//
// AnimObjCtrl
//
void AnimObjCtrl::fillDefaults() {}


//
// Controllers
//
void AnimObjCtrlLinear::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  for (int i = 0; i < list.size(); i++)
  {
    bnl = tree.findBnl(list[i].node);
    if (bnl)
    {
      bnls.addNameId(bnl->name + suffix);
      a2d.addNameId(bnl->a2dfname);
    }
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlLinear::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 150;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
    case 1:
      info.w = 60;
      info.name = "Start";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 2:
      info.w = 60;
      info.name = "End";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlLinear::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlLinear::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlLinear::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = String(64, " %f", a.start); break;
    case 2: text = String(64, " %f", a.end); break;
  }
}

void AnimObjCtrlLinear::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
  }
}

void AnimObjCtrlLinear::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: val = a.start; break;
    case 2: val = a.end; break;
  }
}

void AnimObjCtrlLinear::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: a.start = val; break;
    case 2: a.end = val; break;
  }
}

void AnimObjCtrlLinearPoly::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  for (int i = 0; i < list.size(); i++)
  {
    bnl = tree.findBnl(list[i].node);
    if (bnl)
    {
      bnls.addNameId(bnl->name + suffix);
      a2d.addNameId(bnl->a2dfname);
    }
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlLinearPoly::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 200;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::CTRL_PARAM;
      break;
    case 1:
      info.w = 60;
      info.name = "p0";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlLinearPoly::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlLinearPoly::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlLinearPoly::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = String(64, " %f", a.p0); break;
  }
}

void AnimObjCtrlLinearPoly::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
  }
}

void AnimObjCtrlLinearPoly::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: val = a.p0; break;
  }
}

void AnimObjCtrlLinearPoly::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: a.p0 = val; break;
  }
}

void AnimObjCtrlRandomSwitch::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  for (int i = 0; i < list.size(); i++)
  {
    bnl = tree.findBnl(list[i].node);
    if (bnl)
    {
      bnls.addNameId(bnl->name + suffix);
      a2d.addNameId(bnl->a2dfname);
    }
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlRandomSwitch::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 150;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::BNL_PARAM;
      break;
    case 1:
      info.w = 60;
      info.name = "wt";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 2:
      info.w = 60;
      info.name = "Max repeat";
      info.type = AnimObjCtrl::RecParamInfo::INT_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlRandomSwitch::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlRandomSwitch::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlRandomSwitch::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = String(64, " %f", a.wt); break;
    case 2: text = String(64, " %i", a.maxRepeat); break;
  }
}

void AnimObjCtrlRandomSwitch::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
  }
}

void AnimObjCtrlRandomSwitch::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: val = a.wt; break;
    case 2: val = a.maxRepeat; break;
  }
}

void AnimObjCtrlRandomSwitch::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: a.wt = val; break;
    case 2: a.maxRepeat = val; break;
  }
}

/***********************************************************************/
void AnimObjCtrlParametricSwitch::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  for (int i = 0; i < list.size(); i++)
  {
    bnl = tree.findBnl(list[i].node);
    if (bnl)
    {
      bnls.addNameId(bnl->name + suffix);
      a2d.addNameId(bnl->a2dfname);
    }
  }

  if (const char *enum_cls = props.getBlockByNameEx("nodes")->getStr("enum_gen", NULL))
  {
    const DataBlock *init_st = tree.initAnimState.getBlockByNameEx("enum")->getBlockByName(enum_cls);
    const char *name_template = props.getBlockByNameEx("nodes")->getStr("name_template", NULL);
    if (init_st && name_template)
      for (int j = 0, je = init_st->blockCount(); j < je; j++)
      {
        const DataBlock *enumBlock = init_st->getBlock(j);
        const char *enum_nm = enumBlock->getBlockName();
        String nm;

        if (strcmp(name_template, "*") == 0)
        {
          nm = enumBlock->getStr(name, "");
          if (nm.empty()) // try find in parents
          {
            // if recursionCount will become more than then blockCount, so we are in deadlock
            int recursionCount = 0;
            String parentEnumBlockName(enumBlock->getStr("_use", ""));
            while (!parentEnumBlockName.empty() && recursionCount < je)
            {
              recursionCount++;
              if (const DataBlock *parentBlock = init_st->getBlockByName(parentEnumBlockName.c_str()))
              {
                nm = parentBlock->getStr(name, "");
                parentEnumBlockName = nm.empty() ? parentBlock->getStr("_use", "") : "";
              }
              else
                parentEnumBlockName.clear(); // break cycle
            }

            if (recursionCount == je || nm.empty())
              continue;
          }
        }
        else
        {
          nm = name_template;
          nm.replace("$1", enum_nm);
        }
        bnl = tree.findBnl(nm);
        debug("addNeededBnl %s", nm);
        if (bnl)
        {
          bnls.addNameId(bnl->name + suffix);
          a2d.addNameId(bnl->a2dfname);
        }
      }
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlParametricSwitch::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 150;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::BNL_PARAM;
      break;
    case 1:
      info.w = 60;
      info.name = "range from";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 2:
      info.w = 60;
      info.name = "range to";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlParametricSwitch::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlParametricSwitch::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlParametricSwitch::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = String(64, " %f", a.r0); break;
    case 2: text = String(64, " %f", a.r1); break;
  }
}

void AnimObjCtrlParametricSwitch::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
  }
}

void AnimObjCtrlParametricSwitch::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: val = a.r0; break;
    case 2: val = a.r1; break;
  }
}

void AnimObjCtrlParametricSwitch::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: a.r0 = val; break;
    case 2: a.r1 = val; break;
  }
}
/***************************************************************************************************/

void AnimObjCtrlHub::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  for (int i = 0; i < list.size(); i++)
  {
    bnl = tree.findBnl(list[i].node);
    if (bnl)
    {
      bnls.addNameId(bnl->name + suffix);
      a2d.addNameId(bnl->a2dfname);
    }
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlHub::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 150;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::CTRL_PARAM;
      break;
    case 1:
      info.w = 60;
      info.name = "w";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 2:
      info.w = 60;
      info.name = "enabled";
      info.type = AnimObjCtrl::RecParamInfo::BOOL_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlHub::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlHub::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlHub::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = String(64, " %f", a.w); break;
    case 2: text = String(64, " %i", (int)a.enabled); break;
  }
}

void AnimObjCtrlHub::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
  }
}

void AnimObjCtrlHub::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: val = a.w; break;
    case 2: val = (int)a.enabled; break;
  }
}

void AnimObjCtrlHub::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 1: a.w = val; break;
    case 2: a.enabled = val; break;
  }
}

void AnimObjCtrlBlender::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  bnl = tree.findBnl(node1);
  if (bnl)
  {
    bnls.addNameId(bnl->name + suffix);
    a2d.addNameId(bnl->a2dfname);
  }
  bnl = tree.findBnl(node2);
  if (bnl)
  {
    bnls.addNameId(bnl->name + suffix);
    a2d.addNameId(bnl->a2dfname);
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlBlender::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 200;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlBlender::insertRec(int i) {}

void AnimObjCtrlBlender::eraseRec(int i) {}

void AnimObjCtrlBlender::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (param_no == -1 || param_no > 0 || rec_no > 1)
    return;
  text = rec_no ? node2 : node1;
}

void AnimObjCtrlBlender::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (param_no == -1 || param_no > 0 || rec_no > 1)
    return;
  String &node = rec_no ? node2 : node1;
  node = text;
}

void AnimObjCtrlBlender::getParamValueReal(int param_no, int rec_no, real &val) const {}

void AnimObjCtrlBlender::setParamValueReal(int param_no, int rec_no, const real &val) {}

void AnimObjCtrlBIS::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *suffix) const
{
  AnimObjBnl *bnl;

  bnl = tree.findBnl(node1);
  if (bnl)
  {
    bnls.addNameId(bnl->name + suffix);
    a2d.addNameId(bnl->a2dfname);
  }
  bnl = tree.findBnl(node2);
  if (bnl)
  {
    bnls.addNameId(bnl->name + suffix);
    a2d.addNameId(bnl->a2dfname);
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlBIS::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 200;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlBIS::insertRec(int i) {}

void AnimObjCtrlBIS::eraseRec(int i) {}

void AnimObjCtrlBIS::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || param_no > 0 || rec_no > 2)
    return;
  switch (rec_no)
  {
    case 0: text = node1; break;
    case 1: text = node2; break;
    case 2: text = fifo;
  }
}

void AnimObjCtrlBIS::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (param_no == -1 || param_no > 0 || rec_no > 2)
    return;
  switch (rec_no)
  {
    case 0: node1 = text; break;
    case 1: node2 = text; break;
    case 2: fifo = text; break;
  }
}

void AnimObjCtrlBIS::getParamValueReal(int param_no, int rec_no, real &val) const {}

void AnimObjCtrlBIS::setParamValueReal(int param_no, int rec_no, const real &val) {}


// static names
const char *AnimObjBnl::typeName[] = {
  "continuous",
  "single",
  "still",
  "parametric",
};
const char *AnimObjCtrl::typeName[] = {
  "null",
  "fifo",
  "fifo3",
  "linear",
  "randomSwitch",
  "paramSwitch",
  "paramSwitchS",
  "hub",
  "directSync",
  "planar",
  "blender",
  "BIS",
  "linearPoly",
  "stub",
  "alignNode",
  "rotateNode",
  "scaleNode",
  "moveNode",
  "paramsCtrl",
  "condHide",
  "animateNode",
};

AnimObjCtrl::RecParamInfo AnimObjCtrlDirectSync::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 80;
      info.name = "Name";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
    case 1:
      info.w = 80;
      info.name = "Linked to name";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
    case 2:
      info.w = 60;
      info.name = "dc";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 3:
      info.w = 60;
      info.name = "scale";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 4:
      info.w = 60;
      info.name = "clamp0";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
    case 5:
      info.w = 60;
      info.name = "clamp1";
      info.type = AnimObjCtrl::RecParamInfo::REAL_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlDirectSync::insertRec(int i)
{
  Rec a;
  if (i == -1 || i >= list.size())
    list.push_back(a);
  else
    insert_items(list, i, 1, &a);
}

void AnimObjCtrlDirectSync::eraseRec(int i) { erase_items(list, i, 1); }

void AnimObjCtrlDirectSync::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: text = a.node; break;
    case 1: text = a.linkedTo; break;
    case 2: text = String(64, " %f", a.dc); break;
    case 3: text = String(64, " %f", a.scale); break;
    case 4: text = String(64, " %f", a.clamp0); break;
    case 5: text = String(64, " %f", a.clamp1); break;
  }
}

void AnimObjCtrlDirectSync::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 0: a.node = text; break;
    case 1: a.linkedTo = text; break;
  }
}

void AnimObjCtrlDirectSync::getParamValueReal(int param_no, int rec_no, real &val) const
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  const Rec &a = list[rec_no];
  switch (param_no)
  {
    case 2: val = a.dc; break;
    case 3: val = a.scale; break;
    case 4: val = a.clamp0; break;
    case 5: val = a.clamp1; break;
  }
}

void AnimObjCtrlDirectSync::setParamValueReal(int param_no, int rec_no, const real &val)
{
  if (rec_no == -1 || rec_no >= list.size())
    return;
  Rec &a = list[rec_no];
  switch (param_no)
  {
    case 2: a.dc = val; break;
    case 3: a.scale = val; break;
    case 4: a.clamp0 = val; break;
    case 5: a.clamp1 = val; break;
  }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlRotateNode::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 80;
      info.name = "Targets";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlRotateNode::insertRec(int i)
{
  NodeDesc a;
  if (i == -1 || i >= target.size())
    target.push_back(a);
  else
    insert_items(target, i, 1, &a);
}

void AnimObjCtrlRotateNode::eraseRec(int i) { erase_items(target, i, 1); }

void AnimObjCtrlRotateNode::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (param_no == -1 || param_no >= target.size())
    return;
  const String &a = target[rec_no].name;
  switch (param_no)
  {
    case 0: text = a; break;
  }
}

void AnimObjCtrlRotateNode::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (rec_no == -1 || rec_no >= target.size())
    return;
  String &a = target[rec_no].name;
  switch (param_no)
  {
    case 0: a = text; break;
  }
}

void AnimObjCtrlRotateNode::getParamValueReal(int param_no, int rec_no, real &val) const {}

void AnimObjCtrlRotateNode::setParamValueReal(int param_no, int rec_no, const real &val) {}

void AnimObjAnimateNode::addNeededBnls(NameMap &bnls, NameMap &a2d, AnimObjGraphTree &tree, const char *) const
{
  const char *def_bnl = props.getStr("bnl", NULL);
  int nid = props.getNameId("anim");
  for (int i = 0; i < props.blockCount(); i++)
    if (props.getBlock(i)->getBlockNameId() == nid)
      if (const char *bnl_nm = props.getBlock(i)->getStr("bnl", def_bnl))
        if (AnimObjBnl *bnl = tree.findBnl(bnl_nm))
        {
          bnls.addNameId(bnl->name);
          a2d.addNameId(bnl->a2dfname);
        }
}

AnimObjCtrl::RecParamInfo AnimObjCtrlAlignNode::getRecParamInfo(int i) const
{
  AnimObjCtrl::RecParamInfo info;
  switch (i)
  {
    case 0:
      info.w = 150;
      info.name = "Scr";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
    case 1:
      info.w = 150;
      info.name = "Target";
      info.type = AnimObjCtrl::RecParamInfo::STR_PARAM;
      break;
  }
  return info;
}

void AnimObjCtrlAlignNode::insertRec(int i) {}

void AnimObjCtrlAlignNode::eraseRec(int i) {}

void AnimObjCtrlAlignNode::getParamValueText(int param_no, int rec_no, String &text) const
{
  if (param_no == -1 || param_no > 1 || rec_no > 0)
    return;
  text = param_no ? target : src;
}

void AnimObjCtrlAlignNode::setParamValueText(int param_no, int rec_no, const char *text)
{
  if (param_no == -1 || param_no > 1 || rec_no > 0)
    return;
  String &node = param_no ? target : src;
  node = text;
}

void AnimObjCtrlAlignNode::getParamValueReal(int param_no, int rec_no, real &val) const {}

void AnimObjCtrlAlignNode::setParamValueReal(int param_no, int rec_no, const real &val) {}

AnimObjCtrl *create_controller(int type)
{
  switch (type)
  {
    case AnimObjCtrl::TYPE_Null:
    case AnimObjCtrl::TYPE_Stub: return new (uimem) AnimObjCtrlNull();
    case AnimObjCtrl::TYPE_Fifo: return new (uimem) AnimObjCtrlFifo();
    case AnimObjCtrl::TYPE_Fifo3: return new (uimem) AnimObjCtrlFifo3();
    case AnimObjCtrl::TYPE_Planar: DEBUG_CTX("not implenmented"); return NULL;
    case AnimObjCtrl::TYPE_Blender: return new (uimem) AnimObjCtrlBlender();
    case AnimObjCtrl::TYPE_BIS: return new (uimem) AnimObjCtrlBIS();
    case AnimObjCtrl::TYPE_AlignNode: return new (uimem) AnimObjCtrlAlignNode();
    case AnimObjCtrl::TYPE_RotateNode: return new (uimem) AnimObjCtrlRotateNode();
    case AnimObjCtrl::TYPE_MoveNode: return new (uimem) AnimObjCtrlMoveNode();
    case AnimObjCtrl::TYPE_ParamsChange: return new (uimem) AnimObjCtrlParamsChange();
    case AnimObjCtrl::TYPE_CondHideNode: return new (uimem) AnimObjCtrlHideNode();
    case AnimObjCtrl::TYPE_AnimateNode: return new (uimem) AnimObjAnimateNode();
    case AnimObjCtrl::TYPE_Linear: return new (uimem) AnimObjCtrlLinear();
    case AnimObjCtrl::TYPE_LinearPoly: return new (uimem) AnimObjCtrlLinearPoly();
    case AnimObjCtrl::TYPE_RandomSwitch: return new (uimem) AnimObjCtrlRandomSwitch();
    case AnimObjCtrl::TYPE_ParamSwitch: return new (uimem) AnimObjCtrlParametricSwitch();
    case AnimObjCtrl::TYPE_ParamSwitchS: return new (uimem) AnimObjCtrlParametricSwitchS();
    case AnimObjCtrl::TYPE_Hub: return new (uimem) AnimObjCtrlHub();
    case AnimObjCtrl::TYPE_DirectSync: return new (uimem) AnimObjCtrlDirectSync();
  }
  DEBUG_CTX("not implenmented");
  return NULL;
}
