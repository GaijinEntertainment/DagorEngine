// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_mathBase.h>
#include "blk_shared.h"

bool dblk::are_approximately_equal(const DataBlock &lhs, const DataBlock &rhs, float eps)
{
  if (&lhs == &rhs)
    return true;
  if (lhs.paramCount() != rhs.paramCount() || lhs.blockCount() != rhs.blockCount())
    return false;
  // todo: comparison of blocks can be optimized if we know they have same shared, or at least shared->ro
  const char *lhs_name = lhs.getName(lhs.getNameId());
  const char *rhs_name = rhs.getName(rhs.getNameId());
  if (!!lhs_name != !!rhs_name || (lhs_name && strcmp(lhs_name, rhs_name) != 0)) //-V575
    return false;

  auto param1 = lhs.getParamsImpl();
  auto param2 = rhs.getParamsImpl();
  for (uint32_t i = 0, e = lhs.paramCount(); i < e; ++i, ++param1, ++param2)
  {
    if (param1->type != param2->type || strcmp(lhs.getName(param1->nameId), rhs.getName(param2->nameId)) != 0) //-V575
      return false;

    bool eq = false;
    const bool isFloatType = param1->type == DataBlock::TYPE_REAL || param1->type == DataBlock::TYPE_POINT2 ||
                             param1->type == DataBlock::TYPE_POINT3 || param1->type == DataBlock::TYPE_POINT4 ||
                             param1->type == DataBlock::TYPE_MATRIX;
    if (isFloatType)
    {
      const int floatCount = dblk::get_type_size(param1->type) / sizeof(float);
      const float *floatDataA = (const float *)lhs.getParamData(*param1);
      const float *floatDataB = (const float *)rhs.getParamData(*param2);

      for (int j = 0; j < floatCount; ++j)
        if (!is_equal_float(floatDataA[j], floatDataB[j], eps))
        {
          return false; // if one of the floats is not equal, the whole two blocks aren't equal
        }
      eq = true;
    }
    else if (param1->type == DataBlock::TYPE_STRING)
      eq = (strcmp(lhs.getParamData(*param1), rhs.getParamData(*param2)) == 0);
    else
      eq = memcmp(lhs.getParamData(*param1), rhs.getParamData(*param2), dblk::get_type_size(param1->type)) == 0;

    if (!eq)
      return false;
  }

  for (uint32_t i = 0, ie = lhs.blockCount(); i < ie; ++i)
    if (!are_approximately_equal(*lhs.getBlock(i), *rhs.getBlock(i), eps))
      return false;

  return true;
}
