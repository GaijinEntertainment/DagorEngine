//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/*
 * @file
 * @brief Common interfaces for interacting with the target graphics API.
 */
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

/**
 * @brief Interface providing driver related callbacks used for resource initialization.
 */
class Driver3dInitCallback
{
public:
  /**
  * @brief Class that represents a range of version numbers.
  */
  struct VersionRange
  {
    uint64_t minVersion; /**< The minimum version number. */
    uint64_t maxVersion; /**< The maximum version number. */
  };
  /**
  * @brief Class that represents the size of a render target.
  */
  struct RenderSize
  {
    int width;
    int height;
  };

  /**
   * @brief Function pointer type for determining whether stereo rendering is needed.
   */
  using NeedStereoRenderFunc = bool (*)();

  /**
   * @brief Function pointer type for obtaining the dimensions of stereo rendering.
   */
  using StereoRenderDimensionFunc = int (*)();

  /**
   * @brief Function pointer type for obtaining stereo rendering extensions.
   */
  using StereoRenderExtensionsFunc = const char *(*)();

  /**
   * @brief Function pointer type for obtaining stereo rendering the versions range.
   */
  using StereoRenderVersionsFunc = VersionRange (*)();

  /**
   * @brief Function pointer type for obtaining the stereo rendering adapter id.
   */
  using StereoRenderAdapterFunc = int64_t (*)();

  /**
   * @brief Verifies the resolution settings.
   * 
   * @param [in] ref_scr_wdt    Reference to the screen width.
   * @param [in] ref_scr_hgt    Reference to the screen height.
   * @param [in] base_scr_wdt   Base screen width.
   * @param [in] base_scr_hgt   Base screen height.
   * @param [in] window_mode    Flag indicating whether window mode is enabled.
   * 
   * The function performs an implementation-defined check and, depending on its result, 
   * may reset screen width and height to their base values.
   */
  virtual void verifyResolutionSettings(int &ref_scr_wdt, int &ref_scr_hgt, int base_scr_wdt, int base_scr_hgt, bool window_mode) const
  {
    G_UNUSED(ref_scr_wdt);
    G_UNUSED(ref_scr_hgt);
    G_UNUSED(base_scr_wdt);
    G_UNUSED(base_scr_hgt);
    G_UNUSED(window_mode);
  }

  /**
   * @brief Checks whether driver description is valid.
   * 
   * @param [in] desc   The description to validate.
   * @return            0 if the description is invalid, non-zero otherwise.
   */
  virtual int validateDesc(Driver3dDesc &) const = 0;

  /**
   * @brief Compares two driver 3D descriptions.
   * 
   * @param [in] A, B   The descriptions to compare.
   * @return            -1 if A is better, 1 if B is better, and 0 if they are equivalent.
   */
  virtual int compareDesc(Driver3dDesc &A, Driver3dDesc &B) const = 0;

  // Stereo render related queries
  /**
   * @brief Determines if stereo rendering is desired.
   * 
   * @return \c True if stereo rendering is desired, otherwise \c false.
   */
  virtual bool desiredStereoRender() const { return false; }

  /**
   * @brief Returns id of the desired adapter for stereo rendering.
   * 
   * @return Id of the desired adapter for stereo rendering.
   */
  virtual int64_t desiredAdapter() const { return 0; }

  /**
   * @brief Returns the desired renderer size for stereo rendering.
   * 
   * @return The desired renderer size.
   */
  virtual RenderSize desiredRendererSize() const { return {0, 0}; }

  /**
   * @brief Returns the desired device extensions for the stereo renderer.
   * 
   * @return A string, containing the desired device extensions.
   */
  virtual const char *desiredRendererDeviceExtensions() const { return nullptr; }

  /**
   * @brief Returns the desired device instance extensions for the stereo renderer.
   *
   * @return A string, containing the desired device instance extensions.
   */
  virtual const char *desiredRendererInstanceExtensions() const { return nullptr; }

  /**
   * @brief Returns the desired version range for the stereo renderer.
   * 
   * @return The desired version range for the stereo renderer.
   */
  virtual VersionRange desiredRendererVersionRange() const { return {0, 0}; }
};

//--- Vertex & index buffer interface -------

/**
 * @brief Base class for a shader resource buffer.
 */
class Sbuffer : public D3dResource
{
public:
  DAG_DECLARE_NEW(midmem)

  /**
   * @brief Callback interface for reloading data associated with an Sbuffer instance.
   */
  struct IReloadData
  {
    /**
     * @brief Destructor
     */
    virtual ~IReloadData() {}

    /**
     * @brief An implementation-defined function responsible for reloading data to a buffer.
     * 
     * @param [in] sb A pointer to the buffer which data is to be reloaded.
     */
    virtual void reloadD3dRes(Sbuffer *sb) = 0;
    /**
     * @brief An implementation-defined function responsible for destroying the object that implements the IReloadData interface.
     */
    virtual void destroySelf() = 0;
  };

  /**
   * @brief Sets the reload callback for a shader buffer.
   * 
   * @param [in] reloadData A pointer to a reload callback instance.
   * @return                \c True if the reload callback was set successfully, otherwise \c false.
   */
  virtual bool setReloadCallback(IReloadData *) { return false; }

  /** 
   * @brief Returns the resouce type (\c RES3D_SBUF).
   * 
   * @return The resouce type (\c RES3D_SBUF).
   */
  int restype() const override final { return RES3D_SBUF; }

