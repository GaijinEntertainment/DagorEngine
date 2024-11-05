//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <util/dag_globDef.h>

#include <drv/3d/dag_consts.h>

class Sbuffer;

namespace d3d
{
/**
 * @brief Sets shader constants for the specified stage.
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 * @param reg_base The base register index.
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constants were set successfully, false otherwise.
 * @note These constants will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
bool set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs);

/**
 * @brief Sets vertex shader constants.
 *
 * @param reg_base The base register index.
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constants were set successfully, false otherwise.
 * @note These constants will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
inline bool set_vs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_VS, reg_base, data, num_regs);
}

/**
 * @brief Sets pixel shader constants.
 *
 * @param reg_base The base register index.
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constants were set successfully, false otherwise.
 * @note These constants will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
inline bool set_ps_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_PS, reg_base, data, num_regs);
}

/**
 * @brief Sets compute shader constants.
 *
 * @param reg_base The base register index.
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constants were set successfully, false otherwise.
 * @note These constants will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
inline bool set_cs_const(unsigned reg_base, const void *data, unsigned num_regs)
{
  return set_const(STAGE_CS, reg_base, data, num_regs);
}

/**
 * @brief Sets a single vertex shader constant register.
 *
 * @param reg The register index.
 * @param v0 The value for the first component.
 * @param v1 The value for the second component.
 * @param v2 The value for the third component.
 * @param v3 The value for the fourth component.
 * @return True if the constant was set successfully, false otherwise.
 * @note This constant will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
inline bool set_vs_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_vs_const(reg, v, 1);
}

/**
 * @brief Sets a single pixel shader constant register.
 *
 * @param reg The register index.
 * @param v0 The value for the first component.
 * @param v1 The value for the second component.
 * @param v2 The value for the third component.
 * @param v3 The value for the fourth component.
 * @return True if the constant was set successfully, false otherwise.
 * @note This constant will be ignored, if a constant buffer is explicitly bound to slot 0.
 */
inline bool set_ps_const1(unsigned reg, float v0, float v1, float v2, float v3)
{
  float v[4] = {v0, v1, v2, v3};
  return set_ps_const(reg, v, 1);
}


/**
 * @brief Sets immediate constants for the specified stage.
 *
 * @details Immediate constants are supposed to be very cheap to set dwords.
 * It is guaranteed to support up to 4 dwords on each stage.
 * Use as little as possible, ideally one or two (or none).
 * On XB1(PS4), it is implemented as user regs (C|P|V)SSetShaderUserData.
 * On DX11, it is implemented as constant buffers.
 * On VK/DX12, it should be implemented as descriptor/push constants buffers.
 * Calling with data = nullptr || num_words == 0 is benign and currently works as "stop using immediate" (probably have to be replaced
 * with shader system).
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 * @param data Pointer to the data to be set.
 * @param num_words The number of words to be set.
 * @return True if the constants were set successfully, false otherwise.
 */
bool set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words);

/**
 * @brief Sets a constant buffer for the specified stage. PS4 specific.
 *
 * Constant buffers are valid until driver acquire call or end of frame.
 * To unbind, use set_const_buffer(stage, 0, nullptr).
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 * @param slot The buffer slot.
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constant buffer was set successfully, false otherwise.
 * @note When slot is 0, and data is not nullptr, it will also override any constants set via set_const and related calls.
 */
bool set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs);

/**
 * @brief Sets a constant buffer for the specified stage using a buffer object.
 *
 * Constant buffers are valid until driver acquire call or end of frame.
 * To unbind, use set_const_buffer(stage, 0, nullptr).
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 * @param slot The buffer slot.
 * @param buffer Pointer to the buffer object. Must be created with d3d::buffers::create_persistent_cb or
 * d3d::buffers::create_one_frame_cb.
 * @param consts_offset The offset of the constants in the buffer. (Not used)
 * @param consts_size The size of the constants in the buffer. (Not used)
 * @todo consts_offset and consts_size are not used. Remove them?
 * @return True if the constant buffer was set successfully, false otherwise.
 */
bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset = 0, uint32_t consts_size = 0);

/**
 * @brief Sets the size of the vertex shader constant buffer that can be filled with set_const call.
 *
 * @param required_size The required size of the constant buffer. If 0, the default size is set.
 * @return The actual size of the constant buffer.
 */
int set_vs_constbuffer_size(int required_size);

/**
 * @brief Sets the size of the compute shader constant buffer that can be filled with set_const call.
 *
 * @param required_size The required size of the constant buffer. If 0, the default size is set.
 * @return The actual size of the constant buffer.
 */
int set_cs_constbuffer_size(int required_size);

/**
 * @brief Sets a constant buffer at slot 0 for the specified stage. Uses the fastest method available on the platform.
 *
 * Constant buffers are valid until driver acquire call or end of frame.
 * To unbind, use release_cb0_data(stage).
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 * @param data Pointer to the data to be set. The size must be a multiple of sizeof(float4).
 * @param num_regs The number of registers (float4) to be set.
 * @return True if the constants were set successfully, false otherwise.
 */
inline bool set_cb0_data(unsigned stage, const float *data, unsigned num_regs)
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

/**
 * @brief Releases the constant buffer at slot 0, which should have been previously set by set_cb0_data.
 *
 * @param stage The shader stage (STAGE_VS, STAGE_PS, STAGE_CS).
 */
inline void release_cb0_data(unsigned stage)
{
#if _TARGET_C1 | _TARGET_C2

#else
  G_UNUSED(stage);
#endif
}

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool set_const(unsigned stage, unsigned reg_base, const void *data, unsigned num_regs)
{
  return d3di.set_const(stage, reg_base, data, num_regs);
}

inline bool set_immediate_const(unsigned stage, const uint32_t *data, unsigned num_words)
{
  return d3di.set_immediate_const(stage, data, num_words);
}

inline int set_vs_constbuffer_size(int required_size) { return d3di.set_vs_constbuffer_size(required_size); }
inline int set_cs_constbuffer_size(int required_size) { return d3di.set_cs_constbuffer_size(required_size); }

inline bool set_const_buffer(unsigned stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size)
{
  return d3di.set_const_buffer(stage, slot, buffer, consts_offset, consts_size);
}

} // namespace d3d
#endif
