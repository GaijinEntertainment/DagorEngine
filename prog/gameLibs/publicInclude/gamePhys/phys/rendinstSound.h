//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

enum TreeSoundCbType
{
  TREE_SOUND_CB_INIT,
  TREE_SOUND_CB_DESTROY,
  TREE_SOUND_CB_FALLING,
  TREE_SOUND_CB_ON_FALLED,
};

struct TreeSoundDesc
{
  float treeHeight = 0.f;
  bool isBush = false;
};

typedef int ri_tree_sound_cb_data_t;
typedef bool (*ri_tree_sound_cb)(TreeSoundCbType call_type, const TreeSoundDesc &desc, ri_tree_sound_cb_data_t &sound_data,
  const TMatrix &tm);