  /**
   * @brief Locks the buffer.
   * 
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to the position where the locking starts.
   * @param [in]    size_bytes  The size in bytes of the region to lock.
   * @param [out]   p           A pointer to the address of the locked region.
   * @param [in]    flags       Flags specifying the locking behavior.
   * @return                    0 on error, otherwise a non-zero value.
   */
  virtual int lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags) = 0;

  /**
   * @brief Unlocks previously locked buffer.
   * 
   * @return 0 on error, otherwise a non-zero value.
   */
  virtual int unlock() = 0;

  /*
   * @brief Returns the flags associated with the buffer resource.
   * 
   * @return The flags associated with the buffer resource.
   */
  virtual int getFlags() const = 0;

  /**
  * @brief Returns the name of the buffer.
  * 
  * @return The buffer name.
  */
  const char *getBufName() const { return getResName(); }

  // for structured buffers
  /**
   * @brief Gets the size of each element in the structured buffer.
   * 
   * @return The size of each element in the structured buffer.
   */
  virtual int getElementSize() const { return 0; }

  /**
   * @brief Gets the number of elements in the structured buffer.
   * 
   * @return The number of elements in the structured buffer.
   */
  virtual int getNumElements() const { return 0; };

  /**
   * @brief Copies the content to another buffer.
   * 
   * @param [in] dest   A pointer to the destination buffer.
   * @return            \c True if the data was copied successfully, otherwise \c false.
   */
  virtual bool copyTo(Sbuffer * /*dest*/) { return false; } // return true, if copied

  /**
   * @brief Copies a specified region of the contained data to another buffer.
   * 
   * @param [in] dest           A pointer to the destination buffer.
   * @param [in] dst_ofs_bytes  The offset in bytes in the destination buffer at which the copied data is added.
   * @param [in] src_ofs_bytes  The offset in bytes in the source buffer at which the data is copied.
   * @param [in] size_bytes     The size in bytes of the data to copy.
   * @return \c True if the data was copied successfully, otherwise \c false.
   */
  virtual bool copyTo(Sbuffer * /*dest*/, uint32_t /*dst_ofs_bytes*/, uint32_t /*src_ofs_bytes*/, uint32_t /*size_bytes*/)
  {
    return false;
  }

  /**
   * @brief Locks the buffer.
   * 
   * @tparam T The type of data in the buffer.
   * 
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to the position where the locking starts.
   * @param [in]    size_bytes  The size in bytes of the region to lock.
   * @param [out]   p           A typed pointer to the address of the locked region.
   * @param [in]    flags       Flags specifying the locking behavior.
   * @return                    1 on success, otherwise 0.
   */
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

  /**
   * @brief Validates the parameters used for locking the buffer.
   * 
   * @param [in] offset     The offset in bytes from the beginning of the buffer to the position where the locking starts.
   * @param [in] size       The size in bytes of the region to lock.
   * @param [in] flags      Flags specifying the locking behavior.
   * @param [in] bufFlags   Flags associated with the buffer.
   * 
   * In case the parameters were invalid, the fucntion fails an assertion.
   */
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

  /**
   * @brief Updates the data in the buffer after locking it.
   *
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to start updating.
   * @param [in]    size_bytes  The size in bytes of the data to update.
   * @param [out]   src         Pointer to the source data.
   * @param [in]    lockFlags   Flags specifying the locking behavior.
   * @return                    \c True if the data was updated successfully, otherwise \c false.
   *
   * The function locks the buffer and then copies the data from \b src into it.
   */
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

  /**
   * @brief Updates the data in the buffer after locking it.
   *
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to start updating.
   * @param [in]    size_bytes  The size in bytes of the data to update.
   * @param [out]   src         Pointer to the source data.
   * @param [in]    lockFlags   Flags specifying the locking behavior.
   * @return                    \c True if the data was updated successfully, otherwise \c false.
   *
   * The function locks the buffer and then copies the data from \b src into it.
   */
  virtual bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
  {
    G_ASSERT_RETURN(size_bytes, false);
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
  }

  /**
   * @brief Locks the buffer (only for index buffers).
   *
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to the position where the locking starts.
   * @param [in]    size_bytes  The size in bytes of the region to lock.
   * @param [out]   p           A pointer to the address of the locked region of the index buffer.
   * @param [in]    flags       Flags specifying the locking behavior.
   * @return                    1 on success, otherwise 0.
   * 
   * The function locks the index buffer and then copies the data from \b src into it.
   * The buffer must use 16-bit indices i.e. flag \c SBCF_INDEX32 must not be set.
   */
  inline int lock(uint32_t ofs_bytes, uint32_t size_bytes, uint16_t **p, int flags)
  {
    G_ASSERT(!(getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }

  /**
   * @brief Locks the buffer (only for index buffers).
   *
   * @param [in]    ofs_bytes   The offset in bytes from the beginning of the buffer to the position where the locking starts.
   * @param [in]    size_bytes  The size in bytes of the region to lock.
   * @param [out]   p           A pointer to the address of the locked region of the index buffer.
   * @param [in]    flags       Flags specifying the locking behavior.
   * @return                    1 on success, otherwise 0.
   *
   * The function locks the index buffer and then copies the data from \b src into it.
   * The buffer must use 32-bit indices i.e. flag \c SBCF_INDEX32 must be set.
   */
  inline int lock32(uint32_t ofs_bytes, uint32_t size_bytes, uint32_t **p, int flags)
  {
    G_ASSERT((getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }

protected:
  ~Sbuffer() override {}
};

/**
* @brief Index buffer
*/
typedef Sbuffer Ibuffer;

/**
 * @brief Vertex buffer
 */
typedef Sbuffer Vbuffer;

// This input type for 'd3d::resource_barrier' allows one function to handle multiple input data layouts.
// Inputs can be simple single values, arrays of values, pointers to values and initializer lists of values.
// To interpret the stored values use the provided enumerate functions to get the buffer and texture barriers.
// For more details on resource barriers see https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers

/**
* @brief Resource Barrier descriptor suitable for various resource types.
* 
* Class that encompasses one or more resources (buffers, textures). It is used to dispatch transitions for each encompassed resource. 
* The class is expected to be used as input for \ref d3d::resource_barrier.
* An instance of the class may be conveniently initialized with a pointer to resource, an array of resource addresses or an intializer list.
* The resources and the barriers can be visited using \ref enumerateBufferBarriers \ref enumerateTextureBarriers.
*/
class ResourceBarrierDesc
{
  // special count value to distinguish array of values from single value
  /**
  * @brief A reserved value assigned to \c bufferCount or \c textureCount indicating that there is only one buffer or texture to dispatch a transition to.
  */
  static constexpr unsigned single_element_count = ~0u;

  /**
  * @brief Either a pointer to the buffer or an array of pointers to the buffers.
  */
  union
  {
    Sbuffer *buffer;
    Sbuffer *const *buffers;
  };

  /**
   * @brief Either a pointer to the texture or an array of pointers to the textures.
   */
  union
  {
    BaseTexture *texture;
    BaseTexture *const *textures;
  };

  /**
   * @brief Either a state or an array of states that the buffer(s) must be transitioned to.
   */
  union
  {
    ResourceBarrier bufferState;
    const ResourceBarrier *bufferStates;
  };

  /**
   * @brief Either a state or an array of states that the texture(s) must be transitioned to.
   */
  union
  {
    ResourceBarrier textureState;
    const ResourceBarrier *textureStates;
  };

  /**
   * @brief Either a texture subresource index or an array of texture subresource indices.
   */
  union
  {
    unsigned textureSubResIndex;
    const unsigned *textureSubResIndices;
  };

  /**
   * @brief Either a texture subresource range or an array of texture subresource ranges.
   */
  union
  {
    unsigned textureSubResRange;
    const unsigned *textureSubResRanges;
  };

  /**
  * @brief The number of encompassed buffers (must equal to \ref single_element_count if there is only one buffer).
  */
  unsigned bufferCount = 0;

  /**
   * @brief The number of encompassed textures (must equal to \ref single_element_count if there is only one texture).
   */
  unsigned textureCount = 0;

public:

  /**
  * @brief Default constructor.
  * 
  * Initializaes all resources as NULL.
  */
  ResourceBarrierDesc() :
    buffer{nullptr},
    texture{nullptr},
    bufferStates{nullptr},
    textureStates{nullptr},
    textureSubResIndices{nullptr},
    textureSubResRanges{nullptr}
  {}

  /**
  * @brief Copy constructor
  * 
  * @param [in] other An instance to copy from.
  */
  ResourceBarrierDesc(const ResourceBarrierDesc &) = default;

  /**
  * @brief Initialization constructor
  * 
  * @param [in] buf A pointer to the buffer to transition.
  * @param [in] rb  The state to transition the buffer to.
  */
  ResourceBarrierDesc(Sbuffer *buf, ResourceBarrier rb) : buffer{buf}, bufferState{rb}, bufferCount{single_element_count} {}

  /**
   * @brief Initialization constructor
   * 
   * @param [in] bufs   Array of pointers to the buffers to transition.
   * @param [in] rb     Array of states to transition the buffers to.
   * @param [in] count  The number of the buffers.
   * 
   * An i-th state should correspond to an i-th buffer.
   */
  ResourceBarrierDesc(Sbuffer *const *bufs, const ResourceBarrier *rb, unsigned count) :
    buffers{bufs}, bufferStates{rb}, bufferCount{count}
  {}

  /**
   * @brief Initialization constructor
   * 
   * @tparam N Number of buffers.
   * 
   * @param [in] bufs   Array of pointers to the buffers to transition.
   * @param [in] rb     Array of states to transition the buffers to.
   * 
   * An i-th state should correspond to an i-th buffer.
   */
  template <unsigned N>
  ResourceBarrierDesc(Sbuffer *(&bufs)[N], ResourceBarrier (&rb)[N]) : buffers{bufs}, bufferStates{rb}, bufferCount{N}
  {}

  /**
   * @brief Initialization constructor
   * 
   * @param [in] bufs   A list of buffer pointers.
   * @param [in] rb     A list of states to transition to.
   * 
   * An i-th state should correspond to an i-th buffer.
   */
  ResourceBarrierDesc(std::initializer_list<Sbuffer *> bufs, std::initializer_list<ResourceBarrier> rb) :
    buffers{bufs.begin()}, bufferStates{rb.begin()}, bufferCount{static_cast<unsigned>(bufs.size())}
  {
    G_ASSERT(bufs.size() == rb.size());
  }

  /**
   * @brief Initialization constructor
   *
   * @param [in] tex            A pointer to the texture to dispatch transition(s) to.
   * @param [in] rb             The state to transition to.
   * @param [in] sub_res_index  Index of the subresource to begin dispatching transitions at.
   * @param [in] sub_res_range  Range of the subresources.
   */
  ResourceBarrierDesc(BaseTexture *tex, ResourceBarrier rb, unsigned sub_res_index, unsigned sub_res_range) :
    texture{tex},
    textureState{rb},
    textureSubResIndex{sub_res_index},
    textureSubResRange{sub_res_range},
    textureCount{single_element_count}
  {}

  /**
   * @brief Initialization constructor
   *
   * @param [in] texs           Array of pointers to the textures to transition.
   * @param [in] rb             Array of states to transition the textures to.
   * @param [in] sub_res_index  Array of subresource indices to begin dispatching transitions at.
   * @param [in] sub_res_range  Array of subresource ranges.
   * @param [in] count          The number of the textures.
   *
   * An i-th state should correspond to the subresources of 
   * an i-th buffer specified by an \c sub_res_index[i] and \c sub_res_range[i].
   */
  ResourceBarrierDesc(BaseTexture *const *texs, const ResourceBarrier *rb, const unsigned *sub_res_index,
    const unsigned *sub_res_range, unsigned count) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{count}
  {}

  /**
   * @brief Initialization constructor
   *
   * @tparam N Number of textures.
   *
   * @param [in] texs           Array of pointers to the textures to transition.
   * @param [in] rb             Array of states to transition the textures to.
   * @param [in] sub_res_index  Array of subresource indices to begin dispatching transitions at.
   * @param [in] sub_res_range  Array of subresource ranges.
   *
   * An i-th state should correspond to the subresources of
   * an i-th buffer specified by an \c sub_res_index[i] and \c sub_res_range[i].
   */
  template <unsigned N>
  ResourceBarrierDesc(BaseTexture *(&texs)[N], ResourceBarrier (&rb)[N], unsigned (&sub_res_index)[N], unsigned (&sub_res_range)[N]) :
    textures{texs}, textureStates{rb}, textureSubResIndices{sub_res_index}, textureSubResRanges{sub_res_range}, textureCount{N}
  {}

  /**
   * @brief Initialization constructor
   *
   * @param [in] texs           A list of texture pointers.
   * @param [in] rb             A list of states to transition to.
   * @param [in] sub_res_index  A list of subresource indices to begin dispatching transitions at.
   * @param [in] sub_res_range  A list of subresource ranges.
   *
   * An i-th state should correspond to the subresources of
   * an i-th texture specified by \c sub_res_index[i] and \c sub_res_range[i].
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
   * @brief Initialization constructor
   *
   * @param [in] bufs               Array of pointers to the buffers to transition.
   * @param [in] b_rb               Array of states to transition the buffers to.
   * @param [in] b_count            The number of the buffers.
   * @param [in] texs               Array of pointers to the textures to transition.
   * @param [in] t_rb               Array of states to transition the textures to.
   * @param [in] t_sub_res_index    Array of texture subresource indices to begin dispatching transitions at.
   * @param [in] t_sub_res_range    Array of texture subresource ranges.
   * @param [in] t_count            The number of the textures.
   *
   * An i-th buffer state should correspond to an i-th buffer.
   * An j-th texture state should correspond to the subresources of
   * an j-th texture specified by \c sub_res_index[j] and \c sub_res_range[j].
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
   * @brief Initialization constructor
   *
   * @param [in] buf                A pointer to the buffer.
   * @param [in] b_rb               The state to transition the buffer to.
   * @param [in] texs               Array of pointers to the textures to transition.
   * @param [in] t_rb               Array of states to transition the textures to.
   * @param [in] t_sub_res_index    Array of texture subresource indices to begin dispatching transitions at.
   * @param [in] t_sub_res_range    Array of texture subresource ranges.
   * @param [in] t_count            The number of the textures.
   *
   * An j-th texture state should correspond to the subresources of
   * an j-th texture specified by \c sub_res_index[j] and \c sub_res_range[j].
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
   * @brief Initialization constructor
   *
   * @param [in] bufs               Array of pointers to the buffers to transition.
   * @param [in] b_rb               Array of states to transition the buffers to.
   * @param [in] b_count            The number of the buffers.
   * @param [in] tex                A pointer to the texture.
   * @param [in] t_rb               The state to transition the texture to.
   * @param [in] t_sub_res_index    Texture subresource index.
   * @param [in] t_sub_res_range    Texture subresource range.
   *
   * An i-th buffer state should correspond to an i-th buffer.
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
   * @brief Initialization constructor
   *
   * @param [in] buf                A pointer to the buffer.
   * @param [in] b_rb               The state to transition the buffer to.
   * @param [in] tex                A pointer to the texture.
   * @param [in] t_rb               The state to transition the texture to.
   * @param [in] t_sub_res_index    Texture subresource index.
   * @param [in] t_sub_res_range    Texture subresource range.
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
   * @brief Initialization constructor
   * 
   * @param [in] bufs           A list of buffer pointers.
   * @param [in] buf_rb         A list of states to transition the buffers to.
   * @param [in] texs           A list of texture pointers.
   * @param [in] tex_rb         A list of states to transition the textures to.
   * @param [in] sub_res_index  A list of subresource indices to begin dispatching transitions at.
   * @param [in] sub_res_range  A list of subresource ranges.
   *
   * An i-th state should correspond to the subresources of
   * an i-th texture specified by \c sub_res_index[i] and \c sub_res_range[i].
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
   * @brief Initialization constructor
   * @param The state to transition a buffer to.
   * 
   * Expected use case is that 'rb' has the RB_FLUSH_UAV flag set for all pending uav access
   */
  explicit ResourceBarrierDesc(ResourceBarrier rb) : buffer{nullptr}, bufferState{rb}, bufferCount{single_element_count} {}

  /**
  * @brief Applies a callback to each encompassed buffer.
  * 
  * @tparam T The type of the callback.
  * 
  * @param [in] clb The callback to apply.
  * 
  * \b T must implement <c> operator() (SBuffer* buf, ResourceBarrier* rb) </c>.
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
   * @brief Applies a callback to each encompassed texture.
   *
   * @tparam T The type of the callback.
   *
   * @param [in] clb The callback to apply.
   *
   * \b T must implement <c> operator() (BaseTexture* tex, ResourceBarrier* rb, unsigned sub_res_index, unsigned sub_res_range) </c>.
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
};

/**
* @brief Defines how the resource is initialized on activation.
*/
enum class ResourceActivationAction : unsigned
{
  REWRITE_AS_COPY_DESTINATION,  /**< The resource is going to be used as a copy destination. */
  REWRITE_AS_UAV,               /**< The resource is initialized as UAV and is going to be rewritten. */
  REWRITE_AS_RTV_DSV,           /**< The resource is initialized as RTV/DSV and is going to be rewritten. */
  CLEAR_F_AS_UAV,               /**< The resource is initialized as UAV and is going to be cleared to a float value. */
  CLEAR_I_AS_UAV,               /**< The resource is initialized as UAV and is going to be cleared to an integer value. */
  CLEAR_AS_RTV_DSV,             /**< The resource is initialized as RTV/DSV and is going to be cleared. */
  DISCARD_AS_UAV,               /**< The resource is initialized as UAV and is going to be discarded. */
  DISCARD_AS_RTV_DSV,           /**< The resource is initialized as RTV/DSV and is going to be discarded. */
};

/**
* Defines how the depth buffer is accessed in a render pass.
*/
enum class DepthAccess
{
  /**
   * @brief Used for regular depth attachement.
   */ 
  RW,

  /**
   * @brief Used for read-only depth attachement which can also be sampled as a texture in the same shader.
   * 
   * @warning If you don't need to sample the depth, use \c z_write=false with \c DepthAccess::RW 
   * instead instead this option.
   * Using this state will cause HiZ decompression on some hardware and
   * split of renderpass with flush of tile data into memory in a TBR.
   */
  SampledRO
};

/**
* @brief Determines the value a resource is going to be cleaned to.
*/
union ResourceClearValue
{
  /**
   * @brief As R32G32B32A32_UINT.
   */
  struct
  {
    uint32_t asUint[4];
  };

  /**
   * @brief As R32G32B32A32_SINT.
   */
  struct
  {
    int32_t asInt[4];
  };

  /**
   * @brief As R32G32B32A32_FLOAT.
   */
  struct
  {
    float asFloat[4];
  };

  /**
  * @brief As D32_FLOAT_S8_UINT.
  */
  struct
  {
    float asDepth;
    uint8_t asStencil;
  };
};

/**
 * @brief Constructs a resource clear value from color components.
 * 
 * @param r,g,b,a [in]  Color components.
 * @return              A ResourceClearValue initialized as R32G32B32A32_UINT.
 */
inline ResourceClearValue make_clear_value(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  ResourceClearValue result;
  result.asUint[0] = r;
  result.asUint[1] = g;
  result.asUint[2] = b;
  result.asUint[3] = a;
  return result;
}

/**
 * @brief Constructs a resource clear value from color components.
 *
 * @param r,g,b,a [in]  Color components.
 * @return              A ResourceClearValue initialized as R32G32B32A32_SINT.
 */
inline ResourceClearValue make_clear_value(int32_t r, int32_t g, int32_t b, int32_t a)
{
  ResourceClearValue result;
  result.asInt[0] = r;
  result.asInt[1] = g;
  result.asInt[2] = b;
  result.asInt[3] = a;
  return result;
}

/**
 * @brief Constructs a resource clear value from color components.
 *
 * @param r,g,b,a [in]  Color components.
 * @return              A ResourceClearValue initialized as R32G32B32A32_FLOAT.
 */
inline ResourceClearValue make_clear_value(float r, float g, float b, float a)
{
  ResourceClearValue result;
  result.asFloat[0] = r;
  result.asFloat[1] = g;
  result.asFloat[2] = b;
  result.asFloat[3] = a;
  return result;
}

/**
 * @brief Constructs a resource clear value from depth and stencil values.
 *
 * @param d [in]  The value to clear depth buffer to.
 * @param s [in]  The value to clear stencil buffer to.
 * @return        A ResourceClearValue initialized as D32_FLOAT_S8_UINT.
 */
inline ResourceClearValue make_clear_value(float d, uint8_t s)
{
  ResourceClearValue result;
  result.asDepth = d;
  result.asStencil = s;
  return result;
}

/**
 * @brief Defines basic resource parameters.
 */
struct BasicResourceDescription
{
  /**
   * @brief Resource creation flags. 
   * Creation flag names begin with either \c SBCF (for shader buffers) or \c TEXCS (for textures).
   * Check \ref dag_drv3dConsts.h for options.
   */
  uint32_t cFlags;

  /*
   * @brief Resource behavior option on activation.
   */
  ResourceActivationAction activation;

  /**
  * @brief Defines the value that elements (for buffers) or texels (for textures) will be reset to, when the resource is cleared.
   */
  ResourceClearValue clearValue;
};

/**
 * @brief Defines basic buffer parameters.
 */
struct BufferResourceDescription : BasicResourceDescription
{
  /**
   * @brief The byte size of an element in th buffer.
   */
  uint32_t elementSizeInBytes;

  /**
   * @brief The number of elements in the buffer.
   */
  uint32_t elementCount;

  /**
  * @brief Buffer view format.
  */
  uint32_t viewFormat;
};

/**
 * @brief Defines basic texture parameters.
 */
struct BasicTextureResourceDescription : BasicResourceDescription
{
  /**
   * @brief The number of mipmaps this texture provides.
   */
  uint32_t mipLevels; 
};

/**
 * @brief Defines 2D texture parameters.
 */
struct TextureResourceDescription : BasicTextureResourceDescription
{
  /**
   * @brief Texture width.
   */
  uint32_t width;

  /**
   * @brief Texture height.
   */
  uint32_t height;
};

/**
 * @brief Defines #D texture parameters.
 */
struct VolTextureResourceDescription : TextureResourceDescription
{
  /**
   * @brief Texture depth.
   */
  uint32_t depth;
};

/**
 * @brief Defines texture array parameters.
 */
struct ArrayTextureResourceDescription : TextureResourceDescription
{
  /**
   * @brief The number of textures in the array.
   */
  uint32_t arrayLayers; 
};

 /**
 * @brief Defines cube texture parameters.
 */
struct CubeTextureResourceDescription : BasicTextureResourceDescription
{
  /**
   * @brief Extent of the cube texture.
   */
  uint32_t extent;
};

/**
 * @brief Defines cube texture array parameters.
 */
struct ArrayCubeTextureResourceDescription : CubeTextureResourceDescription
{
  /**
   * @brief The number of cube textures in the array.
   */
  uint32_t cubes;
};

/**
 * @brief Defines resource parameters.
 */
struct ResourceDescription
{
  /**
   * @brief The type of the resource.
   * See \ref dag_d3dResource.h for options.
   */
  uint32_t resType;

  /**
  * @brief The description of the resource. 
  */
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

  /**
   * @brief Default constructor.
   */
  ResourceDescription() = default;

  /**
   * @brief Copy constructor.
   * 
   * @param [in] other The description to copy from.
   */
  ResourceDescription(const ResourceDescription &) = default;

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] buf Buffer description.
   * 
   * Initializes resource as buffer.
   */
  ResourceDescription(const BufferResourceDescription &buf) : resType{RES3D_SBUF}, asBufferRes{buf} {}

  /**
   * @brief Initialization constructor.
   *
   * @param [in] tex Texture description.
   *
   * Initializes resource as texture.
   */
  ResourceDescription(const TextureResourceDescription &tex) : resType{RES3D_TEX}, asTexRes{tex} {}

  /**
   * @brief Initialization constructor.
   *
   * @param [in] tex Volume texture description.
   *
   * Initializes resource as volume texture.
   */
  ResourceDescription(const VolTextureResourceDescription &tex) : resType{RES3D_VOLTEX}, asVolTexRes{tex} {}

  /**
   * @brief Initialization constructor.
   *
   * @param [in] tex Texture array description.
   *
   * Initializes resource as texture array.
   */
  ResourceDescription(const ArrayTextureResourceDescription &tex) : resType{RES3D_ARRTEX}, asArrayTexRes{tex} {}

  /**
   * @brief Initialization constructor.
   *
   * @param [in] tex Cube texture description.
   *
   * Initializes resource as cube texture.
   */
  ResourceDescription(const CubeTextureResourceDescription &tex) : resType{RES3D_CUBETEX}, asCubeTexRes{tex} {}

  /**
   * @brief Initialization constructor.
   *
   * @param [in] tex Cube texture array description.
   *
   * Initializes resource as cube texture array.
   */
  ResourceDescription(const ArrayCubeTextureResourceDescription &tex) : resType{RES3D_CUBEARRTEX}, asArrayCubeTexRes{tex} {}

  /**
   * @brief Checks if two resource descriptions are same.
   * 
   * @param [in] The description to compare to.
   */
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
  /**
   * @brief The type of hash value.
   */
  using HashT = size_t;

  /**
   * @brief Evaluates hash value of the description.
   */
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
  // @cond
  static inline HashT hashPack() { return 0; }
  // @endcond

  /**
   * @brief Recursively evaluates hash of a pack of arguments.
   */
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
/**
 * @brief Hash class template specialization for evaluating \ref ResourceDescription hash.
 */
template <>
class hash<ResourceDescription>
{
public:

  /**
   * @brief Evaluates hash value of a \ref ResourceDescription instance.
   */
  size_t operator()(const ResourceDescription &desc) const { return desc.hash(); }
};
} // namespace eastl

struct ResourceHeapGroup;

/**
 * @brief Determines parameters of a group of \ref ResourceHeap "resource heaps".
 */
struct ResourceHeapGroupProperties
{
  /**
   * @brief Allows resource heap group parameters to be addressed separately or as an integer flag.
   */
  union
  {
    /**
     * @brief The flag representation of resource heap group parameters values.
     */
    uint32_t flags;
    struct
    {
      /**
       * @brief Indicates heap direct accessibility by the CPU.
       * 
       * If \c true, the CPU can access this group memory directly.
       * On consoles this is usually true for all heap groups, 
       * on PC it is \c true only for system memory heap groups.
       */
      bool isCPUVisible : 1;

      /**
       * @brief Indicates direct heap accessibility by the GPU.
       * 
       * If \c true, the GPU can access this group memory directly without going over a bus like PCIE. 
       * On consoles this is usually \c true for all heap groups, 
       * on PC it is \c true only for memory dedicated to the GPU.
       */
      bool isGPULocal : 1;

      /**
       * @brief Indicates whether a heap in this group is special on chip memory, like ESRAM of the XB1.
       */
      bool isOnChip : 1;
    };
  };

  /**
   * @brief Determines the maximum size of a resource that can be placed into a heap of this group.
   */
  uint64_t maxResourceSize;

  /**
   * @brief Determines the maximum size of a heap in this group.
   * 
   * The value is usually limited by the amount that is installed in the system. 
   * Drivers may impose other limitations.
   */
  uint64_t maxHeapSize;

  /**
   * @brief Determines the desired maximum size of a heap in this group optimal for memory management.
   * 
   * This is a hint for the user to try to aim for this heap size for best performance.
   * Larger heaps until \ref \c maxHeapSize are still possible, but larger heaps than optimalMaxHeapSize
   * may yield worse performance, as the runtime may have to use sub-optimal memory sources
   * to satisfy the allocation request.
   * A value of 0 indicates that there is no optimal size and any size is expected to
   * perform similarly.
   * For example on DX12 on Windows the optimal size is 64 MiBytes, suggested by MS
   * representatives, as windows may not be able to provide heaps in the requested
   * memory source.
   */
  uint64_t optimalMaxHeapSize;
};

/**
 * @brief Determines parameters for resource memory allocation.
 */
struct ResourceAllocationProperties
{
  /**
   * @brief Resource size.
   */
  size_t sizeInBytes;

  /**
   * @brief Resource alignment.
   */
  size_t offsetAlignment;

  /**
   * @brief A pointer to the group this heap belongs to.
   */
  ResourceHeapGroup *heapGroup;
};

/**
 * @brief A set of flags that steer the behavior of the driver during creation of resource heaps.
 */
enum ResourceHeapCreateFlag
{
  /**
   * @brief The default value.
   * 
   * By default the drivers are allowed to use already reserved memory of internal heaps to allocate the needed memory.
   * Drivers are also allowed to allocate larger memory heaps and use the excess memory for their internal resource
   * and memory management.
   */
  RHCF_NONE = 0,

  /**
   * @brief The heaps in this group will use their own dedicated memory.
   * 
   * Resource heaps created with this flag, will use their own dedicate memory heap to supply the memory for resources.
   * When this flag is not used to create a resource heap, the driver is allowed to source the need memory from existing
   * driver managed heaps, or create a larger underlying memory heap and use the excess memory for its internal resource
   * and memory management.
   * This flag should be used only when really necessary, as it restricts the drivers option to use already reserved
   * memory for this heap and increase the memory pressure on the system.
   */
  RHCF_REQUIRES_DEDICATED_HEAP = 1u << 0,
};

/**
 * @brief The type of \ref ResourceHeap creation flags. 
 * 
 * See \ref ResourceHeapCreateFlag for options.
 */
using ResourceHeapCreateFlags = uint32_t;

struct ResourceHeap;

/**
 * @brief Determines render target data.
 */
struct RenderTarget
{
  /**
   * @brief A pointer to the \ref BaseTexture object used as the render target.
   */
  BaseTexture *tex; 

  /**
   * @brief Level of the mipmap used as the render target within the texture.
   */
  uint32_t mip_level;

  /**
   * @brief Index of the texture used as the render target within the texture array.
   */
  uint32_t layer;
};

/** \defgroup RenderPassStructs
 * @{
 */

/**
 * @brief Description of render target bind inside a render pass.
 * 
 * Fully defines the operation that will happen with the target at defined subpass slot
 */
struct RenderPassBind
{
  /**
   * @brief Index of the render target in render target array that will be used for this bind.
   */
  int32_t target;

  /**
   * @brief Index of the subpass where bind happens.
   */
  int32_t subpass;

  /**
   * @brief Index of the slot where target will be used.
   */
  int32_t slot;

  /**
   * @brief Defines what will happen with the target used in binding.
   */
  RenderPassTargetAction action;

  /**
   * @brief Optional user barrier for generic(emulated) implementation.
   */
  ResourceBarrier dependencyBarrier;
};

/**
 * @brief Early description of render target.
 * 
 * Gives necessary info at render pass creation so the render pass is compatible with the targets of the same type later on.
 */
struct RenderPassTargetDesc
{
  /**
   * @brief Resource from which information is extracted, can be \c NULL.
   */
  BaseTexture *templateResource;

  /**
   * @brief Raw texture create flags; used if the template resource is not provided.
   */
  unsigned texcf;

  /**
   * @brief Must be set if template resource is empty and target will use aliased memory
   */
  bool aliased;
};

/**
 * @brief Description of target that is used inside render pass.
 */
struct RenderPassTarget
{
  /**
   * @brief The actual render target subresource data.
   */
  RenderTarget resource;

  /**
   * @brief Clear value that is used on clear action.
   */
  ResourceClearValue clearValue;
};

/**
 * @brief Render pass resource description, used to create a RenderPass object
 */
struct RenderPassDesc
{
  /**
   * @brief Name that is visible only in developer builds and inside graphics tools, if possible.
   */
  const char *debugName;

   /**
   * @brief Total amount of targets inside the render pass.
   */
  uint32_t targetCount;

  /**
   * @brief Total amount of bindings for the render pass.
   */
  uint32_t bindCount;

  /**
   * @brief Array of \ref \c targetCount elements supplying early descriptions of render targets.
   */
  const RenderPassTargetDesc *targetsDesc;

  /**
   * @brief Array of \ref \c bindCount elements describing all subpasses.
   */
  const RenderPassBind *binds;

  /**
   * @brief Texture binding offset for shader subpass reads used on APIs without native render passes.
   * 
   * Generic(emulated) implementation will use registers starting from this offset, to bind input attachments.
   * This must be properly handled inside shader code for generic implementation to work properly!
   */
  uint32_t subpassBindingOffset;
};

/**
  * @brief Area of the render target where rendering will happen within the render pass.
  */
struct RenderPassArea
{
  /**
   * @brief Left edge pixel coordinate of the area.
   */
  uint32_t left;

  /**
   * @brief Top edge pixel coordinate of the area.
   */
  uint32_t top;

  /**
   * @brief Width of the area in pixels.
   */
  uint32_t width;

  /**
   * @brief Height of the area in pixels.
   */
  uint32_t height;

  /**
   * @brief Determines the minimal rendered depth value for this area from 0.0 to 1.0.
   */
  float minZ;

  /**
   * @brief Determines the maximal rendered depth value for this area from 0.0 to 1.0.
   */
  float maxZ;
};

/**@}*/

// @cond
namespace d3d
{
//! opaque class that represents render pass
struct RenderPass;
} // namespace d3d
// @endcond

/**
 * @brief Contains information about texture tiling.
 */
struct TextureTilingInfo
{
  /**
   * @brief Number of tiles composing the texture.
   */
  size_t totalNumberOfTiles;

  /**
   * @brief Number of unpacked mipmaps.
   */
  size_t numUnpackedMips;

  /**
   * @brief Number of packed mipmaps.
   */
  size_t numPackedMips;

  /**
   * @brief Number of tiles needed for packed mipmaps.
   * 
   * @note Even if \c numPackedMips is zero, \c numTilesNeededForPackedMips may be greater than zero, which is a special case,
   * and \c numTilesNeededForPackedMips tiles still needs to be assigned at \c numUnpackedMips.
   */
  size_t numTilesNeededForPackedMips;

  /**
   * @brief The offset of the first packed tile for the texture in the overall range of tiles.
   */
  size_t firstPackedTileIndex;

  /**
   * @brief Pixel width of a tile.
   */
  size_t tileWidthInPixels;

  /**
   * @brief Pixel height of a tile.
   */
  size_t tileHeightInPixels;

  /**
   * @brief Pixel depth of a tile.
   */
  size_t tileDepthInPixels;

  /**
   * @brief Memory size required for a tile.
   */
  size_t tileMemorySize;

  /**
   * @brief Tile width of the texture.
   */
  size_t subresourceWidthInTiles;

  /**
   * @brief Tile height of the texture.
   */
  size_t subresourceHeightInTiles;

  /**
   * @brief Tile depth of the texture.
   */
  size_t subresourceDepthInTiles;

  /**
   * @brief The index of the first tile corresponding to the texture.
   */
  size_t subresourceStartTileIndex;
};

/**
 * @brief Represents mapping information of a tile within a texture heap.
 */
struct TileMapping
{
  /**
   * @brief The tile X-coordinate in tiles.
   */
  size_t texX;

  /**
   * @brief The tile Y-coordinate in tiles,.
   */
  size_t texY;

  /**
   * @brief The tile Z-coordinate in tiles.
   */
  size_t texZ;

  /**
   * @brief Index of the subresource.
   */
  size_t texSubresource;

  /**
   * @brief Index of the tile in the heap.
   */
  size_t heapTileIndex;

  /**
   * @brief Number of tiles to map. 0 is invalid, and unless it is 1, an array of tiles is mapped to the specified location. 
   * 
   * Example usage for this is packed mip tails:
   * /li Map to subresource \ref \c TextureTilingInfo::numUnpackedMips at 0, 0, 0.
   * /li Use a span of \ref \c TextureTilingInfo::numTilesNeededForPackedMips so the whole mip tail can be mapped at once.
   * /li From \ref \c heapTileIndex the given number of tiles will be mapped to the packed mip tail.
   */
  size_t heapTileSpan;   
};

/**
 * @brief Determines level of API support for a device. 
 */
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
/**
 * @brief Determines texture format usage flags.
 */
enum
{
  USAGE_TEXTURE = 0x01,         /**<Default usage flag for a texture format. */
  USAGE_DEPTH = 0x02,           /**<Indicates the texture format supports depth stencil usage. */
  USAGE_RTARGET = 0x04,         /**<Indicates the texture format supports render target usage. */
  USAGE_AUTOGENMIPS = 0x08,     /**<Indicates the texture format supports automatic generation of mipmaps. */
  USAGE_FILTER = 0x10,          /**<Indicates the texture format supports filtering.*/
  USAGE_BLEND = 0x20,           /**<Indicates the texture format supports blending. */
  USAGE_VERTEXTEXTURE = 0x40,   /**<Indicates the texture format supports vertex texture sampling. */
  USAGE_SRGBREAD = 0x80,        /**<Indicates the texture format supports sRGB conversion when sampling from the texture. */
  USAGE_SRGBWRITE = 0x100,      /**<Indicates the texture format supports sRGB conversion when rendering to the texture. */
  USAGE_SAMPLECMP = 0x200,      /**<Indicates the texture format supports shader comparison functions. */
  USAGE_PIXREADWRITE = 0x400,   /**<Indicates the texture format supports readings and writings by a pixel shader*/
  USAGE_TILED = 0x800,          /**<Indicates the texture format supports usage as tiled resource. */
  USAGE_UNORDERED = 0x1000,     /**<Indicates the texture format supports UAV. */
  USAGE_UNORDERED_LOAD = 0x2000 /**<Indicates the texture format supports unordered loads.*/
};

/**
 * @brief Determines screen capture format.
 */
enum
{
  CAPFMT_X8R8G8B8,  /**<A 32-bit format with 8 bits for each color and 8-bit padding. Suitable for fast captures.*/
  CAPFMT_R8G8B8,    /**<A 24-bit format with 8 bits for each color.*/
  CAPFMT_R5G6B5,    /**<A 16-bit format with 5 bits for red and blue colors and 6 bits for green color.*/
  CAPFMT_X1R5G5B5,  /**<A 16 bit format with 5 bits for each color and 1 bit padding.*/
};

/**
 * @brief Updates window mode according to global settings.
 */
void update_window_mode();

/**
 * @brief Guesses GPU vendor ID as well as additional data.
 * 
 * @param [out] out_gpu_desc    A pointer to write GPU description to. If \c NULL is passed, nothing is written.
 * @param [out] out_drv_ver     A pointer to to write driver version to. If \c NULL is passed, nothing is written.
 * @param [out] out_drv_date    A pointer to write driver date to. If \c NULL is passed, nothing is written.
 * @param [out] device_id       A pointer to write device ID to. If \c NULL is passed, nothing is written.
 * @return                      GPU vendor ID. Check \ref dag_drvDecl.h for options.
 * 
 * May be called before \ref d3d::init_driver().
 */
int guess_gpu_vendor(String *out_gpu_desc = NULL, uint32_t *out_drv_ver = NULL, DagorDateTime *out_drv_date = NULL,
  uint32_t *device_id = nullptr);

/**
 * @brief Returns GPU driver date.
 * 
 * @param [in] vendor   The ID of the GPU vendor providing the driver.
 * @return              The driver date.
 */
DagorDateTime get_gpu_driver_date(int vendor);

/**
 * @brief Determines the size of dedicated GPU memory in KiB.
 * 
 * @return The size of dedicated GPU memory in KiB.
 * 
 * The function may be called before \ref d3d::init_driver().
 */
unsigned get_dedicated_gpu_memory_size_kb();

/**
 * @brief Determines the size of free dedicated GPU memory in KiB.
 *
 * @return The size of free dedicated GPU memory in KiB.
 * The function may be called before \ref d3d::init_driver().
 */
unsigned get_free_dedicated_gpu_memory_size_kb();

/**
 * @brief Determines the size of dedicated GPU memory in KiB (only for NVidia GPUs).
 *
 * @param [out] dedicated_total A pointer to write the size of dedicated memory to. If NULL is passed, nothing is written.
 * @param [out] dedicated_free  A pointer to write the size of free dedicated memory to. If NULL is passed, nothing is written.
 * 
 * This is a faster implementation to get memory size during the game, but it supports only NVidia GPUs.
 * If NVidia API is not found, zeros will be written.
 */
void get_current_gpu_memory_kb(uint32_t *dedicated_total, uint32_t *dedicated_free);

/**
 * @brief Determines GPU frequency in MHzs.
 *
 * @param [out] out_freq A string to write the frequency to.
 */
bool get_gpu_freq(String &out_freq);

/**
 * @brief Determines GPU temperature in degree Celsius.
 *
 * @return The temperature of the GPU.
 */
int get_gpu_temperature();

/**
 * @brief Returns a string containing video adapter info.
 *
 * @return The string with video adapter info.
 */
void get_video_vendor_str(String &out_str);

/**
 * @brief Returns scaled DPI of the primary display.
 *
 * The result is divided by 96 (96 dpi is for 100% scale in Windows).
 * @return The scaled DPI value.
 */
float get_display_scale();

/**
 * @brief Disables SLI mode.
 * 
 * Must be called before device creation.
 */
void disable_sli();

/**
 * @brief When passed as a function argument for a texture array layer, instructs the function to render to each texture in the array.
 */
static constexpr int RENDER_TO_WHOLE_ARRAY = 1023;
#if !_TARGET_D3D_MULTI
// Driver initialization API

/**
 * @brief Initalizes 3D device driver.
 *
 * @return \c true on success, \c false otherwise.
 */
bool init_driver();

/**
 * @brief Checks whether 3D API was initialized.
 * 
 * @return \c true if the API was init, \c false otherwise.
 */
bool is_inited();

/**
 * @brief Creates application output window and initializes API.
 * 
 * @param [in] hinst        A platform-specific application instance handle.
 * @param [in] wndproc      A pointer to the callback which proccesses main window messages.             
 * @param [in] wcname       Window class name.
 * @param [in] ncmdshow     Controls how the output window is to be shown. For details check the corresponding parameter description <a href="https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow">here</a>.
 * @param [in] mainwnd      A platform-specific application main window handle.
 * @param [in] renderwnd    A platform-specific handle to the window serving as a target for rendering. If NULL is passed, then the manin window is used for rendering.
 * @param [in] hicon        A platform-specific application icon handle.
 * @param [in] title        Window title.
 * @param [in] cb           A pointer to the callback used during the initialization.
 * @return                  \c true on success, \c false on failure.
 * 
 * @note If <c>renderwnd!=NULL</c>, both \b mainwnd and \b renderwnd are used for rendering (when possible).
 */
bool init_video(void *hinst, main_wnd_f *, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

/**
 * @brief Destroys the application output window and releases all resources.
 */
void release_driver();

/**
 * @brief Fills the function-pointer table for d3d API if supported.
 * 
 * @param [out] d3dit   The table to fill.
 * @return              \c true if function-pointer table is supported, \c false otherwise.
 */
bool fill_interface_table(D3dInterfaceTable &d3dit);

/**
 * @brief Retrieves the driver name.
 * 
 * @return A pointer to the static string containing the driver name.
 */
const char *get_driver_name();

/**
 * @brief Retrieves the code of the active driver.
 * 
 * @return The code of the active driver.
 */
DriverCode get_driver_code();

/**
 * @brief Checks if d3d-stub driver is used.
 * 
 * @return \c true if the active driver if d3d-stub, \c false otherwise.
 */
static inline bool is_stub_driver() { return get_driver_code().is(d3d::stub); }

/**
 * @brief Retrieves the device driver version.
 * 
 * @return A pointer to the static string containing the device driver version.
 */
const char *get_device_driver_version();

/**
 * @brief Retrieves the device name.
 *
 * @return A pointer to the static string containing the device name.
 */
const char *get_device_name();

/**
 * @brief Retrieves the last error message.
 *
 * @return A pointer to the static string containing the last error message.
 */
const char *get_last_error();

/**
 * @brief Retrieves the last error code.
 *
 * @return The last error code.
 */
uint32_t get_last_error_code();

/**
 * @brief Performs implementation and platform specific actions prior to the window destruction.
 */
void prepare_for_destroy();

/**
 * @brief Performs implementation and platform specific actions at the window destruction.
 * 
 * @param [in] The destroyed window platform-specific handle.
 */
void window_destroyed(void *hwnd);


// Device management

/// returns raw pointer to device interface

/**
 * @brief Retrieves a raw pointer to the device interface (implementation and platform specific). 
 * 
 * @return A raw pointer to the device interface.
 */
void *get_device();

/**
 * @brief Retrieves a raw pointer to the device context interface (implementation and platform specific).
 *
 * @return A raw pointer to the device context interface.
 * 
 * @note No usage and implementation is found.
 */
void *get_context();

/// returns driver description (pointer to static object)
/**
 * @brief Retrieves the acive driver description.
 * @return A reference to the static instance of \c Driver3dDesc class containing the description of the active driver.
 */
const Driver3dDesc &get_driver_desc();

/**
 * @brief Sends a command to the active driver.
 * 
 * @param [in] command          Enumerator (DRD3D_COMMAND_) of the command to set. See \ref dag_drv3dCmd.h for options.
 * @param [in] par1, par2, par3 Implementation-specific parameters.
 * @return                      An implementation-specific value.
 */
int driver_command(int command, void *par1, void *par2, void *par3);

/**
 * @brief Checks if the device is lost.
 * 
 * @param [out] can_reset_now   An address by which the function will write \c true if it is safe to reset the device and \c false otherwise. If \c NULL is passed, nothing is written.
 * @return                      \c true if the device is lost, \c false otherwise.
 */
bool device_lost(bool *can_reset_now);

/**
 * @brief Resets the device.
 *
 * @return \c true if the reset was successful, \c false otherwise.
 */
bool reset_device();

/**
 * @brief Checks if the device if lost or being reset.
 * 
 * @return \c true if the device is either lost or being reset now.
 */
bool is_in_device_reset_now();

/**
 * @brief Checks if the game rendering window is completely occluded now.
 *
 * @return \c true if the game rendering window is completely occluded now, \c false otherwise.
 */
bool is_window_occluded();

// Texture management

/**
 * @brief Checks if compute commands should be used for texture processing.
 * 
 * @param [in] formats  A list of texture creation flags.
 * @return              \c true if compute commands should be preferred, \c false if only graphic commands can be used.
 */
bool should_use_compute_for_image_processing(std::initializer_list<unsigned> formats);

/**
 * @brief Checks whether a format is available for a simple texture creation.
 * 
 * @param [in] cflg Texture creation flags.
 * @return          \c false if a texture of the specified format can't be created.
 */
bool check_texformat(int cflg);

/**
 * @brief Retrieves maximum sample count for a given texture format.
 *
 * @param [in] cflg Texture creation flags.
 * @return          The maximum sample format supported by the texture format.   
 */
int get_max_sample_count(int cflg);

/**
 * @brief Retrieves usage flags from creation flags of a texture resource.
 * 
 * @param [in] cflg     Creation flags of a texture resource.
 * @param [in] restype  The resource type. It must be a value from \c ES3D_ enumeration.
 * @return              Texture resource usage flags.
 */
unsigned get_texformat_usage(int cflg, int restype = RES3D_TEX);

/**
 * @brief Checks whether two simple texture creation flags correspond to the same texture format.
 * 
 * @param [in] cflg1, cflg2 Creation flags to compare.
 * @return                  \c true if the flags correspond to the same texture format.
 */
bool issame_texformat(int cflg1, int cflg2);

/**
 * @brief Checks whether a format is available for a cube texture creation.
 * 
 * @param [in] cflg Cube texture creation flags.
 * @return          \c false if a cube texture of the specified format can't be created.
 */
bool check_cubetexformat(int cflg);

/**
 * @brief Checks whether two cube texture creation flags correspond to the same texture format.
 *
 * @param [in] cflg1, cflg2 Creation flags to compare.
 * @return                  \c true if the flags correspond to the same texture format.
 */
bool issame_cubetexformat(int cflg1, int cflg2);

/**
 * @brief Checks whether a format is available for a volume texture creation.
 *
 * @param [in] cflg Volume texture creation flags.
 * @return          \c false if a volume texture of the specified format can't be created.
 */
bool check_voltexformat(int cflg);

/**
 * @brief Checks whether two volume texture creation flags correspond to the same texture format.
 *
 * @param [in] cflg1, cflg2 Creation flags to compare.
 * @return                  \c true if the flags correspond to the same texture format.
 */
bool issame_voltexformat(int cflg1, int cflg2);

/**
 * @brief Creates a texture object from an image.
 * 
 * @param [in] img          A pointer to the image to create the texture from. If \c NULL is passed, the texture will be created empty.
 * @param [in] w            Texture width.
 * @param [in] h            Texture height.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support. 
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 * 
 * @note If \c NULL is passed for \b img, proper values for \b w and \b h must be specified.
 * @note If either \b w or \h is 0, the corresponding value is taken from \b img (hence, it must not be \c NULL).
 */
BaseTexture *create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a texture object from a DDSx stream.
 * 
 * @param [in] crd          A callback supporting file read interface.
 * @param [in] flg          Texture creation flags.
 * @param [in] quality_id   Texture quality level index (0=high, 1=medium, 2=low, 3=ultralow).
 * @param [in] levels       The number of mipmaps to load. If 0 is passed, the whole set is loaded, given the device support. 
 * @param [in] stat_name    Texture debug name.
 * @return                  A pointer to the created texture object or NULL on error.
 * 
 */
BaseTexture *create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels = 0, const char *stat_name = NULL);

/**
 * @brief Creates a texture object from a DDSx header and allocates memory (without loading) for the texture contents.
 * 
 * @param [in] hdr          DDSx header to create the texture from.
 * @param [in] flg          Texture creation flags.
 * @param [in] quality_id   Texture quality level index (0=high, 1=medium, 2=low, 3=ultralow).
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support. 
 * @param [in] stat_name    Texture debug name.
 * @param [in] stub_tex_idx Index of the stub texture to use. If \b stub_tex_idx is negative, no stub texture is used.
 * @return                  A pointer to the created texture object or NULL on error.
 */
BaseTexture *alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels = 0, const char *stat_name = NULL,
  int stub_tex_idx = -1);


/**
 * @brief Loads texture content from a DDSx stream into a previously created texture object.
 * 
 * @param [in, out] t       A pointer to the previously created texture object.
 * @param [in]      hdr     DDSx header for the created texture object.
 * @param [in]      crd     A callback supporting file read interface.
 * @param [in]      q_id    Texture quality level index (0=high, 1=medium, 2=low, 3=ultralow).
 * @return                  \c true on success, \c false otherwise.
 * 
 * @note Texture object \b t has to have been created using \ref alloc_ddsx_tex.
 */
static inline bool load_ddsx_tex_contents(BaseTexture *t, const ddsx::Header &hdr, IGenLoad &crd, int q_id)
{
  return d3d_load_ddsx_tex_contents ? d3d_load_ddsx_tex_contents(t, t->getTID(), BAD_TEXTUREID, hdr, crd, q_id, 0, 0) : false;
}

/**
 * @brief Creates a cube texture object.
 * 
 * @param [in] size         Cube texture edge length.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support. 
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 */
BaseTexture *create_cubetex(int size, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a volume texture object.
 * 
 * @param [in] w, h, d      Volume texture dimensions.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support. 
 * @param [in] stat_name    Debug name of the texture.
 * @return                  A pointer to the created texture object or NULL on error.
 */
BaseTexture *create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a texture array object.
 * 
 * @param [in] w, h         Texture dimensions.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture array.
 * @return                  A pointer to the created texture array object or NULL on error.
 */
BaseTexture *create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name);

/**
 * @brief Creates a cube texture array object.
 *
 * @param [in] side         Cube texture edge length.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture array.
 * @return                  A pointer to the created texture array object or NULL on error.
 * 
 * @note The function actually creates array of <c>d * 6 <\c> simple textures, where group of 6 layers make up a cube map.
 */
BaseTexture *create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name); // total layers d*6

/**
 * @brief Creates a simple texture alias from another texture object.
 * 
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] img          A pointer to the image to create the texture from. If \c NULL is passed, the texture will be created empty.
 * @param [in] w, h         Texture dimensions.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 * 
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_tex, except creating alias does not result in memory allocation, 
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture. 
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 * 
 * @note    If \c NULL is passed for \b img, proper values for \b w and \b h must be specified.
 * @note    If either \b w or \h is 0, the corresponding value is taken from \b img (hence, it must not be \c NULL).
 * @warning Passing non-NULL for \b img will result in \b baseTexture content overwrite.
 */
BaseTexture *alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a cube texture alias from another texture object.
 *
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] size         Cube texture edge length.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 *
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_cubetex, except creating alias does not result in memory allocation,
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture.
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 */
BaseTexture *alias_cubetex(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a volume texture alias from another texture object.
 *
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] w, h, d      Texture dimensions.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 *
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_voltex, except creating alias does not result in memory allocation,
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture.
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 */
BaseTexture *alias_voltex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name = NULL);

