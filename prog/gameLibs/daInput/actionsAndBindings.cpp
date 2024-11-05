// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "actionData.h"
#include <drv/hid/dag_hiVrInput.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <debug/dag_assert.h>
#include <EASTL/optional.h>
#include <osApiWrappers/dag_miscApi.h>
#include <drv/hid/dag_hiXInputMappings.h>


// We can't have dynamic action sets in OpenXr, so we make only one "everything" action set
static const int VR_DUMMY_ACTN_SET = 1;


static void patch_thumbstick_legacy_id(dainput::SingleButtonId &obj)
{
  using namespace HumanInput;
  if (obj.devId == dainput::DEV_gamepad)
  {
    if (obj.btnId == _JOY_XINPUT_LEGACY_L_THUMB_CENTER)
      obj.btnId = JOY_XINPUT_REAL_BTN_L_THUMB;
    else if (obj.btnId == _JOY_XINPUT_LEGACY_R_THUMB_CENTER)
      obj.btnId = JOY_XINPUT_REAL_BTN_R_THUMB;
  }
}

static void patch_thumbstick_legacy_id(dainput::DigitalActionBinding &obj)
{
  using namespace HumanInput;
  if (obj.devId == dainput::DEV_gamepad)
  {
    if (obj.ctrlId == _JOY_XINPUT_LEGACY_L_THUMB_CENTER)
      obj.ctrlId = JOY_XINPUT_REAL_BTN_L_THUMB;
    else if (obj.ctrlId == _JOY_XINPUT_LEGACY_R_THUMB_CENTER)
      obj.ctrlId = JOY_XINPUT_REAL_BTN_R_THUMB;
  }
}

