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

    // states that become dynamic via VK_EXT_extended_dynamic_state; filled only when the device
    // supports the extension, otherwise left at zero and kept baked into the pipeline static state.
    // depth-write is intentionally NOT here: at pipeline build it is masked by forceNoZWrite (a
    // render-pass property, not a render-state one), which dynamic state can not track correctly yet.
    uint8_t extCullMode; // raw dagor cull: 0 none, 1 cw, 2 ccw
    uint8_t extDepthTestEnable;
    uint8_t extDepthTestFunc; // VkCompareOp
    uint8_t extDepthBoundsTestEnable;
    uint8_t extStencilTestEnable;
    uint8_t extStencilTestFunc;          // VkCompareOp
    uint8_t extStencilTestOpStencilFail; // VkStencilOp
    uint8_t extStencilTestOpDepthFail;   // VkStencilOp
    uint8_t extStencilTestOpPass;        // VkStencilOp

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

      if (v.extCullMode != extCullMode || v.extDepthTestEnable != extDepthTestEnable || v.extDepthTestFunc != extDepthTestFunc ||
          v.extDepthBoundsTestEnable != extDepthBoundsTestEnable || v.extStencilTestEnable != extStencilTestEnable ||
          v.extStencilTestFunc != extStencilTestFunc || v.extStencilTestOpStencilFail != extStencilTestOpStencilFail ||
          v.extStencilTestOpDepthFail != extStencilTestOpDepthFail || v.extStencilTestOpPass != extStencilTestOpPass)
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
