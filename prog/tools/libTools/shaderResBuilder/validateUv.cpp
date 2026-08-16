// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/validateUv.h>
#include <ioSys/dag_dataBlock.h>

void UvValidationSettings::loadValidationSettings(const DataBlock &blk)
{
  validate = blk.getBool("validate", true);
  warnOnly = blk.getBool("warnOnly", true);
}
