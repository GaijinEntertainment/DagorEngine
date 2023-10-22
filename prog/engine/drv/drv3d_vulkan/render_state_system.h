#pragma once

#include <3d/dag_drv3d.h>
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
    uint8_t stencilMask;
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

  class Backend
  {
    eastl::vector<GraphicsPipelineStaticState> staticParts;
    eastl::vector<DynamicState> dynamicParts;
    eastl::vector<DriverRenderState> states;

    GraphicsPipelineStaticState extractStaticState(const shaders::RenderState &state, Device &device);
    DynamicState extractDynamicState(const shaders::RenderState &state);

    template <typename StatePart>
    LinearStorageIndex addOrReusePart(eastl::vector<StatePart> &array, const StatePart &new_part)
    {
      auto ref = eastl::find(begin(array), end(array), new_part);

      LinearStorageIndex uniquePartId = eastl::distance(begin(array), ref);
      if (uniquePartId == array.size())
        array.push_back(new_part);

      return uniquePartId;
    };

  public:
    DriverRenderState get(shaders::DriverRenderStateId id) { return states[(uint32_t)id]; }

    const DynamicState &getDynamic(LinearStorageIndex dynamic_id) { return dynamicParts[dynamic_id]; }

    const GraphicsPipelineStaticState &getStatic(LinearStorageIndex static_id) { return staticParts[static_id]; };

    void setRenderStateData(shaders::DriverRenderStateId id, const shaders::RenderState &state, Device &device);
  };

  RenderStateSystem() = default;

  shaders::DriverRenderStateId registerState(DeviceContext &ctx, const shaders::RenderState &state);

private:
  std::atomic<uint32_t> maxId{0};
};

} // namespace drv3d_vulkan
