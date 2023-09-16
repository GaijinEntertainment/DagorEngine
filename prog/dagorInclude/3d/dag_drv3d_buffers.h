//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "dag_drv3d.h"

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

namespace d3d_buffers
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
 * \brief The enum which indicates if the driver relies on buffer restoration on user side when device reset happen.
 *
 * For consoles, Metal and Vulkan this flag doesn't make any sense. Used only in DX11/DX12 drivers.
 * If the buffer has Maybelost::No flag, driver stores a copy of its content on CPU side (extra memory).
 * Of course, this flag doesn't make any sense for buffers filled by GPU.
 *
 * \todo Get rid of this flag and restore buffer content explicitly.
 */
enum class Maybelost : uint32_t
{
  No,  ///< DirectX driver stores additional copy of the buffer on CPU side, to restore it after device reset event.
  Yes, ///< User will restore driver content after device reset manually.
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
inline Sbuffer *create_one_frame_cb(uint32_t registers_count, const char *name, Init buffer_init = Init::No)
{
  return d3d::create_sbuffer(CBUFFER_REGISTER_SIZE, registers_count,
    SBCF_CB_ONE_FRAME | (buffer_init == Init::Zero ? SBCF_ZEROMEM : 0), 0, name);
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
 * \param maybelost Indicated buffer restoration policy on device reset.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_tbuf(uint32_t elements_count, uint32_t format, const char *name,
  Maybelost maybelost = Maybelost::Yes)
{
  return d3d::create_sbuffer(get_tex_format_desc(format).bytesPerElement, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | (maybelost == Maybelost::Yes ? SBCF_MAYBELOST : 0), format, name);
}


/**
 * \brief Create a byte address buffer, which can be used through a shader resource view in shaders.
 * In a shader you can declare the buffer using ByteAddressBuffer.
 *
 * It is a persistent buffer, so you can update it with VBLOCK_WRITEONLY flag. Locked part of the buffer content will be overwritten.
 *
 * \param size_in_dwords The size of the buffer in dwords.
 * \param name The name of the buffer, used for debugging purposes, like showing in in statistcs, and frame debuggers like PIX.
 * \param maybelost Indicated buffer restoration policy on device reset.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_byte_address(uint32_t size_in_dwords, const char *name, Maybelost maybelost = Maybelost::Yes)
{
  return d3d::create_sbuffer(BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_ALLOW_RAW | (maybelost == Maybelost::Yes ? SBCF_MAYBELOST : 0), 0, name);
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
 * \param maybelost Indicated buffer restoration policy on device reset.
 * \return A pointer to the created buffer.
 */
inline Sbuffer *create_persistent_sr_structured(uint32_t structure_size, uint32_t elements_count, const char *name,
  Maybelost maybelost = Maybelost::Yes)
{
  return d3d::create_sbuffer(structure_size, elements_count,
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED | (maybelost == Maybelost::Yes ? SBCF_MAYBELOST : 0), 0, name);
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
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_MAYBELOST, format, name);
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
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_ALLOW_RAW | SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_MAYBELOST, 0, name);
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
    SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED | SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_MAYBELOST, 0, name);
}

} // namespace d3d_buffers
