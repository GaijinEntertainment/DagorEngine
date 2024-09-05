//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_lsbVisitor.h>

class BaseTexture;
struct Driver3dRenderTarget;
struct ViewProjMatrixContainer;
class String;

/**
 * @brief Retrieves current render target.
 * 
 * @param [out] rt  Current render target.
 */
extern void d3d_get_render_target(Driver3dRenderTarget &rt);

/**
 * @brief Sets current render target.
 *
 * @param [in] rt   Render target to assign.
 * @return          \c true on success, \c false otherwise.
 */
extern void d3d_set_render_target(Driver3dRenderTarget &rt);

/**
 * @brief Retrieves current view and projection martices as well as perspective.
 * 
 * @param [out] vp  Output object storing the retrieved matrices as well as perspective. 
 */
extern void d3d_get_view_proj(ViewProjMatrixContainer &vp);

/**
 * @brief Sets current view and projection matrices as well as perspective.
 * 
 * @param [in] vp   An object storing matrices and perspective to assign.
 */
extern void d3d_set_view_proj(const ViewProjMatrixContainer &vp);

/**
 * @brief Retrieves current viewport data.
 * 
 * @param [out] viewX, viewY    Viewport top-top left corner position.
 * @param [out] viewW, viewH    Viewport size.
 * @param [out] viewN           Viewport near plane position.
 * @param [out] viewF           Viewport far plane position.
 */
extern void d3d_get_view(int &viewX, int &viewY, int &viewW, int &viewH, float &viewN, float &viewF);

/**
 * @brief Sets current viewport data.
 *
 * @param [in] viewX, viewY    Viewport top-top left corner position.
 * @param [in] viewW, viewH    Viewport size.
 * @param [in] viewN           Viewport near plane position.
 * @param [in] viewF           Viewport far plane position.
 */
extern void d3d_set_view(int viewX, int viewY, int viewW, int viewH, float viewN, float viewF);

/**
 * @brief Concatenates 2 arguments.
 */
#define __CONCAT_HELPER__(x, y) x##y

/**
 * @brief Concatenates 2 arguments.
 */
#define __CONCAT__(x, y)        __CONCAT_HELPER__(x, y)

/**
 * @brief Creates a scoped instance of \ref ViewProjMatrixContainer.
 * 
 * See \ref ScopeViewProjMatrix for more details.
 */
#define SCOPE_VIEW_PROJ_MATRIX ScopeViewProjMatrix __CONCAT__(scopevpm, __LINE__)

/**
 * @brief Creates a scoped instance storing viewport data.
 *
 * See \ref ScopeViewport for more details.
 */
#define SCOPE_VIEWPORT         ScopeViewport __CONCAT__(scopevp, __LINE__)

/**
 * @brief Creates scoped instances of \ref Driver3dRenderTarget and viewport.
 *
 * See \ref ScopeRenderTarget and \ref ScopeViewport for more details.
 */
#define SCOPE_RENDER_TARGET \
  SCOPE_VIEWPORT;           \
  ScopeRenderTarget __CONCAT__(scopert, __LINE__)

//--- Data structures -------

/**
 * @brief Determines scissors rectangle geometry.
 */
struct ScissorRect
{
  /**
   * @brief Scissors rectangle top left corner position.
   */
  int x, y;

  /**
   * @brief Scissors rectangle size.
   */
  int w, h;
};

/**
 * @brief Determines viewport geometry.
 */
struct Viewport : public ScissorRect
{
  /**
   * @brief Minimum depth of the viewport. Ranges between 0 and 1.
   */
  float minz;

  /**
   * @brief Maximum depth of the viewport. Ranges between 0 and 1.
   */
  float maxz;

  /**
   * @brief Maximal number of viewports/ scissors suported.
   */
  static constexpr uint32_t MAX_VIEWPORT_COUNT = 16;
};

/**
 * @brief Structure that stores perspective projection data.
 */