void dainput::reset_actions()
{
  {
    G_STATIC_ASSERT(sizeof(dainput::SingleButtonId) == 2);
    G_STATIC_ASSERT(sizeof(dainput::DigitalActionBinding) == 10);
    G_STATIC_ASSERT(sizeof(dainput::AnalogAxisActionBinding) == 36);
    G_STATIC_ASSERT(sizeof(dainput::AnalogStickActionBinding) == 40);
  }

  clear_and_shrink(dainput::actionSetStack);
  agData = ActionGlobData();
  clear_and_shrink(actionSets);
  for (auto &m : actionNameBackMap)
    m.clear();
  actionsNativeOrdered.clear();
  actionNameIdx.reset();
  actionSetNameIdx.reset();
  customPropsScheme.clearData();

  if (dev5_vr && dev5_vr->canCustomizeBindings())
    dev5_vr->setInitializationCallback({});
}
void dainput::init_actions(const DataBlock &blk)
{
  dainput::set_control_thread_id(get_main_thread_id());
  configVer = blk.getInt("configVer", 0);
  agData.longPressDur = blk.getInt("longPressDur", 300);
  agData.dblClickDur = blk.getInt("dblClickDur", 220);


  eastl::vector<eastl::pair<eastl::string, int>> actToNid;

  if (const DataBlock *b = blk.getBlockByName("actionSets"))
  {
    for (int i = 0; i < b->blockCount(); i++)
    {
      const DataBlock *bSet = b->getBlock(i);
      int set_nid = actionSetNameIdx.addNameId(bSet->getBlockName());
      if (set_nid != actionSets.size())
      {
        logerr("duplicate action set <%s>", actionSetNameIdx.getName(set_nid));
        continue;
      }
      dainput::ActionSet &set = actionSets.push_back();

      Tab<action_handle_t> handles;
      bool setIsInternal = bSet->getBool("internal", false);
      for (int j = 0; j < bSet->blockCount(); j++)
      {
        const DataBlock *bAct = bSet->getBlock(j);
        int act_nid = actionNameIdx.getStrId(bAct->getBlockName());
        if (act_nid < 0)
        {
          const char *typestr = bAct->getStr("type", "");
          if (strcmp(typestr, "absolute_mouse") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_STICK | agData.as.size());
            agData.as.push_back().init(act_nid, TYPE_ABSMOUSE);
          }
          else if (strcmp(typestr, "system_mouse") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_STICK | agData.as.size());
            agData.as.push_back().init(act_nid, TYPE_SYSMOUSE);
          }
          else if (strcmp(typestr, "joystick_move") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_STICK | agData.as.size());
            agData.as.push_back().init(act_nid, TYPE_STICK);
          }
          else if (strcmp(typestr, "joystick_delta") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_STICK | agData.as.size());
            agData.as.push_back().init(act_nid, TYPE_STICK_DELTA);
          }
          else if (strcmp(typestr, "trigger") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_AXIS | agData.aa.size());
            agData.aa.push_back().init(act_nid, TYPE_TRIGGER);
          }
          else if (strcmp(typestr, "steerwheel") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_AXIS | agData.aa.size());
            agData.aa.push_back().init(act_nid, TYPE_STEERWHEEL);
          }
          else if (strcmp(typestr, "digital_button") == 0)
          {
            act_nid = actionNameIdx.addStrId(bAct->getBlockName(), TYPEGRP_DIGITAL | agData.ad.size());
            agData.ad.push_back().init(act_nid, TYPE_BUTTON);
            agData.ad[act_nid & ~TYPEGRP__MASK].uiHoldToToggleDur = bAct->getInt("uiHoldToToggleDurMsec", 0x40000000); // disabled by
                                                                                                                       // default
          }
          else
          {
            logerr("bad/missing action <%s> type <%s>", bAct->getBlockName(), typestr);
            continue;
          }

          // fill actionNameBackMap
          Tab<const char *> &m = actionNameBackMap[act_nid >> 14];
          while (m.size() <= (act_nid & ~TYPEGRP__MASK))
            m.push_back(nullptr);
          m[act_nid & ~TYPEGRP__MASK] = actionNameIdx.getStrSlow(act_nid);
          actionsNativeOrdered.push_back(act_nid);

          // read flags and exclusive tag
          int exclTag = 0, flags = 0, grpTag = 0;
          const char *etag = bAct->getStr("exclusive_tag", "");
          const char *gtag = bAct->getStr("group_tag", bSet->getStr("group_tag", ""));
          if (*etag)
            exclTag = tagNames.addNameId(etag) + 1;
          if (*gtag)
            grpTag = tagNames.addNameId(gtag) + 1;
          if (bAct->getBool("internal", setIsInternal))
            flags |= ACTIONF_internal;
          if (bAct->getBool("maskImmediate", false))
            flags |= ACTIONF_maskImmediate;
          if (bAct->getBool("eventToGui", bSet->getBool("eventToGui", (act_nid & TYPEGRP__MASK) == TYPEGRP_DIGITAL)))
            flags |= ACTIONF_eventToGui;

          switch (act_nid & TYPEGRP__MASK)
          {
            case TYPEGRP_DIGITAL:
              agData.ad[act_nid & ~TYPEGRP__MASK].flags = flags;
              agData.ad[act_nid & ~TYPEGRP__MASK].exclTag = exclTag;
              agData.ad[act_nid & ~TYPEGRP__MASK].groupTag = grpTag;
              break;
            case TYPEGRP_AXIS:
              if (bAct->getBool("stateful", false))
                flags |= ACTIONF_stateful;
              agData.aa[act_nid & ~TYPEGRP__MASK].flags = flags;
              agData.aa[act_nid & ~TYPEGRP__MASK].exclTag = exclTag;
              agData.aa[act_nid & ~TYPEGRP__MASK].groupTag = grpTag;
              break;
            case TYPEGRP_STICK:
              agData.as[act_nid & ~TYPEGRP__MASK].flags = flags;
              agData.as[act_nid & ~TYPEGRP__MASK].exclTag = exclTag;
              agData.as[act_nid & ~TYPEGRP__MASK].groupTag = grpTag;
              agData.as[act_nid & ~TYPEGRP__MASK].mmScaleX = bAct->getReal("mouseMoveScaleX", bAct->getReal("mouseMoveScale", 1));
              agData.as[act_nid & ~TYPEGRP__MASK].mmScaleY = bAct->getReal("mouseMoveScaleY", bAct->getReal("mouseMoveScale", 1));
              agData.as[act_nid & ~TYPEGRP__MASK].gaScaleX = bAct->getReal("gpadAxisScaleX", bAct->getReal("gpadAxisScale", 1));
              agData.as[act_nid & ~TYPEGRP__MASK].gaScaleY = bAct->getReal("gpadAxisScaleY", bAct->getReal("gpadAxisScale", 1));
              agData.as[act_nid & ~TYPEGRP__MASK].smoothValue = bAct->getReal("smoothValue", 0.f);
              break;
          }

          actToNid.emplace_back(bAct->getBlockName(), act_nid);
        }
        else if (!bAct->getBool("reuse_defined_earlier", false))
        {
          logerr("duplicate action <%s>", bAct->getBlockName());
          continue;
        }
        else
          erase_item_by_value(handles, act_nid);

        switch (act_nid & TYPEGRP__MASK)
        {
          case TYPEGRP_DIGITAL:
          case TYPEGRP_AXIS:
          case TYPEGRP_STICK:
            if (const char *a_nm = bAct->getStr("beforeAction", nullptr))
            {
              int ref_pos = find_value_idx(handles, actionNameIdx.getStrId(a_nm));
              if (ref_pos >= 0)
                insert_item_at(handles, ref_pos, act_nid);
              else
                logerr("cannot insert action <%s> before missing <%s>", bAct->getBlockName(), a_nm);
            }
            else if (const char *a_nm = bAct->getStr("afterAction", nullptr))
            {
              int ref_pos = find_value_idx(handles, actionNameIdx.getStrId(a_nm));
              if (ref_pos >= 0)
                insert_item_at(handles, ref_pos + 1, act_nid);
              else
                logerr("cannot insert action <%s> after missing <%s>", bAct->getBlockName(), a_nm);
            }
            else
              handles.push_back(act_nid);
            break;
          default: logerr("inconsistent act_nid=%X (%d:%d)", act_nid, act_nid & 0x7F0000, act_nid & 0xFFFF);
        }
      }
      set.actions = handles;
    }
  }

  if (const DataBlock *b = blk.getBlockByName("actionSetsOrder"))
  {
    for (int i = 0; i < actionSets.size(); i++)
      actionSets[i].ordPriority = 32767;
    for (int i = 0, prio = 1; i < b->blockCount(); i++, prio++)
    {
      const DataBlock *bSet = b->getBlock(i);
      prio = bSet->getInt("priority", prio);
      action_set_handle_t h = get_action_set_handle(bSet->getBlockName());
      if (h == BAD_ACTION_SET_HANDLE)
        logerr("dainput: bad action set <%s> in %s", bSet->getBlockName(), b->getBlockName());
      else
        actionSets[h].ordPriority = prio;
    }

    for (int i = 0; i < b->blockCount(); i++)
    {
      const DataBlock *bSet = b->getBlock(i);
      action_set_handle_t h = get_action_set_handle(bSet->getBlockName());
      if (h == BAD_ACTION_SET_HANDLE)
        continue;
      const char *ref_set = nullptr;
      int ref_pos = 0;
      if ((ref_set = bSet->getStr("before", nullptr)) != nullptr)
        ref_pos = -1;
      else if ((ref_set = bSet->getStr("after", nullptr)) != nullptr)
        ref_pos = +1;
      else
        ref_set = bSet->getStr("same_as", nullptr);
      if (!ref_set)
        continue;
      action_set_handle_t h2 = get_action_set_handle(ref_set);
      if (h2 == BAD_ACTION_SET_HANDLE)
      {
        logerr("dainput: bad action set <%s> (referenced by %s in %s)", ref_set, bSet->getBlockName(), b->getBlockName());
        continue;
      }
      int ref_prio = actionSets[h2].ordPriority;
      if (ref_prio == 32767)
      {
        logerr("dainput: undefined action set <%s> prio=%d (referenced by %s in %s)", ref_set, ref_prio, bSet->getBlockName(),
          b->getBlockName());
        continue;
      }
      if (ref_pos == 0)
        actionSets[h].ordPriority = ref_prio;
      else
      {
        for (auto &s : actionSets)
          if (s.ordPriority != 32767 && s.ordPriority >= ref_prio + ref_pos)
            s.ordPriority++;
        actionSets[h].ordPriority = ref_prio + ref_pos;
      }
    }

    for (int i = 0; i < actionSets.size(); i++)
      if (actionSets[i].ordPriority == 32767)
      {
        logerr("dainput: priority for action set <%s> not defined in %s", get_action_set_name(i), b->getBlockName());
        actionSets[i].ordPriority = 0;
      }
  }
  customPropsScheme = *blk.getBlockByNameEx("customUserProps");


  if (dev5_vr && dev5_vr->canCustomizeBindings())
  {
    const DataBlock *vrBindingsBlock = blk.getBlockByName("suggestedVrBindings");
    dev5_vr->setAndCallInitializationCallback(
      [actToNid = eastl::move(actToNid),
        // Need to __copy__ the datablock if it exists
        vrBindingsBlock = vrBindingsBlock ? eastl::optional{*vrBindingsBlock} : eastl::nullopt]() {
        dev5_vr->addActionSet(VR_DUMMY_ACTN_SET, "default");

        for (auto &[actionName, actionNid] : actToNid)
        {
          {
            using namespace HumanInput;
            VrInput::ActionType type = VrInput::ActionType::DIGITAL;
            switch (actionNid & TYPEGRP__MASK)
            {
              case TYPEGRP_DIGITAL: type = VrInput::ActionType::DIGITAL; break;
              case TYPEGRP_AXIS: type = VrInput::ActionType::ANALOG; break;
              case TYPEGRP_STICK: type = VrInput::ActionType::STICK; break;
            }
            dev5_vr->addAction(VR_DUMMY_ACTN_SET, actionNid, type, actionName.c_str(), actionName.c_str());
            if (vrBindingsBlock.has_value())
              if (const DataBlock *binds = vrBindingsBlock->getBlockByName(actionName.c_str()))
                for (int i = 0; i < binds->paramCount(); ++i)
                  dev5_vr->suggestBinding(actionNid, binds->getParamName(i), binds->getStr(i));
          }
        }

        dev5_vr->setupHands(VR_DUMMY_ACTN_SET, "grip_pose", "aim_pose", "haptics");
        dev5_vr->completeActionsInit();
        dev5_vr->activateActionSet(VR_DUMMY_ACTN_SET);
      });
  }
}

