//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>

#include <drv/3d/dag_d3dResource.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_resourceChecker.h>

/**
 * @brief The Sbuffer class represents a buffer used for 3D rendering.
 *
 * This class is derived from the D3dResource class and provides functionality for locking and unlocking the buffer,
 * retrieving buffer information, and updating buffer data. It represents all possible buffer types in the engine.
 */
class Sbuffer : public D3dResource, public ResourceChecker
{
public:
  DAG_DECLARE_NEW(midmem)

  /**
   * @brief Interface for a callback that is called when the buffer is restored after device reset.
   */
  struct IReloadData
  {
    virtual ~IReloadData() {}
    /**
     * @brief Reloads content of the buffer.
     *
     * @param sb The buffer to fill.
     */
    virtual void reloadD3dRes(Sbuffer *sb) = 0;
    /**
     * @brief Destroys the callback object. Will be called on a buffer destruction.
     */
    virtual void destroySelf() = 0;
  };
  /**
   * @brief Set the Reload Callback object for the buffer
   *
   * @return true if the callback was successfully set, false otherwise
   */
  virtual bool setReloadCallback(IReloadData *) { return false; }

  /**
   * @brief Returns the type of the D3dResource. It is always RES3D_SBUF for Sbuffer objects.
   *
   * @todo make RES3D_ a enum class.
   * @return int RES3D_SBUF.
   */
  int restype() const override final { return RES3D_SBUF; }

  /**
   * Locks a portion of the buffer for reading or writing.
   *
   * @warning It is better to use lock_sbuffer method for more safety.
   *
   * @param ofs_bytes The offset in bytes from the beginning of the buffer.
   * @param size_bytes The size in bytes of the portion to lock. The whole buffer will be locked if 0.
   * @param p A pointer to a void pointer that will receive the locked memory address.
   * @param flags Additional flags to control the locking behavior.
   * @return An integer representing the result of the lock operation. 0 if the lock failed, 1 if the lock succeeded.
   */
  virtual int lock(uint32_t ofs_bytes, uint32_t size_bytes, void **p, int flags) = 0;

  /**
   * Unlocks the buffer after it has been locked.
   *
   * @return An integer representing the result of the unlock operation. 0 if the unlock failed, 1 if the unlock succeeded.
   */
  virtual int unlock() = 0;
  /**
   * @brief Get the Flags object
   *
   * @return Flags that control the buffer behavior and was set on a buffer creation.
   */
  virtual int getFlags() const = 0;
  /**
   * @brief Get the Buffer name
   *
   * @return The name of the buffer.
   */
  const char *getBufName() const { return getResName(); }

  /**
   * @brief Get the size of the structured buffer element.
   *
   * @details This method works only for structured buffers.
   *
   * @return The size of the buffer element.
   */
  virtual int getElementSize() const { return 0; }
  /**
   * @brief Get the amount of elements in the structured buffer.
   *
   * @details This method works only for structured buffers.
   *
   * @return int amount of elements in the buffer.
   */
  virtual int getNumElements() const { return 0; };
  /**
   * @brief Copy current buffer to another buffer. The sizes of the buffers should match exactly.
   *
   * @param dest The destination buffer.
   * @return true if the buffer was successfully copied, false otherwise.
   */
  virtual bool copyTo(Sbuffer *dest)
  {
    G_UNUSED(dest);
    return false;
  }
  /**
   * @brief Copy a portion of the buffer to another buffer. Both buffers must be large enough for the copied portion offset and size.
   *
   * @param dest The destination buffer.
   * @param dst_ofs_bytes The offset in bytes from the beginning of the destination buffer.
   * @param src_ofs_bytes The offset in bytes from the beginning of the source buffer.
   * @param size_bytes The size in bytes of the portion to copy.
   * @return true if the buffer was successfully copied, false otherwise.
   */
  virtual bool copyTo(Sbuffer *dest, uint32_t dst_ofs_bytes, uint32_t src_ofs_bytes, uint32_t size_bytes)
  {
    G_UNUSED(dest);
    G_UNUSED(dst_ofs_bytes);
    G_UNUSED(src_ofs_bytes);
    G_UNUSED(size_bytes);
    return false;
  }

