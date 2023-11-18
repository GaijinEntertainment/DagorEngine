//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_d3dResource.h>
#include <3d/dag_drvDecl.h>
#include <3d/dag_drv3dConsts.h>
#include <3d/dag_tex3d.h>
#include <3d/dag_sampler.h>
#include <util/dag_globDef.h>
#include <vecmath/dag_vecMathDecl.h>
#include <3d/dag_renderStates.h>
#include <3d/dag_renderStateId.h>
#include <EASTL/initializer_list.h>
#include <3d/dag_hangHandler.h>

// forward declarations for external classes
struct TexImage32;
class IGenLoad;
class String;
struct DagorDateTime;
struct D3dInterfaceTable;
namespace ddsx
{
struct Header;
}

typedef TMatrix4 Matrix44;

//--- Driver3dDesc & initialization -------
#include "rayTrace/dag_drvRayTrace.h"

// main window proc
typedef intptr_t main_wnd_f(void *, unsigned, uintptr_t, intptr_t);

class Driver3dInitCallback
{
public:
  struct VersionRange
  {
    uint64_t minVersion;
    uint64_t maxVersion;
  };

  struct RenderSize
  {
    int width;
    int height;
  };

  using NeedStereoRenderFunc = bool (*)();
  using StereoRenderDimensionFunc = int (*)();
  using StereoRenderExtensionsFunc = const char *(*)();
  using StereoRenderVersionsFunc = VersionRange (*)();
  using StereoRenderAdapterFunc = int64_t (*)();

  virtual void verifyResolutionSettings(int &ref_scr_wdt, int &ref_scr_hgt, int base_scr_wdt, int base_scr_hgt, bool window_mode) const
  {
    G_UNUSED(ref_scr_wdt);
    G_UNUSED(ref_scr_hgt);
    G_UNUSED(base_scr_wdt);
    G_UNUSED(base_scr_hgt);
    G_UNUSED(window_mode);
  }

  // returns non-0 if valid
  virtual int validateDesc(Driver3dDesc &) const = 0;

  // returns -1 if A is better, +1 if B is better, and 0 if equivalent
  virtual int compareDesc(Driver3dDesc &A, Driver3dDesc &B) const = 0;

  // Stereo render related queries
  virtual bool desiredStereoRender() const { return false; }
  virtual int64_t desiredAdapter() const { return 0; }
  virtual RenderSize desiredRendererSize() const { return {0, 0}; }
  virtual const char *desiredRendererDeviceExtensions() const { return nullptr; }
  virtual const char *desiredRendererInstanceExtensions() const { return nullptr; }
  virtual VersionRange desiredRendererVersionRange() const { return {0, 0}; }
};

//--- Vertex & index buffer interface -------
class Sbuffer : public D3dResource
{
public:
  DAG_DECLARE_NEW(midmem)

  struct IReloadData
  {
    virtual ~IReloadData() {}
    virtual void reloadD3dRes(Sbuffer *sb) = 0;
    virtual void destroySelf() = 0;
  };
  virtual bool setReloadCallback(IReloadData *) { return false; }

  int restype() const override final { return RES3D_SBUF; }

