#pragma once

#include "sqmodules.h"

namespace sqm
{

SqModules::string format_string(const char *fmt, ...);
void append_format(SqModules::string &s, const char *fmt, ...);

}
