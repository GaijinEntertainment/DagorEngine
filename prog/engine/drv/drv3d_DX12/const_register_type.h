#pragma once

#include <EASTL/iterator.h>
#include <EASTL/algorithm.h>

#include "constants.h"


namespace drv3d_dx12
{

struct ConstRegisterType
{
  uint32_t components[SHADER_REGISTER_ELEMENTS];
};
inline bool operator==(const ConstRegisterType &l, const ConstRegisterType &r)
{
  return eastl::equal(eastl::begin(l.components), eastl::end(l.components), eastl::begin(r.components));
}

} // namespace drv3d_dx12