  // lock buffer; returns 0 on error
  // size_bytes==0 means entire buffer
  virtual int lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags) = 0;

  virtual int unlock() = 0;
  virtual int getFlags() const = 0;
  const char *getBufName() const { return getResName(); }

  // for structured buffers
  virtual int getElementSize() const { return 0; }
  virtual int getNumElements() const { return 0; };
  virtual bool copyTo(Sbuffer * /*dest*/) { return false; } // return true, if copied
  virtual bool copyTo(Sbuffer * /*dest*/, uint32_t /*dst_ofs_bytes*/, uint32_t /*src_ofs_bytes*/, uint32_t /*size_bytes*/)
  {
    return false;
  }

  template <typename T>
  inline int lockEx(uint32_t ofs_bytes, uint32_t size_bytes, T **p, int flags)
  {
    void *vp;
    if (!lock(ofs_bytes, size_bytes, &vp, flags))
    {
      *p = NULL;
      return 0;
    }
    *p = (T *)vp;
    return 1;
  }

  void checkLockParams(uint32_t offset, uint32_t size, int flags, int bufFlags)
  {
    G_UNUSED(offset);
    G_UNUSED(size);
    G_UNUSED(flags);
    if ((bufFlags & SBCF_FRAMEMEM) == 0)
      return;

    // this is somewhat arbitrary but we don't want to upload too much data each frame
    const uint32_t max_dynamic_buffer_size = 256 << 10;
    G_UNUSED(max_dynamic_buffer_size);
    G_ASSERT(offset == 0);
    G_ASSERT(size <= max_dynamic_buffer_size);
    G_ASSERT(bufFlags & SBCF_DYNAMIC);
    G_ASSERT((bufFlags & SBCF_BIND_UNORDERED) == 0);
    G_ASSERT(flags & VBLOCK_DISCARD);
    G_ASSERT((flags & (VBLOCK_READONLY | VBLOCK_NOOVERWRITE)) == 0);
  }

  bool updateDataWithLock(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, int lockFlags)
  {
    G_ASSERT_RETURN(size_bytes, false);
    void *data = 0;
    if (lock(ofs_bytes, size_bytes, &data, lockFlags | VBLOCK_WRITEONLY))
    {
      if (data)
        memcpy(data, src, size_bytes);
      unlock();
      return true;
    }
    return false;
  }

  virtual bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
  {
    G_ASSERT_RETURN(size_bytes, false);
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
  }

  inline int lock(uint32_t ofs_bytes, uint32_t size_bytes, uint16_t **p, int flags)
  {
    G_ASSERT(!(getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }
  inline int lock32(uint32_t ofs_bytes, uint32_t size_bytes, uint32_t **p, int flags)
  {
    G_ASSERT((getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }

protected:
  ~Sbuffer() override {}
};

typedef Sbuffer Ibuffer;
typedef Sbuffer Vbuffer;

// This input type for 'd3d::resource_barrier' allows one function to handle multiple input data layouts.
// Inputs can be simple single values, arrays of values, pointers to values and initializer lists of values.
// To interpret the stored values use the provided enumerate functions to get the buffer and texture barriers.
// For more details on resource barriers see https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers
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

public:
  ResourceBarrierDesc() :
    buffer{nullptr},
    texture{nullptr},
    bufferStates{nullptr},
    textureStates{nullptr},
    textureSubResIndices{nullptr},
    textureSubResRanges{nullptr}
  {}
  ResourceBarrierDesc(const ResourceBarrierDesc &) = default;
  ResourceBarrierDesc(Sbuffer *buf, ResourceBarrier rb) : buffer{buf}, bufferState{rb}, bufferCount{single_element_count} {}
  ResourceBarrierDesc(Sbuffer *const *bufs, const ResourceBarrier *rb, unsigned count) :
    buffers{bufs}, bufferStates{rb}, bufferCount{count}
  {}
  template <unsigned N>
  ResourceBarrierDesc(Sbuffer *(&bufs)[N], ResourceBarrier (&rb)[N]) : buffers{bufs}, bufferStates{rb}, bufferCount{N}
  {}
  ResourceBarrierDesc(std::initializer_list<Sbuffer *> bufs, std::initializer_list<ResourceBarrier> rb) :
    buffers{bufs.begin()}, bufferStates{rb.begin()}, bufferCount{static_cast<unsigned>(bufs.size())}
  {
    G_ASSERT(bufs.size() == rb.size());
  }
  ResourceBarrierDesc(BaseTexture *tex, ResourceBarrier rb, unsigned sub_res_index, unsigned sub_res_range) :
    texture{tex},
    textureState{rb},
    textureSubResIndex{sub_res_index},
    textureSubResRange{sub_res_range},
    textureCount{single_element_count}
  {}
  ResourceBarrierDesc(BaseTexture *const *texs, const ResourceBarrier *rb, const unsigned *sub_res_index,
    const unsigned *sub_res_range, unsigned count) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{count}
  {}
  template <unsigned N>
  ResourceBarrierDesc(BaseTexture *(&texs)[N], ResourceBarrier (&rb)[N], unsigned (&sub_res_index)[N], unsigned (&sub_res_range)[N]) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{N}
  {}
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
  // Expected use case is that 'rb' has the RB_FLUSH_UAV flag set for all pending uav access
  explicit ResourceBarrierDesc(ResourceBarrier rb) : buffer{nullptr}, bufferState{rb}, bufferCount{single_element_count} {}
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
};

enum class ResourceActivationAction : unsigned
{
  REWRITE_AS_COPY_DESTINATION,
  REWRITE_AS_UAV,
  REWRITE_AS_RTV_DSV,
  CLEAR_F_AS_UAV,
  CLEAR_I_AS_UAV,
  CLEAR_AS_RTV_DSV,
  DISCARD_AS_UAV,
  DISCARD_AS_RTV_DSV,
};

enum class DepthAccess
{
  /// For regular depth attachement.
  RW,

  /**
   * For read-only depth attachement which can also be sampled as a texture in the same shader.
   * IF YOU DON'T NEED TO SAMPLE THE DEPTH, USE z_write=false WITH DepthAccess::RW INSTEAD.
   * Using this state will cause HiZ decompression on some hardware and
   * split of renderpass with flush of tile data into memory in a TBR.
   */
  SampledRO
};

union ResourceClearValue
{
  struct
  {
    uint32_t asUint[4];
  };
  struct
  {
    int32_t asInt[4];
  };
  struct
  {
    float asFloat[4];
  };
  struct
  {
    float asDepth;
    uint8_t asStencil;
  };
};

inline ResourceClearValue make_clear_value(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  ResourceClearValue result;
  result.asUint[0] = r;
  result.asUint[1] = g;
  result.asUint[2] = b;
  result.asUint[3] = a;
  return result;
}

inline ResourceClearValue make_clear_value(int32_t r, int32_t g, int32_t b, int32_t a)
{
  ResourceClearValue result;
  result.asInt[0] = r;
  result.asInt[1] = g;
  result.asInt[2] = b;
  result.asInt[3] = a;
  return result;
}

inline ResourceClearValue make_clear_value(float r, float g, float b, float a)
{
  ResourceClearValue result;
  result.asFloat[0] = r;
  result.asFloat[1] = g;
  result.asFloat[2] = b;
  result.asFloat[3] = a;
  return result;
}

inline ResourceClearValue make_clear_value(float d, uint8_t s)
{
  ResourceClearValue result;
  result.asDepth = d;
  result.asStencil = s;
  return result;
}

struct BasicResourceDescription
{
  uint32_t cFlags;
  ResourceActivationAction activation;
  ResourceClearValue clearValue;
};

struct BufferResourceDescription : BasicResourceDescription
{
  uint32_t elementSizeInBytes;
  uint32_t elementCount;
  uint32_t viewFormat;
};

struct BasicTextureResourceDescription : BasicResourceDescription
{
  uint32_t mipLevels;
};

struct TextureResourceDescription : BasicTextureResourceDescription
{
  uint32_t width;
  uint32_t height;
};

struct VolTextureResourceDescription : TextureResourceDescription
{
  uint32_t depth;
};

struct ArrayTextureResourceDescription : TextureResourceDescription
{
  uint32_t arrayLayers;
};

struct CubeTextureResourceDescription : BasicTextureResourceDescription
{
  uint32_t extent;
};

struct ArrayCubeTextureResourceDescription : CubeTextureResourceDescription
{
  uint32_t cubes;
};

struct ResourceDescription
{
  uint32_t resType;
  union
  {
    BasicResourceDescription asBasicRes;
    BufferResourceDescription asBufferRes;
    BasicTextureResourceDescription asBasicTexRes;
    TextureResourceDescription asTexRes;
    VolTextureResourceDescription asVolTexRes;
    ArrayTextureResourceDescription asArrayTexRes;
    CubeTextureResourceDescription asCubeTexRes;
    ArrayCubeTextureResourceDescription asArrayCubeTexRes;
  };

  ResourceDescription() = default;
  ResourceDescription(const ResourceDescription &) = default;

  ResourceDescription(const BufferResourceDescription &buf) : resType{RES3D_SBUF}, asBufferRes{buf} {}

  ResourceDescription(const TextureResourceDescription &tex) : resType{RES3D_TEX}, asTexRes{tex} {}

  ResourceDescription(const VolTextureResourceDescription &tex) : resType{RES3D_VOLTEX}, asVolTexRes{tex} {}

  ResourceDescription(const ArrayTextureResourceDescription &tex) : resType{RES3D_ARRTEX}, asArrayTexRes{tex} {}

  ResourceDescription(const CubeTextureResourceDescription &tex) : resType{RES3D_CUBETEX}, asCubeTexRes{tex} {}

  ResourceDescription(const ArrayCubeTextureResourceDescription &tex) : resType{RES3D_CUBEARRTEX}, asArrayCubeTexRes{tex} {}

  bool operator==(const ResourceDescription &r) const
  {
#define FIELD_MATCHES(field) (this->field == r.field)
    if (!FIELD_MATCHES(resType) || !FIELD_MATCHES(asBasicRes.cFlags))
      return false;
    if (resType == RES3D_SBUF)
      return FIELD_MATCHES(asBufferRes.elementCount) && FIELD_MATCHES(asBufferRes.elementSizeInBytes) &&
             FIELD_MATCHES(asBufferRes.viewFormat);
    if (!FIELD_MATCHES(asBasicTexRes.mipLevels))
      return false;
    if (resType == RES3D_CUBETEX || resType == RES3D_CUBEARRTEX)
    {
      if (!FIELD_MATCHES(asCubeTexRes.extent))
        return false;
      if (resType == RES3D_CUBEARRTEX)
        return FIELD_MATCHES(asArrayCubeTexRes.cubes);
    }
    if (!FIELD_MATCHES(asTexRes.width) || !FIELD_MATCHES(asTexRes.height))
      return false;
    if (resType == RES3D_VOLTEX)
      return FIELD_MATCHES(asVolTexRes.depth);
    if (resType == RES3D_ARRTEX)
      return FIELD_MATCHES(asArrayTexRes.arrayLayers);
    return true;
#undef FIELD_MATCHES
  }

  using HashT = size_t;
  HashT hash() const
  {
    HashT hashValue = resType;
    switch (resType)
    {
      case RES3D_SBUF: return hashPack(hashValue, asBufferRes.elementCount, asBufferRes.elementSizeInBytes, asBufferRes.viewFormat);
      case RES3D_TEX:
        return hashPack(hashValue, eastl::to_underlying(asTexRes.activation), asTexRes.cFlags, asTexRes.mipLevels, asTexRes.height,
          asTexRes.width, asTexRes.clearValue.asUint[0], asTexRes.clearValue.asUint[1], asTexRes.clearValue.asUint[2],
          asTexRes.clearValue.asUint[3]);
      case RES3D_VOLTEX:
        return hashPack(hashValue, eastl::to_underlying(asVolTexRes.activation), asVolTexRes.cFlags, asVolTexRes.mipLevels,
          asVolTexRes.height, asVolTexRes.width, asVolTexRes.depth, asVolTexRes.clearValue.asUint[0], asVolTexRes.clearValue.asUint[1],
          asVolTexRes.clearValue.asUint[2], asVolTexRes.clearValue.asUint[3]);
      case RES3D_ARRTEX:
        return hashPack(hashValue, eastl::to_underlying(asArrayTexRes.activation), asArrayTexRes.arrayLayers, asArrayTexRes.cFlags,
          asArrayTexRes.height, asArrayTexRes.mipLevels, asArrayTexRes.height, asArrayTexRes.width, asArrayTexRes.clearValue.asUint[0],
          asArrayTexRes.clearValue.asUint[1], asArrayTexRes.clearValue.asUint[2], asArrayTexRes.clearValue.asUint[3]);
      case RES3D_CUBETEX:
        return hashPack(hashValue, eastl::to_underlying(asCubeTexRes.activation), asCubeTexRes.cFlags, asCubeTexRes.extent,
          asCubeTexRes.mipLevels, asCubeTexRes.clearValue.asUint[0], asCubeTexRes.clearValue.asUint[1],
          asCubeTexRes.clearValue.asUint[2], asCubeTexRes.clearValue.asUint[3]);
      case RES3D_CUBEARRTEX:
        return hashPack(hashValue, eastl::to_underlying(asArrayCubeTexRes.activation), asArrayCubeTexRes.cFlags,
          asArrayCubeTexRes.mipLevels, asArrayCubeTexRes.cubes, asArrayCubeTexRes.extent, asArrayCubeTexRes.clearValue.asUint[0],
          asArrayCubeTexRes.clearValue.asUint[1], asArrayCubeTexRes.clearValue.asUint[2], asArrayCubeTexRes.clearValue.asUint[3]);
    }
    return hashValue;
  }

private:
  static inline HashT hashPack() { return 0; }
  template <typename T, typename... Ts>
  static inline HashT hashPack(const T &first, const Ts &...other)
  {
    HashT hashVal = hashPack(other...);
    hashVal ^= first + 0x9e3779b9 + (hashVal << 6) + (hashVal >> 2);
    return hashVal;
  }
};

namespace eastl
{
template <>
class hash<ResourceDescription>
{
public:
  size_t operator()(const ResourceDescription &desc) const { return desc.hash(); }
};
} // namespace eastl

struct ResourceHeapGroup;

struct ResourceHeapGroupProperties
{
  union
  {
    uint32_t flags;
    struct
    {
      // If true, the CPU can access this memory directly.
      // On consoles this is usually true for all heap groups, on PC only
      // for system memory heap groups.
      bool isCPUVisible : 1;
      // If true, the GPU can access this memory directly without going
      // over a bus like PCIE.
      // On consoles this is usually true for all heap groups, on PC only
      // for memory dedicated to the GPU.
      bool isGPULocal : 1;
      // Special on chip memory, like ESRAM of the XB1.
      bool isOnChip : 1;
    };
  };
  // The maximum size of a resource that can be placed into a heap of this group.
  uint64_t maxResourceSize;
  // the maximum size of a individual heap, this is usually limited by the
  // amount that is installed in the system. Drivers may impose other limitations.
  uint64_t maxHeapSize;
};

struct ResourceAllocationProperties
{
  size_t sizeInBytes;
  size_t offsetAlignment;
  ResourceHeapGroup *heapGroup;
};

// A set of flags that steer the behavior of the driver during creation of resource heaps.
enum ResourceHeapCreateFlag
{
  // By default the drivers are allowed to use already reserved memory of internal heaps, to source the needed memory.
  // Drivers are also allowed to allocate larger memory heaps and use the excess memory for their internal resource
  // and memory management.
  RHCF_NONE = 0,
  // Resource heaps created with this flag, will use their own dedicate memory heap to supply the memory for resources.
  // When this flag is not used to create a resource heap, the driver is allowed to source the need memory from existing
  // driver managed heaps, or create a larger underlying memory heap and use the excess memory for its internal resource
  // and memory management.
  // This flag should be used only when really necessary, as it restricts the drivers option to use already reserved
  // memory for this heap and increase the memory pressure on the system.
  RHCF_REQUIRES_DEDICATED_HEAP = 1u << 0,
};

using ResourceHeapCreateFlags = uint32_t;

struct ResourceHeap;

struct RenderTarget
{
  BaseTexture *tex;
  uint32_t mip_level;
  uint32_t layer;
};

/** \defgroup RenderPassStructs
 * @{
 */

/// \brief Description of render target bind inside render pass
/// \details Fully defines operation that will happen with target at defined subpass slot
struct RenderPassBind
{
  /// \brief Index of render target in render targets array that will be used for this bind
  int32_t target;
  /// \brief Index of subpass where bind happens
  int32_t subpass;
  /// \brief Index of slot where target will be used
  int32_t slot;

  /// \brief defines what will happen with target used in binding
  RenderPassTargetAction action;
  /// \brief optional user barrier for generic(emulated) implementation
  ResourceBarrier dependencyBarrier;
};

/// \brief Early description of render target
/// \details Gives necessary info at render pass creation so render pass is compatible with targets of same type later on
struct RenderPassTargetDesc
{
  /// \brief Resource from which information is extracted, can be nullptr
  BaseTexture *templateResource;
  /// \brief Raw texture create flags, used if template resource is not provided
  unsigned texcf;
  /// \brief Must be set if template resource is empty and target will use aliased memory
  bool aliased;
};

/// \brief Description of target that is used inside render pass
struct RenderPassTarget
{
  /// \brief Actual render target subresource reference
  RenderTarget resource;
  /// \brief Clear value that is used on clear action
  ResourceClearValue clearValue;
};

/// \brief Render pass resource description, used to create RenderPass object
struct RenderPassDesc
{
  /// \brief Name that is visible only in developer builds and inside graphics tools, if possible
  const char *debugName;
  /// \brief Total amount of targets inside render pass
  uint32_t targetCount;
  /// \brief Total amount of bindings for render pass
  uint32_t bindCount;

  /// \brief Array of targetCount elements, supplying early description of render target
  const RenderPassTargetDesc *targetsDesc;
  /// \brief Array of bindCount elements, describing all subpasses
  const RenderPassBind *binds;

  /// \brief Texture binding offset for shader subpass reads used on APIs without native render passes
  /// \details Generic(emulated) implementation will use registers starting from this offset, to bind input attachments.
  /// This must be properly handled inside shader code for generic implementation to work properly!
  uint32_t subpassBindingOffset;
};

/// \brief Area of render target where rendering will happen inside render pass
struct RenderPassArea
{
  uint32_t left;
  uint32_t top;
  uint32_t width;
  uint32_t height;
  float minZ;
  float maxZ;
};

/**@}*/

namespace d3d
{
//! opaque class that represents render pass
struct RenderPass;
} // namespace d3d

// NOTE: even if numPackedMips is zero, numTilesNeededForPackedMips may be greater than zero, which is a special case,
// and numTilesNeededForPackedMips tiles still need to be assigned at numUnpackedMips.
struct TextureTilingInfo
{
  size_t totalNumberOfTiles;
  size_t numUnpackedMips;
  size_t numPackedMips;
  size_t numTilesNeededForPackedMips;
  size_t firstPackedTileIndex;
  size_t tileWidthInPixels;
  size_t tileHeightInPixels;
  size_t tileDepthInPixels;
  size_t tileMemorySize;

  size_t subresourceWidthInTiles;
  size_t subresourceHeightInTiles;
  size_t subresourceDepthInTiles;
  size_t subresourceStartTileIndex;
};

struct TileMapping
{
  size_t texX;           // The tile coordinates in tiles, not pixels!
  size_t texY;           // The tile coordinates in tiles, not pixels!
  size_t texZ;           // The tile coordinates in tiles, not pixels!
  size_t texSubresource; // The index of the subresource.
  size_t heapTileIndex;  // The index of the tile in the heap. Not bytes, but tile index!
  size_t heapTileSpan;   // The number of tiles to map. Zero is invalid, and if it is not one, an array of tiles will be
                         // mapped, to the specified location.
                         // Example usage for this is packed mip tails.
                         // - Map to subresource TextureTilingInfo::numUnpackedMips at 0, 0, 0.
                         // - Use a span of TextureTilingInfo::numTilesNeededForPackedMips so the whole mip tail can be mapped at once.
                         // - From TileMapping::heapTileIndex the given number of tiles will be mapped to the packed mip tail.
};

enum class APISupport
{
  FULL_SUPPORT,
  OUTDATED_DRIVER,
  BLACKLISTED_DRIVER,
  NO_DEVICE_FOUND
};

//! opaque class that holds data for resource/texture update
namespace d3d
{
class ResUpdateBuffer;
}

//--- 3d driver interface -------
namespace d3d
{
enum
{
  USAGE_TEXTURE = 0x01,
  USAGE_DEPTH = 0x02,
  USAGE_RTARGET = 0x04,
  USAGE_AUTOGENMIPS = 0x08,
  USAGE_FILTER = 0x10,
  USAGE_BLEND = 0x20,
  USAGE_VERTEXTEXTURE = 0x40,
  USAGE_SRGBREAD = 0x80,
  USAGE_SRGBWRITE = 0x100,
  USAGE_SAMPLECMP = 0x200,
  USAGE_PIXREADWRITE = 0x400,
  USAGE_TILED = 0x800,
  USAGE_UNORDERED = 0x1000,
};

enum
{
  CAPFMT_X8R8G8B8,
  CAPFMT_R8G8B8,
  CAPFMT_R5G6B5,
  CAPFMT_X1R5G5B5,
};

void update_window_mode();

/// guesses and returns GPU vendor ID;
/// may be called before d3d::init_driver()
int guess_gpu_vendor(String *out_gpu_desc = NULL, uint32_t *out_drv_ver = NULL, DagorDateTime *out_drv_date = NULL,
  uint32_t *device_id = nullptr);

DagorDateTime get_gpu_driver_date(int vendor);

/// determines and returns size of dedicated GPU memory in KB;
/// may be called before d3d::init_driver()
unsigned get_dedicated_gpu_memory_size_kb();
unsigned get_free_dedicated_gpu_memory_size_kb();

/// fast implementation to get the memory during the game, supports only Nvidia GPUs.
void get_current_gpu_memory_kb(uint32_t *dedicated_total, uint32_t *dedicated_free);

bool get_gpu_freq(String &out_freq);
int get_gpu_temperature();
void get_video_vendor_str(String &out_str);

/// DPI of the primary display divided by 96 (96 dpi is for 100% scale in Windows)
float get_display_scale();

// Override profile SLI settings, must be called before device creation.
void disable_sli();

static constexpr int RENDER_TO_WHOLE_ARRAY = 1023;
#if !_TARGET_D3D_MULTI
// Driver initialization API

/// initalizes 3d device driver
bool init_driver();

/// returns true when d3d API is inited
bool is_inited();

/// start up, read cfg, create window, init 3d hard&soft, ...
/// if renderwnd!=NULL, mainwnd and renderwnd are used for rendering (when possible)
/// on error, returns false
/// on sucess, return true
bool init_video(void *hinst, main_wnd_f *, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

/// shutdown driver and release all resources
void release_driver();

/// fills function-pointers table for d3d API, or returns false if unsupported
bool fill_interface_table(D3dInterfaceTable &d3dit);


/// returns driver name (pointer to static string)
const char *get_driver_name();

/// returns driver code
DriverCode get_driver_code();
/// returns true when d3d-stub driver is used
static inline bool is_stub_driver() { return get_driver_code().is(d3d::stub); }

/// returns device driver version (pointer to static string)
const char *get_device_driver_version();

/// returns device name (pointer to static string)
const char *get_device_name();

/// returns pointer to static buffer containing last error message
const char *get_last_error();

uint32_t get_last_error_code();

/// notify driver before window destruction
void prepare_for_destroy();

/// notify driver on window destruction
void window_destroyed(void *hwnd);


// Device management

/// returns raw pointer to device interface (implementation and platform specific)
void *get_device();

/// returns raw pointer to device interface
void *get_context();

/// returns driver description (pointer to static object)
const Driver3dDesc &get_driver_desc();

/// send specific command to driver (see dag_drv3dCmd.h)
int driver_command(int command, void *par1, void *par2, void *par3);

/// returns true when device is lost; when it is safe to reset device can_reset_now is set with 1
/// returns false when device is ok
bool device_lost(bool *can_reset_now);

/// performs device reset; returns true if succeded
bool reset_device();

/// returns true when device is lost or device is reseted right now
bool is_in_device_reset_now();

/// returns if the game rendering window is completely occluded right now
bool is_window_occluded();

/// Returns if for image processing, compute or graphics commands should be preferred
bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats);


// Texture management

/// check whether this texture format is available
/// returns false if texture of the specified format can't be created
bool check_texformat(int cflg);

/// returns the maximum sample count for the given texture format
int get_max_sample_count(int cflg);

unsigned get_texformat_usage(int cflg, int restype = RES3D_TEX);
/// check whether specified texture creation flags result in the same format
bool issame_texformat(int cflg1, int cflg2);

/// check whether this cube texture format is available
/// returns false if cube texture of the specified format can't be created
bool check_cubetexformat(int cflg);

/// check whether specified cube texture creation flags result in the same format
bool issame_cubetexformat(int cflg1, int cflg2);

/// check whether this volume texture format is available
/// returns false if cube texture of the specified format can't be created
bool check_voltexformat(int cflg);

/// check whether specified volume texture creation flags result in the same format
bool issame_voltexformat(int cflg1, int cflg2);

/// create texture; if img==NULL, size (w/h) must be specified
/// if img!=NULL and size==0, dimension will be taken from img
/// levels specify maximum number of mipmap levels
/// if levels==0, whole mipmap set will be created if device supports mipmaps
/// img format is TexPixel32
/// returns NULL on error
BaseTexture *create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL);

/// create texture from DDSx stream;
/// quality_id specifies quality index (0=high, 1=medium, 2=low, 3=ultralow)
/// levels specifies number of loaded mipmaps (0=all, >0=only first 'levels' mipmaps)
/// returns NULL on error
BaseTexture *create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels = 0, const char *stat_name = NULL);


/// create texture object using DDSx header (not texture contents loaded at this time)
BaseTexture *alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels = 0, const char *stat_name = NULL,
  int stub_tex_idx = -1);