struct Driver3dPerspective
{
  /**
  * @brief Default constructor.
  */
  Driver3dPerspective() : wk(0.f), hk(0.f), zn(0.f), zf(0.f), ox(0.f), oy(0.f) {}

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] wk_, hk_   Perspective projection matrix [0][0] and [1][1] elements. (Look <a href="https://solarianprogrammer.com/2013/05/22/opengl-101-matrices-projection-view-model/">here</a> for info)
   * @param [in] zn_, zf_   Frustrum near and far plane positions.
   * @param [in] oz_, oy_   Perspective projection matrix [0][2] and [1][2] elements. (Look <a href="https://solarianprogrammer.com/2013/05/22/opengl-101-matrices-projection-view-model/">here</a> for info)
   */
  Driver3dPerspective(float wk_, float hk_, float zn_, float zf_, float ox_ = 0.f, float oy_ = 0.f) :
    wk(wk_), hk(hk_), zn(zn_), zf(zf_), ox(ox_), oy(oy_)
  {}

  /**
   * @brief Perspective projection matrix [0][0] and [1][1] elements. 
   *        (Look <a href="https://solarianprogrammer.com/2013/05/22/opengl-101-matrices-projection-view-model/">here</a> for info)
   */
  float wk, hk;

  /**
   * @brief Frustrum near and far plane positions.
   */
  float zn, zf;

  /**
   * @brief Perspective projection matrix [0][2] and [1][2] elements.
   *        (Look <a href="https://solarianprogrammer.com/2013/05/22/opengl-101-matrices-projection-view-model/">here</a> for info)
   */
  float ox, oy;
};

/**
 * @brief Class that defines render target array.
 */
struct Driver3dRenderTarget
{
  /**
   * @brief Class that defines a render target texture and its parameters.
   */
  struct RTState
  {

    /**
     * @brief A pointer to the texture containing the subresource that is used as render target.
     */
    BaseTexture *tex;

    /**
     * @brief Level of the mipmap in \c tex that is used as render target.
     */
    unsigned char level;

    /**
     * Defines either a cube texture face or a texture array index to use as render target.
     */
    union
    {
      unsigned short face;
      unsigned short layer;
    };

    /**
     * @brief Sets render target texture subresource.
     * 
     * @param [in] t    A pointer to the texture to use as a render target.
     * @param [in] lev  Level of the mipmap in \b t to use as a render target.
     * @param [in] f_l  Defines either a cube texture face or a texture array index to use as render target.
     */
    void set(BaseTexture *t, int lev, int f_l)
    {
      tex = t;
      level = lev;
      face = f_l;
    }

    /**
     * @brief Compares two render target states.
     * 
     * @param [in] l, r Objects to compare.
     * @return          \c true if states are identical, \c false otherwise.
     */
    friend bool operator==(const RTState &l, const RTState &r)
    {
      return (l.tex == r.tex) && (l.level == r.level) && (l.face == r.face);
    }

    /**
     * @brief Compares two render target states.
     *
     * @param [in] l, r Objects to compare.
     * @return          \c true if states are different, \c false otherwise.
     */
    friend bool operator!=(const RTState &l, const RTState &r) { return !(l == r); }
  };

  /**
   * @brief Determines maximal number of simultaneous render targets.
   */
  static constexpr int MAX_SIMRT = 8;
#ifndef INSIDE_DRIVER
protected:
#endif
  /**
   * @brief An array of states of each render target and the depth buffer.
   */
  RTState color[MAX_SIMRT], depth;

  /**
   * @brief Flags and masks for marking currently used render target slots.
   */
  enum
  {
    COLOR0 = 1 << 0,
    COLOR1 = COLOR0 << 1,
    COLOR2 = COLOR1 << 1,
    COLOR3 = COLOR2 << 1,
    COLOR4 = COLOR3 << 1,
    COLOR5 = COLOR4 << 1,
    COLOR6 = COLOR5 << 1,
    COLOR7 = COLOR6 << 1,
    COLOR_MASK = (1 << MAX_SIMRT) - 1,  /*<<Used to check whether any render target slots are used. */
    DEPTH = 1 << MAX_SIMRT,             /*<<Used to check whether depth slot is used. */
    DEPTH_READONLY = DEPTH << 1,
    TOTAL_MASK = COLOR_MASK | DEPTH,    /*<<Used to check whether any slots are used. */
  };

  /**
   * @brief Flags that demonstrate current render target and depth usage.
   */
  uint32_t used = 0;                    

  /**
   * @brief Resets render target and depth slots usage.
   */
  void reset() { used = 0; }

