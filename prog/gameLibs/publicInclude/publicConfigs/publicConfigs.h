//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>

namespace pubcfg
{

typedef void (*pubcfg_cb_t)(dag::ConstSpan<char> data, void *arg);

void init(const char *cache_dir, const char *default_public_configs_url);
void shutdown();

bool get(const char *filename, pubcfg_cb_t cb, void *arg);

} // namespace pubcfg
