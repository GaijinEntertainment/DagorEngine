// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daNet/bitStream.h>
#include <daNet/daNetTypes.h>

#include <util/dag_globDef.h>
#include <memory/dag_framemem.h>

namespace net
{

namespace delta
{

inline danet::BitStream diff_impl(const danet::BitStream &buf0, const danet::BitStream &buf1)
{
  int size0 = buf0.GetNumberOfBytesUsed();
  int size1 = buf1.GetNumberOfBytesUsed();
  int lowerLimit = min(size0, size1);
  int upperLimit = max(size0, size1);

  danet::BitStream ret(upperLimit, framemem_ptr());
  for (int i = 0; i < lowerLimit; ++i)
    *(ret.GetData() + i) = *(buf0.GetData() + i) ^ *(buf1.GetData() + i);

  const danet::BitStream &src = size1 < size0 ? buf0 : buf1;
  for (int i = lowerLimit; i < upperLimit; ++i)
    *(ret.GetData() + i) = *(src.GetData() + i);

  ret.SetWriteOffset(BYTES_TO_BITS(upperLimit));
  return ret;
}

} // namespace delta

} // namespace net