  /**
   * @brief Binds the texture subresource to a render target slot.
   * 
   * @param [in] index  Index of the render target slot to bind \b t to.
   * @param [in] t      A pointer to the texture to bind.
   * @param [in] lev    Mipmap level in \b t to use as render target.
   * @param [in] f_l    Defines either a cube texture face or a texture array index to use as render target.
   */
  void setColor(int index, BaseTexture *t, int lev, int f_l)
  {
    G_ASSERT(index < MAX_SIMRT);
    color[index].set(t, lev, f_l);
    used |= COLOR0 << index;
  }

  /**
   * @brief Sets the color texture of the back buffer to be the only render target.
   * 
   * All other color textures get unbound.
   */
  void setBackbufColor()
  {
    used = (used & ~COLOR_MASK);
    setColor(0, NULL, 0, 0);
  }

  /**
   * @brief Sets the depth texture of the back buffer to be the only render target.
   */
  void setBackbufDepth() { setDepth(NULL); }

  /**
   * @brief Removes the texture from the render target slot.
   * 
   * @param [in] index  Index of the render target slot from which the texture should be removed.
   */
  void removeColor(int index)
  {
    G_ASSERT(index < MAX_SIMRT);
    used &= ~(COLOR0 << index);
    color[index].set(NULL, 0, 0);
  }

  /**
   * @brief Binds the texture to the depth render target slot.
   * 
   * @param [in] t  A pointer to the texture to bind.
   * @param [in] ro Indicates whether depth texture usage should be read-only.
   * 
   * The first mipmap and the first array layer are bound. Depth texture must have no mipmaps.
   * If a particular array layer is to be bound, use a different overload.
   */
  void setDepth(BaseTexture *t, bool ro = false)
  {
    depth.set(t, 0, 0);
    used = DEPTH | (-int32_t(ro) & DEPTH_READONLY) | (used & (~DEPTH_READONLY));
  }

  /**
   * @brief Binds the texture to the depth render target slot.
   *
   * @param [in] t      A pointer to the texture to bind.
   * @param [in] layer  Texture array layer to bind.
   * @param [in] ro     Indicates whether depth texture usage should be read-only.
   *
   */
  void setDepth(BaseTexture *t, int layer, bool ro = false)
  {
    depth.set((BaseTexture *)t, 0, layer);
    used = DEPTH | (-int32_t(ro) & DEPTH_READONLY) | (used & (~DEPTH_READONLY));
  }

  /**
   * @brief Unbinds the texture from the depth render target slot.
   */
  void removeDepth()
  {
    used &= ~DEPTH;
    depth.set(NULL, 0, 0);
  }

public:

  /**
   * @brief Retrieves render target state of a color slot.
   * 
   * @param [in] index  Index of the slot to retrieve state of.
   * @return            Object storing data of the render target color slot, including the texture bound.
   */
  const RTState &getColor(int index) const { return color[index]; }

    /**
   * @brief Retrieves render target state of the depth slot.
   *
   * @return Object storing data of the render target depth slot, including the texture bound.
   */
  const RTState &getDepth() const { return depth; }

  /**
   * @brief Checks whether the current color render target is set to the back buffer.
   * 
   * @return \c true if the current color render target is set to the back buffer, \c false otherwise.
   */
  bool isBackBufferColor() const { return (used & COLOR0) && !color[0].tex; }

  /**
   * @brief Checks whether the current depth render target is set to the back buffer.
   *
   * @return \c true if the current depth render target is set to the back buffer, \c false otherwise.
   */
  bool isBackBufferDepth() const { return (used & DEPTH) && !depth.tex; }

  /**
   * @brief Checks whether a texture subresource is bound to a color render target slot.
   *
   * @param [in] index  Index of the slot to check.
   * @return            \c true if a texture is bound to the slot, \c false otherwise.
   */
  bool isColorUsed(int index) const { return used & (COLOR0 << index); }

  /**
   * @brief Checks whether a texture subresource is bound to the depth render target slot.
   *
   * @return            \c true if a texture is bound to the slot, \c false otherwise.
   */
  bool isDepthUsed() const { return used & DEPTH; }

  /**
   * @brief Checks whether a texture subresource is bound to any color render target slot.
   *
   * @return            \c true if a texture is bound, \c false otherwise.
   */
  bool isColorUsed() const { return (used & COLOR_MASK); }

  /**
   * @brief Sets depth render target texture usage to read-only.
   */
  void setDepthReadOnly() { used |= DEPTH_READONLY; }

  /**
   * @brief Sets depth render target texture usage to read&write.
   */
  void setDepthRW() { used &= ~DEPTH_READONLY; }

