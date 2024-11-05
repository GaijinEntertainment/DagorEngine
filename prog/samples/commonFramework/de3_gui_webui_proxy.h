// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <webui/editVarPlugin.h>
class DataBlock;

typedef GuiControlDescWebUi GuiControlDesc;

inline void de3_imgui_init(const char *, const char *) { de3_webui_init(); }
inline void de3_imgui_term() { de3_webui_term(); }
inline void de3_imgui_build(const GuiControlDesc *desc, int desc_num) { de3_webui_build_c(desc, desc_num); }
inline bool de3_imgui_act(float dt) { return false; }
inline void de3_imgui_before_render() {}
inline void de3_imgui_render() {}
inline void de3_imgui_enable_obj(const char *name, bool enable) { de3_webui_enable_obj(name, enable); }
inline bool de3_imgui_is_obj_enabled(const char *name) { return de3_webui_is_obj_enabled(name); }

inline void de3_imgui_save_values(DataBlock &dest_blk) { de3_webui_save_values(dest_blk); }
inline void de3_imgui_load_values(const DataBlock &src_blk) { de3_webui_load_values(src_blk); }
inline void de3_imgui_set_gui_active(bool) {}