void dainput::reset_actions_binding()
{
  agData.bindingsColumnCount = 0;
  clear_and_shrink(agData.adb);
  clear_and_shrink(agData.aab);
  clear_and_shrink(agData.asb);
}
int dainput::get_actions_binding_columns() { return agData.bindingsColumnCount; }

void dainput::append_actions_binding(const DataBlock &blk)
{
  G_ASSERT_RETURN(agData.bindingsColumnCount < 4, );

  Tab<DigitalActionBinding> adb;
  Tab<AnalogAxisActionBinding> aab;
  Tab<AnalogStickActionBinding> asb;

  int prev_cnt = agData.bindingsColumnCount;
  agData.bindingsColumnCount++;
  adb.resize(agData.ad.size() * agData.bindingsColumnCount);
  mem_set_0(adb);
  aab.resize(agData.aa.size() * agData.bindingsColumnCount);
  mem_set_0(aab);
  asb.resize(agData.as.size() * agData.bindingsColumnCount);
  mem_set_0(asb);

  if (agData.bindingsColumnCount > 1)
  {
    for (int i = 0; i < agData.ad.size(); i++)
      memcpy(&adb[i * agData.bindingsColumnCount], &agData.adb[i * prev_cnt], elem_size(adb) * prev_cnt);

    for (int i = 0; i < agData.aa.size(); i++)
      memcpy(&aab[i * agData.bindingsColumnCount], &agData.aab[i * prev_cnt], elem_size(aab) * prev_cnt);

    for (int i = 0; i < agData.as.size(); i++)
      memcpy(&asb[i * agData.bindingsColumnCount], &agData.asb[i * prev_cnt], elem_size(asb) * prev_cnt);
  }
  agData.adb = eastl::move(adb);
  agData.aab = eastl::move(aab);
  agData.asb = eastl::move(asb);

  load_actions_binding(blk, agData.bindingsColumnCount - 1);
}
void dainput::clear_actions_binding(int column)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
  {
    logerr("bad column=%d for %s (agData.bindingsColumnCount=%d)", column, __FUNCTION__, agData.bindingsColumnCount);
    return;
  }

  for (int i = 0; i < agData.ad.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    agData.adb[b_idx] = DigitalActionBinding();
  }
  for (int i = 0; i < agData.aa.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    agData.aab[b_idx] = AnalogAxisActionBinding();
  }
  for (int i = 0; i < agData.as.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    agData.asb[b_idx] = AnalogStickActionBinding();
  }
}
void dainput::load_actions_binding(const DataBlock &blk, int column,
  void (*on_config_ver_differ)(DataBlock &blk, int config_ver, int bindings_ver))
{
  int bVer = blk.getInt("configVer", configVer);
  if (bVer != configVer && on_config_ver_differ)
  {
    DataBlock alt_blk;
    alt_blk = blk;
    on_config_ver_differ(alt_blk, configVer, bVer);
    return dainput::load_actions_binding(alt_blk, column);
  }

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock *bAct = blk.getBlock(i);
    int act_nid = actionNameIdx.getStrId(bAct->getBlockName());
    if (act_nid < 0)
    {
      logwarn("skipping undefined action <%s>", bAct->getBlockName());
      continue;
    }

    int b_idx = agData.get_binding_idx(act_nid, column);
    switch (act_nid & TYPEGRP__MASK)
    {
      case TYPEGRP_DIGITAL: load_action_binding_digital(*bAct, agData.adb[b_idx]); break;
      case TYPEGRP_AXIS: load_action_binding_axis(*bAct, agData.aab[b_idx]); break;
      case TYPEGRP_STICK: load_action_binding_stick(*bAct, agData.asb[b_idx]); break;
      default: logerr("inconsistent act_nid=%X (%d:%d)", act_nid, act_nid & 0x7F0000, act_nid & 0xFFFF);
    }
  }
}
void dainput::save_actions_binding(DataBlock &blk, int column) { dainput::save_actions_binding_ex(blk, column, nullptr); }
bool dainput::save_actions_binding_ex(DataBlock &blk, int column, const DataBlock *base_preset)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
  {
    logerr("bad column=%d for %s (agData.bindingsColumnCount=%d)", column, __FUNCTION__, agData.bindingsColumnCount);
    return false;
  }

  for (int i = 0; i < agData.ad.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    const char *a_name = get_action_name_fast(agData.ad[i].nameId);
    if (agData.adb[b_idx].devId == DEV_none)
    {
      if (base_preset && base_preset->getBlockByName(a_name))
        blk.addBlock(a_name);
      continue;
    }
    DataBlock &b = *blk.addBlock(a_name);
    if (!b.paramCount())
      save_action_binding_digital(b, agData.adb[b_idx]);
    else
      logerr("block <%s> is not empty for save_actions_binding", b.getBlockName());
  }
  for (int i = 0; i < agData.aa.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    const char *a_name = get_action_name_fast(agData.aa[i].nameId);
    if (agData.aab[b_idx].devId == DEV_none)
    {
      if (base_preset && base_preset->getBlockByName(a_name))
        blk.addBlock(a_name);
      continue;
    }
    DataBlock &b = *blk.addBlock(a_name);
    if (!b.paramCount())
      save_action_binding_axis(b, agData.aab[b_idx]);
    else
      logerr("block <%s> is not empty for save_actions_binding", b.getBlockName());
  }
  for (int i = 0; i < agData.as.size(); i++)
  {
    int b_idx = agData.get_binding_idx(i, column);
    const char *a_name = get_action_name_fast(agData.as[i].nameId);
    if (agData.asb[b_idx].devId == DEV_none)
    {
      if (base_preset && base_preset->getBlockByName(a_name))
        blk.addBlock(a_name);
      continue;
    }
    DataBlock &b = *blk.addBlock(a_name);
    if (!b.paramCount())
      save_action_binding_stick(b, agData.asb[b_idx]);
    else
      logerr("block <%s> is not empty for save_actions_binding", b.getBlockName());
  }
  blk.setInt("configVer", configVer);

  if (base_preset && equalDataBlocks(*base_preset, blk))
  {
    blk.clearData();
    blk.setInt("configVer", configVer);
    return false;
  }
  return true;
}