  /**
   * @brief Checks whether depth render target texture usage is read-only.
   * 
   * @return \c true if depth render target texture usage is read-only, \c false otherwise.
   */
  bool isDepthReadOnly() const { return (used & (DEPTH | DEPTH_READONLY)) == (DEPTH | DEPTH_READONLY); }

  /**
   * @brief Default constructor.
   */
  Driver3dRenderTarget() {} //-V730

  /**
   * @brief Compares two render target arrays.
   * 
   * @param [in] l, r   Arrays to compare.
   * @return            \c true if the arrays are identical, \c false otherwise.
   * 
   * The slots to which there is no texture bound are ignored during the comparison.
   */
  friend bool operator==(const Driver3dRenderTarget &l, const Driver3dRenderTarget &r)
  {
    if (l.used != r.used)
      return false;

    if ((l.used & Driver3dRenderTarget::DEPTH) && l.depth != r.depth)
      return false;

    for (auto i : LsbVisitor{l.used & Driver3dRenderTarget::COLOR_MASK})
      if (l.color[i] != r.color[i])
        return false;

    return true;
  }

  /**
   * @brief Compares two render target arrays.
   *
   * @param [in] l, r   Arrays to compare.
   * @return            \c true if the arrays are different, \c false otherwise.
   *
   * The slots to which there is no texture bound are ignored during the comparison.
   */
  friend bool operator!=(const Driver3dRenderTarget &l, const Driver3dRenderTarget &r) { return !(l == r); }
};

/**
 * @brief This class provides an object that caches the current render target upon creation and resets it back to the cached value upon destruction.
 * 
 * User can modify the current render target during \c ScopeRenderTarget instance lifetime. 
 * Maintaining a copy of the non-modified render target object is delegated to this instance.
 */
struct ScopeRenderTarget
{
  /**
   * @brief Stores the cached render target.
   */
  Driver3dRenderTarget prevRT;

  /**
  * @brief Initialization constructor.
  * 
  * Assigns the current render target value to \ref prevRT.
  */
  ScopeRenderTarget() { d3d_get_render_target(prevRT); }

  /**
   * @brief Destructor.
   * 
   * Resets the current render target to the cached value.
   */
  ~ScopeRenderTarget() { d3d_set_render_target(prevRT); }
};

/**
 * @brief This class provides an object that caches the current viewport upon creation and resets it back to the cached value upon destruction.
 *
 * User can modify the current viewport during \c ScopeViewport instance lifetime.
 * Maintaining a copy of the non-modified viewport object is delegated to this instance.
 */
struct ScopeViewport
{
  /**
   * @brief Cached viewport position and dimensions.
   */
  int viewX, viewY, viewW, viewH;

  /**
   * @brief Cached viewport near and far plane positions.
   */
  float viewN, viewF;

  /**
   * @brief Initialization constructor.
   *
   * Caches the current viewport value.
   */
  ScopeViewport() { d3d_get_view(viewX, viewY, viewW, viewH, viewN, viewF); }

    /**
   * @brief Destructor.
   *
   * Resets the current viewport to the cached value.
   */
  ~ScopeViewport() { d3d_set_view(viewX, viewY, viewW, viewH, viewN, viewF); }
};

/**
 * @brief Encapsulates current view and projection matrices as well as perspective data.
 */
struct ViewProjMatrixContainer
{

  /**
   * @brief Returns the stored view matrix.
   * 
   * @return View matrix.
   */
  const TMatrix &getViewTm() const { return savedView; }

  /**
   * @brief Returns the stored projection matrix.
   *
   * @return Projection matrix.
   */
  const TMatrix4 &getProjTm() const { return savedProj; }


  /**
   * @brief View matrix.
   */
  TMatrix savedView = TMatrix::IDENT;

  /**
   * @brief Projection matrix.
   */
  TMatrix4 savedProj = TMatrix4::IDENT;

  /**
   * @brief Perspective data.
   */
  Driver3dPerspective p;

  /**
   * @brief Used to store \ref getpersp call result.
   */
  bool p_ok = false;
};

/**
 * @brief This class provides an object that caches the current view and projection matrices as well as perspective upon creation and resets them back to the cached value upon destruction.
 *
 * User can modify the current view and projection matrices as well as perspective during \c ScopeViewProjMatrix instance lifetime.
 * Maintaining a copy of the non-modified values object is delegated to this instance.
 */