/// load texture content from DDSx stream using DDSx header for previously allocated texture
static inline bool load_ddsx_tex_contents(BaseTexture *t, const ddsx::Header &hdr, IGenLoad &crd, int q_id)
{
  return d3d_load_ddsx_tex_contents ? d3d_load_ddsx_tex_contents(t, t->getTID(), BAD_TEXTUREID, hdr, crd, q_id, 0, 0) : false;
}


/// create cubic texture
/// levels specify maximum number of mipmap levels
/// if levels==0, whole mipmap set will be created if device supports mipmaps
/// returns NULL on error
BaseTexture *create_cubetex(int size, int flg, int levels, const char *stat_name = NULL);

/// create volume texture
/// levels specify maximum number of mipmap levels
/// if levels==0, whole mipmap set will be created if device supports mipmaps
/// returns NULL on error
BaseTexture *create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name = NULL);

/// create texture2d array
/// levels specify maximum number of mipmap levels
/// if levels==0, whole mipmap set will be created if device supports mipmaps
/// returns NULL on error
BaseTexture *create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name);
// it will just create ArrayTexture with d*6 layers, ehich also has Cube
BaseTexture *create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name); // total layers d*6

/// Create texture alias, a texture using the same memory as an other texture,
/// but as a different format. You can think of it as the old texture data is
/// cast to the aliased texture format;
/// Calling it is the same as create_tex, but the GPU memory of baseTexture will
/// be used as the backing GPU memory for this texture
/// As this is only an alias to the GPU memory area, the lifetime of the GPU memory
/// area is still controlled by baseTexture, so no alias texture should be used
/// in the rendering pipeline after their base texture is deleted.
BaseTexture *alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL);
BaseTexture *alias_cubetex(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name = NULL);
BaseTexture *alias_voltex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name = NULL);
BaseTexture *alias_array_tex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name);
BaseTexture *alias_cube_array_tex(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name);

