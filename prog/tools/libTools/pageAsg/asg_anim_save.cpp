#include <libTools/pageAsg/asg_anim_tree.h>
#include <libTools/pageAsg/asg_anim_ctrl.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

void AnimObjGraphTree::save(DataBlock &blk, NameMap *a2d_map)
{
  int i, j;
  DataBlock *cb;

  if (!preview.isEmpty())
    blk.addNewBlock(&preview, "preview");
  if (!initAnimState.isEmpty())
    blk.addNewBlock(&initAnimState, "initAnimState");
  if (!stateDesc.isEmpty())
    blk.addNewBlock(&stateDesc, "stateDesc");

  // write nodemasks
  if (sharedNodeMaskFName.length())
  {
    DataBlock exb;
    saveNodemasks(exb);
    exb.saveToTextFile(sharedNodeMaskFName);
    blk.setStr("sharedNodeMask", sharedNodeMaskFName);
  }
  else
    saveNodemasks(blk);

  // write all blend node leaves
  if (sharedBnlFName.length())
  {
    DataBlock exb;
    saveAnimBnl(exb, nullptr);
    exb.saveToTextFile(sharedBnlFName);
    blk.setStr("sharedAnimBnl", sharedBnlFName);
  }
  else
    saveAnimBnl(blk, a2d_map);

  // write all controllers
  if (sharedCtrlFName.length())
  {
    DataBlock exb;
    saveAnimCtrl(exb);
    exb.saveToTextFile(sharedCtrlFName);
    blk.setStr("sharedAnimCtrl", sharedCtrlFName);
  }
  else
    saveAnimCtrl(blk);

  blk.setStr("root", root);
}

void AnimObjGraphTree::saveNodemasks(DataBlock &blk) const
{
  for (int i = 0; i < nodemask.size(); i++)
    nodemask[i]->save(*blk.addNewBlock("nodeMask"));
}
void AnimObjGraphTree::saveAnimCtrl(DataBlock &blk) const
{
  DataBlock *cb = blk.addNewBlock("AnimBlendCtrl");
  for (int i = 0; i < ctrlList.size(); i++)
  {
    DataBlock *cbCtrl = cb->addNewBlock(AnimObjCtrl::typeName[ctrlList[i]->type]);
    cbCtrl->setStr("name", ctrlList[i]->name);
    if (!ctrlList[i]->used)
      cbCtrl->setBool("use", ctrlList[i]->used);
    ctrlList[i]->save(*cbCtrl, String(""));
  }
}

void AnimObjGraphTree::compact()
{
  NameMap nm;
  nm = eastl::move(a2dList);

  for (int i = 0; i < nm.nameCount(); i++)
  {
    for (int j = 0; j < bnlList.size(); j++)
    {
      if (stricmp(nm.getStringDataUnsafe(i), bnlList[j]->a2dfname) == 0)
      {
        a2dList.addNameId(nm.getStringDataUnsafe(i));
        // debug("COMPACT: add %s - used in bnl %d", nm.getName(i), j);
        break;
      }
    }
  }
}

void AnimObjGraphTree::saveAnimBnl(DataBlock &blk, NameMap *a2d_map)
{
  if (!a2d_map)
    compact();

  for (int i = 0; i < a2dList.nameCount(); i++)
  {
    DataBlock *cb = blk.addNewBlock("AnimBlendNodeLeaf");
    if (a2d_map)
    {
      int id = a2d_map->getNameId(a2dList.getStringDataUnsafe(i));
      if (id < 0)
      {
        debug("extra anim %s", a2dList.getStringDataUnsafe(i));
        continue;
      }
      cb->setInt("a2d_id", id);
    }
    else
      cb->setStr("a2d", a2dList.getStringDataUnsafe(i));

    for (int j = 0; j < bnlList.size(); j++)
      if (stricmp(a2dList.getStringDataUnsafe(i), bnlList[j]->a2dfname) == 0)
      {
        DataBlock *cbBnl = cb->addNewBlock(AnimObjBnl::typeName[bnlList[j]->type]);
        cbBnl->setStr("name", bnlList[j]->name);
        bnlList[j]->save(*cbBnl);
      }
  }
}

