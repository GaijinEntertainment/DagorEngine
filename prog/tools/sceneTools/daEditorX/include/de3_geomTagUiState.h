//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <de3_splineGenSrv.h>
#include <propPanel/control/container.h>
#include <propPanel/constants.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <EASTL/algorithm.h>
#include <osApiWrappers/dag_localConv.h>

// UI state of one tag of a tagged geometry debug view, session-only and never saved
struct GeomTagUiState
{
  SimpleString tag;
  E3DCOLOR color = LOFT_TAGGED_GEOM_DEF_COLOR;
  bool show = false;
};

// states are sorted by tag without case, so add them only with find_or_add_geom_tag_ui_state
inline const GeomTagUiState *find_geom_tag_ui_state(dag::ConstSpan<GeomTagUiState> states, const char *tag)
{
  auto it = eastl::lower_bound(states.begin(), states.end(), tag,
    [](const GeomTagUiState &v, const char *t) { return dd_stricmp(v.tag.c_str(), t) < 0; });
  if (it == states.end() || dd_stricmp(it->tag.c_str(), tag) != 0)
    return nullptr;
  return &*it;
}

inline GeomTagUiState &find_or_add_geom_tag_ui_state(Tab<GeomTagUiState> &states, const char *tag)
{
  auto it = eastl::lower_bound(states.begin(), states.end(), tag,
    [](const GeomTagUiState &v, const char *t) { return dd_stricmp(v.tag.c_str(), t) < 0; });
  if (it == states.end() || dd_stricmp(it->tag.c_str(), tag) != 0)
    return *states.insert(it, GeomTagUiState{SimpleString(tag)});
  return *it;
}

inline constexpr auto DEFAULT_TAG_FILTER = [](const char *) -> bool { return true; };

// the tags in out point into states, so do not change states while out is used
// filter(tag) returns false for tags to skip
template <typename FilterFn = decltype(DEFAULT_TAG_FILTER)>
inline void gather_shown_geom_tags(dag::ConstSpan<GeomTagUiState> states, Tab<GeomTagDrawParams> &out, FilterFn &&filter = FilterFn{})
{
  out.clear();
  for (const GeomTagUiState &s : states)
  {
    if (!s.show || !filter(s.tag.c_str()))
      continue;
    GeomTagDrawParams &p = out.push_back();
    p.tag = s.tag.c_str();
    p.color = s.color;
  }
}

// a panel reserves GEOM_TAG_MAX * GEOM_TAG_CONTROLS control ids from its pid_begin
inline constexpr int GEOM_TAG_MAX = 128;
enum
{
  GEOM_TAG_CTRL_SHOW,
  GEOM_TAG_CTRL_COLOR,
  GEOM_TAG_CONTROLS
};

inline int geom_tag_pid(int pid_begin, int tag_idx, int ctrl) { return pid_begin + tag_idx * GEOM_TAG_CONTROLS + ctrl; }

// creates a check box and a color button per tag in lexical order, out_tags gets the tags in panel order
// tags after the first GEOM_TAG_MAX (in lexical order) get no row
inline void create_geom_tag_controls(PropPanel::ContainerPropertyControl &parent, int pid_begin, const OAHashNameMap<true> &tags,
  Tab<SimpleString> &out_tags)
{
  out_tags.clear();
  PropPanel::ContainerPropertyControl &grp =
    *parent.createContainer(-1, true, hdpi::_pxScaled(PropPanel::Constants::DEFAULT_CONTROLS_INTERVAL));
  grp.setUseFixedWidthColumns(true);
  grp.createStatic(-1, "Tags:");
  iterate_names_in_lexical_order(tags, [&](int, const char *nm) {
    if (out_tags.size() >= GEOM_TAG_MAX)
      return;
    const int tagIdx = out_tags.size();
    grp.createCheckBox(geom_tag_pid(pid_begin, tagIdx, GEOM_TAG_CTRL_SHOW), nm);
    grp.createSimpleColor(geom_tag_pid(pid_begin, tagIdx, GEOM_TAG_CTRL_COLOR), "", LOFT_TAGGED_GEOM_DEF_COLOR, true, false,
      PropPanel::Constants::SIMPLE_COLOR_BUTTON_FRAME_SIZE);
    out_tags.push_back(SimpleString(nm));
  });
}

// state=nullptr shows the defaults
inline void set_geom_tag_controls(PropPanel::ContainerPropertyControl &panel, int pid_begin, int tag_idx, const GeomTagUiState *state)
{
  panel.setBool(geom_tag_pid(pid_begin, tag_idx, GEOM_TAG_CTRL_SHOW), state && state->show);
  panel.setColor(geom_tag_pid(pid_begin, tag_idx, GEOM_TAG_CTRL_COLOR), state ? state->color : LOFT_TAGGED_GEOM_DEF_COLOR);
}

// returns false when pid is not a tag control of the panel
inline bool decode_geom_tag_pid(int pid, int pid_begin, int tag_count, int &tag_idx, int &ctrl_type)
{
  if (pid < pid_begin || pid >= pid_begin + tag_count * GEOM_TAG_CONTROLS)
    return false;
  tag_idx = (pid - pid_begin) / GEOM_TAG_CONTROLS;
  ctrl_type = (pid - pid_begin) % GEOM_TAG_CONTROLS;
  return true;
}

inline void apply_geom_tag_change(PropPanel::ContainerPropertyControl &panel, int pid, int ctrl_type, GeomTagUiState &state)
{
  switch (ctrl_type)
  {
    case GEOM_TAG_CTRL_SHOW: state.show = panel.getBool(pid); break;
    case GEOM_TAG_CTRL_COLOR: state.color = panel.getColor(pid); break;
    default: break;
  }
}
