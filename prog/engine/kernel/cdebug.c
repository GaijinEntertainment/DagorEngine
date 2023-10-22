#include <debug/dag_debug.h>

#if DAGOR_DBGLEVEL < 1 && !DAGOR_FORCE_LOGS
void cdebug(const char *fmt, ...) { (void)(fmt); }
void cdebug_(const char *fmt, ...) { (void)(fmt); }
void debug_dump_stack(const char *text, int skip_frames)
{
  (void)(text);
  (void)(skip_frames);
}
#endif