/**
 * @brief Creates a texture array alias from another texture object.
 *
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] w, h         Texture dimensions.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 *
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_array_tex, except creating alias does not result in memory allocation,
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture.
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 */
BaseTexture *alias_array_tex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name);

/**
 * @brief Creates a texture array alias from another texture object.
 *
 * @param [in] baseTexture  A pointer to the texture to create an alias for.
 * @param [in] side         Cube texture edge length.
 * @param [in] d            Number of layers in the array.
 * @param [in] flg          Texture creation flags.
 * @param [in] levels       The number of mipmaps to create. If 0 is passed, the whole set will be created, given the device support.
 * @param [in] stat_name    Debug name of the texture alias.
 * @return                  A pointer to the created alias or NULL on error.
 *
 * An alias can be considered as a view of or as a cast to its base texture but of a different format.
 * Calling this function has same effects as \ref create_cube_array_tex, except creating alias does not result in memory allocation,
 * and the GPU memory of the base texture will be used as the backing GPU memory for the alias.
 * For an alias is just a view of the GPU memory area, its lifetime is controlled by the base texture.
 * Hence, no alias texture should be used in the rendering pipeline after its base texture has been deleted.
 */
BaseTexture *alias_cube_array_tex(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name);

// @cond
// There is no implementation found for these functions.
bool set_tex_usage_hint(int w, int h, int mips, const char *format, unsigned int tex_num);
/// discards all managed textures
void discard_managed_textures();
// @endcond

