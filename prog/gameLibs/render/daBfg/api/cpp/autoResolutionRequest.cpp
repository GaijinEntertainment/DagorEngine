// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/autoResolutionRequest.h>

#include <common/genericPoint.h>
#include <frontend/internalRegistry.h>


namespace dabfg
{

template <int D>
IPoint<D> AutoResolutionRequest<D>::get() const
{
  if (!eastl::holds_alternative<IPoint<D>>(provider->resolutions[autoResTypeId]))
  {
    // Very unlikely to actually happen, so this message is enough
    logerr("daBfg: attempted to get an auto-resolution with a different type! Returning zeros!");
    return {};
  }
  const auto &res = eastl::get<IPoint<D>>(provider->resolutions[autoResTypeId]);
  return scale_by(res, multiplier);
}

template class AutoResolutionRequest<2>;
template class AutoResolutionRequest<3>;

} // namespace dabfg
