//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

class DataBlock;

int buildVromfs(DataBlock *explicit_build_rules, dag::ConstSpan<const char *> argv);