bool set_tex_usage_hint(int w, int h, int mips, const char *format, unsigned int tex_num);

/// discards all managed textures
void discard_managed_textures();

bool stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc = NULL, RectInt *rdst = NULL);

bool copy_from_current_render_target(BaseTexture *to_tex);

void get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump);

// Texture states setup

// r/o and r/w resource slots are independent
// shader_stage is STAGE_xS
bool set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler = true);
static inline bool settex(int slot, BaseTexture *tex) { return set_tex(STAGE_PS, slot, tex); }
static inline bool settex_vs(int slot, BaseTexture *tex) { return set_tex(STAGE_VS, slot, tex); }

// Creates the sampler with the given sampler info
// Identical infos may yield both identical handles and different ones, it depends on driver implementation
// This call is thread-safe and does not require external synchronization
SamplerHandle create_sampler(const SamplerInfo &sampler_info);

// Destroys given sampler
// The driver may not actually destroy the sampler
// Using this sampler after calling this function is UB
void destroy_sampler(SamplerHandle sampler);

// Binds given sampler to the slot
// This call is not thread-safe, requires global gpu lock to be holded
void set_sampler(unsigned shader_stage, unsigned slot, SamplerHandle sampler);

// Registers the sampler portion of the given texture into the global bindless sampler table.
// Textures with identical samplers may yield identical return values.
uint32_t register_bindless_sampler(BaseTexture *texture);

