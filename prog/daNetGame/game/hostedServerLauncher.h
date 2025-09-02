// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>

namespace Sqrat
{
class Table;
}

ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStart);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerDidStop);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStart, ecs::List<ecs::string> /*cmd*/);
ECS_BROADCAST_EVENT_TYPE(EventHostedInternalServerToStop);

void bind_hosting_internal_dedicated_server(Sqrat::Table &ns);
static void terminate_and_unload_dlls();
void shutdown_internal_server_on_host_exit();
void launch_internal_dedicated_server_with_args(int external_argc, char **external_argv);
void kill_internal_dedicated_server();