//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/initializer_list.h>

#include <debug/dag_assert.h>

#include <drv/3d/dag_consts.h>

class Sbuffer;
class BaseTexture;
struct RaytraceBottomAccelerationStructure;

/**
 * @brief The ResourceBarrierDesc class represents the input type for 'd3d::resource_barrier'.
 * It allows one function to handle multiple input data layouts.
 * Inputs can be simple single values, arrays of values, pointers to values, and initializer lists of values.
 * To interpret the stored values, use the provided enumerate functions to get the buffer and texture barriers.
 * For more details on resource barriers, see https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers
 */
class ResourceBarrierDesc
{
  // special count value to distinguish array of values from single value
  static constexpr unsigned single_element_count = ~0u;
  union
  {
    Sbuffer *buffer;
    Sbuffer *const *buffers;
  };
  union
  {
    BaseTexture *texture;
    BaseTexture *const *textures;
  };
  union
  {
    RaytraceBottomAccelerationStructure *blas;
    RaytraceBottomAccelerationStructure *const *blases;
  };
  union
  {
    ResourceBarrier bufferState;
    const ResourceBarrier *bufferStates;
  };
  union
  {
    ResourceBarrier textureState;
    const ResourceBarrier *textureStates;
  };
  union
  {
    unsigned textureSubResIndex;
    const unsigned *textureSubResIndices;
  };
  union
  {
    unsigned textureSubResRange;
    const unsigned *textureSubResRanges;
  };
  unsigned bufferCount = 0;
  unsigned textureCount = 0;
  unsigned blasCount = 0;

public:
  /**
   * @brief Default constructor for ResourceBarrierDesc.
   */
  ResourceBarrierDesc() :
    buffer{nullptr},
    texture{nullptr},
    blas{nullptr},
    bufferStates{nullptr},
    textureStates{nullptr},
    textureSubResIndices{nullptr},
    textureSubResRanges{nullptr}
  {}

  /**
   * @brief Copy constructor for ResourceBarrierDesc.
   * @param desc The ResourceBarrierDesc object to be copied.
   */
  ResourceBarrierDesc(const ResourceBarrierDesc &) = default;

  /**
   * @brief Constructor for ResourceBarrierDesc with a single buffer and resource barrier.
   * @param buf The buffer.
   * @param rb The resource barrier.
   */
  ResourceBarrierDesc(Sbuffer *buf, ResourceBarrier rb) : buffer{buf}, bufferState{rb}, bufferCount{single_element_count} {}