/**
 * @brief Copies and stretches texture.
 * 
 * @param [in]      src     A pointer to the texture to copy from.
 * @param [in, out] dst     A pointer to the texture to copy to.
 * @param [in]      rsrc    A pointer to the area rectangle to copy from.   
 * @param [in]      rdst    A pointer to the area rectangle to copy to.
 * @return                  \c true on success, \c false otherwise.
 * 
 * @note In case NULL is passed for \b src/\b dst, copy is performed from/to the back buffer texture.
 * @note In case NULL is passed for \b rsrc/\b rdst, missing area data is taken from the source/destination texture.
 * @note In case NULL is passed for \b src/\b dst as well as for \b rsrc/\b rdst, the copy is performed from/to the top left corner of the back buffer.
 * @warning At least either \b src or \b dst has to be a valid pointer to the texture.
 */
bool stretch_rect(BaseTexture *src, BaseTexture *dst, RectInt *rsrc = NULL, RectInt *rdst = NULL);

/**
 * @brief Copies the current render target to a specified texture.
 * 
 * @param [out] to_tex  A pointer to the texture to copy to.
 * @return              \c true on success, \c false otherwise.
 * 
 * @note The copied texture is stretched to match the destination size.
 */
bool copy_from_current_render_target(BaseTexture *to_tex);

