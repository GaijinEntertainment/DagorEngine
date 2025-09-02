// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animStatesType.h"
#include "../animTreeIcons.h"

static const Tab<AnimStatesType> state_desc_types = {AnimStatesType::STATE, AnimStatesType::CHAN, AnimStatesType::STATE_ALIAS};
static const Tab<String> state_desc_block_names = {String("state"), String("chan"), String("state_alias")};
static const Tab<const char *> state_desc_icons = {STATE_LEAF_ICON, CHANNEL_LEAF_ICON, ALIAS_LEAF_ICON};

AnimStatesType get_state_desc_cbox_enum_value(int idx)
{
  if (idx >= state_desc_types.size() || idx < 0)
    return AnimStatesType::UNSET;

  return state_desc_types[idx];
}

int get_state_desc_cbox_index(AnimStatesType type)
{
  for (int i = 0; i < state_desc_types.size(); ++i)
    if (type == state_desc_types[i])
      return i;

  return -1;
}

AnimStatesType get_state_desc_cbox_enum_value(const char *block_name)
{
  for (int i = 0; i < state_desc_block_names.size(); ++i)
    if (state_desc_block_names[i] == block_name)
      return state_desc_types[i];

  return AnimStatesType::UNSET;
}

const char *get_state_desc_cbox_block_name(int idx)
{
  if (idx >= state_desc_block_names.size() || idx < 0)
    return nullptr;

  return state_desc_block_names[idx];
}

const char *get_state_desc_cbox_block_name(AnimStatesType type)
{
  for (int i = 0; i < state_desc_types.size(); ++i)
    if (type == state_desc_types[i])
      return state_desc_block_names[i];

  return nullptr;
}

const char *get_state_desc_icon(AnimStatesType type)
{
  for (int i = 0; i < state_desc_types.size(); ++i)
    if (type == state_desc_types[i])
      return state_desc_icons[i];

  return nullptr;
}
