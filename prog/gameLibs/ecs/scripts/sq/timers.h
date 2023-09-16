#pragma once

#include <daECS/core/event.h>
#include <sqrat.h>

namespace ecs
{

namespace sq
{

void bind_timers(Sqrat::Table &exports);
void shutdown_timers(HSQUIRRELVM vm);
void update_timers(float dt, float rt_dt);

ECS_UNICAST_EVENT_TYPE(Timer, eastl::string, Sqrat::Object, float);

} // namespace sq

} // namespace ecs
