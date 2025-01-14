// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <sqplus.h>
#include <propPanel/control/container.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <libTools/util/blkUtil.h>

#include <scriptPanelWrapper/spw_main.h>
#include <scriptPanelWrapper/spw_param.h>
#include <scriptPanelWrapper/spw_ext.h>

#include <debug/dag_debug.h>


//=============================================================================

ScriptExtContainer::ScriptExtContainer(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption) :
  ScriptPanelContainer(wrapper, parent, name, caption),
  mExtItemPids(midmem),
  itemsCountMin(0),
  itemsCountMax(16),
  removeConfirm(false),
  isRenameble(false)
{}


void ScriptExtContainer::createControl()
{
  setDefValues(mParam);

  if (mPanel && !mPanel->getById(mPid))
  {
    for (int i = 0; i < itemsCountMin; ++i)
    {
      appendExBlock();
      int next_pid = mParent ? mParent->getNextVisiblePid(mPid) : -1;
      int _pid = mExtItemPids.size() ? mExtItemPids.back() : -1;
      if (_pid != -1 && next_pid != -1)
        mPanel->moveById(_pid, next_pid);
    }

    mPanel->createExtensible(mPid);
    mPanel->setText(mPid, mCaption);
  }
  updateMenuFlags();
}


void ScriptExtContainer::getValueFromScript(SquirrelObject param)
{
  SquirrelObject itemsCount = param.GetValue("itemsCount");
  itemsCountMin = 0;
  itemsCountMax = 16;
  if (!itemsCount.IsNull() && itemsCount.GetType() == OT_ARRAY && itemsCount.Len() == 2)
  {
    itemsCountMin = itemsCount.GetInt(SQInteger(0));
    itemsCountMax = itemsCount.GetInt(1);
  }

  if (param.Exists("remove_confirm"))
    removeConfirm = param.GetBool("remove_confirm");
}


void ScriptExtContainer::appendExBlock()
{
  int &new_pid = mPanelWrapper->getFreePid();
  PropPanel::ContainerPropertyControl *ext_container = NULL;
  if (mPanel && mVisible)
    ext_container = mPanel->createExtensible(++new_pid);
  else
    ++new_pid;
  mExtItemPids.push_back(new_pid);
  scriptControlFactory(ext_container, ++new_pid, mParam);
  if (ext_container)
    ext_container->setEnabled(mEnabled);
}


void ScriptExtContainer::onChange(int pid, PropPanel::ContainerPropertyControl &panel) { ScriptPanelContainer::onChange(pid, panel); }


void ScriptExtContainer::onClick(int pid, PropPanel::ContainerPropertyControl &panel)
{
  if (mPid == pid || (searchPidIndex(pid) != -1))
  {
    panel.setPostEvent(pid);
    return;
  }

  ScriptPanelContainer::onClick(pid, panel);
}


void ScriptExtContainer::onPostEvent(int pid, PropPanel::ContainerPropertyControl &panel)
{
  if (mPid == pid && mExtItemPids.size() < itemsCountMax)
  {
    insertExBlock(mPid);
    updateMenuFlags();
    return;
  }

  int ind = -1;
  if ((ind = searchPidIndex(pid)) != -1)
  {
    switch (panel.getInt(pid))
    {
      case PropPanel::EXT_BUTTON_REMOVE: removeExBlock(pid, ind); break;

      case PropPanel::EXT_BUTTON_INSERT: insertExBlock(pid, ind); break;

      case PropPanel::EXT_BUTTON_UP: moveBlockExUp(pid, ind); break;

      case PropPanel::EXT_BUTTON_DOWN: moveBlockExDown(pid, ind); break;
    }
    updateMenuFlags();
    return;
  }

  ScriptPanelContainer::onPostEvent(pid, panel);
}


int ScriptExtContainer::searchPidIndex(int pid)
{
  for (int i = 0; i < mExtItemPids.size(); ++i)
    if (mExtItemPids[i] == pid)
      return i;
  return -1;
}


int ScriptExtContainer::getFirstPid()
{
  if (mExtItemPids.size())
    return mExtItemPids[0];
  else
    return mPid;
}


bool ScriptExtContainer::onMessage(int pid, int msg, void *arg)
{
  if (pid == 0 || pid == mPid)
  {
    if (msg == MSG_APPEND_ITEM)
    {
      insertExBlock(mPid);
      updateMenuFlags();
      if (mExtItemPids.size() >= itemsCountMax)
        return false;
      const char *text = (const char *)arg;
      mItems.back()->onMessage(0, MSG_SET_TEXT, arg);
      return true;
    }
    return false;
  }
  return false;
}