struct ScopeViewProjMatrix : public ViewProjMatrixContainer
{
  /**
   * @brief Initialization constructor.
   * 
   * Caches view and projection matrices as well as perspective data.
   */
  ScopeViewProjMatrix() { d3d_get_view_proj(*this); }

  /**
   * @brief Destructor.
   * 
   * Resets the current view and projection matrices as well as perspective data to the cached values.
   */
  ~ScopeViewProjMatrix() { d3d_set_view_proj(*this); }
};

/**
 * @brief Encapsulates arguments for \ref draw_indirect call.
 * 
 * The structure can be used as a base for \c args buffer for \ref draw_indirect call. 
 */
struct DrawIndirectArgs
{

  /**
   * @brief Number of vertices to draw, per instance.
   */
  uint32_t vertexCountPerInstance;

  /**
   * @brief Number of instances.
   */
  uint32_t instanceCount;

  /**
   * @brief Index of the first vertex to start drawing from
   */
  uint32_t startVertexLocation;

  /**
   * @brief Index of the first instance to start drawing from.
   */
  uint32_t startInstanceLocation;
};

/**
 * @brief Encapsulates arguments for \ref draw_indexed_indirect call.
 *
 * The structure can be used as a base for \c args buffer for \ref draw_indexed_indirect call.
 */
struct DrawIndexedIndirectArgs
{

  /**
   * @brief Number of indices read from the index buffer for each instance.
   */
  uint32_t indexCountPerInstance;

  /**
   * @brief Number of instances to draw.
   */
  uint32_t instanceCount;

  /**
   * @brief Index of the first index read by the GPU from the index buffer.
   */
  uint32_t startIndexLocation;

  /**
   * @brief Value added to each index before reading a vertex from the vertex buffer.
   */
  int32_t baseVertexLocation;

  /**
   * @brief Value added to each index before reading per-instance data from a vertex buffer.
   */
  uint32_t startInstanceLocation;
};

/**
 * @brief Determines the number of values stored in argument buffer for \ref draw_indirect call.
 */
#define DRAW_INDIRECT_NUM_ARGS            4

/**
 * @brief Determines the number of values stored in argument buffer for \ref draw_indexed_indirect call.
 */
#define DRAW_INDEXED_INDIRECT_NUM_ARGS    5

/**
 * @brief Determines the number of values stored in argument buffer for \ref dispatch_indirect call.
 */
#define DISPATCH_INDIRECT_NUM_ARGS        3

/**
 * @brief Determines the size of an argument stored in argument buffer for an indirect draw/ dispatch call. 
 */
#define INDIRECT_BUFFER_ELEMENT_SIZE      sizeof(uint32_t)

/**
 * @brief Determines the size of argument buffer for \ref draw_indirect call.
 */
#define DRAW_INDIRECT_BUFFER_SIZE         (DRAW_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)

/**
 * @brief Determines the size of argument buffer for \ref draw_indexed_indirect call.
 */
#define DRAW_INDEXED_INDIRECT_BUFFER_SIZE (DRAW_INDEXED_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)

/**
 * @brief Determines the size of argument buffer for \ref dispatch_indirect call.
 */
#define DISPATCH_INDIRECT_BUFFER_SIZE     (DISPATCH_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)


/**
 * @brief Determines the number of vertex input streams.
 */
#define INPUT_VERTEX_STREAM_COUNT 4

/**
 * @brief Encapsulates various data to use on different shader stages.
 * 
 * (unused)
 */
struct ShaderWarmUpInfo
{
  /**
   * @brief A handle to the shader program to use.
   */
  int32_t shaderProgram;

  /**
   * @brief Determines formats for each used color render target slot.
   */
  uint32_t colorFormats[Driver3dRenderTarget::MAX_SIMRT];

  /**
   * @brief Determines current depth stencil format.
   */
  uint32_t depthStencilFormat;

  /**
   * @brief Indicates which color render target slots are used and whether depth stencil is used (the 31st bit).
   */
  uint32_t validColorTargetAndDepthStencilMask;

  /**
   * @brief Determines which color channels (R, G, B, A) are allowed to be written to for each render target.
   * 
   * To access the mask for the i-th render target, use <c> colorWriteMask & (15u << i) </c>. ???
   */
  uint32_t colorWriteMask;