/**
 * @brief Gathers statistic data on texture usage.
 * 
 * @param [out] num_textures    An address to write the number of textures used to. In case NULL is passed, nothing is written.
 * @param [out] total_mem       An address to write the amount of memory occupied by textures to. In case NULL is passed, nothing is written.
 * @param [out] out_dump        An address to write detailed statistic information to. In case NULL is passed, nothing is written.
 */
void get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump);

// Texture states setup

// r/o and r/w resource slots are independent
/**
 * @brief Binds a texture to a resource slot of a shader stage.
 * 
 * @param [in] shader_stage Shader stage to bind the texture to. It must be a value from STAGE_ enumeration. Check \ref dag_drv3dConsts.h for options.
 * @param [in] slot         Resource slot index to bind the texture to.
 * @param [in] tex          A pointer to the texture to bind.
 * @param [in] use_sampler  Indicates whether sampling data from the texture object will be used to create and bind sampler to the corresponding resource slot. If \c false is passed, no sampler will be bound.
 * @return                  \c true on success, \c false otherwise.
 */
bool set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex, bool use_sampler = true);

/**
 * @brief Binds a texture to a resource slot of the pixel shader stage.
 *
 * @param [in] slot Pixel shader stage resource slot index to bind the texture to.
 * @param [in] tex  A pointer to the texture to bind.
 * @return          \c true on success, \c false otherwise.
 * 
 * @note Calling this function also binds a sampler created from sampling data from the texture object to the corresponding resource slot of the shader stage.
 */
static inline bool settex(int slot, BaseTexture *tex) { return set_tex(STAGE_PS, slot, tex); }

/**
 * @brief Binds a texture to a resource slot of the vertex shader stage.
 *
 * @param [in] slot Vertex shader stage resource slot index to bind the texture to.
 * @param [in] tex  A pointer to the texture to bind.
 * @return          \c true on success, \c false otherwise.
 *
 * @note Calling this function also binds a sampler created from sampling data from the texture object to the corresponding
 * resource slot of the shader stage.
 */
static inline bool settex_vs(int slot, BaseTexture *tex) { return set_tex(STAGE_VS, slot, tex); }

/**
 * @brief Creates a sampler.
 * 
 * @param [in] sampler_info Sampling data to create the sampler from.
 * @return                  A handle of the created sampler.
 * 
 * Identical infos may yield both identical handles and different ones, it depends on driver implementation
 * This call is thread-safe and does not require external synchronization. ???
 */
SamplerHandle create_sampler(const SamplerInfo &sampler_info);

// Destroys given sampler
// 
/**
 * @brief Destroys a given sampler.
 * 
 * @param [in] sampler A handle of the sample to destroy.
 *
 * @warning The driver may not actually destroy the sampler. Using a destroyed sampler after calling this function leads to undefined behavior.
 */
void destroy_sampler(SamplerHandle sampler);

/**
 * @brief Binds a given sampler to a specified slot.
 * 
 * @param [in] shader_stage Shader stage to bind sampler to.
 * @param [in] slot         Index of the slot to bind the sampler to.
 * @param [in] sampler      A handle of the sampler to bind.
 * 
 * @warning This call is not thread-safe, requires global gpu lock to be imposed.
 */
void set_sampler(unsigned shader_stage, unsigned slot, SamplerHandle sampler);

/**
 * @brief Creates a bindless sampler and registers it in the global bindless sampler table.
 * 
 * @param [in] texture  A pointer to the texture from which sampling data is retrieved.
 * @return              Index of the sampler in the table.
 * 
 * @note Textures with identical samplers may result in identical return values.
 */
uint32_t register_bindless_sampler(BaseTexture *texture);

/**
 * @brief Allocates a persistent bindless slot range for a specified resource type.
 * 
 * @param [in] resource_type    Resource type for which the slots are allocated. It must be one of \c RES3D_ anum values.
 * @param [in] count            Number of slots to allocate. It must be positive.
 * @return                      Index of the first allocated slot in the bindless heap.
 */
uint32_t allocate_bindless_resource_range(uint32_t resource_type, uint32_t count);

/**
 * @brief Resizes a given bindless slot range.
 *
 * @param [in] resource_type    The resource type the freed slot has been previously allocated with. The type must be one of \c RES3D_ enum values.
 * @param [in] index            The index of the first slot to free. It must be within a previously allocated bindless range.
 * @param [in] count            Number of slots to free. It must fit within a previously allocated bindless slot range starting at \b
 * index.
 *
 * @note When \b count is 0, the function does nothing. In that case, any value may be passed for \b index.
 *
 * Frees previously allocated slot range.
 * This can also be used to shrink ranges, similarly to \c \ref resize_bindless_resource_range.
 */
uint32_t resize_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t current_count, uint32_t new_count);

/**
 * @brief Frees a given bindless slot range. 
 * 
 * @param [in] resource_type    The resource type the freed slot has been previously allocated with. The type must be one of \c RES3D_ enum values.
 * @param [in] index            The index of the first slot to free. It must be within a previously allocated bindless range.
 * @param [in] count            Number of slots to free. It must fit within a previously allocated bindless slot range starting at \b index.
 * 
 * @note When \b count is 0, the function does nothing. In that case, any value may be passed for \b index.
 * 
 * Frees previously allocated slot range. 
 * This can also be used to shrink ranges, similarly to \c \ref resize_bindless_resource_range.
 */
void free_bindless_resource_range(uint32_t resource_type, uint32_t index, uint32_t count);

/**
 * @brief Updates a given bindless slot with the reference to a specified resource.
 * 
 * @param [in] index    Index of the slot to update. It must be within a previously allocated bindless range.
 * @param [in] res      A pointer to the resource to update the slot with. The resource must be a valid D3DResource object.
 * 
 * @note The resource type the updated slot has been previously allocated with has to match the type of \b res.
 */
void update_bindless_resource(uint32_t index, D3dResource *res);

/**
 * @brief  Updates one or more bindless slots with a 'null' resource of the given type.
 * 
 * @param [in] resource_type    The resource type to update slots with. It must be one of \c RES3D_ enum values.
 * @param [in] index            Index of the first slot to update. It must be within a previously allocated bindless slot range.
 * @param [in] count            The number of slots to update. It must fit within a previously allocated bindless slot range starting at \b index.
 * 
 * Updates one or more bindless slots with a "null" resource of the given type.
 * Shader access to those slots will read all zeros and writes will be discarded.
 */
void update_bindless_resources_to_null(uint32_t resource_type, uint32_t index, uint32_t count);

/**
 * @brief Assigns a UAV texture to a parameter slot for a shader.
 * 
 * @param [in] shader_stage Shader stage index for which the texture will be assigned. It must be one of \c STAGE_ enum values. Check \ref dag_drv3dConst.h for options.
 * @param [in] slot         Shader parameter slot to which the texture will be assigned.
 * @param [in] tex          A pointer to the texture to assign.
 * @param [in] face         Texture array index of the texture to assign.
 * @param [in] mip_level    Index of the mipmap level to assign.
 * @param [in] as_uint      Tells whether the texture will be viewed as uint in UAV.
 * 
 * For details on \b as_uint see https://msdn.microsoft.com/en-us/library/windows/desktop/ff728749(v=vs.85).aspx.
 */
bool set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint = false);

/**
 * @brief Clears a given UAV texture and fills it with specified integer values.
 *
 * @param [in] buf          A pointer to the buffer to clear.
 * @param [in] val          The value to fill the cleared buffer with.
 * @param [in] face         Texture array index of the texture to clear.
 * @param [in] mip_level    Index of the mipmap level to clear.
 * @return                  \c true on success, \c false otherwise.
 */
bool clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level);

/**
 * @brief Clears a given UAV texture and fills it with specified float values.
 *
 * @param [in] buf          A pointer to the buffer to clear.
 * @param [in] val          The value to fill the cleared buffer with.
 * @param [in] face         Texture array index of the texture to clear.
 * @param [in] mip_level    Index of the mipmap level to clear.
 * @return                  \c true on success, \c false otherwise.
 */
bool clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level);

/**
 * @brief Clears a given UAV buffer and fills it with specified integer values.
 * 
 * @param [in] buf  A pointer to the buffer to clear.
 * @param [in] val  The value to fill the cleared buffer with.
 * @return          \c true on success, \c false otherwise.
 */
bool clear_rwbufi(Sbuffer *buf, const unsigned val[4]);

/**
 * @brief Clears a given UAV buffer and fills it with specified float values.
 * @param [in] buf  A pointer to the buffer to clear.
 * @param [in] val  The value to fill the cleared buffer with.
 * @return          \c true on success, \c false otherwise.
 */
bool clear_rwbuff(Sbuffer *buf, const float val[4]);

// Combined programs

/**
 * @brief Creates a combined program and adds it to the program pool.
 * 
 * @param [in] vprog    A handle to the \ref VertexShader instance (i.e. vertex program) to use.
 * @param [in] fsh      A handle to the \ref PixelShader instance to use.
 * @param [in] vdecl    A handle to the vertex declaration to use.
 * @param [in] strides  Array of vertex strides, where each element corresponds to a vertex buffer (unimplemented).
 * @param [in] streams  Number of vertex buffers used (unimplemented).
 * @return              Index of the created program in the programm pool.
 * 
 * @note In case \b strides and \b streams are not set, the necessary values are extracted from the input layout.
 * @note Created programs should be deleted externally.
 * 
 * Indices passed have to correspond to objects stored in static object pools (see \ref shaders.cpp for details).
 * 
 */
PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides = 0, unsigned streams = 0);

/**
 * @brief Creates a combined program and adds it to the program pool.
 *
 * @param [in] vpr_native   Vertex program binary data.
 * @param [in] fsh_native   Pixel shader binary data.
 * @param [in] vdecl        A handle to the vertex declaration to use.
 * @param [in] strides      Array of vertex strides, where each element corresponds to a vertex buffer (unimplemented).
 * @param [in] streams      Number of vertex buffers used (unimplemented).
 * @return                  Index of the created program in the programm pool.
 *
 * @note In case \b strides and \b streams are not set, the necessary values are extracted from the input layout.
 * @note Created programs should be deleted externally.
 *
 * Indices passed have to correspond to objects stored in static object pools (see \ref shaders.cpp for details).
 *
 */
PROGRAM create_program(const uint32_t *vpr_native, const uint32_t *fsh_native, VDECL vdecl, unsigned *strides = 0,
  unsigned streams = 0);

/**
 * @brief Creates a compute program and adds it to the program pool.
 * 
 * @param [in] cs_native    Compute shader binary data.
 * @param [in] preloaded    Indicates whether the shader is preloaded.
 * @return                  Index of the created program in the programm pool.
 */
PROGRAM create_program_cs(const uint32_t *cs_native, CSPreloaded preloaded);

/**
 * @brief Binds shader programs to pipeline stages.
 * 
 * @param [in] prg  A handle to the program to bind.
 * @return          \true if the program was bound successfully, \c false otherwise.
 * 
 * Depending on which members in \b prg are set, the function chooses the pipeline (compute or graphic) to bind 
 * to. In case graphic pipeline is chosen, vertex program, pixel shader and vertex declaration get bound.
 */
bool set_program(PROGRAM prg);   

/**
* @brief Deletes the vertex program or a pixel shader.
* 
* @param [in] prg A handle to the program to delete. 
*   
* Vertex declaration should be deleted independently
*/
void delete_program(PROGRAM prg); // deletes vprog and fshader. VDECL 

// Vertex programs

/**
 * @brief Creates a vertex program and adds it to the object pool.
 * 
 * @param [in] native_code  Binary data of the program to create.
 * @return                  Index of the created program.
 */
VPROG create_vertex_shader(const uint32_t *native_code);

/**
 * @brief Deletes the vertex program.
 * 
 * @param [in] vs   A handle to the program to delete.
 */
void delete_vertex_shader(VPROG vs);

/**
 * @brief Sets data in the constant buffer.
 * 
 * @param [in] stage    Index of the stage for which the buffer data is set.
 * @param [in] reg_base Index of the first shader resource register to set the data for.
 * @param [in] data     Data to set.
 * @param [in] num_regs Number of registers to set the data for.
 */
bool set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs);

/**
 * @brief Sets data in a given constant buffer of a vertex stage.
 *
 * @param [in] reg_base Index of the first shader resource register to set the data for.
 * @param [in] data     Data to set.
 * @param [in] num_regs Number of registers to set the data for.
 */
static inline bool set_vs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_VS, reg_base, data, num_regs);
}

/**
 * @brief Sets data in a given constant buffer of a pixel stage.
 *
 * @param [in] reg_base Index of the first shader resource register to set the data for.
 * @param [in] data     Data to set.
 * @param [in] num_regs Number of registers to set the data for.
 */
static inline bool set_ps_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_PS, reg_base, data, num_regs);
}

/**
 * @brief Sets data in a given constant buffer of a compute shader.
 *
 * @param [in] reg_base Index of the first shader resource register to set the data for.
 * @param [in] data     Data to set.
 * @param [in] num_regs Number of registers to set the data for.
 */
static inline bool set_cs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_CS, reg_base, data, num_regs);
}

/**
 * @brief Sets data in a given constant buffer of a vertex shader.
 *
 * @param [in] reg              Index of the shader resource register to set the data for.
 * @param [in] v0, v1, v2, v3   Data to set.
 */
static inline bool set_vs_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_vs_const(reg, v, 1);
}

/**
 * @brief Sets data in a given constant buffer of a pixel shader.
 *
 * @param [in] reg              Index of the shader resource register to set the data for.
 * @param [in] v0, v1, v2, v3   Data to set.
 */
static inline bool set_ps_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_ps_const(reg, v, 1);
}
//???????
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

/**
 * @brief Creates a pixel shader and adds it to the object pool.
 *
 * @param [in] native_code  Binary data of the shader to create.
 * @return                  Index of the created shader.
 */
FSHADER create_pixel_shader(const uint32_t *native_code);

/**
 * @brief Deletes a given pixel shader.
 *
 * @param [in] ps   Index of the shader to delete.
 */
void delete_pixel_shader(FSHADER ps);

// Immediate constant buffers - valid within min(driver acquire, frame)
// to unbind, use set_const_buffer(stage, 0, NULL [,0]) - generic set_const_buffer()
// if slot = 0 is empty (PS/VS/CS stages), buffered constants are used
// [PS4 specific]
//???????
bool set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs);
bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset = 0, uint32_t consts_size = 0);

/**
 * @brief Sets the size of a vertex stage constant buffer to an adjusted value.
 * 
 * @param [in] required_size    Desired value of the buffer size.
 * @return                      The adjusted value.
 * 
 * The adjusted value is guaranteed to be greater or equal to \b required_size,
 * unless it is greater than the maximal possible size.
 */
int set_vs_constbuffer_size(int required_size);

/**
 * @brief Sets the size of a pixel stage constant buffer to an adjusted value.
 *
 * @param [in] required_size    Desired value of the buffer size.
 * @return                      The adjusted value.
 *
 * The adjusted value is guaranteed to be greater or equal to \b required_size, 
 * unless it is greater than the maximal possible size.
 */
int set_cs_constbuffer_size(int required_size);

//??????/
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