// Allocates a persistent bindless slot range of the given 'resource_type' resource type.
// * 'resource_type' must be one of RES3D_ enum values.
// * 'count' must be larger than 0.
// Returned value is the first slot index into the bindless heap of the requested range.
uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count);
// Resizes a previously allocate bindless slot range. It can shrink and enlarge a slot range.
// The contents of all slots of the old range are migrated to the new range, so only new entries
// have to be updated.
// * 'resource_type' must be one of RES3D_ enum values.
// * 'index' must be in a previously allocated bindless range, or any value if 'current_count' is 0.
// * 'current_count' most be either within a previously allocated bindless slot range or 0,
//   when 0 then it behaves like 'allocate_bindless_resource_range'.
// * 'new_count' can be larger or smaller than 'current_count', shrinks or enlarges the slot
//   range accordingly.
// The returned value is the first slot of the new range.
uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count);
// Frees previously allocated slot range. This can also be used to shrink ranges, similarly to
// 'resize_bindless_resource_range'.
// * 'resource_type' must be one of RES3D_ enum values.
// * 'index' must be in a previously allocated bindless range, or any value if 'count' is 0.
// * 'count' plus 'index' most not be outside of any previously allocated bindless slot range,
//   can be 0 which will be a no-op.
void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count);
// Updates a given bindless slot with the reference to 'res'. The slot has to be allocated previously
// with the corresponding allocation methods with 'resource_type' matching the restype of 'res'.
// * 'index' must be in a previously allocated bindless range.
// * 'res' must be a valid D3DResource object.
void update_bindless_resource(uint32_t index, D3dResource *res);
// Updates one or more bindless slots with a "null" resource of the given type.
// Shader access to those slots will read all zeros and writes will be discarded.
// * 'res_type' must be one of RES3D_ enum values.
// * 'index' must be in a previously allocated bindless range.
// * 'count' plus 'index' most not be outside of any previously allocated bindless slot range.
void update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count);

