#pragma once

namespace das {
  void das_throw(const char * msg);
}
#if (!defined(DAS_ENABLE_EXCEPTIONS)) || (!DAS_ENABLE_EXCEPTIONS)
#define FMT_THROW(x)    das::das_throw(((x).what()))
#endif

#include <fmt/core.h>
#include <fmt/format.h>