void dainput::get_action_binding(action_handle_t action, int column, DataBlock &out_binding)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
  {
    logerr("bad column=%d for %s (agData.bindingsColumnCount=%d)", column, __FUNCTION__, agData.bindingsColumnCount);
    return;
  }
  out_binding.clearData();
  if (action == BAD_ACTION_HANDLE)
    return;

  int b_idx = agData.get_binding_idx(action, column);
  switch (action & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: save_action_binding_digital(out_binding, agData.adb[b_idx]); break;
    case TYPEGRP_AXIS: save_action_binding_axis(out_binding, agData.aab[b_idx]); break;
    case TYPEGRP_STICK: save_action_binding_stick(out_binding, agData.asb[b_idx]); break;
  }
}
void dainput::set_action_binding(action_handle_t action, int column, const DataBlock &binding)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
  {
    logerr("bad column=%d for %s (agData.bindingsColumnCount=%d)", column, __FUNCTION__, agData.bindingsColumnCount);
    return;
  }
  if (action == BAD_ACTION_HANDLE)
    return;

  int b_idx = agData.get_binding_idx(action, column);
  switch (action & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: load_action_binding_digital(binding, agData.adb[b_idx]); break;
    case TYPEGRP_AXIS: load_action_binding_axis(binding, agData.aab[b_idx]); break;
    case TYPEGRP_STICK: load_action_binding_stick(binding, agData.asb[b_idx]); break;
  }
}
void dainput::reset_action_binding(action_handle_t action, int column)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
  {
    logerr("bad column=%d for %s (agData.bindingsColumnCount=%d)", column, __FUNCTION__, agData.bindingsColumnCount);
    return;
  }
  int b_idx = agData.get_binding_idx(action, column);
  switch (action & TYPEGRP__MASK)
  {
    case TYPEGRP_DIGITAL: agData.adb[b_idx].devId = 0; break;
    case TYPEGRP_AXIS: agData.aab[b_idx].devId = 0; break;
    case TYPEGRP_STICK: agData.asb[b_idx].devId = 0; break;
  }
}
dainput::DigitalActionBinding *dainput::get_digital_action_binding(dainput::action_handle_t action, int column)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
    return nullptr;
  return (action & TYPEGRP__MASK) == TYPEGRP_DIGITAL ? &agData.adb[agData.get_binding_idx(action, column)] : nullptr;
}
dainput::AnalogAxisActionBinding *dainput::get_analog_axis_action_binding(dainput::action_handle_t action, int column)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
    return nullptr;
  return (action & TYPEGRP__MASK) == TYPEGRP_AXIS ? &agData.aab[agData.get_binding_idx(action, column)] : nullptr;
}
dainput::AnalogStickActionBinding *dainput::get_analog_stick_action_binding(dainput::action_handle_t action, int column)
{
  if (column < 0 || column >= agData.bindingsColumnCount)
    return nullptr;
  return (action & TYPEGRP__MASK) == TYPEGRP_STICK ? &agData.asb[agData.get_binding_idx(action, column)] : nullptr;
}

