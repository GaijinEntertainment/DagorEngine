/*
 * Copyright (c) 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Intel Corporation nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT, substantial rework and amendments
// (for conditions of use see prog/license.txt)
//
#pragma once

/*!
 *  \file MaskedOcclusionCulling.h
 *  \brief Masked Occlusion Culling
 *
 *  General information
 *   - Input to all API functions are (x,y,w) clip-space coordinates (x positive left, y positive up, w positive away from camera).
 *     We entirely skip the z component and instead compute it as 1 / w, see next bullet. For TestRect the input is NDC (x/w, y/w).
 *   - We use a simple z = 1 / w transform, which is a bit faster than OGL/DX depth transforms. Thus, depth is REVERSED and z = 0 at
 *     the far plane and z = inf at w = 0. We also have to use a GREATER depth function, which explains why all the conservative
 *     tests will be reversed compared to what you might be used to (for example zMaxTri >= zMinBuffer is a visibility test)
 *   - We support different layouts for vertex data (basic AoS and SoA), but note that it's beneficial to store the position data
 *     as tightly in memory as possible to reduce cache misses. Big strides are bad, so it's beneficial to keep position as a separate
 *     stream (rather than bundled with attributes) or to keep a copy of the position data for the occlusion culling system.
 *   - The resolution width must be a multiple of 8 and height a multiple of 4.
 *   - The hierarchical Z buffer is stored OpenGL-style with the y axis pointing up. This includes the scissor box.
 *   - This code is only tested with Visual Studio 2015, but should hopefully be easy to port to other compilers.
 */


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines used to configure the implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef QUICK_MASK
/*!
 * Configure the algorithm used for updating and merging hierarchical z buffer entries. If QUICK_MASK
 * is defined to 1, use the algorithm from the paper "Masked Software Occlusion Culling", which has good
 * balance between performance and low leakage. If QUICK_MASK is defined to 0, use the algorithm from
 * "Masked Depth Culling for Graphics Hardware" which has less leakage, but also lower performance.
 */
#define QUICK_MASK 1

#endif

#ifndef USE_D3D
/*!
 * Configures the library for use with Direct3D (default) or OpenGL rendering. This changes whether the
 * screen space Y axis points downwards (D3D) or upwards (OGL), and is primarily important in combination
 * with the PRECISE_COVERAGE define, where this is important to ensure correct rounding and tie-breaker
 * behaviour. It also affects the ScissorRect screen space coordinates and the memory layout of the buffer
 * returned by ComputePixelDepthBuffer().
 */
#define USE_D3D 1

#endif

#ifndef PRECISE_COVERAGE
/*!
 * Define PRECISE_COVERAGE to 1 to more closely match GPU rasterization rules. The increased precision comes
 * at a cost of slightly lower performance.
 */
#define PRECISE_COVERAGE 0

#endif

#ifndef ENABLE_STATS
/*!
 * Define ENABLE_STATS to 1 to gather various statistics during occlusion culling. Can be used for profiling
 * and debugging. Note that enabling this function will reduce performance significantly.
 */
#define ENABLE_STATS 0

#endif

#define MASKED_TARGET_SIMD_SSE _TARGET_SIMD_SSE
// #define MASKED_TARGET_SIMD_SSE 4

