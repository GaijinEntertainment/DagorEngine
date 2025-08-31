// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace darg
{
struct IGuiScene;
}
namespace ioevents
{
class IOEventsDispatcher;
}


void dargbox_app_init();
void dargbox_app_shutdown();

darg::IGuiScene *get_ui_scene();
ioevents::IOEventsDispatcher *get_io_events_poll();
int dargbox_get_exit_code();
