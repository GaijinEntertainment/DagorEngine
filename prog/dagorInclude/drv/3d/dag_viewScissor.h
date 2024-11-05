//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>

struct ScissorRect;
struct Viewport;

namespace d3d
{
/**
 * @brief Set scissor for the current render target. Part of the render target that is outside the scissor rectangle is not rendered.
 *
 * @param x - left corner of the scissor rectangle
 * @param y - top corner of the scissor rectangle
 * @param w - width of the scissor rectangle
 * @param h - height of the scissor rectangle
 * @return true if the scissor rectangle was set successfully
 */
bool setscissor(int x, int y, int w, int h);

/**
 * @brief Set scissor for the current set of render targets. Part of the render target that is outside the scissor rectangle is not
 * rendered.
 *
 * @param scissorRects - array of scissor rectangles. Should be the same size as the number of render targets.
 * @return true if the scissor rectangles were set successfully
 */
bool setscissors(dag::ConstSpan<ScissorRect> scissorRects);

/**
 * @brief Set view for the current render target. Part of the render target that is outside the view rectangle is not rendered.
 *
 * @param x - left corner of the view rectangle
 * @param y - top corner of the view rectangle
 * @param w - width of the view rectangle
 * @param h - height of the view rectangle
 * @param minz - minimum depth value of the view rectangle
 * @param maxz - maximum depth value of the view rectangle
 * @return true if the view rectangle was set successfully
 */
bool setview(int x, int y, int w, int h, float minz, float maxz);

/**
 * @brief Set view for the current set of render targets. Part of the render target that is outside the view rectangle is not rendered.
 *
 * @param viewports - array of view rectangles. Should be the same size as the number of render targets.
 * @return true if the view rectangles were set successfully
 */
bool setviews(dag::ConstSpan<Viewport> viewports);

/**
 * @brief Get view for the current render target.
 *
 * @deprecated Don't use it since this method relies on the global state.
 *
 * @param x - left corner of the view rectangle
 * @param y - top corner of the view rectangle
 * @param w - width of the view rectangle
 * @param h - height of the view rectangle
 * @param minz - minimum depth value of the view rectangle
 * @param maxz - maximum depth value of the view rectangle
 * @return true if the view rectangle was retrieved successfully
 */
bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz);
} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline bool setscissor(int x, int y, int w, int h) { return d3di.setscissor(x, y, w, h); }
inline bool setscissors(dag::ConstSpan<ScissorRect> scissorRects) { return d3di.setscissors(scissorRects); }
inline bool setview(int x, int y, int w, int h, float minz, float maxz) { return d3di.setview(x, y, w, h, minz, maxz); }
inline bool setviews(dag::ConstSpan<Viewport> viewports) { return d3di.setviews(viewports); }
inline bool getview(int &x, int &y, int &w, int &h, float &minz, float &maxz) { return d3di.getview(x, y, w, h, minz, maxz); }
} // namespace d3d
#endif
