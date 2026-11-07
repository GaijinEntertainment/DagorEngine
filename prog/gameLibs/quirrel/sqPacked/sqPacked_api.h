// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>

class SqModules;
class DataBlock;

namespace bindquirrel
{
void register_packed_module(SqModules *module_mgr);
SQRESULT push_packed_datablock(HSQUIRRELVM vm, const DataBlock &blk);
} // namespace bindquirrel
