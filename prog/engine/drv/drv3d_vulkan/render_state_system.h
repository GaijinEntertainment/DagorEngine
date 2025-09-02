// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_renderStates.h>
#include <atomic>
#include "shader.h"

namespace drv3d_vulkan
{

class DeviceContext;

class RenderStateSystem
{
public:
  struct DynamicState
  {
    float depthBias;
    float slopedDepthBias;
    uint8_t stencilRef;
    uint16_t stencilMask;
    bool enableScissor;

    bool operator==(const DynamicState &v) const
    {
      if (v.depthBias != depthBias)
        return false;

      if (v.slopedDepthBias != slopedDepthBias)
        return false;

      if (v.stencilRef != stencilRef)
        return false;

      if (v.stencilMask != stencilMask)
        return false;

      if (v.enableScissor != enableScissor)
        return false;

      return true;
    }
  };

  RenderStateSystem() = default;

  shaders::DriverRenderStateId registerState(DeviceContext &ctx, const shaders::RenderState &state);

private:
  std::atomic<uint32_t> maxId{0};
};

class RenderStateSystemBackend
{
  dag::Vector<GraphicsPipelineStaticState> staticParts;
  dag::Vector<RenderStateSystem::DynamicState> dynamicParts;
  dag::Vector<DriverRenderState> states;

  GraphicsPipelineStaticState extractStaticState(const shaders::RenderState &state);
  RenderStateSystem::DynamicState extractDynamicState(const shaders::RenderState &state);

  template <typename StatePart>
  LinearStorageIndex addOrReusePart(dag::Vector<StatePart> &array, const StatePart &new_part)
  {
    auto ref = eastl::find(begin(array), end(array), new_part);

    LinearStorageIndex uniquePartId = eastl::distance(begin(array), ref);
    if (uniquePartId == array.size())
      array.push_back(new_part);

    return uniquePartId;
  };

public:
  DriverRenderState get(shaders::DriverRenderStateId id) { return states[(uint32_t)id]; }

  const RenderStateSystem::DynamicState &getDynamic(LinearStorageIndex dynamic_id) { return dynamicParts[dynamic_id]; }

  const GraphicsPipelineStaticState &getStatic(LinearStorageIndex static_id) { return staticParts[static_id]; };

  void setRenderStateData(shaders::DriverRenderStateId id, const shaders::RenderState &state);
};

} // namespace drv3d_vulkan
