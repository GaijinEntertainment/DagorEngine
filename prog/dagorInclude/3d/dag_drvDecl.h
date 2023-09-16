//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

class BaseTexture;
struct Driver3dRenderTarget;
struct ViewProjMatrixContainer;
class String;

extern void d3d_get_render_target(Driver3dRenderTarget &rt);
extern void d3d_set_render_target(Driver3dRenderTarget &rt);

extern void d3d_get_view_proj(ViewProjMatrixContainer &vp);
extern void d3d_set_view_proj(const ViewProjMatrixContainer &vp);

extern void d3d_get_view(int &viewX, int &viewY, int &viewW, int &viewH, float &viewN, float &viewF);
extern void d3d_set_view(int viewX, int viewY, int viewW, int viewH, float viewN, float viewF);

#define __CONCAT_HELPER__(x, y) x##y
#define __CONCAT__(x, y)        __CONCAT_HELPER__(x, y)

#define SCOPE_VIEW_PROJ_MATRIX ScopeViewProjMatrix __CONCAT__(scopevpm, __LINE__)
#define SCOPE_VIEWPORT         ScopeViewport __CONCAT__(scopevp, __LINE__)
#define SCOPE_RENDER_TARGET \
  SCOPE_VIEWPORT;           \
  ScopeRenderTarget __CONCAT__(scopert, __LINE__)

//--- Data structures -------
struct ScissorRect
{
  int x, y, w, h;
};

struct Viewport : public ScissorRect
{
  float minz, maxz;

  static constexpr uint32_t MAX_VIEWPORT_COUNT = 16;
};

struct Driver3dPerspective
{
  Driver3dPerspective() : wk(0.f), hk(0.f), zn(0.f), zf(0.f), ox(0.f), oy(0.f) {}

  Driver3dPerspective(float wk_, float hk_, float zn_, float zf_, float ox_ = 0.f, float oy_ = 0.f) :
    wk(wk_), hk(hk_), zn(zn_), zf(zf_), ox(ox_), oy(oy_)
  {}

  float wk, hk;
  float zn, zf;
  float ox, oy;
};

struct Driver3dRenderTarget
{
  struct RTState
  {
    BaseTexture *tex;
    unsigned char level;
    union
    {
      unsigned short face;
      unsigned short layer;
    };
    void set(BaseTexture *t, int lev, int f_l)
    {
      tex = t;
      level = lev;
      face = f_l;
    }

    friend bool operator==(const RTState &l, const RTState &r)
    {
      return (l.tex == r.tex) && (l.level == r.level) && (l.face == r.face);
    }
    friend bool operator!=(const RTState &l, const RTState &r) { return !(l == r); }
  };
  static constexpr int MAX_SIMRT = 8;
#ifndef INSIDE_DRIVER
protected:
#endif
  RTState color[MAX_SIMRT], depth;
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
    COLOR_MASK = (1 << MAX_SIMRT) - 1,
    DEPTH = 1 << MAX_SIMRT,
    DEPTH_READONLY = DEPTH << 1,
    TOTAL_MASK = COLOR_MASK | DEPTH,
  };
  uint32_t used = 0;
  void reset() { used = 0; }
  void setColor(int index, BaseTexture *t, int lev, int f_l)
  {
    G_ASSERT(index < MAX_SIMRT);
    color[index].set(t, lev, f_l);
    used |= COLOR0 << index;
  }
  void setBackbufColor()
  {
    used = (used & ~COLOR_MASK);
    setColor(0, NULL, 0, 0);
  }
  void setBackbufDepth() { setDepth(NULL); }
  void removeColor(int index)
  {
    G_ASSERT(index < MAX_SIMRT);
    used &= ~(COLOR0 << index);
    color[index].set(NULL, 0, 0);
  }
  void setDepth(BaseTexture *t, bool ro = false)
  {
    depth.set(t, 0, 0);
    used = DEPTH | (-int32_t(ro) & DEPTH_READONLY) | (used & (~DEPTH_READONLY));
  }
  void setDepth(BaseTexture *t, int layer, bool ro = false)
  {
    depth.set((BaseTexture *)t, 0, layer);
    used = DEPTH | (-int32_t(ro) & DEPTH_READONLY) | (used & (~DEPTH_READONLY));
  }
  void removeDepth()
  {
    used &= ~DEPTH;
    depth.set(NULL, 0, 0);
  }

public:
  const RTState &getColor(int index) const { return color[index]; }
  const RTState &getDepth() const { return depth; }
  bool isBackBufferColor() const { return (used & COLOR0) && !color[0].tex; }
  bool isBackBufferDepth() const { return (used & DEPTH) && !depth.tex; }
  bool isColorUsed(int index) const { return used & (COLOR0 << index); }
  bool isDepthUsed() const { return used & DEPTH; }
  bool isColorUsed() const { return (used & COLOR_MASK); }
  void setDepthReadOnly() { used |= DEPTH_READONLY; }
  void setDepthRW() { used &= ~DEPTH_READONLY; }
  bool isDepthReadOnly() const { return (used & (DEPTH | DEPTH_READONLY)) == (DEPTH | DEPTH_READONLY); }
  Driver3dRenderTarget() {} //-V730

  friend bool operator==(const Driver3dRenderTarget &l, const Driver3dRenderTarget &r)
  {
    if (l.used != r.used)
      return false;

    if ((l.used & Driver3dRenderTarget::DEPTH) && l.depth != r.depth)
      return false;

    for (uint32_t mask = l.used & Driver3dRenderTarget::COLOR_MASK, i = 0; mask && i < Driver3dRenderTarget::MAX_SIMRT;
         ++i, mask >>= 1)
      if ((mask & 1) && (l.color[i] != r.color[i]))
        return false;

    return true;
  }

  friend bool operator!=(const Driver3dRenderTarget &l, const Driver3dRenderTarget &r) { return !(l == r); }
};