void AnimObjGraphTree::exportAnimCtrl(DataBlock &blk, const AnimObjExportContext &ctx) const
{
  DataBlock *cb = blk.addNewBlock("AnimBlendCtrl");
  for (int i = 0; i < ctrlList.size(); i++)
  {
    DataBlock *cbCtrl = cb->addNewBlock(AnimObjCtrl::typeName[ctrlList[i]->type]);
    cbCtrl->setStr("name", ctrlList[i]->name);
    if (!ctrlList[i]->used)
      cbCtrl->setBool("use", ctrlList[i]->used);
    ctrlList[i]->save(*cbCtrl, String(""));

    for (int k = 0; k < ctx.clones.size(); k++)
      if (ctx.controllers.getNameId(ctrlList[i]->name + ctx.clones[k].suffix) != -1)
      {
        DataBlock *cbCtrl = cb->addNewBlock(AnimObjCtrl::typeName[ctrlList[i]->type]);
        cbCtrl->setStr("name", ctrlList[i]->name + ctx.clones[k].suffix);
        if (!ctrlList[i]->used)
          cbCtrl->setBool("use", ctrlList[i]->used);
        ctrlList[i]->save(*cbCtrl, String(ctx.clones[k].suffix));
      }
  }
}

void AnimObjGraphTree::exportTree(DataBlock &blk, const AnimObjExportContext &ctx) const
{
  int i, j;
  DataBlock *cb;

  if (dontUseAnim)
  {
    blk.setBool("dont_use_anim", true);
    return;
  }

  // write nodemasks
  saveNodemasks(blk);

  // write all blend node leaves
  for (i = 0; i < a2dList.nameCount(); i++)
  {
    //    if (ctx.a2d.getNameId(a2dList.getName(i)) == -1)
    //      continue;

    cb = blk.addNewBlock("AnimBlendNodeLeaf");
    //    if (ctx.a2d_repl.nameCount() == ctx.a2d.nameCount())
    //      cb->setStr("a2d", ctx.a2d_repl.getName(ctx.a2d.getNameId(a2dList.getName(i))));
    //    else

    String a2dFilename(a2dList.getStringDataUnsafe(i));

    cb->setStr("a2d", a2dFilename);

    for (j = 0; j < bnlList.size(); j++)
      if (stricmp(a2dList.getStringDataUnsafe(i), bnlList[j]->a2dfname) == 0)
      {
        if (ctx.bnls.getNameId(bnlList[j]->name) != -1)
        {
          DataBlock *cbBnl = cb->addNewBlock(AnimObjBnl::typeName[bnlList[j]->type]);
          cbBnl->setStr("name", bnlList[j]->name);
          bnlList[j]->save(*cbBnl);
        }

        for (int k = 0; k < ctx.clones.size(); k++)
          if (ctx.bnls.getNameId(bnlList[j]->name + ctx.clones[k].suffix) != -1)
          {
            DataBlock *cbBnl = cb->addNewBlock(AnimObjBnl::typeName[bnlList[j]->type]);
            cbBnl->setStr("name", bnlList[j]->name + ctx.clones[k].suffix);
            bnlList[j]->save(*cbBnl);

            if (strlen(ctx.clones[k].nodemask) > 0)
              cbBnl->setStr("apply_node_mask", ctx.clones[k].nodemask);
          }
      }
  }

  // write all controllers
  exportAnimCtrl(blk, ctx);

  blk.setStr("root", root);
}

