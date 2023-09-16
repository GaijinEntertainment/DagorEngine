#pragma once

#define MAX_TRACE_LEVEL     2
#define DEFAULT_TRACE_LEVEL 1

namespace datacache
{

#if MAX_TRACE_LEVEL > 0
void set_trace_level(int level);
void _do_trace_impl(int level, const char *fmt, ...);
#else
inline void set_trace_level(int) {}
#endif

#if MAX_TRACE_LEVEL > 0
#define DOTRACE1(fmt, ...) _do_trace_impl(1, fmt, ##__VA_ARGS__)
#else
#define DOTRACE1(fmt, ...) \
  do                       \
  {                        \
  } while (0)
#endif

#if MAX_TRACE_LEVEL > 1
#define DOTRACE2(fmt, ...) _do_trace_impl(2, fmt, ##__VA_ARGS__)
#else
#define DOTRACE2(fmt, ...) \
  do                       \
  {                        \
  } while (0)
#endif

#if MAX_TRACE_LEVEL > 2
#define DOTRACE3(fmt, ...) _do_trace_impl(3, fmt, ##__VA_ARGS__)
#else
#define DOTRACE3(fmt, ...) \
  do                       \
  {                        \
  } while (0)
#endif

}; // namespace datacache
