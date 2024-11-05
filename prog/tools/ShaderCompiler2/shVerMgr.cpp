// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <shaders/shUtils.h>
#include "shLog.h"
#include "shHardwareOpt.h"

static d3d::shadermodel::Version forceFSH = d3d::smAny;
static bool forceFSHisInited = false;

// return maximum permitted FSH version
d3d::shadermodel::Version getMaxFSHVersion() { return forceFSH; }
void limitMaxFSHVersion(d3d::shadermodel::Version f) { forceFSH = f; }

ShHardwareOptions ShHardwareOptions::Defaults(4.0_sm);

void ShHardwareOptions::appendOpts(String &fname) const
{
  auto str = d3d::as_ps_string(fshVersion);
  G_ASSERT(str[0] != '\0');
  fname.aprintf(8, ".%s", str);
}


void ShHardwareOptions::dumpInfo() const { sh_debug(SHLOG_INFO, "Shader hardware options: ps=%s", ShUtils::fsh_version(fshVersion)); }


// init filename from source shader file name & options
void ShVariantName::init(const char *dest_base_filename, const ShHardwareOptions &_opt)
{
  opt = _opt;
  dest = dest_base_filename;
  opt.appendOpts(dest);
  dest = dest + ".lib.bin";
}
