//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>

class Sbuffer;

using Ibuffer = Sbuffer;
using Vbuffer = Sbuffer;

namespace d3d
{
/**
 * @brief Creates a vertex buffer.
 *
 * This function creates a vertex buffer with the specified size and flags.
 *
 * @todo Make the name parameter mandatory.
 *
 * @param size_bytes The size of the vertex buffer in bytes.
 * @param flags The creation flags.
 * @param name The name of the vertex buffer (temporarily optional).
 * @return A pointer to the created vertex buffer.
 */
Sbuffer *create_vb(int size_bytes, int flags, const char *name = "");

/**
 * @brief Creates an index buffer.
 *
 * This function creates an index buffer with the specified size and flags.
 *
 * @todo Make the name parameter mandatory.
 *
 * @param size_bytes The size of the index buffer in bytes.
 * @param flags The creation flags.
 * @param stat_name The name of the index buffer (temporarily optional).
 * @return A pointer to the created index buffer.
 */
Sbuffer *create_ib(int size_bytes, int flags, const char *stat_name = "ib");

/**
 * @brief Sets the vertex buffer as a stream source.
 *
 * This function sets the vertex buffer as a stream source for the specified stream.
 *
 * @param stream The stream index.
 * @param vb A pointer to the vertex buffer.
 * @param offset The offset in bytes from the start of the vertex buffer.
 * @param stride_bytes The stride in bytes between vertices.
 * @return True if the vertex stream source was set successfully, false otherwise.
 */
bool setvsrc_ex(int stream, Sbuffer *vb, int offset, int stride_bytes);

/**
 * @brief Sets the vertex buffer as a stream source.
 *
 * This function sets the vertex buffer as a stream source for the specified stream.
 *
 * @param stream The stream index.
 * @param vb A pointer to the vertex buffer.
 * @param stride_bytes The stride in bytes between vertices.
 * @return True if the vertex stream source was set successfully, false otherwise.
 */
inline bool setvsrc(int stream, Sbuffer *vb, int stride_bytes) { return setvsrc_ex(stream, vb, 0, stride_bytes); }

/**
 * @brief Sets the index buffer.
 *
 * This function sets the index buffer for rendering.
 *
 * @param ib A pointer to the index buffer.
 * @return True if the indices were set successfully, false otherwise.
 */
bool setind(Sbuffer *ib);

/**
 * @brief Creates a DX8-style vertex declaration.
 *
 * This function creates a DX8-style vertex declaration based on the specified vertex shader declaration.
 *
 * @todo Do we really need to support DX8-style vertex declarations? Maybe it's time to look for a more modern solution.
 *
 * @param vsd The vertex shader declaration.
 * @return The created vertex declaration, or BAD_VDECL on error.
 */
VDECL create_vdecl(VSDTYPE *vsd);

/**
 * @brief Deletes a vertex declaration.
 *
 * This function deletes a vertex declaration.
 *
 * @param vdecl The vertex declaration to delete.
 */
void delete_vdecl(VDECL vdecl);

/**
 * @brief Sets the current vertex declaration.
 *
 * This function sets the current vertex declaration for rendering.
 *
 * @param vdecl The vertex declaration to set.
 * @return True if the vertex declaration was set successfully, false otherwise.
 */
bool setvdecl(VDECL vdecl);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline Sbuffer *create_vb(int sz, int f, const char *name) { return d3di.create_vb(sz, f, name); }
inline Sbuffer *create_ib(int size_bytes, int flags, const char *stat_name) { return d3di.create_ib(size_bytes, flags, stat_name); }
inline bool setvsrc_ex(int s, Sbuffer *vb, int ofs, int stride_bytes) { return d3di.setvsrc_ex(s, vb, ofs, stride_bytes); }
inline bool setind(Sbuffer *ib) { return d3di.setind(ib); }
inline VDECL create_vdecl(VSDTYPE *vsd) { return d3di.create_vdecl(vsd); }
inline void delete_vdecl(VDECL vdecl) { return d3di.delete_vdecl(vdecl); }
inline bool setvdecl(VDECL vdecl) { return d3di.setvdecl(vdecl); }
} // namespace d3d
#endif