bool ScriptExtContainer::insertExBlock(int pid, int ind)
{
  if (mExtItemPids.size() >= itemsCountMax)
    return false;

  setDefValues(mParam);

  int &new_pid = mPanelWrapper->getFreePid();
  PropPanel::ContainerPropertyControl *ext_container = NULL;
  if (mPanel && mVisible)
    ext_container = mPanel->createExtensible(++new_pid);
  else
    ++new_pid;
  if (pid == mPid)
    mExtItemPids.push_back(new_pid);
  else
    insert_items(mExtItemPids, ind, 1, &new_pid);

  if (mPanel)
  {
    mPanel->moveById(new_pid, pid);
    if (mPanel->getById(mPid) && mExtItemPids.size() == itemsCountMax)
      mPanel->removeById(mPid);
  }
  scriptControlFactory(ext_container, ++new_pid, mParam);

  if (ext_container)
    ext_container->setEnabled(mEnabled);

  if (pid != mPid && mItems.size() > 1)
  {
    ScriptPanelParam *sparam = mItems.back();
    insert_items(mItems, ind, 1, &sparam);
    erase_items(mItems, mItems.size() - 1, 1);
  }

  return true;
}


void ScriptExtContainer::removeExBlock(int pid, int ind)
{
  if (mExtItemPids.size() <= itemsCountMin)
    return;

  if (removeConfirm && wingw::message_box(wingw::MBS_OKCANCEL | wingw::MBS_QUEST, "Remove", "Are you sure?") != wingw::MB_ID_OK)
    return;

  if (!mPanel->getById(mPid) && mExtItemPids.size() < itemsCountMax + 1)
  {
    int last_pid = mExtItemPids.back();
    mPanel->createExtensible(mPid);
    mPanel->setText(mPid, mCaption);
    mPanel->moveById(mPid, last_pid, true);
  }

  erase_items(mExtItemPids, ind, 1);
  for (int i = 0; i < mItems.size(); ++i)
    if (mItems[i]->getPid() == getItemPid(pid))
    {
      delete mItems[i];
      erase_items(mItems, i, 1);
      break;
    }
  mPanel->removeById(pid);
}


void ScriptExtContainer::moveBlockExUp(int pid, int ind)
{
  if (ind == 0)
    return;

  int prior_pid = mExtItemPids[ind - 1];
  mPanel->moveById(pid, prior_pid);
  mExtItemPids[ind - 1] = pid;
  mExtItemPids[ind] = prior_pid;

  if (mItems.size() == mExtItemPids.size())
  {
    ScriptPanelParam *sparam = mItems[ind - 1];
    mItems[ind - 1] = mItems[ind];
    mItems[ind] = sparam;
  }
}


void ScriptExtContainer::moveBlockExDown(int pid, int ind)
{
  if (ind == mExtItemPids.size() - 1)
    return;

  int next_pid = mExtItemPids[ind + 1];
  mPanel->moveById(next_pid, pid);
  mExtItemPids[ind + 1] = pid;
  mExtItemPids[ind] = next_pid;

  if (mItems.size() == mExtItemPids.size())
  {
    ScriptPanelParam *sparam = mItems[ind + 1];
    mItems[ind + 1] = mItems[ind];
    mItems[ind] = sparam;
  }
}


void ScriptExtContainer::clear()
{
  if (mPanel)
    for (int i = 0; i < mExtItemPids.size(); ++i)
      mPanel->removeById(mExtItemPids[i]);
  clear_and_shrink(mExtItemPids);
  ScriptPanelContainer::clear();
}


bool ScriptExtContainer::scriptExtFactory(PropPanel::ContainerPropertyControl *panel, int &pid, SquirrelObject param) { return false; }


void ScriptExtContainer::updateParams() {}

void ScriptExtContainer::setDefValues(SquirrelObject so)
{
  SquirrelObject def = so.GetValue("def");
  if (!so.IsNull())
    so.SetValue("value", def);

  SquirrelObject controls = so.GetValue("controls");
  if (!controls.IsNull() && controls.GetType() == OT_ARRAY && controls.BeginIteration())
  {
    SquirrelObject control, key;

    while (controls.Next(key, control))
      if (control.GetType() == OT_TABLE)
        setDefValues(control);

    controls.EndIteration();
  }
}


void ScriptExtContainer::updateMenuFlags()
{
  if (!mPanel)
    return;

  for (int i = 0; i < mExtItemPids.size(); ++i)
  {
    int flags = 0;
    if (mExtItemPids.size() < itemsCountMax)
      flags |= (1 << PropPanel::EXT_BUTTON_INSERT);
    if (mExtItemPids.size() > itemsCountMin)
      flags |= (1 << PropPanel::EXT_BUTTON_REMOVE);
    if (i > 0)
      flags |= (1 << PropPanel::EXT_BUTTON_UP);
    if (i + 1 < mExtItemPids.size())
      flags |= (1 << PropPanel::EXT_BUTTON_DOWN);

    mPanel->setInt(mExtItemPids[i], flags);
  }
}