  /**
   * @brief Constructor for ResourceBarrierDesc with an array of buffers and resource barriers.
   * @param bufs The array of buffers.
   * @param rb The array of resource barriers.
   * @param count The number of buffers and resource barriers.
   */
  ResourceBarrierDesc(Sbuffer *const *bufs, const ResourceBarrier *rb, unsigned count) :
    buffers{bufs}, bufferStates{rb}, bufferCount{count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with a fixed-size array of buffers and resource barriers.
   * @tparam N The size of the fixed-size array.
   * @param bufs The fixed-size array of buffers.
   * @param rb The fixed-size array of resource barriers.
   */
  template <unsigned N>
  ResourceBarrierDesc(Sbuffer *(&bufs)[N], ResourceBarrier (&rb)[N]) : buffers{bufs}, bufferStates{rb}, bufferCount{N}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with initializer lists of buffers and resource barriers.
   * @param bufs The initializer list of buffers.
   * @param rb The initializer list of resource barriers.
   */
  ResourceBarrierDesc(std::initializer_list<Sbuffer *> bufs, std::initializer_list<ResourceBarrier> rb) :
    buffers{bufs.begin()}, bufferStates{rb.begin()}, bufferCount{static_cast<unsigned>(bufs.size())}
  {
    G_ASSERT(bufs.size() == rb.size());
  }

  /**
   * @brief Constructor for ResourceBarrierDesc with a single texture, resource barrier, and sub-resource index and range.
   * @param tex The texture.
   * @param rb The resource barrier.
   * @param sub_res_index The sub-resource index.
   * @param sub_res_range The sub-resource range.
   */
  ResourceBarrierDesc(BaseTexture *tex, ResourceBarrier rb, unsigned sub_res_index, unsigned sub_res_range) :
    texture{tex},
    textureState{rb},
    textureSubResIndex{sub_res_index},
    textureSubResRange{sub_res_range},
    textureCount{single_element_count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with an array of textures, resource barriers, sub-resource indices, and sub-resource
   * ranges.
   * @param texs The array of textures.
   * @param rb The array of resource barriers.
   * @param sub_res_index The array of sub-resource indices.
   * @param sub_res_range The array of sub-resource ranges.
   * @param count The number of textures, resource barriers, sub-resource indices, and sub-resource ranges.
   */
  ResourceBarrierDesc(BaseTexture *const *texs, const ResourceBarrier *rb, const unsigned *sub_res_index,
    const unsigned *sub_res_range, unsigned count) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with a fixed-size array of textures, resource barriers, sub-resource indices, and
   * sub-resource ranges.
   * @tparam N The size of the fixed-size array.
   * @param texs The fixed-size array of textures.
   * @param rb The fixed-size array of resource barriers.
   * @param sub_res_index The fixed-size array of sub-resource indices.
   * @param sub_res_range The fixed-size array of sub-resource ranges.
   */
  template <unsigned N>
  ResourceBarrierDesc(BaseTexture *(&texs)[N], ResourceBarrier (&rb)[N], unsigned (&sub_res_index)[N], unsigned (&sub_res_range)[N]) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{N}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with initializer lists of textures, resource barriers, sub-resource indices, and
   * sub-resource ranges.
   * @param texs The initializer list of textures.
   * @param rb The initializer list of resource barriers.
   * @param sub_res_index The initializer list of sub-resource indices.
   * @param sub_res_range The initializer list of sub-resource ranges.
   */
  ResourceBarrierDesc(std::initializer_list<BaseTexture *> texs, std::initializer_list<ResourceBarrier> rb,
    std::initializer_list<unsigned> sub_res_index, std::initializer_list<unsigned> sub_res_range) :
    textures{texs.begin()},
    textureStates{rb.begin()},
    textureSubResIndices{sub_res_index.begin()},
    textureSubResRanges{sub_res_range.begin()},
    textureCount{static_cast<unsigned>(texs.size())}
  {
    G_ASSERT(texs.size() == rb.size());
    G_ASSERT(texs.size() == sub_res_index.size());
    G_ASSERT(texs.size() == sub_res_range.size());
  }

  /**
   * @brief Constructor for ResourceBarrierDesc with a single bottom level acceleration structure.
   * @param blas The bottom level acceleration structure.
   */
  ResourceBarrierDesc(RaytraceBottomAccelerationStructure *blas) : blas{blas}, blasCount{single_element_count} {}

  /**
   * @brief Constructor for ResourceBarrierDesc with an array of bottom level acceleration structures.
   * @param blases The array of bottom level acceleration structures.
   * @param count The number of bottom level acceleration structures.
   */
  ResourceBarrierDesc(RaytraceBottomAccelerationStructure *const *blases, unsigned count) : blases{blases}, blasCount{count} {}

  /**
   * @brief Constructor for ResourceBarrierDesc with a fixed-size array of bottom level acceleration structures.
   * @tparam N The size of the fixed-size array.
   * @param blases The fixed-size array of bottom level acceleration structures.
   */
  template <unsigned N>
  ResourceBarrierDesc(RaytraceBottomAccelerationStructure *(&blases)[N]) : blases{blases}, blasCount{N}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with initializer lists of bottom level acceleration structures.
   * @param blases The initializer list of bottom level acceleration structures.
   */
  ResourceBarrierDesc(std::initializer_list<RaytraceBottomAccelerationStructure *> blases) :
    blases{blases.begin()}, blasCount{static_cast<unsigned>(blases.size())}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with arrays of buffers, resource barriers, textures, resource barriers, sub-resource
   * indices, and sub-resource ranges.
   * @param bufs The array of buffers.
   * @param b_rb The array of resource barriers for buffers.
   * @param b_count The number of buffers and resource barriers for buffers.
   * @param texs The array of textures.
   * @param t_rb The array of resource barriers for textures.
   * @param t_sub_res_index The array of sub-resource indices for textures.
   * @param t_sub_res_range The array of sub-resource ranges for textures.
   * @param t_count The number of textures, resource barriers for textures, sub-resource indices for textures, and sub-resource ranges
   * for textures.
   */
  ResourceBarrierDesc(Sbuffer *const *bufs, const ResourceBarrier *b_rb, unsigned b_count, BaseTexture *const *texs,
    const ResourceBarrier *t_rb, const unsigned *t_sub_res_index, const unsigned *t_sub_res_range, unsigned t_count) :
    buffers{bufs},
    bufferStates{b_rb},
    bufferCount{b_count},
    textures{texs},
    textureStates{t_rb},
    textureSubResIndices{t_sub_res_index},
    textureSubResRanges{t_sub_res_range},
    textureCount{t_count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with a single buffer, resource barrier, array of textures, resource barriers,
   * sub-resource indices, and sub-resource ranges.
   * @param buf The buffer.
   * @param b_rb The resource barrier for the buffer.
   * @param texs The array of textures.
   * @param t_rb The array of resource barriers for textures.
   * @param t_sub_res_index The array of sub-resource indices for textures.
   * @param t_sub_res_range The array of sub-resource ranges for textures.
   * @param t_count The number of textures, resource barriers for textures, sub-resource indices for textures, and sub-resource ranges
   * for textures.
   */
  ResourceBarrierDesc(Sbuffer *buf, ResourceBarrier b_rb, BaseTexture *const *texs, const ResourceBarrier *t_rb,
    const unsigned *t_sub_res_index, const unsigned *t_sub_res_range, unsigned t_count) :
    buffer{buf},
    bufferState{b_rb},
    bufferCount{single_element_count},
    textures{texs},
    textureStates{t_rb},
    textureSubResIndices{t_sub_res_index},
    textureSubResRanges{t_sub_res_range},
    textureCount{t_count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with arrays of buffers, resource barriers, a single texture, resource barrier,
   * sub-resource index, and sub-resource range.
   * @param bufs The array of buffers.
   * @param b_rb The array of resource barriers for buffers.
   * @param b_count The number of buffers and resource barriers for buffers.
   * @param tex The texture.
   * @param t_rb The resource barrier for the texture.
   * @param t_sub_res_index The sub-resource index for the texture.
   * @param t_sub_res_range The sub-resource range for the texture.
   */
  ResourceBarrierDesc(Sbuffer *const *bufs, const ResourceBarrier *b_rb, unsigned b_count, BaseTexture *tex, ResourceBarrier t_rb,
    unsigned t_sub_res_index, unsigned t_sub_res_range) :
    buffers{bufs},
    bufferStates{b_rb},
    bufferCount{b_count},
    texture{tex},
    textureState{t_rb},
    textureSubResIndex{t_sub_res_index},
    textureSubResRange{t_sub_res_range},
    textureCount{single_element_count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with a single buffer, resource barrier, a single texture, resource barrier,
   * sub-resource index, and sub-resource range.
   * @param buf The buffer.
   * @param b_rb The resource barrier for the buffer.
   * @param tex The texture.
   * @param t_rb The resource barrier for the texture.
   * @param t_sub_res_index The sub-resource index for the texture.
   * @param t_sub_res_range The sub-resource range for the texture.
   */
  ResourceBarrierDesc(Sbuffer *buf, ResourceBarrier b_rb, BaseTexture *tex, ResourceBarrier t_rb, unsigned t_sub_res_index,
    unsigned t_sub_res_range) :
    buffer{buf},
    bufferState{b_rb},
    bufferCount{single_element_count},
    texture{tex},
    textureState{t_rb},
    textureSubResIndex{t_sub_res_index},
    textureSubResRange{t_sub_res_range},
    textureCount{single_element_count}
  {}

  /**
   * @brief Constructor for ResourceBarrierDesc with initializer lists of buffers, resource barriers, textures, resource barriers,
   * sub-resource indices, and sub-resource ranges.
   * @param bufs The initializer list of buffers.
   * @param buf_rb The initializer list of resource barriers for buffers.
   * @param texs The initializer list of textures.
   * @param tex_rb The initializer list of resource barriers for textures.
   * @param sub_res_index The initializer list of sub-resource indices for textures.
   * @param sub_res_range The initializer list of sub-resource ranges for textures.
   */
  ResourceBarrierDesc(std::initializer_list<Sbuffer *> bufs, std::initializer_list<ResourceBarrier> buf_rb,
    std::initializer_list<BaseTexture *> texs, std::initializer_list<ResourceBarrier> tex_rb,
    std::initializer_list<unsigned> sub_res_index, std::initializer_list<unsigned> sub_res_range) :
    buffers{bufs.begin()},
    bufferStates{buf_rb.begin()},
    bufferCount{static_cast<unsigned>(bufs.size())},
    textures{texs.begin()},
    textureStates{tex_rb.begin()},
    textureSubResIndices{sub_res_index.begin()},
    textureSubResRanges{sub_res_range.begin()},
    textureCount{static_cast<unsigned>(texs.size())}
  {
    G_ASSERT(bufs.size() == buf_rb.size());
    G_ASSERT(texs.size() == tex_rb.size());
    G_ASSERT(texs.size() == sub_res_index.size());
    G_ASSERT(texs.size() == sub_res_range.size());
  }

  /**
   * @brief Constructor for ResourceBarrierDesc with a single resource barrier.
   * The expected use case is that 'rb' has the RB_FLUSH_UAV flag set for all pending UAV access.
   * @param rb The resource barrier.
   */
  explicit ResourceBarrierDesc(ResourceBarrier rb) : buffer{nullptr}, bufferState{rb}, bufferCount{single_element_count} {}

  /**
   * @brief Enumerates the buffer barriers and calls the provided callback function for each buffer barrier.
   * @tparam T The type of the callback function.
   * @param clb The callback function that takes a buffer and a resource barrier as arguments.
   */
  template <typename T>
  void enumerateBufferBarriers(T clb)
  {
    if (single_element_count == bufferCount)
    {
      clb(buffer, bufferState);
    }
    else
    {
      for (unsigned i = 0; i < bufferCount; ++i)
      {
        clb(buffers[i], bufferStates[i]);
      }
    }
  }

  /**
   * @brief Enumerates the texture barriers and calls the provided callback function for each texture barrier.
   * @tparam T The type of the callback function.
   * @param clb The callback function that takes a texture, a resource barrier, a sub-resource index, and a sub-resource range as
   * arguments.
   */
  template <typename T>
  void enumerateTextureBarriers(T clb)
  {
    if (single_element_count == textureCount)
    {
      clb(texture, textureState, textureSubResIndex, textureSubResRange);
    }
    else
    {
      for (unsigned i = 0; i < textureCount; ++i)
      {
        clb(textures[i], textureStates[i], textureSubResIndices[i], textureSubResRanges[i]);
      }
    }
  }

  /**
   * @brief Enumerates the bottom level acceleration structure barriers and calls the provided callback function for each bottom level
   * acceleration structure barrier.
   * @tparam T The type of the callback function.
   * @param clb The callback function that takes a bottom level acceleration structure as argument.
   */
  template <typename T>
  void enumerateBlasBarriers(T clb)
  {
    if (single_element_count == blasCount)
    {
      clb(blas);
    }
    else
    {
      for (unsigned i = 0; i < blasCount; ++i)
      {
        clb(blases[i]);
      }
    }
  }
};
