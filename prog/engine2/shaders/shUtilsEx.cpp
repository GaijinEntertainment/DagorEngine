#include <shaders/dag_shaderCommon.h>
#include <shaders/shUtils.h>
#include "shadersBinaryData.h"

namespace ShUtils
{
void shcod_dump_global(const int *ptr, int num) { shcod_dump(dag::ConstSpan<int>(ptr, num), &shBinDump().globVars); }
} // namespace ShUtils
