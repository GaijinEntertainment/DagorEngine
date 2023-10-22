#pragma once

#include <3d/dag_drv3dConsts.h>

#include "vulkan_device.h"

namespace drv3d_vulkan
{

class DeviceContext;
class Image;
class Device;

// platforms like mac/ios can't handle bgra vertex layout
#if VULKAN_PLATFORM_WITHOUT_BGRA_VERTEX
constexpr VkFormat E3DCOLOR_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
#else
constexpr VkFormat E3DCOLOR_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
#endif

BEGIN_BITFIELD_TYPE(VertexAttributeDesc, uint32_t)

  ADD_BITFIELD_MEMBER(used, 0, 1)
  ADD_BITFIELD_MEMBER(location, 1, 5)
  ADD_BITFIELD_MEMBER(binding, 6, 2)
  ADD_BITFIELD_MEMBER(formatIndex, 8, 7)
  ADD_BITFIELD_MEMBER(offset, 15, 17)

  VkVertexInputAttributeDescription toVulkan() const
  {
    VkVertexInputAttributeDescription result;
    result.location = location;
    result.binding = binding;
    result.format = VkFormat(uint32_t(formatIndex));
    result.offset = offset;
    return result;
  }

END_BITFIELD_TYPE()

typedef carray<VertexAttributeDesc, MAX_VERTEX_ATTRIBUTES> VertexAttributeSet;

struct GraphicsPipelineStaticState
{
  union
  {
    struct
    {
      uint32_t conservativeRasterEnable : 1;
      uint32_t cullMode : 2;
      uint32_t depthTestEnable : 1;
      uint32_t depthWriteEnable : 1;
      uint32_t depthTestFunc : 3;
      uint32_t depthBoundsEnable : 1;
      uint32_t depthClipEnable : 1;
      uint32_t stencilTestEnable : 1;
      uint32_t stencilTestFunc : 3;
      uint32_t stencilTestOpStencilFail : 3;
      uint32_t stencilTestOpDepthFail : 3;
      uint32_t stencilTestOpPass : 3;
      uint32_t indenpendentBlendEnabled : 1;
      // stores power of two of count plus one so that 0 is 0 not 1
      uint32_t forcedSampleCountExponent : 3;
      uint32_t alphaToCoverage : 1;
    };
    uint32_t bitField;
  };

  uint32_t colorMask;

  struct BlendState
  {
    uint32_t blendEnable : 3;              // 1 bit used, 2 free
    uint32_t blendSeparateAlphaEnable : 3; // 1 bit used, 2 free
    uint32_t blendOp : 3;
    uint32_t blendOpAlpha : 3;
    uint32_t blendSrcFactor : 5;
    uint32_t blendDstFactor : 5;
    uint32_t blendSrcAlphaFactor : 5;
    uint32_t blendDstAlphaFactor : 5;
  };
  BlendState blendStates[shaders::RenderState::NumIndependentBlendParameters];

  static_assert(sizeof(BlendState) == 4);
  static_assert(sizeof(blendStates) == shaders::RenderState::NumIndependentBlendParameters * sizeof(BlendState));

  bool operator==(const GraphicsPipelineStaticState &v) const
  {
    if (v.bitField != bitField)
      return false;

    if (v.colorMask != colorMask)
      return false;

    const uint32_t blendBytesToCompare = indenpendentBlendEnabled ? sizeof(blendStates) : sizeof(BlendState);
    if (memcmp(v.blendStates, blendStates, blendBytesToCompare) != 0)
      return false;

    return true;
  }

  void setForcedSamplerCount(uint32_t cnt)
  {
    G_ASSERT(cnt <= 16);
    // yes, not shift, just compare result adding up
    uint32_t pow = cnt > 0; //-V602
    pow += cnt > 1;         //-V602
    pow += cnt > 2;         //-V602
    pow += cnt > 4;         //-V602
    pow += cnt > 8;         //-V602
    forcedSampleCountExponent = pow;
  }

  uint32_t getForcedSamplerCount() const { return (1u << forcedSampleCountExponent) >> 1u; }

  void reset()
  {
    bitField = 0;
    cullMode = VK_CULL_MODE_BACK_BIT;
    depthTestEnable = 1;
    depthWriteEnable = 1;
    depthTestFunc = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthClipEnable = 0;
    depthBoundsEnable = 0;
    stencilTestEnable = 0;
    stencilTestFunc = VK_COMPARE_OP_ALWAYS;
    stencilTestOpStencilFail = VK_STENCIL_OP_KEEP;
    stencilTestOpDepthFail = VK_STENCIL_OP_KEEP;
    stencilTestOpPass = VK_STENCIL_OP_KEEP;
    indenpendentBlendEnabled = 0;
    for (auto &blendState : blendStates)
    {
      blendState.blendEnable = 0;
      blendState.blendSeparateAlphaEnable = 0;
      blendState.blendOp = VK_BLEND_OP_ADD;
      blendState.blendOpAlpha = VK_BLEND_OP_ADD;
      blendState.blendSrcFactor = VK_BLEND_FACTOR_ONE;
      blendState.blendDstFactor = VK_BLEND_FACTOR_ZERO;
      blendState.blendSrcAlphaFactor = VK_BLEND_FACTOR_ONE;
      blendState.blendDstAlphaFactor = VK_BLEND_FACTOR_ZERO;
    }
    forcedSampleCountExponent = 0;
    conservativeRasterEnable = 0;
    colorMask = 0;
    alphaToCoverage = 0;
  }
};

BEGIN_BITFIELD_TYPE(InputStreamSet, uint8_t)

