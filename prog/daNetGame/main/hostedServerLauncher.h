// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/componentTypes.h>

namespace Sqrat
{
class Table;
}

ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStart);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStop);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStart, ecs::List<ecs::string> /*cmd*/);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStop);

void bind_hosting_internal_server(Sqrat::Table &ns);
void shutdown_internal_server_on_host_exit();
void schedule_new_internal_server_with_args(int external_argc, char **external_argv);
void kill_internal_server(bool wait);
void prelaunch_internal_server_if_needed();