void ScriptExtContainer::setVisible(bool visible)
{
  ScriptPanelParam::setVisible(visible);
  if (mVisible)
    load(invisibleBlk);
}


bool ScriptExtContainer::getParamVisible(SquirrelObject param) { return mVisible; }


void ScriptExtContainer::setEnabled(bool enabled)
{
  ScriptPanelParam::setEnabled(enabled);
  for (int i = 0; i < mExtItemPids.size(); ++i)
    mPanel->setEnabledById(mExtItemPids[i], mEnabled);
}


// datablock routines

void ScriptExtContainer::save(DataBlock &blk)
{
  saveExt(blk);
  invisibleBlk.reset();
  saveExt(invisibleBlk);
}

void ScriptExtContainer::saveExt(DataBlock &blk)
{
  blk.removeBlock(paramName);
  blk.removeParam(paramName);

  DataBlock *cur_block = &blk;
  if (isRenameble)
    cur_block = blk.addNewBlock(paramName);

  for (int i = 0; i < mItems.size(); ++i)
  {
    DataBlock tmp_blk;
    mItems[i]->save(tmp_blk);

    if (tmp_blk.paramCount())
      blk_util::copyBlkParam(tmp_blk, 0, *cur_block);
    else if (tmp_blk.blockCount())
      cur_block->addNewBlock(tmp_blk.getBlock(0), isRenameble ? NULL : paramName.str());
  }
}


void ScriptExtContainer::load(const DataBlock &blk)
{
  int name_id = blk.getNameId(paramName);
  if (name_id == -1)
    return;

  clear();

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock *item_blk = blk.getBlock(i);
    if (item_blk->getBlockNameId() != name_id)
      continue;

    DataBlock tmp_blk;
    if (!isRenameble)
    {
      tmp_blk.addNewBlock(item_blk, paramName);
      if (insertExBlock(mPid) && mItems.size())
        mItems.back()->load(tmp_blk);
    }
    else
      for (int j = 0; j < item_blk->blockCount(); ++j)
      {
        const DataBlock *sub_item_blk = item_blk->getBlock(j);
        tmp_blk.reset();
        tmp_blk.addNewBlock(sub_item_blk);
        if (insertExBlock(mPid) && mItems.size())
        {
          int ind = mItems.size() - 1;
          mItems[ind]->setParamName(sub_item_blk->getBlockName());
          mItems[ind]->load(tmp_blk);
        }
      }
  }

  for (int i = 0; i < blk.paramCount(); ++i)
  {
    if (blk.getParamNameId(i) != name_id)
      continue;

    DataBlock tmp_blk;
    blk_util::copyBlkParam(blk, i, tmp_blk);
    if (insertExBlock(mPid) && mItems.size())
      mItems.back()->load(tmp_blk);
  }
  updateMenuFlags();
}


//-----------------------------------------------------------------------------


ScriptExtGroup::ScriptExtGroup(CSQPanelWrapper *wrapper, ScriptPanelContainer *parent, const char *name, const char *caption) :
  ScriptExtContainer(wrapper, parent, name, caption)
{
  removeConfirm = true;
}


void ScriptExtGroup::getValueFromScript(SquirrelObject param)
{
  ScriptExtContainer::getValueFromScript(param);
  if (param.Exists("is_renameble"))
    isRenameble = param.GetBool("is_renameble");
}


void ScriptExtGroup::appendExBlock()
{
  int &new_pid = mPanelWrapper->getFreePid();
  mExtItemPids.push_back(new_pid + 1);
  scriptControlFactory(mPanel, ++new_pid, mParam);
}


bool ScriptExtGroup::insertExBlock(int pid, int ind)
{
  if (mExtItemPids.size() >= itemsCountMax)
    return false;

  setDefValues(mParam);

  int &new_pid = mPanelWrapper->getFreePid();
  int new_pid_val = new_pid + 1;
  if (pid == mPid)
    mExtItemPids.push_back(new_pid_val);
  else
    insert_items(mExtItemPids, ind, 1, &new_pid_val);
  scriptControlFactory(mPanel, ++new_pid, mParam);

  if (pid != mPid && mItems.size() > 1)
  {
    ScriptPanelParam *sparam = mItems.back();
    insert_items(mItems, ind, 1, &sparam);
    erase_items(mItems, mItems.size() - 1, 1);
  }

  if (mPanel)
  {
    mPanel->moveById(new_pid_val, pid);
    if (mPanel->getById(mPid) && mExtItemPids.size() == itemsCountMax)
      mPanel->removeById(mPid);
  }

  return true;
}
