#include "shadersBinaryData.h"
#include <debug/dag_debug.h>


#if DAGOR_DBGLEVEL > 0
const shaderbindump::ShaderClass *shaderbindump::shClassUnderDebug = NULL;
static int shaderOrdId = -1;
#endif


void debug_next_shader()
{
#if DAGOR_DBGLEVEL > 0
  if (shaderOrdId >= shBinDump().classes.size() - 1)
    return;

  shaderOrdId++;
  shaderbindump::shClassUnderDebug = &shBinDump().classes[shaderOrdId];
  debug("Selected shader for debug: '%s'", shaderbindump::shClassUnderDebug->name.data());
#endif
}

void debug_prev_shader()
{
#if DAGOR_DBGLEVEL > 0
  if (shaderOrdId == -1)
    return;

  shaderOrdId--;
  if (shaderOrdId == -1)
  {
    debug("Deselected shader for debug");
    shaderbindump::shClassUnderDebug = NULL;
  }
  else
  {
    shaderbindump::shClassUnderDebug = &shBinDump().classes[shaderOrdId];
    debug("Selected shader: '%s'", shaderbindump::shClassUnderDebug->name.data());
  }
#endif
}