/**
 * @brief Creates an empty vertex buffer and adds it to the buffer list.
 * 
 * @param [in] size_bytes   Buffer size.
 * @param [in] flags        Buffer creation flags.
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
Sbuffer *create_vb(int size_bytes, int flags, const char *name = "");

/**
 * @brief Creates an empty index buffer and adds it to the buffer list.
 *
 * @param [in] size_bytes   Buffer size.
 * @param [in] flags        Buffer creation flags.
 * @param [in] stat_name    Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
Sbuffer *create_ib(int size_bytes, int flags, const char *stat_name = "ib");


/**
 * @brief Creates structured buffer and adds it to the buffer list.
 * 
 * @param [in] struct_size  Size of the structure.
 * @param [in] elements     Number of elements in the buffer.
 * @param [in] flags        Buffer creation flags.
 * @param [in] texfmt       Element format (for non-structured buffers used in rendering).
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name);

// set structure buffer - uses same slots as textures
/**
 * @brief Binds structured buffer to a shader stage slot. 
 * 
 * @param [in] shader_stage Index of the shader stage to bind the buffer to. Must be a value from \c STAGE_ enum.
 * @param [in] slot         Index of the slot to bind the buffer to.
 * @param [in]              A pointer to the buffer to bind.
 * @return                  \c true on success, \c false otherwise.
 * 
 * @note Buffers and textures use same slots.
 * @note For UAV buffers use \ref set_rwbuffer.
 */
bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

/**
 * @brief Binds read-write buffer to a shader stage slot.
 *
 * @param [in] shader_stage Index of the shader stage to bind the buffer to. Must be a value from \c STAGE_ enum.
 * @param [in] slot         Index of the slot to bind the buffer to.
 * @param [in]              A pointer to the buffer to bind.
 * @return                  \c true on success, \c false otherwise.
 */
bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

// Render targets

/**
 * @brief Resets current render target to the back buffer color texture.
 * 
 * @return  \c true on success, \c false otherwise.
 */
bool set_render_target();

/// set depth texture target. NULL means NO depth. use set_backbuf_depth for backbuf render target
/**
 * @brief Sets current render target depth texture to a given texture.
 * 
 * @param [in] tex      A pointer to the texture to use as depth texture. If NULL is passed, depth rendering is disabled.
 * @param [in] access   Depth texture access parameters.    
 * @return              \c true on success, \c false otherwise. 
 */
bool set_depth(BaseTexture *tex, DepthAccess access);

/**
 * @brief Sets current render target depth texture to a given texture.
 *
 * @param [in] tex      A pointer to the texture to use as depth texture. If NULL is passed, depth rendering is disabled.
 * @param [in] layer    ???
 * @param [in] access   Depth texture access parameters.
 * @return              \c true on success, \c false otherwise.
 */
bool set_depth(BaseTexture *tex, int layer, DepthAccess access);

/**
 * @brief Resets current render target depth texture to back buffer depth texture.
 *
 * @return  \c true on success, \c false otherwise.
 */
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

/**
 * @brief Retrieves current render target.
 * 
 * @param [out] Current render target.
 */
void get_render_target(Driver3dRenderTarget &out_rt);


bool set_render_target(const Driver3dRenderTarget &rt);

/**
 * @brief Retrieves current render target resoultion.
 * 
 * @param [out] w, h    Current render target resolution.  
 * @return              \true on success, \c false otherwise.
 */
bool get_target_size(int &w, int &h);

/**
 * @brief Retrieves resolution of a render target or back buffer.
 *
 * @param [out] w, h    Render target resolution.
 * @param [in]  rt_tex  Render target texture. It must yield \c TEXCF_RTARGET flag.
 * @return              \true on success, \c false otherwise.
 * 
 * @note If NULL is passed, the function returns resoultion of the back buffer texture. 
 */
bool get_render_target_size(int &w, int &h, BaseTexture *rt_tex, int lev = 0);

/**
 * @brief Sets variable rate shading setup for next draw calls.
 * 
 * @param [in] rate_x, rate_y   Specify per-draw-call shading rate.
 * @param [in] vertex_combiner  Specifies how per-draw-call shading rate is combined with shading-rate possible output from the vertex or geometry shader.
 * @param [in] pixel_combiner   Specifies how \b vertex_combiner output is combined with the image-based shading rate determined by the source texture.
 * 
 * @note    Except that 1x4 and 4x1 VRS values are invalid, 
 *          they may be any combination of 1, 2 or 4 
            (provided that the corresponding feature is supported).\n
 * @note    Depth/Stencil values are always computed at full rate and so
 *          shaders that modify depth value output may interfere with \b pixel_combiner. \n
 * @note    For shader outputs see SV_ShadingRate, note
 *          that the provoking vertex or the per primitive value is used./n
 * @note    Use \ref set_variable_rate_shading_texture to set image-based shading rate source texture.
 */
void set_variable_rate_shading(unsigned rate_x, unsigned rate_y,
  VariableRateShadingCombiner vertex_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH,
  VariableRateShadingCombiner pixel_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH);

/**
 * @brief Sets the variable rate shading texture for the next draw calls.
 * 
 * @param [in] rate_texture The texture to use as a shading rate source.
 * 
 * @note    When you start modifying the used texture, you
 *          should reset the used shading rate texture to NULL to ensure that on
 *          next use as a shading rate source, the texture is in a state the device
 *          can use. \n
 * @note    It is invalid to call this when DeviceDriverCapabilities::hasVariableRateShadingTexture
 *          feature is not supported.
 */
void set_variable_rate_shading_texture(BaseTexture *rate_texture = nullptr);

// Rendering

/**
 * @brief Sets the transformation matrix for the current frame state.
 * 
 * @param [in] which    Index of the transformation matrix to set. It has to be either \c TM_WORLD, \c TM_VIEW or \c TM_PROJ.
 * @param [in] tm       A pointer to the assigned matrix.
 * @return              \c true on success, \c false otherwise.
 */
bool settm(int which, const Matrix44 *tm);

/**
 * @brief Sets the transformation matrix for the current frame state.
 *
 * @param [in] which    Index of the transformation matrix to set. It has to be either \c TM_WORLD, \c TM_VIEW or \c TM_PROJ.
 * @param [in] tm       Assigned matrix.
 * @return              \c true on success, \c false otherwise.
 */
bool settm(int which, const TMatrix &tm);

/**
 * @brief Sets the transformation matrix for the current frame state.
 *
 * @param [in] which    Index of the transformation matrix to set. It has to be either \c TM_WORLD, \c TM_VIEW or \c TM_PROJ.
 * @param [in] out_tm   Assigned matrix.
 */
void settm(int which, const mat44f &out_tm);

/**
 * @brief Retrieves the transformation matrix from the current frame state.
 * 
 * @param [in]  which       Specifies the transformation matrix to retrieve. It has to be a value from \c TM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [out] out_tm      A pointer to the retrieved matrix.
 * @return                  \c true on success, \ false otherwise.
 */
bool gettm(int which, Matrix44 *out_tm);

/**
 * @brief Retrieves the transformation matrix from the current frame state.
 *
 * @param [in]  which       Specifies the transformation matrix to retrieve. It has to be a value from \c TM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [out] out_tm      A reference to the retrieved matrix.
 * @return                  \c true on success, \ false otherwise.
 */
bool gettm(int which, TMatrix &out_tm);

/**
 * @brief Retrieves the transformation matrix from the current frame state.
 *
 * @param [in]  which       Specifies the transformation matrix to retrieve. It has to be a value from \c TM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [out] out_tm      A reference to the retrieved matrix.
 */
void gettm(int which, mat44f &out_tm);

/**
 * @brief Retrieves the transformation matrix from the current frame state.
 *
 * @param [in]  which   Specifies the transformation matrix to retrieve.
 * @return              A reference to the retrieved matrix.
 */
const mat44f &gettm_cref(int which);

/**
 * @brief Retrieves the model-to-view transformation matrix from the current frame state.
 * 
 * @param [in] out_m2v A reference to the retrieved matrix.
 */
void getm2vtm(TMatrix &out_m2v);

/**
 * @brief Retrieves the model-to-clip(full) transformation matrix and (optional) scale coefficients from the current frame state.
 *
 * @param [in] out_tm A reference to the retrieved matrix.
 */
void getglobtm(Matrix44 &);

/**
 * @brief Sets the model-to-clip(full) transformation matrix for the current frame state.
 *
 * @param [in] tm The matrix to assign.
 */
void setglobtm(Matrix44 &);

/**
 * @brief Retrieves the model-to-clip(full) transformation matrix from the current frame state.
 *
 * @param [in] out_tm A reference to the retrieved matrix.
 */
void getglobtm(mat44f &);

/**
 * @brief Sets the model-to-clip(full) transformation matrix for the current frame state.
 *
 * @param [in] tm The matrix to assign.
 */
void setglobtm(const mat44f &);

/**
 * @brief Calculates a perspective projection matrix from the perspective data and sets it for the current frame state.
 * 
 * @param [in]  p       Perspective data.
 * @param [out] proj_tm An address to write the calculated matrix. If NULL is passed, nothing is written.
 * @return              \c true on success, \c false otherwise.
 */
bool setpersp(const Driver3dPerspective &p, TMatrix4 *proj_tm = nullptr);

/**
 * @brief Retrieves the perspective data from the current frame state.
 * 
 * @param [out] p   The retrieved data.
 * @return          \c true on success, \c false otherwise.
 */
bool getpersp(Driver3dPerspective &p);

/**
 * @brief Validates the persective data.
 * 
 * @param [in] p    The data to validate.
 * @return          \c true if the data is valid, \c false otherwise.
 */
bool validatepersp(const Driver3dPerspective &p);

/**
 * @brief Calculates a perspective projection matrix from the perspective data.
 *
 * @param [in]  p       Perspective data.
 * @param [out] proj_tm The calculated matrix.
 * @return              \c true on success, \c false otherwise.
 * 
 * @note To set perspective projection matrix from persepctive data use \ref setpersp.
 */
bool calcproj(const Driver3dPerspective &p, mat44f &proj_tm);

/**
 * @brief Calculates a perspective projection matrix from the perspective data.
 *
 * @param [in]  p       Perspective data.
 * @param [out] proj_tm The calculated matrix.
 * @return              \c true on success, \c false otherwise.
 *
 * @note To set perspective projection matrix from persepctive data use \ref setpersp.
 */
bool calcproj(const Driver3dPerspective &p, TMatrix4 &proj_tm);

/**
 * @brief Calculates a model-to-clip matrix.
 *
 * @param [in]  view_tm View matrix.
 * @param [in]  proj_tm Projection matrix.
 * @param [out] result  The calculated matrix.
 *
 * @note To set model-to-clip matrix use \ref setglobtm.
 */
void calcglobtm(const mat44f &view_tm, const mat44f &proj_tm, mat44f &result);

/**
 * @brief Calculates a model-to-clip matrix.
 *
 * @param [in]  view_tm View matrix.
 * @param [in]  persp   Perspective projection data.
 * @param [out] result  The calculated matrix.
 *
 * @note To set model-to-clip matrix use \ref setglobtm.
 */
void calcglobtm(const mat44f &view_tm, const Driver3dPerspective &persp, mat44f &result);

/**
 * @brief Calculates a model-to-clip matrix.
 *
 * @param [in]  view_tm View matrix.
 * @param [in]  proj_tm Projection matrix.
 * @param [out] result  The calculated matrix.
 *
 * @note To set model-to-clip matrix use \ref setglobtm.
 */
void calcglobtm(const TMatrix &view_tm, const TMatrix4 &proj_tm, TMatrix4 &result);

/**
 * @brief Calculates a model-to-clip matrix.
 *
 * @param [in]  view_tm View matrix.
 * @param [in]  persp   Perspective projection data.
 * @param [out] result  The calculated matrix.
 *
 * @note To set model-to-clip matrix use \ref setglobtm.
 */
void calcglobtm(const TMatrix &view_tm, const Driver3dPerspective &persp, TMatrix4 &result);

/**
 * @brief Sets scissor rectangles for the first viewport.
 * 
 * @param [in] x, y Coordinates of the scissor rectangle top-left corcer position.
 * @param [in] w, h Scissor rectangle dimensions.
 * @return          \c true on success, \c false otherwise.
 */
bool setscissor(int x, int y, int w, int h);

/**
 * @brief Sets scissor rectangles for each viewport.
 * 
 * @param [in] scissorRects A span of scissor rectangles to assign.
 * @return                  \c true on success, \c false otherwise.
 */
bool setscissors(dag::ConstSpan<ScissorRect> scissorRects);

/**
 * @brief Sets rectangle for the first viewport.
 * 
 * @param [in] x, y Coordinates of the viewport rectangle top-left corner.
 * @param [in] w, h Dimensions of the viewport rectangle.
 * @param [in] minz Minimal depth of the viewport.
 * @param [in] maxz Maximal depth of the viewport.
 * @return          \c true on success, \c false othrwise.
 */
bool setview(int x, int y, int w, int h, float minz, float maxz);

/**
 * @brief Sets rectangles for each viewport.
 * 
 * @param [in] viewports    A span of viewport rectangles to assign.
 * @return                  \c true on success, \c false otherwise.
 */
bool setviews(dag::ConstSpan<Viewport> viewports);

/** 
 * @brief Returns the first viewport rectangle. 
 * 
 * @param [out] x, y    Coordinates of the top-left corner of the first viewport rectangle.
 * @param [out] w, h    Dimensions of the first viewport rectangle.
 * @param [out] minz    Minimal depth of the first viewport rectangle.
 * @param [out] maxz    Maximal depth of the first viewport rectangle.
 * @return              \c true on success, \c false otherwise.
 */
bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz);

/// clear all view
/**
 * @brief Clears all all viewports.
 * 
 * @param [in] what     Flags indicating what will be cleared (render target, depth or stencil buffer). It has to be a combination of values from \c CLEAR_ enumeration.
 * @param [in] color    Color to fill render target with.
 * @param [in] z        Value to fill depth buffer with.
 * @param [in] stecnil  Value to fill stencil buffer with.
 * @return              \c true on success, \c false otherwise.
 */
bool clearview(int what, E3DCOLOR, float z, uint32_t stencil);

/**
 * @brief Swaps buffers in the swap chain and, if necessary, copies the back buffer to the screen.
 * 
 * @param [in] app_active   Indicates whether the app window is active or in the background.
 * @return                  \c true on success, \c false otherwise.
 */
bool update_screen(bool app_active = true);

/**
 * @brief Sets the vertex stream source for a vertex buffer.
 * 
 * @param [in] stream       Stream slot.
 * @param [in] vb           A pointer to the vertex buffer.
 * @param [in] offset       Offset from the beginning of the vertex data to the data associated with this stream.
 * @param [in] stride_bytes Vertex stride.
 * @return                  \c true on success, \c false otherwise.
 */
bool setvsrc_ex(int stream, Sbuffer *, int offset, int stride_bytes);

/**
 * @brief Sets the vertex stream source for a vertex buffer without offset.
 *
 * @param [in] stream       Stream slot.
 * @param [in] vb           A pointer to the vertex buffer.
 * @param [in] stride_bytes Vertex stride.
 * @return                  \c true on success, \c false otherwise.
 */
