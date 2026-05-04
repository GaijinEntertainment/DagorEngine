// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

#define ANIM_STATES_LIST                                                                                                            \
  DECL_STATE(INCLUDE_ROOT, includeRoot), DECL_STATE(INIT_ANIM_STATE, initAnimState), DECL_STATE(ENUM_ROOT, enumRoot),               \
    DECL_STATE(ENUM, enum), DECL_STATE(ENUM_ITEM, enumItem), DECL_STATE(STATE_DESC, stateDesc), DECL_STATE(CHAN, chan),             \
    DECL_STATE(STATE, state), DECL_STATE(STATE_ALIAS, stateAlias), DECL_STATE(PREVIEW, preview), DECL_STATE(INIT_FIFO3, initFifo3), \
    DECL_STATE(POST_BLEND_CTRL_ORDER, postBlendCtrlOrder), DECL_STATE(ROOT_PROPS, rootProps)

#define DECL_STATE(x, y) String(#y)
inline const Tab<String> state_types = {ANIM_STATES_LIST};
#undef DECL_STATE

#define DECL_STATE(x, y) x
enum class AnimStatesType
{
  UNSET = -1,
  ANIM_STATES_LIST,
};
#undef DECL_STATE

inline const Tab<String> state_desc_combo_box_types = {String("State"), String("Channel"), String("State alias")};

AnimStatesType get_state_desc_cbox_enum_value(int idx);
int get_state_desc_cbox_index(AnimStatesType type);
AnimStatesType get_state_desc_cbox_enum_value(const char *block_name);
const char *get_state_desc_cbox_block_name(int idx);
const char *get_state_desc_cbox_block_name(AnimStatesType type);
const char *get_state_desc_icon(AnimStatesType type);