struct ScopeRenderTarget
{
  Driver3dRenderTarget prevRT;
  ScopeRenderTarget() { d3d_get_render_target(prevRT); }
  ~ScopeRenderTarget() { d3d_set_render_target(prevRT); }
};

struct ScopeViewport
{
  int viewX, viewY, viewW, viewH;
  float viewN, viewF;
  ScopeViewport() { d3d_get_view(viewX, viewY, viewW, viewH, viewN, viewF); }
  ~ScopeViewport() { d3d_set_view(viewX, viewY, viewW, viewH, viewN, viewF); }
};

struct ViewProjMatrixContainer
{
  const TMatrix &getViewTm() const { return savedView; }
  const TMatrix4 &getProjTm() const { return savedProj; }

  TMatrix savedView = TMatrix::IDENT;
  TMatrix4 savedProj = TMatrix4::IDENT;
  Driver3dPerspective p;
  bool p_ok = false;
};

struct ScopeViewProjMatrix : public ViewProjMatrixContainer
{
  ScopeViewProjMatrix() { d3d_get_view_proj(*this); }
  ~ScopeViewProjMatrix() { d3d_set_view_proj(*this); }
};

struct DrawIndirectArgs
{
  uint32_t vertexCountPerInstance;
  uint32_t instanceCount;
  uint32_t startVertexLocation;
  uint32_t startInstanceLocation;
};

struct DrawIndexedIndirectArgs
{
  uint32_t indexCountPerInstance;
  uint32_t instanceCount;
  uint32_t startIndexLocation;
  int32_t baseVertexLocation;
  uint32_t startInstanceLocation;
};

#define DRAW_INDIRECT_NUM_ARGS            4
#define DRAW_INDEXED_INDIRECT_NUM_ARGS    5
#define DISPATCH_INDIRECT_NUM_ARGS        3
#define INDIRECT_BUFFER_ELEMENT_SIZE      sizeof(uint32_t)
#define DRAW_INDIRECT_BUFFER_SIZE         (DRAW_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)
#define DRAW_INDEXED_INDIRECT_BUFFER_SIZE (DRAW_INDEXED_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)
#define DISPATCH_INDIRECT_BUFFER_SIZE     (DISPATCH_INDIRECT_NUM_ARGS * INDIRECT_BUFFER_ELEMENT_SIZE)

#define INPUT_VERTEX_STREAM_COUNT 4

struct ShaderWarmUpInfo
{
  int32_t shaderProgram;
  uint32_t colorFormats[Driver3dRenderTarget::MAX_SIMRT];
  uint32_t depthStencilFormat;
  // bit 31 indicates depth stencil use (1) or not (0)
  uint32_t validColorTargetAndDepthStencilMask;
  uint32_t colorWriteMask;

  uint32_t vertexInputStreamStrides[INPUT_VERTEX_STREAM_COUNT];

  // Mask of 1u << <TOPOLOGY>
  // Backend may be able to combine loads with different topology when more than one bit is set
  uint8_t primitiveTopologyMask;
  uint8_t cullMode;
  uint8_t depthTestFunction;
  uint8_t stencilTestFunction;
  uint8_t stencilOnStencilFail;
  uint8_t stencilOnDepthFail;
  uint8_t stencilOnAllPass;
  uint8_t blendOpRGB;
  uint8_t blendOpAlpha;
  uint8_t blendSrcFactorRGB;
  uint8_t blendSrcFactorAlpha;
  uint8_t blendDstFactorRGB;
  uint8_t blendDstFactorAlpha;

  uint32_t polyLineEnable : 1;
  uint32_t flipCullEnable : 1;
  uint32_t depthTestEnable : 1;
  uint32_t depthWriteEnable : 1;
  uint32_t depthBoundsEnable : 1;
  uint32_t depthClipEnable : 1;
  uint32_t stencilTestEnable : 1;
  uint32_t blendEnable : 1;
  uint32_t separateBlendEnable : 1;
};

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

int d3d_get_vendor(uint32_t vendor_id, const char *description = nullptr);
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

#define STATE_GUARD(func_call, val, default_value)                                                                                \
  auto STATE_GUARD_CAT2(lambda, __LINE__) = [&](const decltype(val) VALUE) { func_call; };                                        \
  STATE_GUARD_IMPL(STATE_GUARD_CAT2(StateGuard, __LINE__), STATE_GUARD_CAT2(guard, __LINE__), STATE_GUARD_CAT2(lambda, __LINE__), \
    val, default_value)

#define STATE_GUARD_NULLPTR(func_call, val) STATE_GUARD(func_call, val, nullptr)
#define STATE_GUARD_0(func_call, val)       STATE_GUARD(func_call, val, 0)
