#include "helpers.h"
#include <cstdarg>

namespace sqm
{

SqModules::string format_string(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  va_list ap_copy;
  va_copy(ap_copy, ap);
  int len = vsnprintf(nullptr, 0, fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0)
  {
    va_end(ap);
    return {};
  }

  SqModules::string s;
  s.resize(len + 1);
  vsnprintf(&s[0], s.size(), fmt, ap);
  va_end(ap);

  s.resize(len);
  return s; // no copy due to RVO or move
}

void append_format(SqModules::string &s, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  va_list ap_copy;
  va_copy(ap_copy, ap);
  const int len = vsnprintf(nullptr, 0, fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0)
  {
    va_end(ap);
    return;
  }

  const size_t old = s.size();
  s.resize(old + len + 1);
  vsnprintf(&s[old], len + 1, fmt, ap);
  va_end(ap);

  s.resize(old + len);
}

}
