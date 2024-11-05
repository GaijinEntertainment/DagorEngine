// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <contentUpdater/version.h>
#include <daECS/core/event.h>

namespace darg
{
struct IGuiScene;
}

struct SQVM;

namespace overlay_ui
{
void init_network_services();
void init_ui();
void shutdown_ui(bool quit);
void shutdown_network_services();

void update();

darg::IGuiScene *gui_scene();
SQVM *get_vm();

void reload_scripts(bool);
void rebind_das();

// Version of the Overlay UI.
// The updater will use this functions in order to reload overlay UI
// in case of changed version.
// Reload took place after leaving a session.
void set_overlay_ui_version(updater::Version version);
updater::Version get_overlay_ui_version();
} // namespace overlay_ui

ECS_BROADCAST_EVENT_TYPE(EventScriptUiInitNetworkServices);
ECS_BROADCAST_EVENT_TYPE(EventScriptUiTermNetworkServices);
ECS_BROADCAST_EVENT_TYPE(EventScriptUiUpdate)
ECS_BROADCAST_EVENT_TYPE(EventScriptUiBeforeEventbusShutdown, bool /*quit*/)
ECS_BROADCAST_EVENT_TYPE(EventScriptUiShutdown, bool /*quit*/)