bool AnimObjBnl::areKeysReducable() const
{
  int len1 = (int)strlen(keyStart);
  int len2 = (int)strlen(keyEnd);
  if (len1 < 7 || len2 < 5 || len1 != len2 + 2)
    return false;
  if (strncmp(keyStart, keyEnd, len1 - 6) != 0)
    return false;
  if (strcmp(&keyStart[len1 - 6], "_start") != 0 || strcmp(&keyEnd[len2 - 4], "_end") != 0)
    return false;
  return true;
}
void AnimObjBnl::save(DataBlock &blk) const
{
  const char *key;

  if (nodemask.length() > 0)
    blk.setStr("apply_node_mask", nodemask);

  switch (type)
  {
    case AnimObjBnl::TYPE_Continuous:
      if (areKeysReducable())
      {
        String key(keyStart.length(), "%.*s", keyStart.length() - 6, keyStart.str());
        blk.setStr("key", key);
      }
      else
      {
        blk.setStr("key_start", keyStart);
        blk.setStr("key_end", keyEnd);
      }

      blk.setReal("time", duration);
      if (moveDist > 0)
        blk.setReal("moveDist", moveDist);

      if (ownTimer)
        blk.setBool("own_timer", ownTimer);
      else if (strcmp(varname, "::GlobalTime") != 0)
        blk.setStr("timer", varname);

      if (eoaIrq)
        blk.setBool("eoa_irq", eoaIrq);
      if (strcmp(syncTime, keyStart) != 0)
        blk.setStr("sync_time", syncTime);
      if (!rewind)
        blk.setBool("rewind", rewind);
      if (timeScaleParam.length())
        blk.setStr("timeScaleParam", timeScaleParam);
      if (nodeMask.length())
        blk.setStr("apply_node_mask", nodeMask);

      if (labels.size() > 0)
        saveNamedRanges(labels, *blk.addNewBlock("labels"));
      break;

    case AnimObjBnl::TYPE_Single:
      if (areKeysReducable())
      {
        String key(keyStart.length(), "%.*s", keyStart.length() - 6, keyStart.str());
        blk.setStr("key", key);
      }
      else
      {
        blk.setStr("key_start", keyStart);
        blk.setStr("key_end", keyEnd);
      }

      blk.setReal("time", duration);
      if (moveDist > 0)
        blk.setReal("moveDist", moveDist);

      if (ownTimer)
        blk.setBool("own_timer", ownTimer);
      else if (strcmp(varname, "::GlobalTime") != 0)
        blk.setStr("timer", varname);
      if (strcmp(syncTime, keyStart) != 0)
        blk.setStr("sync_time", syncTime);
      if (timeScaleParam.length())
        blk.setStr("timeScaleParam", timeScaleParam);
      if (nodeMask.length())
        blk.setStr("apply_node_mask", nodeMask);

      if (labels.size() > 0)
        saveNamedRanges(labels, *blk.addNewBlock("labels"));
      break;

    case AnimObjBnl::TYPE_Still:
      blk.setStr("key", keyStart);
      if (nodeMask.length())
        blk.setStr("apply_node_mask", nodeMask);
      break;

    case AnimObjBnl::TYPE_Parametric:
      if (areKeysReducable())
      {
        String key(keyStart.length(), "%.*s", keyStart.length() - 6, keyStart.str());
        blk.setStr("key", key);
      }
      else
      {
        blk.setStr("key_start", keyStart);
        blk.setStr("key_end", keyEnd);
      }
      if (nodeMask.length())
        blk.setStr("apply_node_mask", nodeMask);
      blk.setStr("varname", varname);
      blk.setReal("p_start", p0);
      blk.setReal("p_end", p1);
      blk.setReal("mulk", mulK);
      blk.setReal("addk", addK);
      blk.setBool("looping", looping);
      if (updOnVarChange)
        blk.setBool("updOnVarChange", updOnVarChange);

      if (labels.size() > 0)
        saveNamedRanges(labels, *blk.addNewBlock("labels"));
      break;

    default: DAG_FATAL("unknow type %d", type);
  }
  if (disableOriginVel)
    blk.setBool("disable_origin_vel", disableOriginVel);
  if (additive)
    blk.setBool("additive", additive);
  if (foreignAnimation)
    blk.setBool("foreignAnimation", foreignAnimation);

  if (addMove.dirh)
    blk.setReal("addMoveDirH", addMove.dirh);
  if (addMove.dirv)
    blk.setReal("addMoveDirV", addMove.dirv);
  if (addMove.dist)
    blk.setReal("addMoveDist", addMove.dist);

  for (int i = 0; i < irqBlks.blockCount(); i++)
    blk.addNewBlock(irqBlks.getBlock(i), "irq");
}
void AnimObjBnl::saveNamedRanges(const Tab<AnimObjBnl::Label> &labels, DataBlock &blk) const
{
  for (int i = 0; i < labels.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock(labels[i].name);
    cb->setStr("start", labels[i].keyStart);
    cb->setStr("end", labels[i].keyEnd);
    if (labels[i].relStart != 0.0 || labels[i].relEnd != 1.0)
    {
      cb->setReal("relative_start", labels[i].relStart);
      cb->setReal("relative_end", labels[i].relEnd);
    }
  }
}
void AnimObjBnl::saveStripes(const Stripes &s, DataBlock &blk) const
{
  blk.setBool("start", s.start);
  for (int i = 0; i < s.cp.size(); i++)
    blk.addReal("pos", s.cp[i]);
}

void AnimObjCtrlFifo::save(DataBlock &blk, const char *suffix) const
{
  AnimObjCtrl::save(blk, suffix);
  blk.setStr("varname", varname);
}
void AnimObjCtrlLinear::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);
  for (int i = 0; i < list.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock("child");
    cb->setStr("name", list[i].node + suffix);
    cb->setReal("start", list[i].start);
    cb->setReal("end", list[i].end);
  }
}
void AnimObjCtrlLinearPoly::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);
  for (int i = 0; i < list.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock("child");
    cb->setStr("name", list[i].node + suffix);
    cb->setReal("val", list[i].p0);
  }
}
void AnimObjCtrlRandomSwitch::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);

  DataBlock *cb = blk.addNewBlock("weight");
  int i;

  for (i = 0; i < list.size(); i++)
    cb->setReal(list[i].node, list[i].wt);

  cb = blk.addNewBlock("maxrepeat");
  for (i = 0; i < list.size(); i++)
    if (list[i].maxRepeat != 1)
      cb->setInt(list[i].node, list[i].maxRepeat);
}

