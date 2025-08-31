// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ui/overlay.h>


// Stubs for dedicated server build

class SqModules;

namespace overlay_ui
{
void init_network_services() {}
void init_network_voicechat_only() {}
void init_ui() {}
void shutdown_ui(bool) {}
void shutdown_network_services() {}
void render() {}
void update() {}

darg::IGuiScene *gui_scene() { return nullptr; }
SQVM *get_vm() { return nullptr; }

void reload_scripts(bool) {}

updater::Version get_overlay_ui_version() { return updater::Version{}; }
} // namespace overlay_ui
