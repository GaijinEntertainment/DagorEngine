#include <soundSystem/debug.h>

class DataBlock;
class TMatrix4;
namespace sndsys
{
void debug_trace_info(const char *, ...) {}
void debug_trace_warn(const char *, ...) {}
void debug_trace_err(const char *, ...) {}
void debug_trace_log(const char *, ...) {}
void debug_draw(const TMatrix4 &) {}
void set_draw_audibility(bool) {}
void debug_enum_events() {}
void debug_init(const DataBlock &) {}
} // namespace sndsys
