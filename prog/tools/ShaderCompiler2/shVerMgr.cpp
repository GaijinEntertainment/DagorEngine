#include <shaders/dag_shaders.h>
#include <shaders/shUtils.h>
#include "shLog.h"
#include "shHardwareOpt.h"

static int forceFSH = FSH_AS_HARDWARE;
static bool forceFSHisInited = false;

// return maximum permitted FSH version
int getMaxFSHVersion() { return forceFSH; }
void limitMaxFSHVersion(int f) { forceFSH = f; }

ShHardwareOptions ShHardwareOptions::Defaults(FSHVER_R300);

void ShHardwareOptions::appendOpts(String &fname) const
{
  const char *ver = NULL;
  switch (fshVersion)
  {
    case FSHVER_30: ver = "ps30"; break;
    case FSHVER_40: ver = "ps40"; break;
    case FSHVER_41: ver = "ps41"; break;
    case FSHVER_50: ver = "ps50"; break;
    case FSHVER_60: ver = "ps60"; break;
    case FSHVER_66: ver = "ps66"; break;
  }
  G_ASSERT(ver);

  fname.aprintf(8, ".%s", ver);
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