// as_uint == true, texture will be viewed as uint in UAV
// https://msdn.microsoft.com/en-us/library/windows/desktop/ff728749(v=vs.85).aspx
bool set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint = false);
bool clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level);
bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level);
bool clear_rwbufi(Sbuffer *buf, const unsigned val[4]);
bool clear_rwbuff(Sbuffer *buf, const float val[4]);

// Combined programs

PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides = 0, unsigned streams = 0);
// if strides & streams are unset, will get them from VDECL
//  should be deleted externally

PROGRAM create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides = 0,
  unsigned streams = 0);
// if strides & streams are unset, will get them from VDECL
PROGRAM create_program_cs(const uint32_t *cs_native, CSPreloaded preloaded);

bool set_program(PROGRAM);    // sets both pixel and vertex shader and vertex declaration
void delete_program(PROGRAM); // deletes vprog and fshader. VDECL should be deleted independently

// Vertex programs

VPROG create_vertex_shader(const uint32_t *native_code);
void delete_vertex_shader(VPROG vs);

bool set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs);
static inline bool set_vs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_VS, reg_base, data, num_regs);
}
static inline bool set_ps_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_PS, reg_base, data, num_regs);
}
static inline bool set_cs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_CS, reg_base, data, num_regs);
}
static inline bool set_vs_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_vs_const(reg, v, 1);
}
static inline bool set_ps_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_ps_const(reg, v, 1);
}

// immediate consts are supposed to be very cheap to set dwords.
// It is guaranteed to support up to 4 dwords on each stage.
// Use as less as possible, ideally one or two (or none).
// on XB1(PS4) implemented as user regs (C|P|V)SSetShaderUserData (user regs)
// on DX11 (and currently everything else)
// on VK/DX12 - should be implemented as descriptor/push constants buffers
// on OpenGL should be implemented as Uniform
// calling with data = nullptr || num_words == 0, is benign, and currently works as "stop using immediate"(probably have to be replaced
// with shader system)
bool set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words);
// Fragment shader

FSHADER create_pixel_shader(const uint32_t *native_code);
void delete_pixel_shader(FSHADER ps);

// Immediate constant buffers - valid within min(driver acquire, frame)
// to unbind, use set_const_buffer(stage, 0, NULL [,0]) - generic set_const_buffer()
// if slot = 0 is empty (PS/VS/CS stages), buffered constants are used
// [PS4 specific]
bool set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs);
bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset = 0, uint32_t consts_size = 0);

int set_vs_constbuffer_size(int required_size);
int set_cs_constbuffer_size(int required_size);

// Set immediate constants to const buffer slot 0 if used in shader explicitly
static inline bool set_cb0_data(unsigned stage, const float *data, unsigned num_regs)
{
#if _TARGET_C1 | _TARGET_C2

#else
  switch (stage)
  {
    case STAGE_CS: return set_cs_const(0, data, num_regs);
    case STAGE_PS: return set_ps_const(0, data, num_regs);
    case STAGE_VS: return set_vs_const(0, data, num_regs);
    default: G_ASSERTF(0, "Stage %d unsupported", stage); return false;
  };
#endif
}
static inline void release_cb0_data(unsigned stage)
{
#if _TARGET_C1 | _TARGET_C2

#else
  (void)stage;
#endif
}

// Vertex buffers

Sbuffer *create_vb(int size_bytes, int flags, const char *name = "");
Sbuffer *create_ib(int size_bytes, int flags, const char *stat_name = "ib");

// structured buffer
// struct_size should match when used as Sbuffer, Ibuffer struct_size is 16 or 32 bits
// for non structured buffers texfmt can be set (if we want to use this buffer in rendering)
Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name);

// set structure buffer - uses same slots as textures
bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);
bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

// Render targets

/// set default render target (display)
/// returns false on error
bool set_render_target();

/// set depth texture target. NULL means NO depth. use set_backbuf_depth for backbuf render target
bool set_depth(BaseTexture *tex, DepthAccess access);
bool set_depth(BaseTexture *tex, int layer, DepthAccess access);

/// set back buffer depth target.
bool set_backbuf_depth();

/// set texture as render target
/// if texture is depth texture format, it is the same as call set_depth()
/// if face is RENDER_TO_WHOLE_ARRAY, then whole Texture Array/Volume Tex will be set as render target
///   this is to be used with geom shader (and Metal allows with vertex shader)
bool set_render_target(int rt_index, BaseTexture *, int fc, int level);
bool set_render_target(int rt_index, BaseTexture *, int level);

inline bool set_render_target(BaseTexture *t, int level) { return set_render_target() && set_render_target(0, t, level); }
inline bool set_render_target(BaseTexture *t, int fc, int level) { return set_render_target() && set_render_target(0, t, fc, level); }
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, dag::ConstSpan<RenderTarget> colors)
{
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (i < colors.size())
      set_render_target(i, colors[i].tex, colors[i].mip_level);
    else
      set_render_target(i, nullptr, 0);
  }
  set_depth(depth.tex, depth_access);
}
inline void set_render_target(RenderTarget depth, DepthAccess depth_access, const std::initializer_list<RenderTarget> colors)
{
  set_render_target(depth, depth_access, dag::ConstSpan<RenderTarget>(colors.begin(), colors.end() - colors.begin()));
}

void get_render_target(Driver3dRenderTarget &out_rt);
bool set_render_target(const Driver3dRenderTarget &rt);

/// get current render target resolution
/// returns false on error
bool get_target_size(int &w, int &h);

/// get size of RT texture or backbuf (for rt_tex==NULL)
bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev = 0);

/**
 * \brief Sets variable rate shading setup for next draw calls.
 *
 * \note Rates (\p rate_x, \p rate_y) of 1 by 4 or 4 by 1 are invalid.
 *
 * \note Depth/Stencil values are always computed at full rate and so
 * shaders that modify depth value output may interfere with the
 * \p pixel_combiner.
 *
 * \param rate_x,rate_y Constant rates for the next draw calls, those
 *   are supported by all VRS capable devices. Valid values
 *   for both are 1, 2 and with the corresponding feature cap 4.
 * \param vertex_combiner The mode in which the constant rate of
 *   \p rate_x and \p rate_y is combined with a possible vertex/geometry
 *   shader rate output. For shader outputs see SV_ShadingRate, note
 *   that the provoking vertex or the per primitive value is used.
 * \param pixel_combiner The mode in which the result of 'vertex_combiner'
 *   is combined with the rate of the sampling rate texture set by
 *   \ref set_variable_rate_shading_texture.
 */
void set_variable_rate_shading(unsigned rate_x, unsigned rate_y,
  VariableRateShadingCombiner vertex_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH,
  VariableRateShadingCombiner pixel_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH);

