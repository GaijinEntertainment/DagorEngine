// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat/sqratFunction.h>

namespace bindquirrel
{

void init_file_drop_handler(Sqrat::Function func);
void release_file_drop_handler();

} // namespace bindquirrel