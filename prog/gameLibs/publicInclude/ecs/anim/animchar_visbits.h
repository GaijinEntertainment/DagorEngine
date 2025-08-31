//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

using animchar_visbits_t = uint16_t;

enum AnimcharVisbits : animchar_visbits_t
{
  VISFLG_WITHIN_RANGE = 1 << 0,      // animchar_before_render_es also calculated animchar_bsph and such when this flag is set
  VISFLG_LOD_CHOSEN = 1 << 1,        // chooseLodByDistSq
  VISFLG_GEOM_TREE_UPDATED = 1 << 2, // prepare_for_render and update_geom_tree
  VISFLG_MAIN_VISIBLE = 1 << 3,
  VISFLG_MAIN_AND_SHADOW_VISIBLE = 1 << 4,
  VISFLG_COCKPIT_VISIBLE = 1 << 5,
  VISFLG_HMAP_DEFORM = 1 << 6,
  VISFLG_OUTLINE_RENDER = 1 << 7,
  VISFLG_BVH = 1 << 8,
  VISFLG_DYNAMIC_MIRROR = 1 << 9,
  VISFLG_RENDER_CUSTOM = VISFLG_HMAP_DEFORM | VISFLG_OUTLINE_RENDER | VISFLG_BVH | VISFLG_DYNAMIC_MIRROR,
  VISFLG_SEMI_TRANS_RENDERED = 1 << 13,
  VISFLG_CSM_SHADOW_RENDERED = 1 << 14,
  VISFLG_MAIN_CAMERA_RENDERED = 1 << 15,
  VISFLG_ALL_BITS = 0xFFFF
};

inline void mark_cockpit_visible(animchar_visbits_t &animchar_visbits)
{
  animchar_visbits &= ~(VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE);
  animchar_visbits |= VISFLG_COCKPIT_VISIBLE;
}

inline void hide_from_main_visibility(animchar_visbits_t &animchar_visbits)
{
  animchar_visbits &= ~(VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_COCKPIT_VISIBLE);
}