/**
 * \brief Sets the variable rate shading texture for the next draw calls.
 * \param rate_texture The texture to use as a shading rate source.
 * \details Note that when you start to modify the used texture, you
 * should reset the used shading rate texture to null to ensure that on
 * next use as a shading rate source, the texture is in a state the device
 * can use.
 * \note It is invalid to call this when DeviceDriverCapabilities::hasVariableRateShadingTexture
 * feature is not supported.
 */
void set_variable_rate_shading_texture(BaseTexture *rate_texture = nullptr);

// Rendering

bool settm(int which, const Matrix44 *tm);
bool settm(int which, const TMatrix &tm);
void settm(int which, const mat44f &out_tm);

bool gettm(int which, Matrix44 *out_tm);
bool gettm(int which, TMatrix &out_tm);
void gettm(int which, mat44f &out_tm);

const mat44f &gettm_cref(int which);

/// get model to view matrix
void getm2vtm(TMatrix &out_m2v);
/// get model to clip (full) matrix and (optional) scale coefficients
void getglobtm(Matrix44 &);
/// set custom globtm matrix
void setglobtm(Matrix44 &);

void getglobtm(mat44f &);
void setglobtm(const mat44f &);

/// calculate and set perspective matrix, optionally returning the matrix itself
bool setpersp(const Driver3dPerspective &p, TMatrix4 *proj_tm = nullptr);
/// get last values set by setpersp()
bool getpersp(Driver3dPerspective &p);
/// check if p has a valid perspective
bool validatepersp(const Driver3dPerspective &p);

/// calculate the perspective matrix, without setting it
bool calcproj(const Driver3dPerspective &p, mat44f &proj_tm);
bool calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm);

/// calculate the globtm, without setting it
void calcglobtm(const mat44f &view_tm, const mat44f &proj_tm, mat44f &result);
void calcglobtm(const mat44f &view_tm, const Driver3dPerspective &persp, mat44f &result);
void calcglobtm(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &result);
void calcglobtm(const TMatrix &view_tm, const Driver3dPerspective &persp, TMatrix4 &result);

bool setscissor(int x, int y, int w, int h);
bool setscissors(dag::ConstSpan<ScissorRect> scissorRects);

bool setview(int x, int y, int w, int h, float minz, float maxz);
bool setviews(dag::ConstSpan<Viewport> viewports);
bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz);

/// clear all view
bool clearview(int what, E3DCOLOR, float z, uint32_t stencil);

/// flip video pages or copy to screen
/// app_active==false is hint to the driver
/// returns false on error
bool update_screen(bool app_active = true);

/// set vertex stream source
bool setvsrc_ex(int stream, Sbuffer *, int offset, int stride_bytes);
inline bool setvsrc(int stream, Sbuffer *vb, int stride_bytes) { return setvsrc_ex(stream, vb, 0, stride_bytes); }
/// set indices
bool setind(Sbuffer *ib);

// create dx8-style vertex declaration
// returns BAD_VDECL on error
VDECL create_vdecl(VSDTYPE *vsd);
/// delete vertex declaration
void delete_vdecl(VDECL vdecl);
/// set current vertex declaration
bool setvdecl(VDECL vdecl);

/// draw primitives
bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance);
inline bool draw(int type, int start, int numprim) { return draw_base(type, start, numprim, 1, 0); }
inline bool draw_instanced(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance = 0)
{
  return draw_base(type, start, numprim, num_instances, start_instance);
}

/// draw indexed primitives
bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance);

inline bool drawind_instanced(int type, int startind, int numprim, int base_vertex, uint32_t num_instances,
  uint32_t start_instance = 0)
{
  return drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}
inline bool drawind(int type, int startind, int numprim, int base_vertex)
{
  return drawind_base(type, startind, numprim, base_vertex, 1, 0);
}
/// draw primitives from user pointer (rather slow)
bool draw_up(int type, int numprim, const void *ptr, int stride_bytes);
/// draw indexed primitives from user pointer (rather slow)
bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes);

/// draw primitives
// struct args{   uint vertexCountPerInstance,InstanceCount, StartVertexLocation, StartInstanceLocation;}
bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

// struct args{   uint IndexCountPerInstance,InstanceCount, StartIndexLocation; int BaseVertexLocation, StartInstanceLocation;}
bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);
bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);

bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset = 0, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
// Max value for each direction is 64k, product of all dimensions can not exceed 2^22
void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
// Args are the same as dispatch_indirect { uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z }
void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset = 0);
// Variant of dispatch_mesh_indirect where 'dispatch_count' is read by the GPU from 'count' buffer at 'count_offset' (as uint32_t),
// this value can not exceed 'max_count'.
void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count);

GPUFENCEHANDLE insert_fence(GpuPipeline gpu_pipeline);
void insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline);
// additional L2 cache flush calls would need to be added to support CPU <-> Compute Context data communication
// (at least on Xbox One, see XDK docs, article "Optimizing Monolithic Driver Performance", part "Manual Cache and Pipeline Flushing")

// Render states

bool setantialias(int aa_type);
int getantialias();

bool set_blend_factor(E3DCOLOR color);
bool setstencil(uint32_t ref);

bool setwire(bool in);

bool set_depth_bounds(float zmin, float zmax);
bool supports_depth_bounds(); // returns true if hardware supports depth bounds. same as get_driver_desc().caps.hasDepthBoundsTestS

// Miscellaneous
bool set_srgb_backbuffer_write(bool); // returns previous result. switch on/off srgb write to backbuffer (default is off)

bool set_msaa_pass();

bool set_depth_resolve();

bool setgamma(float);

bool isVcolRgba();

float get_screen_aspect_ratio();
void change_screen_aspect_ratio(float ar);

// Screen capture

/// capture screen to buffer.fast, but not guaranteed
/// many captures can be followed by only one end_fast_capture_screen()
void *fast_capture_screen(int &w, int &h, int &stride_bytes, int &format);
void end_fast_capture_screen();

/// capture screen to TexPixel32 buffer
/// slow, and not 100% guaranted
/// returns NULL on error
TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes);
/// release buffer used to capture screen
void release_capture_buffer();

//! returns the size of the backbuffer.
//! it can be different from the size of the framebuffer
void get_screen_size(int &w, int &h);

//! conditional rendering.
//! conditional rendering is used to skip rendering of triangles completelyon GPU.
//! the only commands, that would be ignored, if survey fails are DIPs
//! (all commands and states will still be executed), so it is better to use
//! reports to completely skip object rendering
//! max index is defined per platform
//! surveying.
int create_predicate(); // -1 if not supported or something goes wrong
void free_predicate(int id);

bool begin_survey(int id); // false if not supported or bad id
void end_survey(int id);

void begin_conditional_render(int id);
void end_conditional_render(int id);

PROGRAM get_debug_program();
void beginEvent(const char *); // for debugging
void endEvent();               // for debugging
bool get_vrr_supported();
bool get_vsync_enabled();
bool enable_vsync(bool enable);

// Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
Texture *get_backbuffer_tex();
Texture *get_secondary_backbuffer_tex();
Texture *get_backbuffer_tex_depth();

#include "rayTrace/rayTracedrv3d.inl.h"