void AnimObjCtrlParametricSwitch::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);
  blk.setReal("morphTime", morphTime);
  if (!list.size())
  {
    blk.addNewBlock(props.getBlockByNameEx("nodes"), "nodes");
    return;
  }

  DataBlock *cb = blk.addNewBlock("nodes");
  int i;

  for (i = 0; i < list.size(); i++)
  {
    DataBlock *nb = cb->addNewBlock(String(128, "range_%d", i));
    nb->setStr("name", list[i].node + suffix);
    nb->setReal("rangeFrom", list[i].r0);
    nb->setReal("rangeTo", list[i].r1);
  }
}

void AnimObjCtrlHub::save(DataBlock &blk, const char *suffix) const
{
  for (int i = 0; i < list.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock("child");
    cb->setStr("name", list[i].node + suffix);
    cb->setReal("weight", list[i].w);
    cb->setBool("enabled", list[i].enabled);
  }
  blk.setBool("const", _const);
}
void AnimObjCtrlDirectSync::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);
  for (int i = 0; i < list.size(); i++)
  {
    DataBlock *cb = blk.addNewBlock("child");
    cb->setStr("name", list[i].node + suffix);
    cb->setStr("linked_to", list[i].linkedTo);
    if (list[i].dc != 0)
      cb->setReal("offset", list[i].dc);
    if (list[i].scale != 1)
      cb->setReal("scale", list[i].scale);
    if (list[i].clamp0 != 0 || list[i].clamp1 != 1)
    {
      cb->setReal("clamp0", list[i].clamp0);
      cb->setReal("clamp1", list[i].clamp1);
    }
  }
}
void AnimObjCtrlBlender::save(DataBlock &blk, const char *suffix) const
{
  if (stricmp(varname, String(64, "%s::var", name.str())) != 0)
    blk.setStr("varname", varname);
  blk.setStr("node1", node1 + suffix);
  blk.setStr("node2", node2 + suffix);
  blk.setReal("duration", duration);
  blk.setReal("morph", morph);
}
void AnimObjCtrlBIS::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("varname", varname);
  blk.setInt("maskAnd", maskAnd);
  blk.setInt("maskEq", maskEq);
  blk.setStr("fifo", fifo);
  blk.setStr("node1", node1 + suffix);
  blk.setReal("morph1", morph1);
  blk.setStr("node2", node2 + suffix);
  blk.setReal("morph2", morph2);
}
void AnimObjCtrlAlignNode::save(DataBlock &blk, const char *suffix) const
{
  blk.setStr("targetNode", target);
  blk.setStr("srcNode", src);
  blk.setPoint3("rot_euler", rot_euler);
  blk.setPoint3("scale", scale);
  blk.setBool("useLocal", useLocal);
}
void AnimObjCtrlRotateNode::save(DataBlock &blk, const char *suffix) const
{
  AnimObjCtrl::save(blk, suffix);
  blk.removeParam("targetNode");
  for (int i = 0; i < target.size(); i++)
    blk.addStr("targetNode", target[i].name);
  if (defWt != 1.0f)
    blk.setReal("targetNodeWt", defWt);
  for (int i = 0; i < target.size(); i++)
    if (target[i].wt != defWt)
      blk.setReal(String(0, "targetNodeWt%d", i + 1), target[i].wt);

  if (kMul != 1.0f)
    blk.setReal("kMul", kMul);
  if (kAdd != 0.0f)
    blk.setReal("kAdd", kAdd);
  blk.setStr("param", paramName);
  if (!axisCourse.empty())
  {
    blk.setStr("axis_course", axisCourse);
    blk.setReal("kCourseAdd", kCourseAdd);
    blk.setPoint3("rotAxis", rotAxis);
  }
  blk.setPoint3("dirAxis", dirAxis);
  if (!leftTm)
    blk.setBool("leftTm", leftTm);
}

void AnimObjNodeMask::save(DataBlock &blk) const
{
  blk.setStr("name", name);
  for (int i = 0; i < nm.nameCount(); i++)
    blk.addStr("node", nm.getName(i));
}