  /**
   * @brief Locks a portion of the buffer for reading or writing.
   *
   * @warning It is better to use lock_sbuffer method for more safety.
   *
   * @tparam T type of the data to lock
   * @param ofs_bytes offset in bytes from the beginning of the buffer
   * @param size_bytes size in bytes of the portion to lock
   * @param p pointer to a pointer that will receive the locked memory address
   * @param flags additional flags to control the locking behavior VBLOCK_*
   * @return int 0 if the lock failed, 1 if the lock succeeded
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
   * @brief Updates buffer content with the specified data using lock/memcpy/unlock.
   *
   * @param ofs_bytes offset in bytes from the beginning of the buffer
   * @param size_bytes size in bytes of the portion to update. Must be non-zero.
   * @param src pointer to the source data
   * @param lockFlags additional flags to control the locking behavior VBLOCK_*
   * @return true if the buffer was successfully updated, false otherwise
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
   * @brief Updates buffer content with the specified data using lock/memcpy/unlock.
   *
   * @param ofs_bytes offset in bytes from the beginning of the buffer
   * @param size_bytes size in bytes of the portion to update. Must be non-zero.
   * @param src pointer to the source data
   * @param lockFlags additional flags to control the locking behavior VBLOCK_*
   * @return true if the buffer was successfully updated, false otherwise
   */
  virtual bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
  {
    G_ASSERT_RETURN(size_bytes, false);
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
  }

  /**
   * @brief Lock method specified for index buffer with 16-bit indices.
   *
   * @param ofs_bytes offset in bytes from the beginning of the buffer
   * @param size_bytes size in bytes of the portion to lock
   * @param p pointer to a pointer that will receive the locked memory address
   * @param flags additional flags to control the locking behavior VBLOCK_*
   * @return int 0 if the lock failed, 1 if the lock succeeded
   */
  inline int lock(uint32_t ofs_bytes, uint32_t size_bytes, uint16_t **p, int flags)
  {
    G_ASSERT(!(getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }
  /**
   * @brief Lock method specified for index buffer with 32-bit indices.
   *
   * @param ofs_bytes offset in bytes from the beginning of the buffer
   * @param size_bytes size in bytes of the portion to lock
   * @param p pointer to a pointer that will receive the locked memory address
   * @param flags additional flags to control the locking behavior VBLOCK_*
   * @return int 0 if the lock failed, 1 if the lock succeeded
   */
  inline int lock32(uint32_t ofs_bytes, uint32_t size_bytes, uint32_t **p, int flags)
  {
    G_ASSERT((getFlags() & SBCF_INDEX32));
    return lockEx(ofs_bytes, size_bytes, p, flags);
  }

protected:
  ~Sbuffer() override {}
};

namespace d3d
{
/**
 * @brief Creates a buffer with the specified parameters.
 *
 * @details The buffer can be used for various purposes, such as constant buffers, structured buffers, byte address buffers, and more.
 * It is an uber method, and not all combinations of parameters are valid. Use d3d::buffers namespace for more specific buffer creation
 * methods.
 *
 * @param struct_size The size of each structure in the buffer.
 * @param elements The number of elements in the buffer.
 * @param flags Additional flags for the buffer.
 * @param texfmt The texture format for tBuffers.
 * @param name The name of the buffer.
 * @return A pointer to the created buffer.
 */
Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name);

/**
 * @brief Sets an Sbuffer for a specific shader stage and slot.
 *
 * @param shader_stage The shader stage to set the buffer for. One of STAGE_ values.
 * @param slot The slot to bind the buffer to.
 * @param buffer A pointer to the Sbuffer to set.
 * @return True if the buffer was successfully set, false otherwise.
 */
bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

/**
 * @brief Sets a read-write Sbuffer for a specific shader stage and slot.
 *
 * @param shader_stage The shader stage to set the buffer for. One of STAGE_ values.
 * @param slot The slot to bind the buffer to.
 * @param buffer A pointer to the Sbuffer to set.
 * @return True if the buffer was successfully set, false otherwise.
 */
bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer);

} // namespace d3d

