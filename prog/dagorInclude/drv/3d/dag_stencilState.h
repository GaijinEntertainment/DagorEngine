//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

#include <debug/dag_assert.h>

namespace shaders
{
/**
 * \brief A structure that describes current stencil state.
 */
struct StencilState
{
  uint8_t func : 4;        ///< Comparison function \p CMPF_*.
  uint8_t fail : 4;        ///< Stencil operation \p STNCLOP_* for failed stencil test.
  uint8_t zFail : 4;       ///< Stencil operation \p STNCLOP_* for passed stencil test and failed depth test.
  uint8_t pass : 4;        ///< Stencil operation \p STNCLOP_* for passed stencil and depth tests.
  uint8_t readMask = 255;  ///< Identify a portion of the depth-stencil buffer for reading stencil data.
  uint8_t writeMask = 255; ///< Identify a portion of the depth-stencil buffer for writing stencil data.
  /**
   * \brief Default constructor.
   */
  StencilState()
  {
    func = 1;                // CMPF_NEVER
    pass = zFail = fail = 1; // STNCLOP_KEEP
    readMask = writeMask = 255;
  }
  /**
   * \brief Constructor with all parameters. It just calls \p set() method.
   *
   * \warning \p stencil_func, \p stencil_fail, \p z_fail, \p stencil_z_pass must be non-zero.
   *
   * \param stencil_func Comparison function \p CMPF_*.
   * \param stencil_fail Stencil operation \p STNCLOP_* for failed stencil test.
   * \param z_fail Stencil operation \p STNCLOP_* for passed stencil test and failed depth test.
   * \param stencil_z_pass Stencil operation \p STNCLOP_* for passed stencil and depth tests.
   * \param write_mask Identify a portion of the depth-stencil buffer for writing stencil data.
   * \param read_mask Identify a portion of the depth-stencil buffer for reading stencil data.
   */
  StencilState(uint8_t stencil_func, uint8_t stencil_fail, uint8_t z_fail, uint8_t stencil_z_pass, uint8_t write_mask,
    uint8_t read_mask)
  {
    set(stencil_func, stencil_fail, z_fail, stencil_z_pass, write_mask, read_mask);
  }

  /**
   * \brief Set all parameters.
   *
   * \warning \p stencil_func, \p stencil_fail, \p z_fail, \p stencil_z_pass must be non-zero.
   *
   * \param stencil_func Comparison function \p CMPF_*.
   * \param stencil_fail Stencil operation \p STNCLOP_* for failed stencil test.
   * \param z_fail Stencil operation \p STNCLOP_* for passed stencil test and failed depth test.
   * \param stencil_z_pass Stencil operation \p STNCLOP_* for passed stencil and depth tests.
   * \param write_mask Identify a portion of the depth-stencil buffer for writing stencil data.
   * \param read_mask Identify a portion of the depth-stencil buffer for reading stencil data.
   */
  void set(uint8_t stencil_func, uint8_t stencil_fail, uint8_t z_fail, uint8_t stencil_z_pass, uint8_t write_mask, uint8_t read_mask)
  {
    func = stencil_func;
    fail = stencil_fail;
    zFail = z_fail;
    pass = stencil_z_pass;
    writeMask = write_mask;
    readMask = read_mask;
    G_ASSERT(func && fail && zFail && pass);
  }
};
} // namespace shaders