float dainput::get_analog_stick_action_smooth_value(action_handle_t a)
{
  G_ASSERTF_RETURN((a & TYPEGRP__MASK) == TYPEGRP_STICK, 0, "action is not stick, type=%d", a & TYPEGRP__MASK);
  return agData.as[a & ~TYPEGRP__MASK].smoothValue;
}
void dainput::set_analog_stick_action_smooth_value(action_handle_t a, float val)
{
  G_ASSERTF_RETURN((a & TYPEGRP__MASK) == TYPEGRP_STICK, , "action is not stick, type=%d", a & TYPEGRP__MASK);
  agData.as[a & ~TYPEGRP__MASK].smoothValue = val;
}

template <class AB>
void load_action_binding_modifiers(AB &b, const DataBlock &blk)
{
  b.modCnt = 0;
  for (int i = 0, nid = blk.getNameId("mod"); i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      if (b.modCnt == 3)
      {
        logerr("too many modifiers for action <%s>", blk.getBlockName());
        continue;
      }
      b.mod[b.modCnt].devId = blk.getBlock(i)->getInt("dev", 0);
      b.mod[b.modCnt].btnId = blk.getBlock(i)->getInt("btn", 0);
      patch_thumbstick_legacy_id(b.mod[b.modCnt]);
      b.modCnt++;
    }
}
static void save_action_binding_modifiers(const dainput::SingleButtonId *mod, int modCnt, DataBlock &blk)
{
  for (int i = 0; i < modCnt; i++)
  {
    DataBlock *m = blk.addNewBlock("mod");
    m->setInt("dev", mod[i].devId);
    m->setInt("btn", mod[i].btnId);
  }
}

