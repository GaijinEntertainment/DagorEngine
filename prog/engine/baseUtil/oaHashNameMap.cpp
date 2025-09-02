// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_oaHashNameMap.h>

#ifdef _TARGET_STATIC_LIB
#include <util/dag_oaHashNameMapImpl.inl>

template struct OAHashNameMap<false>;
template struct OAHashNameMap<true>;
template struct OAHashNameMap<false, FNV1OAHasher<false>>;
#endif