// See ResourceBarrierDesc and https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers
void resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc);
ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags);
void destroy_resource_heap(ResourceHeap *heap);
Sbuffer *place_buffere_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);
BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);
ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group);

// Maps a memory area of the heap to the specified xyz location of the texture.
// Use heap == nullptr to remove the link between a tile and the mapped heap portion.
void map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

TextureTilingInfo get_texture_tiling_info(BaseTexture *tex, size_t subresource);

void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

// Allocates a update buffer to update the subregion described by offset_x, offset_y, offset_z, width, height and depth.
//
// - dest_base_texture can not be nullptr
// - dest_mip must be a valid mipmap level for dest_base_texture
// - dest_slice must be a valid array index / cube face for dest_base_texture when it is a array, cube or cube array texture, otherwise
// has to be 0
// - offset_x must be within the width of dest_base_texture of miplevel dest_mip and aligned to the texture format block size
// - offset_y must be within the height of dest_base_texture of miplevel dest_mip and aligned to the texture format block size
// - offset_z must be within the depth of dest_base_texture of miplevel dest_mip when the texture is a vol tex, otherwise has to be 0
// - width plus offset_x must be within the width of dest_base_texture of miplevel dest_mip and aligned to the texture format block
// size
// - height plus offset_y must be within the height of dest_base_texture of miplevel dest_mip and aligned to the texture format block
// size
// - depth plus offset_z must be within the depth of dest_base_texture of miplevel dest_mip when the texture is a vol tex, otherwise
// has to be 1
//
// May return nullptr if either inputs violate the rules above, the driver can currently not provide the memory required or the driver
// is unable to perform the needed copy operation on update.
ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
  unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth);
//! allocates update buffer in system memory to be filled directly and then dispatched to apply_tex_update_buffer
//! this method can fail if too much allocations happens in N-frame period
//! caller should retry after rendering frame on screen if this happens
ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice);
//! releases update buffer (clears pointer afterwards); skips any update, just releases data
void release_update_buffer(ResUpdateBuffer *&rub);
//! returns data address to fill update data
char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub);
//! returns size of update buffer
size_t get_update_buffer_size(ResUpdateBuffer *rub);
//! returns pitch of update buffer (if applicable)
size_t get_update_buffer_pitch(ResUpdateBuffer *rub);
// Returns the pitch of one 2d image slice for volumetric textures.
size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub);
//! applies update to target d3dres and releases update buffer (clears pointer afterwards)
bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub);

/** \defgroup RenderPassD3D
 * @{
 */

/// \brief Creates render pass object
/// \details Render pass objects are intended to be created once (and ahead of time), used many times
/// \note No external sync required
/// \warning Do not run per frame/realtime!
/// \warning Avoid using at time sensitive places!
/// \param rp_desc Description of render pass to be created
/// \result Pointer to opaque RenderPass object, may be nullptr if description is invalid
RenderPass *create_render_pass(const RenderPassDesc &rp_desc);
/// \brief Deletes render pass object
/// \note Sync with usage is required (must not delete object that is in use in current CPU frame)
/// \warning All usage to object becomes invalid right after method call
/// \param rp Object to be deleted
void delete_render_pass(RenderPass *rp);

/// \brief Begins render pass rendering
/// \details After this command, viewport is reset to area supplied
/// and subpass 0, described in render pass object, is started
/// \note Must be external synced (GPU lock required)
/// \warning When inside pass, all other GPU execution methods aside of Draw* are prohibited!
/// \warning Avoid writes/reads outside area, it is UB in general
/// \warning Will assert-fail if other render pass is already in process
/// \param rp Render pass resource to begin with
/// \param area Rendering area restriction
/// \param targets Array of targets that will be used in rendering
void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets);
/// \brief Advances to next subpass
/// \details Increases subpass number and executes necessary synchronization as well as binding,
/// described for this subpass
/// \details Viewport is reset to render area on every call
/// \note Must be external synced (GPU lock required)
/// \warning Will assert-fail if there is no subpass to advance to
/// \warning Will assert-fail if called outside of render pass
void next_subpass();
/// \brief Ends render pass
/// \details Processes store&sync operations described in render pass
/// \details After this call, any non Draw operations are allowed and render targets are reset to backbuffer
/// \note Must be external synced (GPU lock required)
/// \warning Will assert-fail if subpass is not final
/// \warning Will assert-fail if called  outside of render pass
void end_render_pass();

/** @}*/

#endif

#if !_TARGET_D3D_MULTI
shaders::DriverRenderStateId create_render_state(const shaders::RenderState &state);
bool set_render_state(shaders::DriverRenderStateId state_id);
void clear_render_states();
#endif

#if _TARGET_XBOX || _TARGET_C1 || _TARGET_C2
void resummarize_htile(BaseTexture *tex);
#else
inline void resummarize_htile(BaseTexture *) {}
#endif

#if _TARGET_XBOX
void set_esram_layout(const wchar_t *);
void unset_esram_layout();
void reset_esram_layout();
void prefetch_movable_textures();
void writeback_movable_textures();
#else
inline void set_esram_layout(const wchar_t *) {}
inline void unset_esram_layout() {}
inline void reset_esram_layout() {}
inline void prefetch_movable_textures() {}
inline void writeback_movable_textures() {}
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_DX11
static constexpr int HALF_TEXEL_OFS = 0;
#define HALF_TEXEL_OFSF 0.f
#elif _TARGET_D3D_MULTI
extern float HALF_TEXEL_OFSFU;
#define HALF_TEXEL_OFSF (const float)d3d::HALF_TEXEL_OFSFU
extern bool HALF_TEXEL_OFS;
#else
extern const float HALF_TEXEL_OFSFU;
#define HALF_TEXEL_OFSF d3d::HALF_TEXEL_OFSFU
extern const bool HALF_TEXEL_OFS;
#endif

#if _TARGET_C1 || _TARGET_XBOX
#define D3D_HAS_QUADS 1
#else
#define D3D_HAS_QUADS 0
#endif
}; // namespace d3d


extern void
#if _MSC_VER >= 1300
  __declspec(noinline)
#endif
    d3derr_in_device_reset(const char *msg);
extern bool dagor_d3d_force_driver_reset;

#define d3derr(c, m)                                                   \
  do                                                                   \
  {                                                                    \
    bool res = (c);                                                    \
    G_ANALYSIS_ASSUME(res);                                            \
    if (!res)                                                          \
    {                                                                  \
      bool canReset;                                                   \
      if (dagor_d3d_force_driver_reset || d3d::device_lost(&canReset)) \
        d3derr_in_device_reset(m);                                     \
      else                                                             \
        fatal("%s:\n%s", m, d3d::get_last_error());                    \
    }                                                                  \
  } while (0)

#define d3d_err(c) d3derr((c), "Driver3d error")

#if _TARGET_D3D_MULTI
#include <3d/dag_drv3d_multi.h>
#else
#include <3d/dag_drv3dCmd.h>
#endif