void dainput::load_action_binding_digital(const DataBlock &blk, dainput::DigitalActionBinding &b)
{
  b.devId = blk.getInt("dev", 0);
  if (int idx = blk.getInt("btn", -1) + 1)
  {
    b.ctrlId = idx - 1;
    b.btnCtrl = 1;
    b.axisCtrlThres = 0;
    patch_thumbstick_legacy_id(b);
  }
  else if (int idx = blk.getInt("axis", -1) + 1)
  {
    b.ctrlId = idx - 1;
    b.btnCtrl = 0;
    b.axisCtrlThres = blk.getInt("axisThres", 0);
    G_ASSERTF(b.axisCtrlThres == blk.getInt("axisThres", 0), "axisThres=%d > 7 (max)", blk.getInt("axisThres", 0));
  }
  else
    b.devId = b.ctrlId = b.btnCtrl = b.axisCtrlThres = b.eventType = 0;

  b.eventType = blk.getInt("type", b.BTN_pressed);
  b.stickyToggle = blk.getBool("stickyToggle", false);
  b.unordCombo = blk.getBool("unordCombo", false);
  load_action_binding_modifiers(b, blk);
}
void dainput::load_action_binding_axis(const DataBlock &blk, dainput::AnalogAxisActionBinding &b)
{
  b.devId = blk.getInt("dev", 0);
  b.axisId = blk.getInt("axis", 0);
  b.invAxis = blk.getBool("inverse", false);
  b.axisRelativeVal = blk.getBool("axisRelativeVal", false);
  b.instantIncDecBtn = blk.getBool("instantIncDecBtn", false);
  b.quantizeValOnIncDecBtn = blk.getBool("quantizeValOnIncDecBtn", false);

  if (const DataBlock *bb = blk.getBlockByName("minBtn"))
  {
    b.minBtn.devId = bb->getInt("dev", 0);
    b.minBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.minBtn);
  }
  else
    b.minBtn.devId = b.minBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("maxBtn"))
  {
    b.maxBtn.devId = bb->getInt("dev", 0);
    b.maxBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.maxBtn);
  }
  else
    b.maxBtn.devId = b.maxBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("incBtn"))
  {
    b.incBtn.devId = bb->getInt("dev", 0);
    b.incBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.incBtn);
  }
  else
    b.incBtn.devId = b.incBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("decBtn"))
  {
    b.decBtn.devId = bb->getInt("dev", 0);
    b.decBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.decBtn);
  }
  else
    b.decBtn.devId = b.decBtn.btnId = 0;

  b.deadZoneThres = blk.getReal("dzone", 0);
  b.nonLin = blk.getReal("nonlin", 0);
  b.maxVal = blk.getReal("maxVal", 1);
  b.relIncScale = blk.getReal("relIncScale", 1);

  load_action_binding_modifiers(b, blk);
}
void dainput::load_action_binding_stick(const DataBlock &blk, dainput::AnalogStickActionBinding &b)
{
  b.devId = blk.getInt("dev", 0);
  b.axisXId = blk.getInt("xAxis", 0);
  b.axisYId = blk.getInt("yAxis", 0);
  b.axisXinv = blk.getBool("inverseX", false);
  b.axisYinv = blk.getBool("inverseY", false);

  if (const DataBlock *bb = blk.getBlockByName("xMinBtn"))
  {
    b.minXBtn.devId = bb->getInt("dev", 0);
    b.minXBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.minXBtn);
  }
  else
    b.minXBtn.devId = b.minXBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("xMaxBtn"))
  {
    b.maxXBtn.devId = bb->getInt("dev", 0);
    b.maxXBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.maxXBtn);
  }
  else
    b.maxXBtn.devId = b.maxXBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("yMinBtn"))
  {
    b.minYBtn.devId = bb->getInt("dev", 0);
    b.minYBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.minYBtn);
  }
  else
    b.minYBtn.devId = b.minYBtn.btnId = 0;

  if (const DataBlock *bb = blk.getBlockByName("yMaxBtn"))
  {
    b.maxYBtn.devId = bb->getInt("dev", 0);
    b.maxYBtn.btnId = bb->getInt("btn", 0);
    patch_thumbstick_legacy_id(b.maxYBtn);
  }
  else
    b.maxYBtn.devId = b.maxYBtn.btnId = 0;

  b.deadZoneThres = blk.getReal("dzone", 0);
  b.axisSnapAngK = blk.getReal("axisSnapAngK", 0);
  b.nonLin = blk.getReal("nonlin", 0);
  b.maxVal = blk.getReal("maxVal", 1);
  b.sensScale = blk.getReal("sensScale", 1);

  load_action_binding_modifiers(b, blk);
}

