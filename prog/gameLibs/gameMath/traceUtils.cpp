// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/traceUtils.h>
#include <util/dag_stlqsort.h>

void TraceMeshFaces::RendinstCache::sort() { stlsort::sort_branchless(cachedData.begin(), cachedData.end()); }