  /**
   * @brief Determines byte strides for each vertex input stream.
   */
  uint32_t vertexInputStreamStrides[INPUT_VERTEX_STREAM_COUNT];

  /**
   * @brief Determines which primitive topology is to be used.
   * 
   * The value is equal to <c>1u << PRIMITIVE_TOPOLOGY<\c>. Backend may be able to combine loads with different topology when more than one bit is set
   */
  uint8_t primitiveTopologyMask;

  /**
   * @brief Determines which cull mode is to be used.
   */
  uint8_t cullMode;

  /**
   * @brief Determines which comparison function is to be used when depth testing.
   */
  uint8_t depthTestFunction;

  /**
   * @brief Determines which comparison function is to be used when stencil testing.
   */
  uint8_t stencilTestFunction;

  /**
   * @brief Determines which operation is to be performed when stencil testing fails.
   */
  uint8_t stencilOnStencilFail;

  /**
   * @brief Determines which operation is to be performed when stencil testing passes, but depth testing fails.
   */
  uint8_t stencilOnDepthFail;

  /**
   * @brief Determines which operation is to be performed when both stencil and depth testing pass.
   */
  uint8_t stencilOnAllPass;

  /**
   * @brief Determines RGB blending operator.
   */
  uint8_t blendOpRGB;

  /**
   * @brief Determines alpha blending operator.
   */
  uint8_t blendOpAlpha;

  /**
   * @brief Determines source blend factor for RGB blending.
   */
  uint8_t blendSrcFactorRGB;

  /**
   * @brief Determines source blend factor for alpha blending.
   */
  uint8_t blendSrcFactorAlpha;

  /**
   * @brief Determines destination blend factor for RGB blending.
   */
  uint8_t blendDstFactorRGB;

  /**
   * @brief Determines destination blend factor for alpha blending.
   */
  uint8_t blendDstFactorAlpha;

  /**
   * @brief ???
   */
  uint32_t polyLineEnable : 1;

  /**
   * @brief ???
   */
  uint32_t flipCullEnable : 1;

  /**
   * @brief Determines whether depth testingis enabled.
   */
  uint32_t depthTestEnable : 1;

  /**
   * @brief Determines whether writing to the depth buffer is enabled.
   */
  uint32_t depthWriteEnable : 1;

  /**
   * @brief Determines whether depth bounds testing is enabled.
   */
  uint32_t depthBoundsEnable : 1;

  /**
   * @brief Determines whether clipping based on distance is enabled.
   */
  uint32_t depthClipEnable : 1;

  /**
   * @brief Determines whether stencil testing is enabled.
   */
  uint32_t stencilTestEnable : 1;

  /**
   * @brief Determines whether blending is enabled.
   */
  uint32_t blendEnable : 1;

  /**
   * @brief Determines whether blending is independent for each render target.
   */
  uint32_t separateBlendEnable : 1;
};

/**
 * @brief Determines the GPU vendor.
 */
enum
{
  D3D_VENDOR_NONE,
  D3D_VENDOR_MESA,
  D3D_VENDOR_IMGTEC,
  D3D_VENDOR_AMD,
  D3D_VENDOR_NVIDIA,
  D3D_VENDOR_INTEL,
  D3D_VENDOR_APPLE,
  D3D_VENDOR_SHIM_DRIVER,
  D3D_VENDOR_ARM,
  D3D_VENDOR_QUALCOMM,
  D3D_VENDOR_SAMSUNG,
  D3D_VENDOR_COUNT,
  D3D_VENDOR_ATI = D3D_VENDOR_AMD,
};

/**
 * @brief Evaluates GPU vendor code by id or description.
 * 
 * @param [in] vendor_id    Hardware vendor ID. For example, it can be obtained from \c DXGI_ADAPTER_DESC::VendorId.
 * @param [in] description  Hardware string name.
 * @return                  A value from \c  D3D_VENDOR_ enumeration indicating the GPU vendor code.
 * 
 * Firstly, vendor code is evaluated by \b vendor_id. On failure, vendor is evaluated by \b description.
 */
int d3d_get_vendor(uint32_t vendor_id, const char *description = nullptr);

/**
 * @brief Translates vendor code into vendor name.
 * 
 * @param [in] vendor   Vendor code to translate.
 * @return              Vendor name (static string).
 */
const char *d3d_get_vendor_name(int vendor);

