// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mathUtils.h>
#include <generic/dag_carray.h>
#include <fcntl.h>
#include <linux/input.h>

template <size_t MaxBits>
class InputBitArray
{
protected:
  using Elem = unsigned long;
  static constexpr size_t ElemBits = sizeof(Elem) * 8;
  static constexpr size_t ArraySize = div_ceil<unsigned>(MaxBits, ElemBits);

  // We could use ConstSizeBitArrayBase here, but it don't expose data array in public interface
  carray<Elem, ArraySize> data;

public:
  bool test(unsigned bit) const
  {
    unsigned bitIdx = bit % ElemBits;
    Elem mask = 1UL << bitIdx;
    return (data[bit / ElemBits] & mask) != 0;
  }

  bool evdevRead(int fd, int type)
  {
    // no need to clear array here. on success full array will be filled by kernel
    return ioctl(fd, EVIOCGBIT(type, data.size() * sizeof(Elem)), data.data()) >= 0;
  }
};

class KeysBitArray : public InputBitArray<KEY_MAX>
{
public:
  bool evdevRead(int fd) { return ioctl(fd, EVIOCGKEY(KEY_MAX), data.data()) >= 0; }
};

inline bool read_axis(int jfd, int axis, input_absinfo &abs_info)
{ //
  return ioctl(jfd, EVIOCGABS(axis), &abs_info) >= 0;
}
