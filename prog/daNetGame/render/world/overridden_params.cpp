// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/world/overridden_params.h"
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

#define GET_ITEM_F(type, fname)                                                                 \
  type lvl_override::get##fname(const DataBlock *data_blk, const char *name, type def)          \
  {                                                                                             \
    type default_val = dgs_get_settings()->getBlockByNameEx("graphics")->get##fname(name, def); \
    return data_blk ? data_blk->get##fname(name, default_val) : default_val;                    \
  }
OVERRIDE_BLK_FUNC_LIST
#undef GET_ITEM_F