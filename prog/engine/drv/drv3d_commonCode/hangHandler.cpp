#include <3d/dag_hangHandler.h>
#include <util/dag_globDef.h>
#include <cstring>
#include <EASTL/type_traits.h>

namespace d3dhang
{

#if DAGOR_DBGLEVEL > 0
static GPUHanger hanger = nullptr;
static char hang_on_event[256] = {0};
#endif // DAGOR_DBGLEVEL > 0

void register_gpu_hanger(GPUHanger newHanger)
{
  G_UNUSED(newHanger);

#if DAGOR_DBGLEVEL > 0
  hanger = newHanger;
#endif // DAGOR_DBGLEVEL > 0
}

void hang_gpu_on(const char *event)
{
  G_UNUSED(event);

#if DAGOR_DBGLEVEL > 0
  G_ASSERT(strlen(event) < eastl::extent<decltype(hang_on_event)>::value);
  strncpy(hang_on_event, event, eastl::extent<decltype(hang_on_event)>::value);
#endif // DAGOR_DBGLEVEL > 0
}

void hang_if_requested(const char *event)
{
  G_UNUSED(event);

#if DAGOR_DBGLEVEL > 0
  if (hanger && hang_on_event[0] && event && strcmp(event, hang_on_event) == 0)
  {
    hanger();
    hang_on_event[0] = 0;
  }
#endif // DAGOR_DBGLEVEL > 0
}

} // namespace d3dhang