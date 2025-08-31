//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_overrideStateId.h>
#include <util/dag_hash.h>
#include <string.h>

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

/**
 * \brief A structure that describes current override for a render state.
 *
 * It is used to override some render state parameters.
 */
struct OverrideState
{
  /**
   * \brief Bit flags that describe which parameters are overridden.
   */
  enum StateBits
  {
    Z_TEST_DISABLE = 1 << 0,       ///< Disable depth test.
    Z_WRITE_DISABLE = 1 << 1,      ///< Disable depth write. Can't be used with \ref StateBits::Z_WRITE_ENABLE "Z_WRITE_ENABLE".
    Z_BOUNDS_ENABLED = 1 << 2,     ///< Enable depth bounds test. Check \p hasDepthBoundsTest driver cap to see if it is supported.
    Z_CLAMP_ENABLED = 1 << 3,      ///< Enable depth clamp.
    Z_FUNC = 1 << 4,               ///< Override depth comparison function.
    Z_BIAS = 1 << 5,               ///< Override depth bias or/and slope depth bias. See \ref OverrideState::zBias "zBias" and \ref
                                   ///< OverrideState::slopeZBias "slopeZBias".
    STENCIL = 1 << 6,              ///< Override the whole stencil state.
    BLEND_OP = 1 << 7,             ///< Override blend operation.
    BLEND_OP_A = 1 << 8,           ///< Override alpha blend operation.
    BLEND_SRC_DEST = 1 << 9,       ///< Override source and destination blend factors.
    BLEND_SRC_DEST_A = 1 << 10,    ///< Override source and destination alpha blend factors.
    CULL_NONE = 1 << 11,           ///< Disables any face culling even if \ref StateBits::FLIP_CULL "FLIP_CULL" is used.
    FLIP_CULL = 1 << 12,           ///< Flip culling mode.
    FORCED_SAMPLE_COUNT = 1 << 13, ///< Override forced sample count. Check \p hasForcedSamplerCount and \p hasUAVOnlyForcedSampleCount
                                   ///< driver caps to see if it is supported. Doesn't have any effect if \ref
                                   ///< OverrideState::forcedSampleCount "forcedSampleCount" is 0.
    CONSERVATIVE = 1 << 14, ///< Enable conservative rasterization. Check \p hasConservativeRassterization driver cap to see if it is
                            ///< supported.
    SCISSOR_ENABLED = 1 << 15,   ///< Enable scissor test.
    ALPHA_TO_COVERAGE = 1 << 16, ///< Enable alpha-to-coverage.
    Z_WRITE_ENABLE = 1 << 17, ///< Enable depth write. Can't be used with \ref StateBits::Z_WRITE_DISABLE "Z_WRITE_DISABLE". It doesn't
                              ///< have any effect if \ref StateBits::FORCED_SAMPLE_COUNT "FORCED_SAMPLE_COUNT" is enabled.
  };
  uint32_t bits = 0;             ///< Bit flags that describe which parameters are overridden.
  uint8_t zFunc;                 ///< Overridden depth function value. \todo zFunc can be 3 bits
  uint8_t forcedSampleCount;     ///< Overridden forced sample count.
  uint8_t blendOp = 0;           ///< Overridden blend operation. \todo blendOp can be 3 bits
  uint8_t blendOpA = 0;          ///< Overridden alpha blend operation. \todo blendOpA can be 3 bits
  uint8_t sblend = 0;            ///< Overridden source blend factor.
  uint8_t dblend = 0;            ///< Overridden destination blend factor.
  uint8_t sblenda = 0;           ///< Overridden source alpha blend factor.
  uint8_t dblenda = 0;           ///< Overridden destination alpha blend factor.
  uint32_t colorWr = 0xFFFFFFFF; ///< Overridden color write mask. It is always \p &= with the color write mask of the current render
                                 ///< state.
  StencilState stencil;          ///< Overridden stencil state.

  float zBias = 0;      ///< Overridden depth bias value.
  float slopeZBias = 0; ///< Overridden slope depth bias value.

  /**
   * \brief Check if any of the given features (bitset) is set. Use it to check only a single feature.
   *
   * \param mask Bitset to check.
   *
   * \return \p true if any of the given features is set.
   */
  bool isOn(uint32_t mask) const { return (bits & mask) != 0; }
  /**
   * \brief Set the given features (bitset).
   *
   * \param mask Bitset to set.
   */
  void set(uint32_t mask) { bits |= mask; }
  /**
   * \brief Reset the given features (bitset).
   *
   * \param mask Bitset to reset.
   */
  void reset(uint32_t mask) { bits &= ~mask; }
  /**
   * \brief Default constructor.
   *
   * None of the features are set.
   * Color write mask is set to 0xFFFFFFFF, so it doesn't have any effect.
   */
  OverrideState()
  {
    memset(this, 0, sizeof(OverrideState));
    colorWr = 0xFFFFFFFF;
  }
  /**
   * \brief Copy constructor.
   *
   * Since it's a POD class, it's safe to use memcpy.
   *
   * \param a The object to copy.
   */
  OverrideState(const OverrideState &a) { memcpy(this, &a, sizeof(*this)); }

  /**
   * \brief Assignment operator.
   *
   * Since it's a POD class, it's safe to use memcpy.
   *
   * \param a The object to copy.
   *
   * \return Reference to this object.
   */
  OverrideState &operator=(const OverrideState &a)
  {
    if (this != &a)
      memcpy(this, &a, sizeof(*this));
    return *this;
  }

  /**
   * \brief Compare two objects.
   *
   * \warning \ref OverrideState::zBias "zBias" and \ref OverrideState::slopeZBias "slopeZBias" are float values, so be careful with
   * the operator because it compares override states bitwise.
   *
   * \param s The object to compare.
   *
   * \return \p true if the objects are equal.
   */
  bool operator==(const OverrideState &s) const { return memcmp(this, &s, sizeof(OverrideState)) == 0; }
  /**
   * \brief Compare two objects.
   *
   * \warning \ref OverrideState::zBias "zBias" and \ref OverrideState::slopeZBias "slopeZBias" are float values, so be careful with
   * the operator because it compares override states bitwise.
   *
   * \param s The object to compare.
   *
   * \return \p true if the objects are not equal.
   */
  bool operator!=(const OverrideState &s) const { return !(*this == s); }

  /**
   * \brief Validate the object.
   *
   * If some features are not set, it sets the corresponding values to 0.
   * Overrides for \ref OverrideState::zBias "zBias", \ref OverrideState::slopeZBias "slopeZBias", \ref
   * OverrideState::forcedSampleCount "forcedSampleCount", \ref OverrideState::zFunc "zFunc", \ref OverrideState::blendOp "blendOp",
   * and \ref OverrideState::stencil "stencil" are set to 0 if corresponding bits are not set.
   *
   * \todo Rename the method since it doesn't validate the object, but moves the object to a valid state.
   */
  void validate()
  {
    if (!isOn(Z_BIAS))
      zBias = slopeZBias = 0.f;
    if (!isOn(FORCED_SAMPLE_COUNT))
      forcedSampleCount = 0;
    if (!isOn(Z_FUNC))
      zFunc = 0;
    if (!isOn(BLEND_OP))
      blendOp = 0;
    if (!isOn(STENCIL))
      memset(&stencil, 0, sizeof(stencil));
  }
};

/**
 * \brief Set reference value for stencil operations.
 *
 * \param ref Reference value to set.
 *
 * \todo In the moment, we use this instead of absent d3d::set_stencil_ref. To be replaced
 */
void set_stencil_ref(uint8_t ref);

}; // namespace shaders
