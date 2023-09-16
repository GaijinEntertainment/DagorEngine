//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class DataBlock;
namespace Sqrat
{
class Object;
}
struct SQVM;
typedef struct SQVM *HSQUIRRELVM;

Sqrat::Object blk_to_sqrat(HSQUIRRELVM vm, const DataBlock &blk);
void sqrat_to_blk(DataBlock &dest_blk, Sqrat::Object sq_tbl);
