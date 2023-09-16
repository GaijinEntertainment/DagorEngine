#include <statsd/statsd.h>
#include <debug/dag_debug.h>

namespace statsd
{
static class DagorLogger final : public statsd::ILogger
{
  virtual void debugImpl(const char *fmt, va_list ap) override { cvlogmessage(LOGLEVEL_DEBUG, fmt, ap); }
  virtual void logerrImpl(const char *fmt, va_list ap) override { cvlogmessage(LOGLEVEL_WARN, fmt, ap); }
} dagor_logger;
ILogger *get_dagor_logger() { return &dagor_logger; }

} // namespace statsd
