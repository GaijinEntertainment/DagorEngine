//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

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

/**
 * Creates a ResourceClearValue object with the specified RGBA values.
 *
 * @param r The red component of the clear value.
 * @param g The green component of the clear value.
 * @param b The blue component of the clear value.
 * @param a The alpha component of the clear value.
 * @return The created ResourceClearValue object.
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
 * Creates a ResourceClearValue object with the specified RGBA values.
 *
 * @param r The red component of the clear value.
 * @param g The green component of the clear value.
 * @param b The blue component of the clear value.
 * @param a The alpha component of the clear value.
 * @return The created ResourceClearValue object.
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
 * Creates a ResourceClearValue object with the specified RGBA values.
 *
 * @param r The red component of the clear value.
 * @param g The green component of the clear value.
 * @param b The blue component of the clear value.
 * @param a The alpha component of the clear value.
 * @return The created ResourceClearValue object.
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
 * Creates a ResourceClearValue object with the specified Depth/Stencil values.
 *
 * @param d The depth component of the clear value.
 * @param s The stencil component of the clear value.
 * @return The created ResourceClearValue object.
 */
inline ResourceClearValue make_clear_value(float d, uint8_t s)
{
  ResourceClearValue result;
  result.asDepth = d;
  result.asStencil = s;
  return result;
}
