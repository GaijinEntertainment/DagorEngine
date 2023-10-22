#pragma once

#include "shadersBinaryData.h"
#include <generic/dag_span.h>
#include <util/dag_stdint.h>
#include <util/dag_bfind.h>

namespace mapbinarysearch
{
// return index of key string in sorted string array (uses binary search)
int bfindStrId(dag::ConstSpan<StrHolder> map, const char *key);

// unpacks key/data from code (HIGH16:LOW16)=data:key
inline int unpackKey(uint32_t code) { return code & 0xFFFF; }
inline int unpackData(uint32_t code) { return code >> 16; }
} // namespace mapbinarysearch
