//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <util/dag_inttypes.h>
#include <generic/dag_tab.h>

/**
 * @brief Holds a direct pointer to a (compressed) shader data in bindump
 *        Supports both compressed (if dictionary not nullptr) and uncompressed data
 */
struct ShaderSource
{
  dag::ConstSpan<uint8_t> compressedData;
  dag::ConstSpan<uint8_t> metadata;
  uint32_t uncompressedSize = 0;
  void *dictionary = nullptr;

  /**
   * @brief Uncompresses the shader data, in case of no compression just copies it
   *
   * @param tmpbuf Temporary memory that will hold resulting shader data
   * @return Pointer to uncompressed shader data.
   */
  const uint32_t *uncompress(Tab<uint8_t> &tmpbuf) const;
};

namespace d3d
{
/**
 * @brief Creates a program with a vertex shader, fragment shader, and vertex declaration.
 *
 * @param vprog The vertex shader program.
 * @param fsh The fragment shader program.
 * @param vdecl The vertex declaration.
 * @param strides The stride values for each vertex stream (optional, default is 0).
 * @param streams The number of vertex streams (optional, default is 0).
 * @return The created program.
 *
 * If strides and streams are not set, they will be obtained from the vertex declaration.
 * The program should be deleted externally using delete_program().
 */
PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides = nullptr, unsigned streams = 0);

/**
 * @brief Creates a compute shader program with native code.
 *
 * @param cs_native The native code for the compute shader.
 * @param preloaded The preloaded data for the compute shader.
 * @return The created program.
 */
PROGRAM create_program_cs(const ShaderSource &cs_native, CSPreloaded preloaded);

/**
 * @brief Sets the program as the current program, including the pixel shader, vertex shader, and vertex declaration.
 *
 * @param program The program to set.
 * @return True if the program was set successfully, false otherwise.
 */
bool set_program(PROGRAM program);

/**
 * @brief Deletes a program, including the vertex shader and fragment shader.
 *
 * @param program The program to delete.
 *
 * @warning The vertex declaration should be deleted independently.
 */
void delete_program(PROGRAM program);

/**
 * @brief Creates a vertex shader with native code.
 *
 * @param native_code The native code for the vertex shader.
 * @return The created vertex shader.
 */
VPROG create_vertex_shader(const ShaderSource &native_code);

/**
 * @brief Deletes a vertex shader.
 *
 * @param vs The vertex shader to delete.
 */
void delete_vertex_shader(VPROG vs);

/**
 * @brief Creates a pixel shader with native code.
 *
 * @param native_code The native code for the pixel shader.
 * @return The created pixel shader.
 */
FSHADER create_pixel_shader(const ShaderSource &native_code);

/**
 * @brief Deletes a pixel shader.
 *
 * @param ps The pixel shader to delete.
 */
void delete_pixel_shader(FSHADER ps);

/**
 * @brief Gets the debug program.
 *
 * @details This program's bytecode is written in the source code of the driver, so the program is always available (if a driver
 * supports this API). The debug program is used to draw debug stuff (vertex colored primitives).
 *
 * @return The debug program.
 */
PROGRAM get_debug_program();
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline PROGRAM create_program(VPROG vprog, FSHADER fsh, VDECL vdecl, unsigned *strides, unsigned streams)
{
  return d3di.create_program_0(vprog, fsh, vdecl, strides, streams);
}

inline PROGRAM create_program_cs(const ShaderSource &cs_native, CSPreloaded preloaded)
{
  return d3di.create_program_cs(cs_native, preloaded);
}

inline bool set_program(PROGRAM p) { return d3di.set_program(p); }

inline void delete_program(PROGRAM p) { return d3di.delete_program(p); }

inline VPROG create_vertex_shader(const ShaderSource &native_code) { return d3di.create_vertex_shader(native_code); }

inline void delete_vertex_shader(VPROG vs) { return d3di.delete_vertex_shader(vs); }

inline FSHADER create_pixel_shader(const ShaderSource &native_code) { return d3di.create_pixel_shader(native_code); }

inline void delete_pixel_shader(FSHADER ps) { return d3di.delete_pixel_shader(ps); }

inline PROGRAM get_debug_program() { return d3di.get_debug_program(); }
} // namespace d3d
#endif