struct DlssParams
{
  BaseTexture *inColor;
  BaseTexture *inDepth;
  BaseTexture *inMotionVectors;
  BaseTexture *inExposure;
  float inSharpness;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inMVScaleX;
  float inMVScaleY;
  int inColorDepthOffsetX;
  int inColorDepthOffsetY;

  BaseTexture *outColor;
};

struct XessParams
{
  BaseTexture *inColor;
  BaseTexture *inDepth;
  BaseTexture *inMotionVectors;
  float inJitterOffsetX;
  float inJitterOffsetY;
  float inInputWidth;
  float inInputHeight;
  int inColorDepthOffsetX;
  int inColorDepthOffsetY;

  BaseTexture *outColor;
};

struct Fsr2Params
{
  BaseTexture *color;
  BaseTexture *depth;
  BaseTexture *motionVectors;
  BaseTexture *output;
  float frameTimeDelta;
  float sharpness;
  float jitterOffsetX;
  float jitterOffsetY;
  float motionVectorScaleX;
  float motionVectorScaleY;
  float cameraNear;
  float cameraFar;
  float cameraFovAngleVertical;
  uint32_t renderSizeX;
  uint32_t renderSizeY;
};

enum class MtlfxColorMode
{
  PERCEPTUAL = 0,
  LINEAR,
  HDR
};
struct MtlFxUpscaleParams
{
  BaseTexture *color;
  BaseTexture *output;
  MtlfxColorMode colorMode;
};

//@cond
#define STATE_GUARD_CAT1(a, b) a##b
#define STATE_GUARD_CAT2(a, b) STATE_GUARD_CAT1(a, b)

#define STATE_GUARD_IMPL(ClassName, obj_name, func_call, val, default_value)                                              \
  struct ClassName                                                                                                        \
  {                                                                                                                       \
    typedef decltype(val) val_type;                                                                                       \
    typedef decltype(func_call) func_type;                                                                                \
    const val_type def_value;                                                                                             \
    const func_type &func;                                                                                                \
    ClassName(const val_type VALUE, const val_type def_value_, const func_type &func) : def_value(def_value_), func(func) \
    {                                                                                                                     \
      func(VALUE);                                                                                                        \
    }                                                                                                                     \
    ~ClassName()                                                                                                          \
    {                                                                                                                     \
      func(def_value);                                                                                                    \
    }                                                                                                                     \
  } obj_name(val, default_value, func_call);
//@endcond

/**
 *
 * @brief Creates a state guard object.
 * 
 * @param [in] func_call        Function call that is invoked at object creation and destruction. 
 * @param [in] val              Value that is passed to \b func_call at object creation.
 * @param [in] default_value    Value that is passed to \b func_call at object destrtuction.
 * 
 * @note \c 'VALUE' must be typed (without quotes) in place of the parameter for which \b val and 
 * \b default_value are passed. 
 * 
 * A state guard is a scoped class object, that invokes the specified 
 * function calls at its construction and destruction.
 */
#define STATE_GUARD(func_call, val, default_value)                                                                                \
  auto STATE_GUARD_CAT2(lambda, __LINE__) = [&](const decltype(val) VALUE) { func_call; };                                        \
  STATE_GUARD_IMPL(STATE_GUARD_CAT2(StateGuard, __LINE__), STATE_GUARD_CAT2(guard, __LINE__), STATE_GUARD_CAT2(lambda, __LINE__), \
    val, default_value)

/**
 * @brief Creates a state guard which uses \c NULL for function call at destruction.
 * 
 * @param [in] func_call        Function call that is invoked at object creation and destruction.
 * @param [in] val              Value that is passed to \b func_call at object creation.
 * 
 * @note \c 'VALUE' must be typed (without quotes) in place of the parameter for which \b val is passed. 
 * 
 * For more information see \ref STATE_GUARD.
 */
#define STATE_GUARD_NULLPTR(func_call, val) STATE_GUARD(func_call, val, nullptr)

/**
 * @brief Creates a state guard which uses 0 for function call at destruction.
 *
 * @param [in] func_call        Function call that is invoked at object creation and destruction.
 * @param [in] val              Value that is passed to \b func_call at object creation.
 *
 * @note \c 'VALUE' must be typed (without quotes) in place of the parameter for which \b val is passed. 
 * 
 * For more information see \ref STATE_GUARD.
 */
#define STATE_GUARD_0(func_call, val)       STATE_GUARD(func_call, val, 0)