#if !_TARGET_PC || _TARGET_SIMD_SSE < 4 || defined(__APPLE__) || !defined(__x86_64__)
#define MOC_VIRTUAL
#define MOC_PURE
#define MOC_OVERRIDE
#define MOC_SINGLE_IMPLEMENTATION
#else
#define MOC_VIRTUAL  virtual
#define MOC_PURE     = 0
#define MOC_OVERRIDE override
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Masked occlusion culling class
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MaskedOcclusionCulling
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Memory management callback functions
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  typedef void *(*pfnAlignedAlloc)(size_t alignment, size_t size);
  typedef void (*pfnAlignedFree)(void *ptr);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Enums
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  enum Implementation
  {
    SSE2 = 0,
    SSE41 = 1,
    AVX2 = 2,
    // AVX512 = 3,
    NEON = 4
  };

  enum BackfaceWinding
  {
    BACKFACE_NONE = 0,
    BACKFACE_CW = 1,
    BACKFACE_CCW = 2,
  };

  enum CullingResult
  {
    VISIBLE = 0x0,
    OCCLUDED = 0x1,
    VIEW_CULLED = 0x3
  };

  enum ClipPlanes
  {
    CLIP_PLANE_NONE = 0x00,
    CLIP_PLANE_NEAR = 0x01,
    CLIP_PLANE_LEFT = 0x02,
    CLIP_PLANE_RIGHT = 0x04,
    CLIP_PLANE_BOTTOM = 0x08,
    CLIP_PLANE_TOP = 0x10,
    CLIP_PLANE_SIDES = (CLIP_PLANE_LEFT | CLIP_PLANE_RIGHT | CLIP_PLANE_BOTTOM | CLIP_PLANE_TOP),
    CLIP_PLANE_ALL = (CLIP_PLANE_LEFT | CLIP_PLANE_RIGHT | CLIP_PLANE_BOTTOM | CLIP_PLANE_TOP | CLIP_PLANE_NEAR)
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Structs
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*!
   * Used to control scissoring during rasterization. Note that we only provide coarse scissor
   * support. The scissor box x coordinates must be a multiple of 32, and the y coordinates a
   * multiple of 8. Scissoring is mainly meant as a means of enabling binning (sort middle)
   * rasterizers in case application developers want to use that approach for multithreading.
   */
  struct ScissorRect
  {
    ScissorRect() {}
    ScissorRect(int minX, int minY, int maxX, int maxY) : mMinX(minX), mMinY(minY), mMaxX(maxX), mMaxY(maxY) {}

    int mMinX = 0; //!< Screen space X coordinate for left side of scissor rect, inclusive and must be a
                   //!< multiple of 32
    int mMinY = 0; //!< Screen space Y coordinate for bottom side of scissor rect, inclusive and must be
                   //!< a multiple of 8
    int mMaxX = 0; //!< Screen space X coordinate for right side of scissor rect, <B>non</B> inclusive
                   //!< and must be a multiple of 32
    int mMaxY = 0; //!< Screen space Y coordinate for top side of scissor rect, <B>non</B> inclusive and
                   //!< must be a multiple of 8
  };

  /*!
   * Used to specify storage area for a binlist, containing triangles. This struct is used for
   * binning and multithreading. The host application is responsible for allocating memory for the
   * binlists.
   */
  struct TriList
  {
    unsigned int mNumTriangles; //!< Maximum number of triangles that may be stored in mPtr
    unsigned int mTriIdx;       //!< Index of next triangle to be written, clear before calling
                                //!< BinTriangles to start from the beginning of the list
    float *mPtr;                //!< Scratchpad buffer allocated by the host application
  };

  /*!
   * Statistics that can be gathered during occluder rendering and visibility to aid debugging
   * and profiling. Must be enabled by changing the ENABLE_STATS define.
   */
  struct OcclusionCullingStatistics
  {
    struct
    {
      long long mNumProcessedTriangles;  //!< Number of occluder triangles processed in total
      long long mNumRasterizedTriangles; //!< Number of occluder triangles passing view frustum and
                                         //!< backface culling
      long long mNumTilesTraversed;      //!< Number of tiles traversed by the rasterizer
      long long mNumTilesUpdated;        //!< Number of tiles where the hierarchical z buffer was updated
      long long mNumTilesMerged;         //!< Number of tiles where the hierarchical z buffer was updated
    } mOccluders;

    struct
    {
      long long mNumProcessedRectangles; //!< Number of rects processed (TestRect())
      long long mNumProcessedTriangles;  //!< Number of ocludee triangles processed (TestTriangles())
      long long mNumRasterizedTriangles; //!< Number of ocludee triangle passing view frustum and
                                         //!< backface culling
      long long mNumTilesTraversed;      //!< Number of tiles traversed by triangle & rect rasterizers
    } mOccludees;
  };

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Functions
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /*!
   * \brief Creates a new object with default state, no z buffer attached/allocated.
   */
  static MaskedOcclusionCulling *Create();

  /*!
   * \brief Creates a new object with default state, no z buffer attached/allocated.
   * \param alignedAlloc Pointer to a callback function used when allocating memory
   * \param alignedFree Pointer to a callback function used when freeing memory
   */
  static MaskedOcclusionCulling *Create(Implementation RequestedSIMD, pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree);

  /*!
   * \brief Destroys an object and frees the z buffer memory. Note that you cannot
   * use the delete operator, and should rather use this function to free up memory.
   */
  static void Destroy(MaskedOcclusionCulling *moc);

  /*!
   * \brief Sets the resolution of the hierarchical depth buffer. This function will
   *        re-allocate the current depth buffer (if present). The contents of the
   *        buffer is undefined until ClearBuffer() is called.
   *
   * \param witdh The width of the buffer in pixels, must be a multiple of 8
   * \param height The height of the buffer in pixels, must be a multiple of 4
   */
  MOC_VIRTUAL void SetResolution(unsigned int width, unsigned int height) MOC_PURE;

  /*!
   * \brief Gets the resolution of the hierarchical depth buffer.
   *
   * \param witdh Output: The width of the buffer in pixels
   * \param height Output: The height of the buffer in pixels
   */
  MOC_VIRTUAL void GetResolution(unsigned int &width, unsigned int &height) const MOC_PURE;

  /*!
   * \brief Returns the tile size for the current implementation.
   *
   * \param nBinsW Number of vertical bins, the screen is divided into nBinsW x nBinsH
   *        rectangular bins.
   * \param nBinsH Number of horizontal bins, the screen is divided into nBinsW x nBinsH
   *        rectangular bins.
   * \param outBinWidth Output: The width of the single bin in pixels (except for the
   *        rightmost bin width, which is extended to resolution width)
   * \param outBinHeight Output: The height of the single bin in pixels (except for the
   *        bottommost bin height, which is extended to resolution height)
   */
  MOC_VIRTUAL void ComputeBinWidthHeight(unsigned int nBinsW, unsigned int nBinsH, unsigned int &outBinWidth,
    unsigned int &outBinHeight) MOC_PURE;

  /*!
   * \brief Sets the distance for the near clipping plane. Default is nearDist = 0.
   *
   * \param nearDist The distance to the near clipping plane, given as clip space w
   */
  MOC_VIRTUAL void SetNearClipPlane(float nearDist) MOC_PURE;

  /*!
   * \brief Gets the distance for the near clipping plane.
   */
  MOC_VIRTUAL float GetNearClipPlane() const MOC_PURE;

  /*!
   * \brief Clears the hierarchical depth buffer.
   */
  MOC_VIRTUAL void ClearBuffer() MOC_PURE;

  /*!
   * \brief Merge a second hierarchical depth buffer into the main buffer.
   */
  MOC_VIRTUAL void MergeBuffer(MaskedOcclusionCulling *BufferB) MOC_PURE;

  /*!
   * \brief Renders a mesh of occluder triangles and updates the hierarchical z buffer
   *        with conservative depth values.
   *
   * This function is optimized for vertex layouts with stride 16 and y and w
   * offsets of 4 and 12 bytes, respectively.
   *
   * \param inVtx Pointer to an array of input vertices, should point to the x component
   *        of the first vertex. The input vertices are given as (x,y,w) coordinates
   *        in clip space. The memory layout can be changed using vtxLayout.
   * \param inTris Pointer to an array of vertex indices. Each triangle is created
   *        from three indices consecutively fetched from the array.
   * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
   *        entries)
   * \param modelToClipMatrix all vertices will be transformed by this matrix before
   *        performing projection. If nullptr is passed the transform step will be skipped
   * \param bfWinding Sets triangle winding order to consider backfacing, must be one one
   *        of (BACKFACE_NONE, BACKFACE_CW and BACKFACE_CCW). Back-facing triangles are culled
   *        and will not be rasterized. You may use BACKFACE_NONE to disable culling for
   *        double sided geometry
   * \param clipPlaneMask A mask indicating which clip planes should be considered by the
   *        triangle clipper. Can be used as an optimization if your application can
   *        determine (for example during culling) that a group of triangles does not
   *        intersect a certain frustum plane. However, setting an incorrect mask may
   *        cause out of bounds memory accesses.
   * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
   *        description). For best performance, it is advisable to store position data
   *        as compactly in memory as possible.
   * \return Will return VIEW_CULLED if all triangles are either outside the frustum or
   *         backface culled, returns VISIBLE otherwise.
   */
  MOC_VIRTUAL CullingResult RenderTriangles(const float *inVtx, const unsigned short *inTris, int nTris,
    const float *modelToClipMatrix = nullptr, BackfaceWinding bfWinding = BACKFACE_CW,
    ClipPlanes clipPlaneMask = CLIP_PLANE_ALL) MOC_PURE;

  /*!
   * \brief Occlusion query for a rectangle with a given depth. The rectangle is given
   *        in normalized device coordinates where (x,y) coordinates between [-1,1] map
   *        to the visible screen area. The query uses a GREATER_EQUAL (reversed) depth
   *        test meaning that depth values equal to the contents of the depth buffer are
   *        counted as visible.
   *
   * \param xmin NDC coordinate of the left side of the rectangle.
   * \param ymin NDC coordinate of the bottom side of the rectangle.
   * \param xmax NDC coordinate of the right side of the rectangle.
   * \param ymax NDC coordinate of the top side of the rectangle.
   * \param ymax NDC coordinate of the top side of the rectangle.
   * \param wmin Clip space W coordinate for the rectangle.
   * \return The query will return VISIBLE if the rectangle may be visible, OCCLUDED
   *         if the rectangle is occluded by a previously rendered  object, or VIEW_CULLED
   *         if the rectangle is outside the view frustum.
   */
  MOC_VIRTUAL CullingResult TestRect(float xmin, float ymin, float xmax, float ymax, float wmin) const MOC_PURE;

  /*!
   * \brief This function is similar to RenderTriangles(), but performs an occlusion
   *        query instead and does not update the hierarchical z buffer. The query uses
   *        a GREATER_EQUAL (reversed) depth test meaning that depth values equal to the
   *        contents of the depth buffer are counted as visible.
   *
   * This function is optimized for vertex layouts with stride 16 and y and w
   * offsets of 4 and 12 bytes, respectively.
   *
   * \param inVtx Pointer to an array of input vertices, should point to the x component
   *        of the first vertex. The input vertices are given as (x,y,w) coordinates
   *        in clip space. The memory layout can be changed using vtxLayout.
   * \param inTris Pointer to an array of triangle indices. Each triangle is created
   *        from three indices consecutively fetched from the array.
   * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
   *        entries)
   * \param modelToClipMatrix all vertices will be transformed by this matrix before
   *        performing projection. If nullptr is passed the transform step will be skipped
   * \param bfWinding Sets triangle winding order to consider backfacing, must be one one
   *        of (BACKFACE_NONE, BACKFACE_CW and BACKFACE_CCW). Back-facing triangles are culled
   *        and will not be occlusion tested. You may use BACKFACE_NONE to disable culling
   *        for double sided geometry
   * \param clipPlaneMask A mask indicating which clip planes should be considered by the
   *        triangle clipper. Can be used as an optimization if your application can
   *        determine (for example during culling) that a group of triangles does not
   *        intersect a certain frustum plane. However, setting an incorrect mask may
   *        cause out of bounds memory accesses.
   * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
   *        description). For best performance, it is advisable to store position data
   *        as compactly in memory as possible.
   * \return The query will return VISIBLE if the triangle mesh may be visible, OCCLUDED
   *         if the mesh is occluded by a previously rendered object, or VIEW_CULLED if all
   *         triangles are entirely outside the view frustum or backface culled.
   */
  MOC_VIRTUAL CullingResult TestTriangles(const float *inVtx, const unsigned short *inTris, int nTris,
    const float *modelToClipMatrix = nullptr, BackfaceWinding bfWinding = BACKFACE_CW,
    ClipPlanes clipPlaneMask = CLIP_PLANE_ALL) MOC_PURE;

  /*!
   * \brief Perform input assembly, clipping , projection, triangle setup, and write
   *        triangles to the screen space bins they overlap. This function can be used to
   *        distribute work for threading (See the CullingThreadpool class for an example)
   *
   * \param inVtx Pointer to an array of input vertices, should point to the x component
   *        of the first vertex. The input vertices are given as (x,y,w) coordinates
   *        in clip space. The memory layout can be changed using vtxLayout.
   * \param inTris Pointer to an array of vertex indices. Each triangle is created
   *        from three indices consecutively fetched from the array.
   * \param nTris The number of triangles to render (inTris must contain atleast 3*nTris
   *        entries)
   * \param triLists Pointer to an array of TriList objects with one TriList object per
   *        bin. If a triangle overlaps a bin, it will be written to the corresponding
   *        trilist. Note that this method appends the triangles to the current list, to
   *        start writing from the beginning of the list, set triList.mTriIdx = 0
   * \param nBinsW Number of vertical bins, the screen is divided into nBinsW x nBinsH
   *        rectangular bins.
   * \param nBinsH Number of horizontal bins, the screen is divided into nBinsW x nBinsH
   *        rectangular bins.
   * \param modelToClipMatrix all vertices will be transformed by this matrix before
   *        performing projection. If nullptr is passed the transform step will be skipped
   * \param clipPlaneMask A mask indicating which clip planes should be considered by the
   *        triangle clipper. Can be used as an optimization if your application can
   *        determine (for example during culling) that a group of triangles does not
   *        intersect a certain frustum plane. However, setting an incorrect mask may
   *        cause out of bounds memory accesses.
   * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
   *        description). For best performance, it is advisable to store position data
   *        as compactly in memory as possible.
   * \param bfWinding Sets triangle winding order to consider backfacing, must be one one
   *        of (BACKFACE_NONE, BACKFACE_CW and BACKFACE_CCW). Back-facing triangles are culled
   *        and will not be binned / rasterized. You may use BACKFACE_NONE to disable culling
   *        for double sided geometry
   */
  MOC_VIRTUAL void BinTriangles(const float *inVtx, const unsigned short *inTris, int nTris, TriList *triLists, unsigned int nBinsW,
    unsigned int nBinsH, const float *modelToClipMatrix = nullptr, BackfaceWinding bfWinding = BACKFACE_CW,
    ClipPlanes clipPlaneMask = CLIP_PLANE_ALL) MOC_PURE;

  /*!
   * \brief Renders all occluder triangles in a trilist. This function can be used in
   *        combination with BinTriangles() to create a threded (binning) rasterizer. The
   *        bins can be processed independently by different threads without risking writing
   *        to overlapping memory regions.
   *
   * \param triLists A triangle list, filled using the BinTriangles() function that is to
   *        be rendered.
   * \param scissor A scissor box limiting the rendering region to the bin. The size of each
   *        bin must be a multiple of 32x8 pixels due to implementation constraints. For a
   *        render target with (width, height) resolution and (nBinsW, nBinsH) bins, the
   *        size of a bin is:
   *          binWidth = (width / nBinsW) - (width / nBinsW) % 32;
   *          binHeight = (height / nBinsH) - (height / nBinsH) % 8;
   *        The last row and column of tiles have a different size:
   *          lastColBinWidth = width - (nBinsW-1)*binWidth;
   *          lastRowBinHeight = height - (nBinsH-1)*binHeight;
   */
  MOC_VIRTUAL void RenderTrilist(const TriList &triList, const ScissorRect *scissor) MOC_PURE;

  /*!
   * \brief Creates a per-pixel depth buffer from the hierarchical z buffer representation.
   *        Intended for visualizing the hierarchical depth buffer for debugging. The
   *        buffer is written in scanline order, from the top to bottom (D3D) or bottom to
   *        top (OGL) of the surface. See the USE_D3D define.
   *
   * \param depthData Pointer to memory where the per-pixel depth data is written. Must
   *        hold storage for atleast width*height elements as set by setResolution.
   */
  MOC_VIRTUAL void ComputePixelDepthBuffer(float *depthData, bool flipY) MOC_PURE;

  /*!
   * \brief Fetch occlusion culling statistics, returns zeroes if ENABLE_STATS define is
   *        not defined. The statistics can be used for profiling or debugging.
   */
  MOC_VIRTUAL OcclusionCullingStatistics GetStatistics() MOC_PURE;

  /*!
   * \brief Returns the implementation (CPU instruction set) version of this object.
   */
  MOC_VIRTUAL Implementation GetImplementation() MOC_PURE;

  /*!
   * \brief Utility function for transforming vertices and outputting them to an (x,y,z,w)
   *        format suitable for the occluder rasterization and occludee testing functions.
   *
   * \param mtx Pointer to matrix data. The matrix should column major for post
   *        multiplication (OGL) and row major for pre-multiplication (DX). This is
   *        consistent with OpenGL / DirectX behavior.
   * \param inVtx Pointer to an array of input vertices. The input vertices are given as
   *        (x,y,z) coordinates. The memory layout can be changed using vtxLayout.
   * \param xfVtx Pointer to an array to store transformed vertices. The transformed
   *        vertices are always stored as array of structs (AoS) (x,y,z,w) packed in memory.
   * \param nVtx Number of vertices to transform.
   * \param vtxLayout A struct specifying the vertex layout (see struct for detailed
   *        description). For best performance, it is advisable to store position data
   *        as compactly in memory as possible. Note that for this function, the
   *        w-component is assumed to be 1.0.
   */
  static void TransformVertices(const float *mtx, const float *inVtx, float *xfVtx, unsigned int nVtx);

  /*!
   * \brief Get used memory alloc/free callbacks.
   */
  void GetAllocFreeCallback(pfnAlignedAlloc &allocCallback, pfnAlignedFree &freeCallback)
  {
    allocCallback = mAlignedAllocCallback, freeCallback = mAlignedFreeCallback;
  }


  MOC_VIRTUAL void CombinePixelDepthBuffer2W(float *depthData, int w, int h) MOC_PURE;
  MOC_VIRTUAL void DecodePixelDepthBuffer2W(float *depthData, int w, int h) MOC_PURE;

  /*!
   * \brief Fetch occlusion culling statistics, returns zeroes if ENABLE_STATS define is
   *        not defined. The statistics can be used for profiling or debugging.
   */
#if ENABLE_STATS != 0
  OcclusionCullingStatistics GetStatistics();
#endif

  MOC_VIRTUAL void mergeOcclusions(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
    uint32_t last_tile) MOC_PURE;
  MOC_VIRTUAL void mergeOcclusionsZmin(MaskedOcclusionCulling **another_occl, uint32_t occl_count, uint32_t first_tile,
    uint32_t last_tile) MOC_PURE;
  MOC_VIRTUAL uint32_t getTilesCount() const MOC_PURE;

protected:
  pfnAlignedAlloc mAlignedAllocCallback;
  pfnAlignedFree mAlignedFreeCallback;

  mutable OcclusionCullingStatistics mStats;

  MOC_VIRTUAL ~MaskedOcclusionCulling() {}
};