/*!
 * \file
 *
 * - \_ua\_ -> allows UAV access (default is formatted/sampled view)
 * - \_sr\_ -> allows SRV access (default is formatted/sampled view)
 * - \_cb -> allows const buffer access
 * - \_one_frame\_ -> for cb to indicate this buffer contents is only valid this frame after a discard write
 * - \_persistent\_ -> for cb counter part to \_one_frame\_, buffer holds its contents until changed
 * - \_byte_address -> for ua and sr to indicate byte address view
 * - \_structured -> for ua and sr to indicate structured view
 * - \_readback -> buffer is used for read back
 * - \_indirect -> buffer can be an indirect argument
 * - \_staging -> staging buffer
 */

namespace d3d::buffers
{
/**
 * \brief Enumeration for buffer initialization options. Not all buffer types currently support it.
 */
enum class Init : uint32_t
{
  No,   ///< We don't know anything about buffer content on creation.
  Zero, ///< A resource guaranteed to be zeroed on the first usage.
};


/**
 * \brief The Indirect enum represents different GPU indirect buffer types.
 */
enum class Indirect : uint32_t
{
  Dispatch,
  Draw,
  DrawIndexed,
};


/**
 * \brief The size of the cbuffer register.
 * It should have a size divisible by sizeof(float4), because the cbuffer is a set of float4 registers.
 */
inline constexpr uint32_t CBUFFER_REGISTER_SIZE = 16;

/**
 * \brief The size of an element of a byte address buffer.
 */
inline constexpr uint32_t BYTE_ADDRESS_ELEMENT_SIZE = sizeof(uint32_t);


/**
 * \brief Calculate the number of register (in const buffer term) count for a given array size of type T.
 * Structure must be aligned as float4 to not have problems with alignment in cbuffer.
 *
 * \tparam T The type of the array elements.
 * \param array_size The size of the array.
 * \return The registers count.
 */
template <typename T>
inline uint32_t cb_array_reg_count(uint32_t array_size)
{
  static_assert(sizeof(T) % CBUFFER_REGISTER_SIZE == 0,
    "Structure should be aligned as float4 to not have problems with alignment in cbuffer");
  return sizeof(T) / CBUFFER_REGISTER_SIZE * array_size;
}


/**
 * \brief Calculate the number of register (in const buffer term) count for a single instance of a type T.
 * Structure must be aligned as float4 to not have problems with alignment in cbuffer.
 *
 * \tparam T The type of the structure.
 * \return The registers count.
 */
template <typename T>
inline uint32_t cb_struct_reg_count()
{
  return cb_array_reg_count<T>(1);
}


/**
 * \brief Create a persistent constant buffer.
 *
 * Such buffers could be updated from time to time.
 * It is recommended to use \ref cb_array_reg_count and \ref cb_struct_reg_count methods to calculate registers_count.
 *
 * \warning This buffer will not be restored after device reset!
 *
 * \param registers_count The number of registers in the buffer. Must be not bigger than 4096.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return Created persistent constant buffer.
 */
inline Sbuffer *create_persistent_cb(uint32_t registers_count, const char *name)
{
  return d3d::create_sbuffer(CBUFFER_REGISTER_SIZE, registers_count, SBCF_CB_PERSISTENT, 0, name);
}

/**
 * \brief Create an one frame constant buffer.
 *
 * Such buffers must be updated every frame (you can skip update if the buffer is not used this frame).
 * Because of that we don't care about its content on device reset.
 * It is recommended to use \ref cb_array_reg_count and \ref cb_struct_reg_count methods to calculate registers_count.
 *
 * \param registers_count The number of registers. Must be not bigger than 4096.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_one_frame_cb(uint32_t registers_count, const char *name)
{
  return d3d::create_sbuffer(CBUFFER_REGISTER_SIZE, registers_count, SBCF_CB_ONE_FRAME, 0, name);
}

/*!
 * \brief Create a byte address buffer, which can be used thorugh an unordered access view or through a shader resource view in
 * shaders. In a shader you can declare the buffer using (RW)ByteAddressBuffer.
 * Such a buffer is always 16-byte aligned.
 *
 * \todo Use registers instead of dwords for size because of alignment.
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords,
    SBCF_UA_SR_BYTE_ADDRESS | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
}

/**
 * \brief Create a structured buffer, which can be used thorugh an unordered access view or through a shader resource view in shaders.
 * In a shader you can declare the buffer using (RW)StructuredBuffer<StructureType>. StructureType is a kind of template parameter
 * here.
 *
 * \param structure_size The size of the structure of the buffer elements. Usually it is a sizeof(StructureType).
 * \param elements_count The number of elements in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 *
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(structure_size, elements_count, SBCF_UA_SR_STRUCTURED | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0,
    name);
}


/**
 * \brief Create a byte address buffer, which can be used thorugh an unordered access view in shaders.
 * In a shader you can declare the buffer using RWByteAddressBuffer.
 * Such a buffer is always 16-byte aligned.
 *
 * \todo Use registers instead of dwords for size because of alignment.
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_byte_address(uint32_t size_in_dwords, const char *name)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords, SBCF_UA_BYTE_ADDRESS, 0, name);
}

/**
 * \brief Create a structured buffer, which can be used thorugh an unordered access view in shaders.
 * In a shader you can declare the buffer using RWStructuredBuffer<StructureType>. StructureType is a kind of template parameter here.
 *
 * \param structure_size The size of the structure of the buffer elements. Usually it is a sizeof(StructureType).
 * \param elements_count The number of elements in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return d3d::create_sbuffer(structure_size, elements_count, SBCF_UA_STRUCTURED, 0, name);
}

/**
 * \brief The same as \ref create_ua_byte_address but its content can be read on CPU.
 * Such a buffer is always 16-byte aligned.
 *
 * \todo Use registers instead of dwords for size because of alignment.
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_byte_address_readback(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords,
    SBCF_UA_BYTE_ADDRESS_READBACK | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
}


/**
 * \brief The same as \ref create_ua_structured but its content can be read on CPU
 *
 * \param structure_size The size of the structure of the buffer elements. Usually it is a sizeof(StructureType).
 * \param elements_count The number of elements in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_structured_readback(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(structure_size, elements_count,
    SBCF_UA_STRUCTURED_READBACK | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
}


/**
 * \brief Returns the amount of dwords per indirect command parameters based on the given indirect buffer type.
 *
 * \param indirect_type The type of indirect operation (Dispatch, Draw, or DrawIndexed).
 *
 * \return The amount of dwords per indirect command parameters.
 */
inline uint32_t dword_count_per_call(Indirect indirect_type)
{
  switch (indirect_type)
  {
    case Indirect::Dispatch: return DISPATCH_INDIRECT_NUM_ARGS;
    case Indirect::Draw: return DRAW_INDIRECT_NUM_ARGS;
    case Indirect::DrawIndexed: return DRAW_INDEXED_INDIRECT_NUM_ARGS;
  }
  G_ASSERT(false); // impossible situation
  return 0;
}

/**
 * \brief Creates an indirect buffer filled by the GPU.
 *
 * \param indirect_type The type of the indirect commands stored in the buffer.
 * \param records_count The number of indirect records in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_ua_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  const uint32_t dwordsCount = records_count * dword_count_per_call(indirect_type);
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, dwordsCount, SBCF_UA_INDIRECT, 0, name);
}


/**
 * \brief Creates an indirect buffer filled by the CPU.
 *
 * \param indirect_type The type of the indirect commands stored in the buffer.
 * \param records_count The number of indirect records in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_indirect(Indirect indirect_type, uint32_t records_count, const char *name)
{
  const uint32_t dwordsCount = records_count * dword_count_per_call(indirect_type);
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, dwordsCount, SBCF_INDIRECT, 0, name);
}


/**
 * \brief Creates a buffer for data transfer from CPU to GPU.
 *
 * \param size_in_bytes The size of the buffer in bytes.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_staging(uint32_t size_in_bytes, const char *name)
{
  return d3d::create_sbuffer(1, size_in_bytes, SBCF_STAGING_BUFFER, 0, name);
}


/**
 * \brief Create a t-buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using Buffer<BufferFormat>. BufferFormat is a kind of template parameter here.
 *
 * \warning The buffer type can be used only for the code which will be used for DX10 compatible hardware.
 *
 * The total size of the buffer is sizeof(format) * elements_count.
 *
 * It is a persistent buffer, so you can update it with VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 *
 * \param elements_count The number of elements in the buffer.
 * \param format The format of each element in the buffer. It must be a valid texture format. Not all texture formats are allowed.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name, Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(get_tex_format_desc(format).bytesPerElement, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), format, name);
}


/**
 * \brief Create a byte address buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using ByteAddressBuffer.
 *
 * It is a persistent buffer, so you can update it with VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 *
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_byte_address(uint32_t size_in_dwords, const char *name, Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_ALLOW_RAW | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
}


/**
 * \brief Create a structured buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using StructuredBuffer<StructureType>. StructureType is a kind of template parameter here.
 *
 * It is a persistent buffer, so you can update it with VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 *
 * Declare StructureType in *.hlsli file and include it both in C++ and shader code.
 *
 * \param structure_size The size of the structure of the buffer elements. Usually it is a sizeof(StructureType).
 * \param elements_count The number of elements in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param buffer_init The initialization option for the buffer.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(structure_size, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
}


/**
 * \brief Create a t-buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using Buffer<BufferFormat>. BufferFormat is a kind of template parameter here.
 *
 * \warning The buffer type can be used only for the code which will be used for DX10 compatible hardware.
 *
 * The total size of the buffer is sizeof(format) * elements_count.
 * To use the buffer, lock it with VBLOCK_DISCARD flag (and with VBLOCK_NOOVERWRITE during the frame) and fill it on CPU.
 * On the next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 *
 * \param elements_count The number of elements in the buffer.
 * \param format The format of each element in the buffer. It must be a valid texture format. Not all texture formats are allowed.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_one_frame_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name)
{
  return d3d::create_sbuffer(get_tex_format_desc(format).bytesPerElement, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC | SBCF_FRAMEMEM, format, name);
}


/**
 * \brief Create a byte address buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using ByteAddressBuffer.
 *
 * To use the buffer, lock it with VBLOCK_DISCARD flag (and with VBLOCK_NOOVERWRITE during the frame) and fill it on CPU.
 * On the next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 *
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_one_frame_sr_byte_address(uint32_t size_in_dwords, const char *name)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_ALLOW_RAW | SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, name);
}


/**
 * \brief Create a structured buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using StructuredBuffer<StructureType>. StructureType is a kind of template parameter here.
 *
 * To use the buffer, lock it with VBLOCK_DISCARD flag (and with VBLOCK_NOOVERWRITE during the frame) and fill it on CPU.
 * On the next frame data in the buffer could be invalid, so don't read from it until you fill it with lock again.
 *
 * Declare StructureType in *.hlsli file and include it both in C++ and shader code.
 *
 * \param structure_size The size of the structure of the buffer elements. Usually it is a sizeof(StructureType).
 * \param elements_count The number of elements in the buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_one_frame_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name)
{
  return d3d::create_sbuffer(structure_size, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED | SBCF_DYNAMIC | SBCF_FRAMEMEM, 0, name);
}

/**
 * \brief Creates a buffer to be used as scratch space for bottom acceleration structure builds and updates.
 * This buffer is used as input for ::raytrace::BottomAccelerationStructureBuildInfo::scratchSpaceBuffer which is used by
 * d3d::build_bottom_acceleration_structure.
 *
 * This buffer is not meant to be accessed in any other way than to be used as a this scratch buffer, as it is used as temporary
 * storage for the on GPU build and update process of bottom acceleration structure and its contents is highly device and device driver
 * specific and can't be used for anything else anyway.
 *
 * Sizes needed are provided by the d3d::create_raytrace_bottom_acceleration_structure function.
 *
 * \param size_in_bytes The size in bytes of the scratch buffer.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \return A pointer to the created buffer. Returns nullptr on failure. Possible failures are device lost state or out of memory.
 *
 */
inline Sbuffer *create_raytrace_scratch_buffer(uint32_t size_in_bytes, const char *name)
{
  return d3d::create_sbuffer(1, size_in_bytes, SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE, 0, name);
}

} // namespace d3d::buffers

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{

inline Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name)
{
  return d3di.create_sbuffer(struct_size, elements, flags, texfmt, name);
}

inline bool set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer) { return d3di.set_buffer(shader_stage, slot, buffer); }

inline bool set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  return d3di.set_rwbuffer(shader_stage, slot, buffer);
}

} // namespace d3d
#endif
