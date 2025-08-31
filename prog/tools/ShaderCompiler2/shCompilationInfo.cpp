// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <shaders/shUtils.h>
#include "shLog.h"
#include "shCompilationInfo.h"

void ShHardwareOptions::appendOptsTo(String &fname) const
{
  auto str = d3d::as_ps_string(fshVersion);
  G_ASSERT(str[0] != '\0');
  fname.aprintf(8, ".%s", str);
}


void ShHardwareOptions::dumpInfo() const { sh_debug(SHLOG_INFO, "Shader hardware options: ps=%s", ShUtils::fsh_version(fshVersion)); }