  ADD_BITFIELD_ARRAY(used, 0, 1, MAX_VERTEX_INPUT_STREAMS)
  ADD_BITFIELD_ARRAY(stepRate, MAX_VERTEX_INPUT_STREAMS, 1, MAX_VERTEX_INPUT_STREAMS)

  inline VkVertexInputBindingDescription toVulkan(uint32_t index, uint32_t stride) const
  {
    VkVertexInputBindingDescription vbd;
    vbd.binding = index;
    vbd.inputRate = (VkVertexInputRate)(uint32_t)stepRate[index];
    vbd.stride = stride;
    return vbd;
  }

END_BITFIELD_TYPE()

struct InputLayout
{
  InputStreamSet streams;
  VertexAttributeSet attribs;

  void fromVdecl(const VSDTYPE *decl);

  void reset()
  {
    streams = InputStreamSet(0);
    mem_set_0(attribs);
  }

  inline InputLayout &operator=(const InputLayout &r)
  {
    streams = r.streams;
    attribs = r.attribs;

    return *this;
  }

  typedef InputLayout CreationInfo;

  InputLayout() = default;
  InputLayout(const CreationInfo &info) { *this = info; };

  void addToContext(DeviceContext &, InputLayoutID, const CreationInfo &){};
  void removeFromContext(DeviceContext &, InputLayoutID){};

  bool isSame(const CreationInfo &info);
  uint32_t getHash32() const
  {
    return mem_hash_fnv1<32>((const char *)&streams, sizeof(streams)) ^ mem_hash_fnv1<32>((const char *)&attribs, sizeof(attribs));
  }
  void onDuplicateAddition() {}
  static constexpr bool alwaysUnique() { return false; }
  static constexpr bool isRemovalPending() { return false; }
  bool release() { return true; }

  static InputLayoutID makeID(LinearStorageIndex index) { return InputLayoutID(index); }
  static LinearStorageIndex getIndexFromID(InputLayoutID id) { return id.get(); }
  static bool checkID(InputLayoutID) { return true; }
};

inline bool operator==(const InputLayout &l, const InputLayout &r)
{
  return l.streams == r.streams && mem_eq(l.attribs, r.attribs.data());
}

struct GraphicsPipelineStateDescription
{
  static constexpr int VERSION = 12;
  DriverRenderState renderState;
  carray<uint32_t, MAX_VERTEX_INPUT_STREAMS> strides; //-V730_NOINIT
  InputLayoutID inputLayout;                          //-V730_NOINIT
  uint8_t polygonLine = 0;

#if DAGOR_DBGLEVEL > 0
  // use direct compare only for debug
  bool compare(const GraphicsPipelineStateDescription &l, bool only_hash_effective_data = false) const
  {
    if (only_hash_effective_data)
    {
      if (renderState.staticIdx != l.renderState.staticIdx)
        return false;
    }
    else
    {
      if (!renderState.equal(l.renderState))
        return false;
    }
    if (!(inputLayout == l.inputLayout))
      return false;
    if (polygonLine != l.polygonLine)
      return false;

    for (int i = 0; i != MAX_VERTEX_INPUT_STREAMS; ++i)
      if (strides[i] != l.strides[i])
        return false;

    return true;
  }
#endif

  void reset()
  {
    inputLayout = InputLayoutID::Null();
    renderState = DriverRenderState();
    mem_set_0(strides);
    polygonLine = 0;
  }

  // This patches the vertex attribute usage and vertex buffer bindings
  void maskInputs(uint32_t attribute_mask, InputLayout &layout_data)
  {
    uint32_t bindingMask = 0;
    for (uint32_t i = 0; i < MAX_VERTEX_ATTRIBUTES; ++i)
    {
      if (layout_data.attribs[i].used)
      {
        if (0 == (attribute_mask & (1u << layout_data.attribs[i].location)))
        {
          layout_data.attribs[i] = VertexAttributeDesc(0);
        }
        else
        {
          bindingMask |= 1u << layout_data.attribs[i].binding;
        }
      }
    }

    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      if (0 == (bindingMask & (1u << i)))
      {
        strides[i] = 0;
        layout_data.streams.used[i] = 0;
        layout_data.streams.stepRate[i] = 0;
      }
    }
  }
};

} // namespace drv3d_vulkan