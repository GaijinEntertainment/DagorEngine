#pragma once

#include "driver.h"
#include "states.h"
#include <3d/dag_sampler.h>

namespace drv3d_dx11
{

class SamplerStateObject
{
public:
  static SamplerState::Key makeKey(const d3d::SamplerInfo &sampler_info);

  SamplerStateObject(const d3d::SamplerInfo &sampler_info) { samplerKey = makeKey(sampler_info); }

  auto getSamplerKey() const { return samplerKey; }

private:
  SamplerState::Key samplerKey;
};

} // namespace drv3d_dx11