inline bool setvsrc(int stream, Sbuffer *vb, int stride_bytes) { return setvsrc_ex(stream, vb, 0, stride_bytes); }
/// set indices
/**
 * @brief Binds the index buffer to the current render state.
 * 
 * @param [in] ib   A pointer to the index buffer to bind.
 * @return          \c true on success, \c false otherwise.
 */
bool setind(Sbuffer *ib);

// create dx8-style vertex declaration
// returns BAD_VDECL on error
VDECL create_vdecl(VSDTYPE *vsd);

/**
 * @brief Deletes the vertex declaration.
 * @param [in] vdecl A handle to the vertex declaration to delete.
 */
void delete_vdecl(VDECL vdecl);


/// set current vertex declaration
/**
 * @brief Binds the vertex declaration to the current render state.
 * 
 * @param [in] vdecl    A handle to the vertex declaration to bind.
 * @return              \c true on success, \c false otherwise.
 */
bool setvdecl(VDECL vdecl);

/**
 * @brief Performs an instanced draw call on non-indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] start            Index of the first primitive to draw.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] num_instances    Number of instances to draw.
 * @param [in] start_instance   Index of the first instance to draw. 
 * @return                      \c true on success, \c false otherwise.
 */
bool draw_base(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance);

/**
 * @brief Performs a non-instanced draw call on non-indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] start            Index of the first primitive to draw.
 * @param [in] numprim          Number of primitives to draw.
 * @return                      \c true on success, \c false otherwise.
 * 
 * For instanced draw call on non-indexed vertices see \ref draw_instanced.\n
 * For non-instanced draw call on indexed vertices see \ref drawind.\n
 * For instanced draw call on indexed vertices see \ref drawind_instanced.
 */
inline bool draw(int type, int start, int numprim) { return draw_base(type, start, numprim, 1, 0); }

/**
 * @brief Performs an instanced draw call on non-indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] start            Index of the first primitive to draw.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] num_instances    Number of instances to draw.
 * @param [in] start_instance   Index of the first instance to draw.
 * @return                      \c true on success, \c false otherwise.
 * 
 * For non-instanced draw call on non-indexed vertices see \ref draw.\n
 * For non-instanced draw call on indexed vertices see \ref drawind.\n
 * For instanced draw call on indexed vertices see \ref drawind_instanced.
 */
inline bool draw_instanced(int type, int start, int numprim, uint32_t num_instances, uint32_t start_instance = 0)
{
  return draw_base(type, start, numprim, num_instances, start_instance);
}

/**
 * @brief Performs an instanced draw call on indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] startind         Offset of the first index.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] base_vertex      Offset of the first indexed vertex. This value is added to each index in the draw call.
 * @param [in] num_instances    Number of instances to draw.
 * @param [in] start_instance   Index of the first instance to draw.
 * @return                      \c true on success, \c false otherwise.
 */
bool drawind_base(int type, int startind, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance);

/**
 * @brief Performs an instanced draw call on indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] startind         Offset of the first index.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] base_vertex      Offset of the first indexed vertex. This value is added to each index in the draw call.
 * @param [in] num_instances    Number of instances to draw.
 * @param [in] start_instance   Index of the first instance to draw.
 * @return                      \c true on success, \c false otherwise.
 *
 * For non-instanced draw call on indexed vertices see \ref drawind.\n
 * For non-instanced draw call on non-indexed vertices see \ref draw.\n
 * For instanced draw call on non-indexed vertices see \ref draw_instanced.
 */
inline bool drawind_instanced(int type, int startind, int numprim, int base_vertex, uint32_t num_instances,
  uint32_t start_instance = 0)
{
  return drawind_base(type, startind, numprim, base_vertex, num_instances, start_instance);
}

/**
 * @brief Performs a non-instanced draw call on indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] startind         Offset of the first index.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] base_vertex      Offset of the first indexed vertex. This value is added to each index in the draw call.
 * @return                      \c true on success, \c false otherwise.
 *
 * For instanced draw call on indexed vertices see \ref drawind_instanced.\n
 * For non-instanced draw call on non-indexed vertices see \ref draw.\n
 * For instanced draw call on non-indexed vertices see \ref draw_instanced.
 */
inline bool drawind(int type, int startind, int numprim, int base_vertex)
{
  return drawind_base(type, startind, numprim, base_vertex, 1, 0);
}

/**
 * @brief Performs a non-instanced draw call on non-indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] ptr              Array of primitives to draw.
 * @param [in] stride_bytes     Vertex stride.
 * @return                      \c true on success, \c false otherwise.
 *
 * For a draw call on indexed vertices see \ref drawind_up.
 * @warning     The function creates an inline vertex buffer from the vertices retrieved from \b ptr.
                This is a slow operation. For quicker rendering use \ref draw.
 */
bool draw_up(int type, int numprim, const void *ptr, int stride_bytes);

/**
 * @brief Performs a non-instanced draw call on indexed vertices.
 *
 * @param [in] type             Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] minvert          (unused; perhaps, it is the index of the first vertex to draw)
 * @param [in] numvert          Number of vertices to draw.
 * @param [in] numprim          Number of primitives to draw.
 * @param [in] ind              Array of vertex indices.
 * @param [in] ptr              Array of vertices to draw.
 * @param [in] stride_bytes     Vertex stride.
 * @return                      \c true on success, \c false otherwise.
 *
 * For a draw call on non-indexed vertices see \ref draw_up.
 * @warning     The function creates inline vertex and index buffers from the data retrieved from the input pointers.
                This is a slow operation. For quicker rendering use \ref drawind.
 */
bool drawind_up(int type, int minvert, int numvert, int numprim, const uint16_t *ind, const void *ptr, int stride_bytes);

/**
 * @brief Performs an indirect draw call on non-indexed vertices.
 * 
 * @param [in] type         Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] args         A pointer to the buffer storing draw call arguments.
 * @param [in] byte_offset  Offset in \b args to the start of the draw call arguments.
 * @return                  \c true on success, \c false otherwise.
 * \b args buffer must have the following layout:
 * <c>  { \n
 *          uint vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation;\n
 *      }\n </c>
 * 
 * For an indirect draw call on indexed vertices see \ref draw_indexed_indirect.
 */
bool draw_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

/**
 * @brief Performs an indirect draw call on indexed vertices.
 *
 * @param [in] type         Primitive topology type to use in this draw call. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] args         A pointer to the buffer storing draw call arguments.
 * @param [in] byte_offset  Offset in \b args to the start of the draw call arguments.
 * @return                  \c true on success, \c false otherwise.
 * 
 * \b args buffer must have the following layout:
 * <c>  { \n
 *          uint indexCountPerInstance, instanceCount, startIndexLocation; \n 
 *          int baseVertexLocation, startInstanceLocation;\n
 *      }\n </c>
 *
 * For an indirect draw call on non-indexed vertices see \ref draw_indirect.
 */
bool draw_indexed_indirect(int type, Sbuffer *args, uint32_t byte_offset = 0);

/**
 * @brief Performs multiple indirect draw calls on non-indexed vertices.
 *
 * @param [in] prim_type    Primitive topology type to use in draw calls. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] args         Array of buffers storing draw call arguments.
 * @param [in] draw_count   Number of draw calls to perform.
 * @param [in] stride_bytes Stride for iterating through \b args.
 * @param [in] byte_offset  Offset in \b args to the start of the draw call arguments.
 * @return                  \c true on success, \c false otherwise.
 *
 * \b args buffer must have the following layout:
 * <c>  { \n
 *          uint vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation;\n
 *      }\n </c>
 * 
 * For multiple indirect draw calls on indexed vertices see \ref multi_draw_indexed_indirect.
 */
bool multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);

/**
 * @brief Performs multiple indirect draw calls on non-indexed vertices.
 *
 * @param [in] prim_type    Primitive topology type to use in draw calls. It must be a value from \c PRIM_ enumeration. See \ref dag_3dConst_base.h for options.
 * @param [in] args         Array of buffers storing draw call arguments.
 * @param [in] draw_count   Number of draw calls to perform.
 * @param [in] stride_bytes Stride for iterating through \b args.
 * @param [in] byte_offset  Offset in \b args to the start of the draw call arguments.
 * @return                  \c true on success, \c false otherwise.
 *
 * \b args buffer must have the following layout:
 * <c>  { \n
 *          uint indexCountPerInstance, instanceCount, startIndexLocation; \n
 *          int baseVertexLocation, startInstanceLocation;\n
 *      }\n </c>
 *
 * For multiple indirect draw calls on non-indexed vertices see \ref multi_draw_indirect.
 */
bool multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset = 0);

/**
 * @brief Submits a command list for computation.
 * 
 * @param [in] thread_group_x, thread_group_y, thread_group_z   Number of thread groups dispatched in each dimension. 
 * @param [in] gpu_pipeline                                     The type of pipeline to which the command queue to use for dispatch corresponds.
 * @return                                                      \c true on success, \c false otherwise.
 */
bool dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Submits a command list for indirect computation.
 *
 * @param [in] args         A pointer to the buffer containing dispatch arguments.
 * @param [in] byte_offset  Offset in \b args to the start of the dispatch arguments.
 * @param [in] gpu_pipeline The type of pipeline to which the command queue to use for dispatch corresponds.
 * @return                  \c true on success, \c false otherwise.
 */
bool dispatch_indirect(Sbuffer *args, uint32_t byte_offset = 0, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
// Max value for each direction is 64k, product of all dimensions can not exceed 2^22

/**
 * @brief Submits a command list for mesh shading pipeline.
 *
 * @param [in] thread_group_x, thread_group_y, thread_group_z Thread group counters for each dimension.
 * 
 * @note    Max value for each direction is 64000. Moreover, the product of all dimensions can not exceed 2^22.
 * @warning Mesh shading pipeline is supported only by DirectX12 and Vulcan. 
 * @warning However, it is implemented only in DirectX12 (PC) API.
 */
void dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
// Args are the same as dispatch_indirect { uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z }

/**
 * @brief Submits a command list for indirect execution in mesh shading pipeline.
 *
 * @param [in] args             Array of buffers to the buffer containing dispatch arguments.
 * @param [in] dispatch_count   Number of dispatch calls.
 * @param [in] stride_bytes     Stride for iterating through \b args.
 * @param [in] byte_offset      Offset in \b args to the start of the dispatch arguments.
 *
 * \b args buffer must have the following layout:
 * <c>  { \n
 *           uint32_t thread_group_x, thread_group_y, thread_group_z
 *      }\n </c>
 * 
 * @note    Max value for each direction is 64000. Moreover, the product of all dimensions can not exceed 2^22.
 * @warning Mesh shading pipeline is supported only by DirectX12 and Vulcan.
 * @warning However, it is implemented only in DirectX12 (PC) API.
 */
void dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset = 0);
// 

/**
 * @brief Submits a command list for indirect execution in mesh shading pipeline.
 *
 * @param [in] args                 Array of buffers containing dispatch arguments.
 * @param [in] args_stride_bytes    Stride for iterating through \b args. 
 * @param [in] byte_offset          Offset in \b args to the start of the dispatch arguments.
 * @param [in] count                A pointer to the buffer containing the number of dispatch calls.
 * @param [in] count_byte_offset    Offset in \b count to the number of dispatch calls.
 * @param [in] max_count            Maximal value for the number of dispatch calls.
 *
 * This function is a variant of \ref dispatch_mesh_indirect where the number of dispatch calls (\c dispatch_count)
 * is read by the GPU from \b count buffer at \b count_offset (as uint32_t). The number of calls cannot exceed \b max_count.
 * 
 * \b args buffer must have the following layout:
 * <c>  { \n
 *           uint32_t thread_group_x, thread_group_y, thread_group_z
 *      }\n </c>
 *
 * @note    Max value for each direction is 64000. Moreover, the product of all dimensions can not exceed 2^22.
 * @warning Mesh shading pipeline is supported only by DirectX12 and Vulcan.
 * @warning However, it is implemented only in DirectX12 (PC) API.
 */
void dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count);

// @cond
// not implemented (always returns BAD_GPUFENCEHANDLE)
GPUFENCEHANDLE insert_fence(GpuPipeline gpu_pipeline);
//not implemented
void insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline);
// additional L2 cache flush calls would need to be added to support CPU <-> Compute Context data communication
// (at least on Xbox One, see XDK docs, article "Optimizing Monolithic Driver Performance", part "Manual Cache and Pipeline Flushing")
// @endcond

// Render states

/**
 * @brief Sets anti-aliasing type to use. 
 * 
 * @param [in] aa_type  Type of FXAA to use. It must be a value from \ref FxaaType enumeration. See \ref antialising.h for options.
 * @return              \c true on success, \c false otherwise.
 * 
 * Currently unimplemented.
 */
bool setantialias(int aa_type);

/**
 * @brief Retrieves currently used anti-aliasing type.
 * 
 * @return A value from \ref FxaaType enumeration determining currently used FXAA type. See \ref antialising.h for options.
 */
int getantialias();

/**
 * @brief Sets blend factors for each of RGBA components.
 * 
 * @param [in] color    Blend factors to assign.
 * @return              \c true on success, \c false otherwise.
 */
bool set_blend_factor(E3DCOLOR color);

/**
 * @brief Sets the reference value for depth stencil tests for the current render state.
 * 
 * @param [in] ref  Reference value to perform against when doing a depth-stencil test.
 * @return          \c true on success, \c false otherwise.
 */
bool setstencil(uint32_t ref);

/**
 * @brief Sets fill mode for the current render state.
 * 
 * @param [in] in   Indicates whether the fill mode must be set to wireframe.
 * @return          \c true on success, \c false otherwise.
 */
bool setwire(bool in);

/**
 * @brief Sets depth test bounds.
 * 
 * @param [in] zmin, zmax   Depth test bound values to assign.
 * @return                  \c false if depth test feature is not supported, \c true otherwise.
 * 
 * @note\b zmin is clamped within [0; 1]. \b zmax is clamped within [\b zmin; 1].
 */
bool set_depth_bounds(float zmin, float zmax);

/**
 * @brief Checks whether depth test feature is supported.
 * 
 * @return \c true if depth test feature is supported, \c false otherwise.
 */
bool supports_depth_bounds(); // returns true if hardware supports depth bounds. same as get_driver_desc().caps.hasDepthBoundsTestS

// Miscellaneous

/**
 * @brief Switches sRGB/linear write to backbuffer.
 * 
 * @param [in] on   Indicates whether sRGB is to be turned on.
 * @return          \c true if sRGB write was enabled before the call, \c false otherwise.
 */
bool set_srgb_backbuffer_write(bool); // returns previous result. switch on/off srgb write to backbuffer (default is off)

//@cond 
//unimplemented
bool set_msaa_pass();
bool set_depth_resolve();
//@endcond

/**
 * @brief Sets gamma value to use.
 * 
 * @param [in]  Gamma power value.
 * @return      \c true on success, \c false otherwise.
 */
bool setgamma(float);

//@cond
//deprecated + unimplemented + unclear what it is
bool isVcolRgba();
//@endcond

/**
 * @brief Retrieves used screen aspect ratio.
 * 
 * @return  The used screen aspect ratio.
 */
float get_screen_aspect_ratio();

/**
 * @brief Sets used screen aspect ratio.
 * 
 * @param [in] ar   Aspect ratio value to assign.
 */
void change_screen_aspect_ratio(float ar);

// Screen capture

