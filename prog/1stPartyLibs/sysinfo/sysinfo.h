#pragma once
#include <EASTL/string.h>

namespace systeminfo
{

eastl::string get_system_id(); // returns empty string on failure

} // namespace systeminfo
