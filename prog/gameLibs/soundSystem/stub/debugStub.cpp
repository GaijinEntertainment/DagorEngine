#include <soundSystem/debug.h>

class DataBlock;
namespace sndsys
{
void debug_trace_info(const char *, ...) {}
void debug_trace_warn(const char *, ...) {}
void debug_trace_err(const char *, ...) {}
void debug_trace_log(const char *, ...) {}
void debug_draw() {}
void set_enable_debug_draw(bool) {}
bool get_enable_debug_draw() { return false; }
void set_draw_audibility(bool) {}
void debug_enum_events() {}
void debug_init(const DataBlock &) {}
} // namespace sndsys
