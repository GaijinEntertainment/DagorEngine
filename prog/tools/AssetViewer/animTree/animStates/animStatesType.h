// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

enum class AnimStatesType
{
  UNSET = -1,
  INCLUDE_ROOT,
  INIT_ANIM_STATE,
  ENUM_ROOT,
  ENUM,
  ENUM_ITEM,
  STATE_DESC,
  CHAN,
  STATE,
  STATE_ALIAS,
  PREVIEW,
  INIT_FIFO3,
  POST_BLEND_CTRL_ORDER,
};

inline const Tab<String> state_desc_combo_box_types = {String("State"), String("Channel"), String("State alias")};

AnimStatesType get_state_desc_cbox_enum_value(int idx);
int get_state_desc_cbox_index(AnimStatesType type);
AnimStatesType get_state_desc_cbox_enum_value(const char *block_name);
const char *get_state_desc_cbox_block_name(int idx);
const char *get_state_desc_cbox_block_name(AnimStatesType type);