void dainput::save_action_binding_digital(DataBlock &blk, const dainput::DigitalActionBinding &b)
{
  blk.setInt("dev", b.devId);
  blk.setInt(b.btnCtrl ? "btn" : "axis", b.ctrlId);
  if (b.eventType)
    blk.setInt("type", b.eventType);
  if (!b.btnCtrl)
    blk.setInt("axisThres", b.axisCtrlThres);
  if (b.stickyToggle)
    blk.setBool("stickyToggle", b.stickyToggle);
  if (b.unordCombo)
    blk.setBool("unordCombo", b.unordCombo);
  save_action_binding_modifiers(b.mod, b.modCnt, blk);
}
void dainput::save_action_binding_axis(DataBlock &blk, const dainput::AnalogAxisActionBinding &b)
{
  blk.setInt("dev", b.devId);
  if (b.devId != dainput::DEV_nullstub)
    blk.setInt("axis", b.axisId);
  if (b.invAxis)
    blk.setBool("inverse", b.invAxis);
  if (b.axisRelativeVal)
    blk.setBool("axisRelativeVal", b.axisRelativeVal);
  if (b.instantIncDecBtn)
    blk.setBool("instantIncDecBtn", b.instantIncDecBtn);
  if (b.quantizeValOnIncDecBtn)
    blk.setBool("quantizeValOnIncDecBtn", b.quantizeValOnIncDecBtn);

  if (b.minBtn.devId)
  {
    DataBlock *bb = blk.addBlock("minBtn");
    bb->setInt("dev", b.minBtn.devId);
    bb->setInt("btn", b.minBtn.btnId);
  }

  if (b.maxBtn.devId)
  {
    DataBlock *bb = blk.addBlock("maxBtn");
    bb->setInt("dev", b.maxBtn.devId);
    bb->setInt("btn", b.maxBtn.btnId);
  }

  if (b.incBtn.devId)
  {
    DataBlock *bb = blk.addBlock("incBtn");
    bb->setInt("dev", b.incBtn.devId);
    bb->setInt("btn", b.incBtn.btnId);
  }

  if (b.decBtn.devId)
  {
    DataBlock *bb = blk.addBlock("decBtn");
    bb->setInt("dev", b.decBtn.devId);
    bb->setInt("btn", b.decBtn.btnId);
  }

  blk.setReal("dzone", b.deadZoneThres);
  blk.setReal("nonlin", b.nonLin);
  blk.setReal("maxVal", b.maxVal);
  blk.setReal("relIncScale", b.relIncScale);
  save_action_binding_modifiers(b.mod, b.modCnt, blk);
}
void dainput::save_action_binding_stick(DataBlock &blk, const dainput::AnalogStickActionBinding &b)
{
  blk.setInt("dev", b.devId);
  if (b.devId != dainput::DEV_nullstub)
  {
    blk.setInt("xAxis", b.axisXId);
    blk.setInt("yAxis", b.axisYId);
  }
  if (b.axisXinv)
    blk.setBool("inverseX", b.axisXinv);
  if (b.axisYinv)
    blk.setBool("inverseY", b.axisYinv);

  if (b.minXBtn.devId)
  {
    DataBlock *bb = blk.addBlock("xMinBtn");
    bb->setInt("dev", b.minXBtn.devId);
    bb->setInt("btn", b.minXBtn.btnId);
  }

  if (b.maxXBtn.devId)
  {
    DataBlock *bb = blk.addBlock("xMaxBtn");
    bb->setInt("dev", b.maxXBtn.devId);
    bb->setInt("btn", b.maxXBtn.btnId);
  }

  if (b.minYBtn.devId)
  {
    DataBlock *bb = blk.addBlock("yMinBtn");
    bb->setInt("dev", b.minYBtn.devId);
    bb->setInt("btn", b.minYBtn.btnId);
  }

  if (b.maxYBtn.devId)
  {
    DataBlock *bb = blk.addBlock("yMaxBtn");
    bb->setInt("dev", b.maxYBtn.devId);
    bb->setInt("btn", b.maxYBtn.btnId);
  }

  blk.setReal("dzone", b.deadZoneThres);
  blk.setReal("axisSnapAngK", b.axisSnapAngK);
  blk.setReal("nonlin", b.nonLin);
  blk.setReal("maxVal", b.maxVal);
  blk.setReal("sensScale", b.sensScale);

  save_action_binding_modifiers(b.mod, b.modCnt, blk);
}
