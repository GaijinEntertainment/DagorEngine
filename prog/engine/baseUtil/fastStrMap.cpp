// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_fastStrMap.h>

#ifdef _TARGET_STATIC_LIB
template class FastStrMapT<int, -1>;
template class FastStrMapT<int, 0>;
#endif

#define EXPORT_PULL dll_pull_baseutil_fastStrMap
#include <supp/exportPull.h>