/**
 * @brief Makes screen capture.
 * 
 * @param [out] w,h             Resoultion of the screenshot.
 * @param [out] stride_bytes    Stride to iterate through rows of pixels of the capture image.
 * @param [out] format          Screen capture data format. It is a value from \c CAPFMT_ enumeration.
 * @return                      A pointer to the captured image. NULL is returned on failure.
 * 
 * A series of many captures is made faster with this function than with \ref capture_screen (hence, the name).
 * Though, a valid result is not guaranteed by both functions.
 * @warning Any series of \ref fast_capture_screen has to be terminated with \ref end_fast_capture_screen call.\n
 * \b stride_bytes is the actual scanline byte size of the capture texture (i.e. row pitch).
 */
void *fast_capture_screen(int &w, int &h, int &stride_bytes, int &format);

/**
 * @brief terminates a series of fast screen captures.
 * 
 * For fast screen captures use fast_capture_screen.
 */
void end_fast_capture_screen();

/**
 * @brief Makes screen capture.
 * 
 * @param [out] w,h             Resoultion of the screenshot.
 * @param [out] stride_bytes    Stride to iterate through rows of pixels of the capture image.
 * @return                      A pointer to the captured image. NULL is returned on failure.
 * 
 * It is recommended to use \ref for multiple screen captures fast_capture_screen.
 */
TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes);

/**
 * @brief Releases the buffer used for screen captures.
 */
void release_capture_buffer();

/**
 * @brief Retrieves the size of the backbuffer.
 * 
 * @param [out] w, h    Resolution of the back buffer.
 * 
 * @note The retrieved size can be different from the size of the framebuffer.
 */
void get_screen_size(int &w, int &h);

//! conditional rendering.

//! conditional rendering is used to skip rendering of triangles completelyon GPU.
//! the only commands, that would be ignored, if survey fails are DIPs
//! (all commands and states will still be executed), so it is better to use
//! reports to completely skip object rendering
//! max index is defined per platform
//! surveying.

/**
 * @brief Creates a predicate for conditional rendering.
 * 
 * @return Id of the created predicate or -1 on failure.
 */
int create_predicate(); // -1 if not supported or something goes wrong

/**
 * @brief Removes previously created predicate.
 * 
 * @param [in] id Index of the predicate to remove.
 * 
 * To create a predicate use \ref create_predicate.
 */
void free_predicate(int id);

/**
 * @brief ???
*/
bool begin_survey(int id); // false if not supported or bad id
void end_survey(int id);

void begin_conditional_render(int id);
void end_conditional_render(int id);

PROGRAM get_debug_program();
void beginEvent(const char *); // for debugging
void endEvent();               // for debugging

/**
 * @brief Checks if variable refresh rate is supported.
 * 
 * @return \c true if variable refresh rate is supported, \c false otherwise.
 */
bool get_vrr_supported();

/**
 * @brief Checks if vertical synchronization is supported.
 *
 * @return \c true if vertical synchronization is supported, \c false otherwise.
 */
bool get_vsync_enabled();

/**
 * @brief Switches vertical synchronization on or off.
 * 
 * @return \c true on success, \c false otherwise.
 */
bool enable_vsync(bool enable);

/**
 * @brief Retrieves a pointer to the backbuffer color texture.
 * 
 * @return A pointer to the backbuffer color texture.
 * 
 * @note Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 */
Texture *get_backbuffer_tex();

//@cond 
//XBOX only
Texture *get_secondary_backbuffer_tex();
//@endcond

/**
 * @brief Retrieves a pointer to the backbuffer depth texture.
 *
 * @return A pointer to the backbuffer depth texture.
 *
 * @note Backbuffer is only valid while the GPU is acquired, and can be recreated in between.
 */
Texture *get_backbuffer_tex_depth();

#include "rayTrace/rayTracedrv3d.inl.h"

/**
 * @brief Adds resource barrier to the command list.
 * 
 * @param [in] desc         Resource barrier description. 
 * @param [in] gpu_pipeline Type of the pipeline to which the command queue to receive the barrier corresponds.
 * 
 * For details see \ref ResourceBarrierDesc as well as <a href="https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers"> this resource </a>.
 */
void resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Retrieves allocation properties of a given resource.
 * 
 * @param [in] desc Description of the resource.
 * @return          Allocation properties of the resource.
 */
ResourceAllocationProperties get_resource_allocation_properties(const ResourceDescription &desc);

/**
 * @brief Creates a resource heap.
 * 
 * @param [in] heap_group   A pointer to the heap group yielding the heap's group properties.
 * @param [in] size         Size of the heap.
 * @param [in] flags        Heap creation flags.
 * @return                  A pointer to the created heap.
 */
ResourceHeap *create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags);

/**
 * @brief Destroys the previously created resource heap.
 * 
 * @param [in] heap A pointer to the heap to destroy.
 * 
 * To create a resource heap use \ref create_resource_heap.
 */
void destroy_resource_heap(ResourceHeap *heap);

/**
 * @brief Creates a buffer and places it to the resource heap.
 * 
 * @param [in] heap         A pointer to the resource heap to place the buffer to.
 * @param [in] desc         Description of the buffer.
 * @param [in] offset       Byte offset from the top of the heap at which the buffer is placed.
 * @param [in] alloc_info   Buffer allocation properties.
 * @param [in] name         Buffer debug name.
 * @return                  A pointer to the created buffer.
 */
Sbuffer *place_buffere_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);

/**
 * @brief Creates a texture and places it to the resource heap.
 *
 * @param [in] heap         A pointer to the resource heap to place the texture to.
 * @param [in] desc         Description of the texture.
 * @param [in] offset       Byte offset from the top of the heap at which the texture is placed.
 * @param [in] alloc_info   Texture allocation properties.
 * @param [in] name         Texture debug name.
 * @return                  A pointer to the created texture.
 */
BaseTexture *place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name);

/**
 * @brief Retrieves group properties of the resource heap group.
 * 
 * @param [in] heap_group   A pointer to the heap group to retrieve properties of.
 * @return                  The retrieved properties.
 */
ResourceHeapGroupProperties get_resource_heap_group_properties(ResourceHeapGroup *heap_group);

/**
 * @brief Maps memory areas of the heap to specified locations in the texture.
 * 
 * @param [in] tex              A pointer to the texture to map the memory to.
 * @param [in] heap             A pointer to the heap to map memory area from. If NULL is passed, the previously created mapping is removed.
 * @param [in] mapping          An array of data, containing information on each mapping, including the locations to map to.
 * @param [in] mapping_count    Number of mappings.
 */
void map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

/**
 * @brief Retrieves texture tiling information.
 * 
 * @param [in] tex          A pointer to the texture to retrieve information about.
 * @param [in] subresource  Index of the texture subresource to retrieve information about.
 * @return                  The retrieved information.
 */
TextureTilingInfo get_texture_tiling_info(BaseTexture *tex, size_t subresource);

/**
 * @brief Renders the buffer active.
 * 
 * @param [in] buf          A pointer to the buffer to activate.
 * @param [in] action       Action to take on activation.
 * @param [in] value        Value to clear the buffer with if necessary.
 * @param [in] gpu_pipeline Type of the pipeline to which the command queue to receive activation command corresponds.
 */
void activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Renders the texture active.
 *
 * @param [in] buf          A pointer to the buffer to activate.
 * @param [in] action       Action to take on activation.
 * @param [in] value        Value to clear the buffer with if necessary.
 * @param [in] gpu_pipeline Type of the pipeline to which the command queue to receive activation command corresponds.
 */
void activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value = {},
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Renders the buffer inactive.
 *
 * @param [in] buf          A pointer to the buffer to activate.
 * @param [in] gpu_pipeline Type of the pipeline to which the command queue to receive activation command corresponds.
 */
void deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Renders the texture inactive.
 *
 * @param [in] buf          A pointer to the buffer to activate.
 * @param [in] gpu_pipeline Type of the pipeline to which the command queue to receive activation command corresponds.
 */
void deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

/**
 * @brief Allocates a buffer to update the texture subregion.
 * 
 * @param [in] dest_base_texture    A pointer to the texture resource to create the update buffer for. It has to be non-NULL.
 * @param [in] dest_mip             Valid \b dest_base_texture mipmap level.
 * @param [in] dest_slice           An index in the texture array or a face index in the cube map, if \b dest_base_texture is one of those, or 0 otherwise.
 * @param [in] offset_x             X-axis offset, defining the subregion. 
 * @param [in] offset_y             Y-axis offset, defining the subregion. 
 * @param [in] offset_z             Z-axis offset, defining the subregion. 
 * @param [in] width                Width of the subregion.
 * @param [in] height               Height of the subregion.
 * @param [in] depth                Depth of the subregion, if \b dest_base_texture is a volume texture, or 1 otherwise.
 * @return                          A pointer to the created buffer or NULL on error.
 * 
 * @note    The subregion defined by \b offset_x, \b offset_y, \b offset_z, \b width, \b height and \b depth
 *          has to fit within the borders of the the texture. It also has to be aligned to the texture format block size.
 * 
 * The function returns NULL if:\n
 *      \li any of the parameter requirements has been violated
 *      \li the driver cannot currently provide the memory required
 *      \li the driver is unable to perform the necessary copy operation on update
 */
ResUpdateBuffer *allocate_update_buffer_for_tex_region(BaseTexture *dest_base_texture, unsigned dest_mip, unsigned dest_slice,
  unsigned offset_x, unsigned offset_y, unsigned offset_z, unsigned width, unsigned height, unsigned depth);

/**
 * @brief Allocates a buffer to update the texture.
 * 
 * @param [in] dest_texture    A pointer to the texture resource to create the update buffer for. It has to be non-NULL.
 * @param [in] dest_mip        Valid \b dest_base_texture mipmap level.
 * @param [in] dest_slice      An index in the texture array or a face index in the cube map, if \b dest_texture is one of those, or 0 otherwise.
 * @return                     A pointer to the created buffer or NULL on error.
 * 
 * Allocates update buffer in system memory to be filled directly and then dispatched to \ref apply_tex_update_buffer.
 * This method can fail if too much allocations happens in an N-frame period.
 * In that case, user can retry after the frame has been rendered on screen.
 */
ResUpdateBuffer *allocate_update_buffer_for_tex(BaseTexture *dest_tex, int dest_mip, int dest_slice);

/**
 * @brief Releases the update buffer and clears the pointer afterwards.
 * 
 * @param [in] A pointer to the buffer to release.
 * 
 * The function skips any update and just releases the buffer.
 */
void release_update_buffer(ResUpdateBuffer *&rub);

/**
 * @brief Retrives data address to write the the resource update data.
 * 
 * @param [in] rub  A pointer to the resource update buffer to write from.
 * @return          The address to write to.
 */
char *get_update_buffer_addr_for_write(ResUpdateBuffer *rub);

/**
 * @brief Returns the size of the resource update buffer.
 *
 * @param [in] rub  A pointer to the resource update buffer.
 * @return          The size of the buffer.
 */
size_t get_update_buffer_size(ResUpdateBuffer *rub);
//! returns pitch of update buffer (if applicable)
/**
 * @brief Returns the pitch of the resource update buffer (if applicable).
 *
 * @param [in] rub  A pointer to the resource update buffer.
 * @return          The row pitch of the buffer, 0 if no pitch corresponds to the buffer.
 */
size_t get_update_buffer_pitch(ResUpdateBuffer *rub);
// Returns the pitch of one 2d image slice for volumetric textures.
size_t get_update_buffer_slice_pitch(ResUpdateBuffer *rub);
//! applies update to target d3dres and releases update buffer (clears pointer afterwards)
bool update_texture_and_release_update_buffer(ResUpdateBuffer *&src_rub);

/** \defgroup RenderPassD3D
 * @{
 */

/**
 * @brief Creates a render pass object.
 * 
 * @param [in]  Description of the render pass to create.
 * @return      A pointer to the opaque RenderPass object, NULL if the description is invalid.
 * 
 * @warning The function should be called neither in realtime, neither in time-sensitive places.
 * @note    No external sync is required.
 * 
 * Render pass objects are intended to be created once (and ahead of time), and then used multiple times.
 */
RenderPass *create_render_pass(const RenderPassDesc &rp_desc);

/**
 * @brief Deletes the render pass object.
 * 
 * @param [in] rp   A pointer to the render pass to delete. 
 * 
 * @note    The deletion has to be synced with the render pass usage, so that it is not deleted until it is no more used.
 * @warning Using the render pass after this method has been called is invalid.
 */
void delete_render_pass(RenderPass *rp);

/**
 * @brief Begins the render pass.
 * 
 * @param [in] rp       A pointer to the render pass resource to begin with
 * @param [in] area     Rendering area restriction 
 * @param [in] targets  Array of targets that will be used for rendering
 * 
 * @note    The call must be externally synced (GPU lock is required).
 * @warning When a pass is in action, using any GPU execution functions except from 
 *          rendering functions (\ref draw, \ref drawind etc.) is prohibited.\n
 * @warning Avoid writes/reads outside \b area, as it results in undefined behavior.
 * @warning The function will assert-fail if other render pass is already in process.
 * 
 * After this function has been called, the viewport is reset to \b area and 
 * subpass 0 described in render pass object begins.
 */
void begin_render_pass(RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets);

/**
 * @brief Advances rendering to the next subpass.
 * 
 * @note    The call must be externally synced (GPU lock required).
 * @warning The function will assert-fail if there is no subpass to advance to.
 * @warning The function will assert-fail if called outside a render pass.
 * 
 * Increases subpass number and executes necessary synchronization 
 * as well as binding described for this subpass.\n
 * Viewport is reset to render area on every call.
 */
void next_subpass();

/**
 * @brief Ends the current render pass.
 * 
 * @note    After the function has been called any non-draw operations (see \ref begin_render_pass)
 *          are allowed and render targets are reset to the backbuffer.\n
 * @note    The call must be externally synced.
 * @warning Will assert-fail if subpass is not final
 * @warning Will assert-fail if called  outside of render pass
 * 
 * Processes store&sync operations described in render pass.
 */
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
//@cond
//unimplemented
inline void resummarize_htile(BaseTexture *) {}
//@endcond
#endif

#if _TARGET_XBOX
void set_esram_layout(const wchar_t *);
void unset_esram_layout();
void reset_esram_layout();
void prefetch_movable_textures();
void writeback_movable_textures();
#else
//@cond
//unimplemented
inline void set_esram_layout(const wchar_t *) {}
inline void unset_esram_layout() {}
inline void reset_esram_layout() {}
inline void prefetch_movable_textures() {}
inline void writeback_movable_textures() {}
//@endcond
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
  /**
  * @brief Throws fatal error informing user of the device being reset.
  * 
  * @param [in] msg Message to translate to user.
  */
    d3derr_in_device_reset(const char *msg);
extern bool dagor_d3d_force_driver_reset;

/**
* @brief Throws fatal error in case the code contains error.
* 
* @param [in] c Code to check.
* @param [in] m Message to translate to user.* 
*/
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
        DAG_FATAL("%s:\n%s", m, d3d::get_last_error());                \
    }                                                                  \
  } while (0)


/**
 * @brief Throws fatal error in case the code contains error.
 *
 * @param [in] c Code to check.
 */
#define d3d_err(c) d3derr((c), "Driver3d error")

#if _TARGET_D3D_MULTI
#include <3d/dag_drv3d_multi.h>
#else
#include <3d/dag_drv3dCmd.h>
#endif
