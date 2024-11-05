// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <userSystemInfo/systemInfo.h>
#include "uuidgen.h"

namespace systeminfo
{
void private_init() {}
bool get_system_guid(String &res) { return uuidgen::get_uuid(1, res); }
} // namespace systeminfo
