// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/screencap.h"
namespace screencap
{
void init(const DataBlock *) {}
void term() {}

void set_comments(const char *) {}
void make_screenshot(Texture *) {}
void make_hdr_screenshot(const ManagedTex &) {}
void schedule_screenshot(bool, bool) {}
bool is_hdr_screenshot_scheduled() { return false; }
bool is_screenshot_scheduled() { return false; }

void toggle_avi_writer() {}

void screenshots_saved() {}
void update(Texture *) {}

bool should_hide_gui() { return true; }
bool should_hide_debug() { return true; }

bool no_tonemap() { return true; }
bool log_tonemap() { return false; }

int subsamples() { return 1; }
int supersamples() { return 1; }
float fixed_act_rate() { return -1; }
}; // namespace screencap
