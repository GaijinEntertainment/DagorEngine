//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath_common.h>
#include <math/dag_bounds2.h>
#include <3d/dag_texMgr.h>
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <util/dag_safeArg.h>
#include <drv/3d/dag_driver.h>

#include <generic/dag_DObject.h>
#include <shaders/dag_shaderCommon.h>
#include <gui/dag_imgui_drawlist.h>
#include <wchar.h>

// ************************************************************************
// * forwards
// ************************************************************************
class ShaderMaterial;
class DataBlock;
class BaseTexture;
typedef BaseTexture Texture;
struct DagorFontBinDump;
class TMatrix4;


#define GUI_POS_SCALE     4.f
#define GUI_POS_SCALE_INV (1.f / GUI_POS_SCALE)

enum BlendMode : uint8_t
{
  NO_BLEND = 0,
  PREMULTIPLIED = 1,
  NONPREMULTIPLIED = 2,
  ADDITIVE = 3,
};

enum FontFxType // see gui_default
{
  FFT_NONE,
  FFT_SHADOW,
  FFT_GLOW,
  FFT_BLUR,
  FFT_OUTLINE,
};

struct StdGuiFontContext
{
  DagorFontBinDump *font;
  unsigned fontHt : 12, monoW : 12;
  int spacing : 8;

  StdGuiFontContext(DagorFontBinDump *f = NULL, int s = 0, int mw = 0, int ht = 0) : font(f), spacing(s), monoW(mw), fontHt(ht) {}
  explicit inline StdGuiFontContext(int font_id, int s = 0, int mw = 0, int ht = 0);
};

// ************************************************************************
// * standard GUI rendering
// ************************************************************************
namespace StdGuiRender
{
namespace P2
{
extern const Point2 ZERO;
extern const Point2 ONE;
extern const Point2 AXIS_X;
extern const Point2 AXIS_Y;
} // namespace P2

struct LayerParams
{
private:
  TEXTUREID texId, texId2;
  TEXTUREID fontTexId;
  TEXTUREID maskTexId;
  d3d::SamplerHandle texSampler;
  d3d::SamplerHandle tex2Sampler;
  d3d::SamplerHandle fontTexSampler;
  d3d::SamplerHandle maskTexSampler;

public:
  struct FontFx
  {
    FontFxType type;
    E3DCOLOR col;
    int factor_x32;
    int ofsX, ofsY;

    void reset()
    {
      type = FFT_NONE;
      col = 0;
      factor_x32 = 0;
      ofsX = 0;
      ofsY = 0;
    }
  };

  FontFx ff;

  BlendMode alphaBlend;
  bool texInLinear;
  TEXTUREID getTexId() const { return texId; }
  TEXTUREID getTexId2() const { return texId2; }
  TEXTUREID getFontTexId() const { return fontTexId; }
  TEXTUREID getMaskTexId() const { return maskTexId; }
  d3d::SamplerHandle getTexSampler() const { return texSampler; }
  d3d::SamplerHandle getTex2Sampler() const { return tex2Sampler; }
  d3d::SamplerHandle getFontTexSampler() const { return fontTexSampler; }
  d3d::SamplerHandle getMaskTexSampler() const { return maskTexSampler; }
  void setTexId(TEXTUREID id, d3d::SamplerHandle smp)
  {
    texId = id;
    texSampler = smp;
  }
  void setTexId2(TEXTUREID id, d3d::SamplerHandle smp)
  {
    texId2 = id;
    tex2Sampler = smp;
  }
  void setFontTexId(TEXTUREID id, d3d::SamplerHandle smp)
  {
    fontTexId = id;
    fontTexSampler = smp;
  }
  void setMaskTexId(TEXTUREID id, d3d::SamplerHandle smp)
  {
    maskTexId = id;
    maskTexSampler = smp;
  }
  Point3 maskTransform0, maskTransform1;
  float colorM[4 * 4];

  bool fontFx;
  bool colorM1;

  void reset();
  bool cmpEq(const LayerParams &other) const;
  void dump() const;
};

struct FontFxAttr
{
private:
  TEXTUREID fontTex;
  d3d::SamplerHandle fontTexSampler;

  TEXTUREID tex2;
  d3d::SamplerHandle tex2Sampler;

public:
  TEXTUREID getFontTex() const { return fontTex; }
  TEXTUREID getTex2() const { return tex2; }
  d3d::SamplerHandle getFontTexSampler() const { return fontTexSampler; }
  d3d::SamplerHandle getTex2Sampler() const { return tex2Sampler; }
  void setFontTex(TEXTUREID id, d3d::SamplerHandle smp)
  {
    fontTex = id;
    fontTexSampler = smp;
  }
  void setTex2(TEXTUREID id, d3d::SamplerHandle smp)
  {
    tex2 = id;
    tex2Sampler = smp;
  }
  int tex2su_x32, tex2sv_x32, tex2bv_ofs;
  float tex2su, tex2sv;

  uint8_t writeToZ;
  uint8_t testDepth;

  void reset();
  void dump() const;
};
} // namespace StdGuiRender

struct GuiVertexTransform
{
  float vtm[2][3];

  void resetViewTm();
  void setViewTm(Point2 ax, Point2 ay, Point2 o);
  void addViewTm(Point2 ax, Point2 ay, Point2 o);
  void setRotViewTm(Color4 &fontTex2rotCCSmS, float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add = false);
  void getViewTm(float dest_vtm[2][3], bool pure_trans = false) const;
  void setViewTm(const float m[2][3]);
  static void inverseViewTm(float dest_vtm[2][3], const float src_vtm[2][3]);

  const float *getVtmRaw(int i) const { return &vtm[i][0]; }
  float transformComponent(float x, float y, int i) const { return x * vtm[i][0] + y * vtm[i][1] + vtm[i][2]; }
};

struct GuiVertex
{
  int16_t px, py; // position in screen coords (fixed 13.2s format; check GUI_POS_SCALE)
  E3DCOLOR color; // vertex color
  union
  {
    struct
    {
      int16_t tc0u, tc0v;
    }; // tex coords0 UV (fixed 3.12s format)
    uint32_t tc0;
  };
  union
  {
    struct
    {
      int16_t tc1u, tc1v;
    }; // tex coords1 UV (fixed 3.12s format or raw 16 bit int)
    uint32_t tc1;
  };

#if _TARGET_SIMD_SSE
  static inline int fast_floori(float v) { return vec_floori(v); }
#else
  static inline int fast_floori(float v) { return (int)floorf(v); }
#endif

  void setPos(const GuiVertexTransform &xf, float x, float y)
  {
    px = (int16_t)clamp((int)fast_floori(x * xf.vtm[0][0] + y * xf.vtm[0][1] + xf.vtm[0][2]), -32768, 32767);
    py = (int16_t)clamp((int)fast_floori(x * xf.vtm[1][0] + y * xf.vtm[1][1] + xf.vtm[1][2]), -32768, 32767);
  }

  void setTc0(float u, float v)
  {
    tc0u = fast_floori(4096 * u + 0.5f);
    tc0v = fast_floori(4096 * v + 0.5f);
  }
  void setTc1(float u, float v)
  {
    tc1u = fast_floori(4096 * u + 0.5f);
    tc1v = fast_floori(4096 * v + 0.5f);
  }

  void setPos(const GuiVertexTransform &xf, Point2 p) { setPos(xf, p.x, p.y); }
  void setTc0(Point2 p) { setTc0(p.x, p.y); }
  void setTc1(Point2 p) { setTc1(p.x, p.y); }

  void zeroTc0() { tc0 = 0; }
  void zeroTc1() { tc1 = 0; }

  // legacy emulation
  static void resetViewTm();
  static void setViewTm(const float m[2][3]);
  static void setViewTm(Point2 ax, Point2 ay, Point2 o, bool add = false);
  static void setRotViewTm(float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add = 0);
  static void getViewTm(float dest_vtm[2][3], bool pure_trans = false);
};


//************************************************************************
//* gui viewport decription
//************************************************************************

class GuiViewPort
{
public:
  Point2 leftTop;
  Point2 rightBottom;
  bool applied, isNull;

  inline GuiViewPort() : leftTop(0, 0), rightBottom(0, 0), applied(false), isNull(true){};

  inline explicit GuiViewPort(real left, real top, real right, real bottom) :
    leftTop(left, top), rightBottom(right, bottom), applied(false)
  {
    normalize();
    isNull = isZero();
  }

  inline GuiViewPort(const GuiViewPort &other) { operator=(other); }

  // get viewport size
  inline const Point2 getSize() const { return Point2(getWidth(), getHeight()); };
  inline real getWidth() const { return rightBottom.x - leftTop.x; };
  inline real getHeight() const { return rightBottom.y - leftTop.y; };
  inline bool isZero() const { return isNull; }

  // make left-top <= right-bottom
  void normalize();

  // dump contens to debug
  void dump(const char *msg = NULL) const;

  // assigment
  inline GuiViewPort &operator=(const GuiViewPort &other)
  {
    leftTop = other.leftTop;
    rightBottom = other.rightBottom;
    applied = other.applied;
    isNull = other.isNull;
    return *this;
  }

  // check for equality
  inline bool operator==(const GuiViewPort &other) const { return (leftTop == other.leftTop) && (rightBottom == other.rightBottom); }

  inline bool operator!=(const GuiViewPort &other) const { return !operator==(other); };

  // check point visiblility (viewport left-top must be <= right-bottom)
  inline bool isVisible(Point2 p) const { return isVisible(p.x, p.y); };

  inline bool isVisible(real x, real y) const
  {
    return (x >= leftTop.x) && (y >= leftTop.y) && (x <= rightBottom.x) && (y <= rightBottom.y);
  }

  // check rectangle visiblility (all values must be normalized (left <= right, top <= bottom))
  inline bool isVisible(Point2 lt, Point2 rb) const
  {
    if (lt.x == rb.x || lt.y == rb.y) // zero area
      return false;
    return isVisible(lt.x, lt.y, rb.x, rb.y);
  };
  inline bool isVisible(real left, real top, real right, real bottom) const
  {
    bool intersects = (right >= leftTop.x) && (bottom >= leftTop.y) && (left <= rightBottom.x) && (top <= rightBottom.y);
    if (!intersects)
      return false;
    //  check inside or overlap with viewport?
    //   if (!applied && ((left < leftTop.x) || (top < leftTop.y) || (right > rightBottom.x) || (bottom > rightBottom.y)))
    //    lazy apply_viewport
    return true;
  }

  // check visiblility for points (viewport left-top must be <= right-bottom)
  bool isVisible(Point2 p0, Point2 p1, Point2 p2, Point2 p3) const;

  // return true, if point <= right-bottom
  inline bool isVisibleRBCheck(Point2 p) const { return (p.x < rightBottom.x) && (p.y < rightBottom.y); }

  // clip other viewport by current viewport
  void clipView(GuiViewPort &view_to_clip) const;

  // checks viewport limits and updates 'isNull' flag
  void updateNull()
  {
    if (leftTop.x >= rightBottom.x || leftTop.y >= rightBottom.y)
    {
      leftTop = rightBottom = StdGuiRender::P2::ZERO;
      isNull = true;
    }
    else
      isNull = false;
  }
}; // class GuiViewPort

namespace StdGuiRender
{
enum // flags to be used with draw_str_scaled_buf/draw_str_scaled_u_buf/render_str_buf (Draw Str to Buf API)
{
  DSBFLAG_abs = 0x0001,      // buffer/render quads with abs positions
  DSBFLAG_checkVis = 0x0002, // check quad visibility during buffer/render
  DSBFLAG_curColor = 0x0004, // apply current color during render

  DSBFLAG_rel = 0,      // !DSBFLAG_abs
  DSBFLAG_allQuads = 0, // !DSBFLAG_checkVis

  DSBFLAG_drawDefault = DSBFLAG_abs | DSBFLAG_checkVis,
  DSBFLAG_renderDefault = DSBFLAG_abs | DSBFLAG_allQuads,
};
extern short dyn_font_atlas_reset_generation; //< public access expected to be read-only

struct GuiState
{
  LayerParams params;
  FontFxAttr fontAttr;
  void reset()
  {
    params.reset();
    fontAttr.reset();
  }
};

struct ExtState // aliased to specific structure
{
  uint8_t data[72];
};

struct CallBackState
{
  GuiState *guiState;
  Point2 pos; // in gui coord system
  Point2 size;
  uintptr_t data;
  int command;
};

// called from rendering loop
// viewport is already set
// must restore renderstates
typedef void (*RenderCallBack)(CallBackState &state);

// typedef uint16_t IndexType;
typedef int IndexType;

using DeviceResetFilter = bool (*)();

// implement custom shader processing
struct GuiShader
{
  Ptr<ShaderMaterial> material;
  Ptr<ShaderElement> element;

  bool inited;
  bool combinedColTexFontShader;

  GuiShader();

  // init with shader name
  bool init(const char *name, bool do_fatal = true);
  // call for close (there is no destructor)
  void close();

  // return shader channels description
  virtual void channels(const CompiledShaderChannelId *&channels, int &num_chans) = 0;
  // add shader variables (material & elem are valid)
  virtual void link() = 0;
  // remove textures from driver
  virtual void cleanup() = 0;
  // set all shader specific states (viewport is already set)
  virtual void setStates(const float viewport[4], // LTWH
    const GuiState &guiState,                     // layers, basic tex and font
    const ExtState *extState, bool viewport_changed, bool guistate_changed, bool extstate_changed, int targetW, int targetH) = 0;
};

struct StdGuiShader : GuiShader
{
  // aliased to ExtState
  struct ExtStateLocal
  {
    Color4 fontTex2ofs;
    Color4 fontTex2rotCCSmS;
    Color4 user[2];
    uint8_t padding[8];

    void setFontTex2Ofs(GuiVertexTransform &xform, float su, float sv, float x0, float y0);
    //      void setFontFx(FontFxType type, E3DCOLOR col, int factor_x32);
    //      void setFontFxTex(int ofs_x, int ofs_y, int tex_w, int tex_h);
    //      void setFontTex2(TEXTUREID tex_id);

    void reset()
    {
      fontTex2ofs = Color4(0, 0, 0, 0);
      user[1] = user[0] = Color4(0, 0, 0, 0);
      resetRotation();
    }

    void resetRotation() { fontTex2rotCCSmS.set(1, 1, 0, 0); }
  };
  static_assert(sizeof(ExtState) == sizeof(ExtStateLocal), "ExtState size mismatch");

  virtual void channels(const CompiledShaderChannelId *&channels, int &num_chans);
  virtual void link();
  virtual void cleanup();

  virtual void setStates(const float viewport[4], const GuiState &guiState, const ExtState *extState, bool viewport_changed,
    bool guistate_changed, bool extstate_changed, int targetW, int targetH) override;
  TEXTUREID lastFontFxTexture = BAD_TEXTUREID;
  uint16_t lastFontFxTW = 0, lastFontFxTH = 0;
};

struct BufferedRenderer;
struct RecorderCallback;
class GuiContext
{
  BufferedRenderer *renderer;
  RecorderCallback *recCb;
  dag_imgui_drawlist::ImDrawList imDrawList;

public:
  // directly accessible data:

  // lifetime: always
  // lifetime: between start/end_render
  int screenWidth;
  int screenHeight;

  // virtual screen coordinate system with square pixels
  int viewX, viewY, viewW, viewH;
  float viewN, viewF;

  Point2 screenSize;     // size of the logical full screen
  Point2 screenScale;    // scale coefficient from real to logical screen
  Point2 screenScaleRcp; // scale coefficient from logical to real screen

  bool isInRender;

  int currentBufferId;
  int currentChunk; // current chunk for stdgui emulation
  GuiShader *currentShader;

  // current state for stdgui text
  Point2 currentPos; // current text output position
  E3DCOLOR currentColor;
  uint32_t curQuadMask; // = 0x00010001;
  bool currentTextureAlpha;
  bool currentTextureFont;

  bool isCurrentFontTexturePow2;
  TEXTUREID prevTextTextureId;
  float halfFontTexelX;
  float halfFontTexelY;
  int currentFontId;
  StdGuiFontContext curRenderFont;

  GuiViewPort currentViewPort; // cuurent view port in logical coordinates
  GuiViewPort deviceViewPort;  // device view port in real coordinates

  // next state (set rollState after modification)
  GuiState guiState;

  ExtState extStateStorage;

  StdGuiShader::ExtStateLocal &extState() { return *(StdGuiShader::ExtStateLocal *)&extStateStorage; }

  enum
  {
    ROLL_GUI_STATE = 1,
    ROLL_EXT_STATE = 2,
    ROLL_ALL_STATES = (ROLL_GUI_STATE | ROLL_EXT_STATE)
  };

private:
  uint8_t rollState;

  Tab<GuiViewPort> viewportList; //(midmem_ptr()); // viewports in logical coords

  bool drawRawLayer;

  // mini quad cache
  // useful for pushing the sequence of individual quads with similar settings
  // in case of raw layer automatically calls drawQuads
  uint8_t qCacheUsed; // current used quads
  uint8_t qCacheStride;
  uint8_t qCacheTotal;
  int qCachePushed; // total pushed quads between draw
  uint8_t DECLSPEC_ALIGN(16) qCacheBuffer[1024] ATTRIBUTE_ALIGN(16);

  GuiVertexTransform vertexTransform;
  GuiVertexTransform vertexTransformInverse;
  BBox2 preTransformViewport;

public:
  GuiContext();
  GuiContext(GuiContext &&) = default;
  ~GuiContext();

  void close(); // call if statically allocated
  void onReset();

  // buffer management

  // create combined v/i buffer in specified slot
  //  vertex_buf_size = quads * 4
  //  index_buf_size  = quads * N + extra_indices
  //  N can be 0, or 6 depending on d3d target
  //  extra indices are for objects like polys and fans
  //  shader is default shader for this buffer
  bool createBuffer(int id, GuiShader *shader, int num_quads, int extra_indices, const char *name);

  void setExtStateLocation(int buffer_id, ExtState *state);

  // set external rendering command handler
  // CallBackState is provided to cb() function
  void setRenderCallback(RenderCallBack cb, uintptr_t data);
  RecorderCallback *getRenderCallback() const { return recCb; }

  // execute cb handler
  // must restore all changed states
  // can't use this gui renderer
  void execCommand(int command);
  void execCommand(int command, Point2 pos, Point2 size);
  void execCommand(int command, Point2 pos, Point2 size, RenderCallBack cb, uintptr_t data);
  uintptr_t execCommandImmediate(int command, Point2 pos, Point2 size);

  // reset positions of all buffers
  void resetFrame();

  // setup render parameters.
  // set screen logical resolution in pixels.
  // if resolution is zero, use current device resolution.
  // call it before beginChunk()
  void setTarget(int screen_width, int screen_height, int left = 0, int top = 0);
  // setup render parameters. call it before any GUI rendering.
  // set screen logical resolution to current device resolution.
  void setTarget();
  // reset render params to defaults, lock buffers. call it before any GUI rendering.
  // record draw commands and parameters in buffer
  // return chunk_id 0-based
  int beginChunk();
  // unlock buffers, restore parameters. call it after all GUI rendering
  void endChunk();
  // render specified chunk
  void renderChunk(int chunk_id);
  // return true, if start_render already has been called
  bool isRenderStarted() const;

  // fast beg/end w/o modifying states (for hudprims)
  void beginChunkImm();
  // flush and render
  void endChunkImm();

  // override shader for current buffer
  // new shader shares variables with buffer-attached shader
  // only single override per chunk
  // material is optional (NULL if shader uses only globals)
  // elem must be valid or NULL to disable override
  void overrideShaderElem(ShaderMaterial *mat, ShaderElement *elem);

  // flush all quad caches
  void flushData() { qCacheFlush(true); }

  void updateTarget();
  void updateTarget(int width, int height);
  void updateView(int x, int y, int w, int h); // updateTarget sets full view

  // set buffer (as target) and current shader for the buffer
  void setBuffer(int buffer, bool force = false);
  // set current shader for specified buffer. NULL is default
  void setShaderToBuffer(int buffer, GuiShader *shader);
  // set shader for current buffer
  void setShader(GuiShader *shader)
  {
    if (currentBufferId >= 0)
      setShaderToBuffer(currentBufferId, shader);
  }

  GuiShader *getShader() { return currentShader; }

  // direct write to write_to_z, test_depth intervals
  void setZMode(bool write_to_z, int test_depth);

  void setRollState(int bits) { rollState |= bits; }

  // qCache control
  // update state and alloc new quad
  void *qCacheAlloc(int num);
  // flush cached quads (w/ old states) then update state
  // for raw layer drawQuads() is called
  void qCacheFlush(bool force);
  // num_quads possible to allocate from cache
  int qCacheCapacity() const { return qCacheTotal; }

  // type-checking version of qCacheAlloc
  template <typename T>
  T *qCacheAllocT(int num)
  {
    G_ASSERTF(qCacheStride == sizeof(T) || recCb, "qCacheStride=%d sizeof(T)=%d recCb=%p", qCacheStride, sizeof(T), recCb);
    return (T *)qCacheAlloc(num);
  }
  void qCacheReturnUnused(int unum)
  {
    G_ASSERTF(qCacheUsed >= unum, "qCacheUsed=%d unum=%d", qCacheUsed, unum);
    qCacheUsed -= unum;
  }

  // ************************************************************************
  // * viewports
  // ************************************************************************
  // set viewport (add it to stack)
  void set_viewport(const GuiViewPort &rt);
  void set_viewport(real left, real top, real right, real bottom) { set_viewport(GuiViewPort(left, top, right, bottom)); }
  // reset to previous viewport (remove it from stack). return false, if no items to remove
  bool restore_viewport();

  // precalculated culling bbox (for tuning)
  BBox2 getPreTransformViewport() { return preTransformViewport; }
  void setPreTransformViewport(BBox2 value) { preTransformViewport = value; }

  // get current viewport
  const GuiViewPort &get_viewport();
  void applyViewport(GuiViewPort &vp);

  inline bool vpBoxIsVisible(Point2 p) const { return preTransformViewport & p; };

  inline bool vpBoxIsVisible(real x, real y) const { return vpBoxIsVisible(Point2(x, y)); }

  // check rectangle visiblility (all values must be normalized (left <= right, top <= bottom))
  inline bool vpBoxIsVisible(Point2 lt, Point2 rb) const { return preTransformViewport & BBox2(lt, rb); };
  inline bool vpBoxIsVisible(real left, real top, real right, real bottom) const
  {
    return vpBoxIsVisible(Point2(left, top), Point2(right, bottom));
  }

  // check visiblility for points (viewport left-top must be <= right-bottom)
  bool vpBoxIsVisible(Point2 p0, Point2 p1, Point2 p2, Point2 p3) const;

  // return true, if point <= right-bottom
  inline bool vpBoxIsVisibleRBCheck(Point2 p) const
  {
    return !preTransformViewport.isempty() && p.x < preTransformViewport.right() && p.y < preTransformViewport.bottom();
  }

  // ************************************************************************
  // * low-level rendering
  // ************************************************************************

  // set opaque semantic user state
  void set_user_state(int state, const float *v, int cnt); // currently no more than 8 floats. to be changed
  template <class T>
  inline void set_user_state(int state, const T &v)
  {
    return set_user_state(state, (const float *)(const char *)&v, sizeof(T));
  }

  // set current color
  void set_color(E3DCOLOR color);
  void set_color(int r, int g, int b, int a = 255) { set_color(E3DCOLOR(r, g, b, a)); }

  // set alpha blending
  void set_alpha_blend(BlendMode blend_mode);
  inline void set_ablend(bool on) { set_alpha_blend(on ? PREMULTIPLIED : NO_BLEND); }
  BlendMode get_alpha_blend();

  // set current texture (if texture changed, flush cached data)
  void set_textures(TEXTUREID tex_id, d3d::SamplerHandle smp_id, TEXTUREID tex_id2, d3d::SamplerHandle smp_id2, bool font_l8 = false,
    bool tex_in_linear = false);
  inline void reset_textures()
  {
    set_textures(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, false, false);
  }
  inline void set_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, bool font_l8 = false, bool tex_in_linear = false)
  {
    set_textures(tex_id, smp_id, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, font_l8, tex_in_linear);
  }
  void set_mask_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, Point3 transform0, Point3 transform1);
  void start_font_str(float s);

  // to be called in d3d reset callback after device was reseted
  // void after_device_reset();

  // to be called from scene::act() to perform pending font rasterisation and other tasks
  // void update_internals_per_act();

  // get current logical resolution
  Point2 screen_size();
  real screen_width();
  real screen_height();

  // get real screen size
  real real_screen_width();
  real real_screen_height();

  // return logical screen-to-real scale coefficient
  Point2 logic2real();

  // return real-to-logical screen scale coefficient
  Point2 real2logic();

  inline float hdpx(float pixels)
  {
    float height = this->screen_height();
    return floorf(pixels * height * (1.0f / 1080.0f) + 0.5f);
  }

  //! begin raw layer (texture must not be changed until call to end_raw_layer())
  void start_raw_layer();
  //! finishes raw layer and renders all cached layers
  void end_raw_layer();

  // for legacy wrapper
  void start_render();
  void end_render();

  // draw vertices
  // function accepts logical screen coordinates only
  // !warn! vertices may be changed by this function!
  void draw_quads(const void *verts, int num_quads);
  void draw_faces(const void *verts, int num_verts, const IndexType *indices, int num_faces);
  void draw_prims(int d3d_prim, int num_prims, const void *verts, const IndexType *indices);

  // draw two polygons, using current color & texture (if present)
  // function accepts logical screen coordinates only
  void render_quad(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc0 = P2::ZERO, Point2 tc1 = Point2(0, 1), Point2 tc2 = P2::ONE,
    Point2 tc3 = Point2(1, 0));

  void render_quad_color(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc0, Point2 tc3, Point2 tc2, Point2 tc1, E3DCOLOR c0,
    E3DCOLOR c3, E3DCOLOR c2, E3DCOLOR c1);

  void render_quad_t(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc_lefttop = P2::ZERO, Point2 tc_rightbottom = P2::ONE);

  // ************************************************************************
  // * render primitives
  // ************************************************************************
  // all of these functions accept logical screen coordinates only
  // ************************************************************************
  // draw frame, using current color
  void render_frame(real left, real top, real right, real bottom, real thickness);

  // draw rectangle, using current color
  void render_box(real left, real top, real right, real bottom);
  void render_box(Point2 left_top, Point2 right_bottom) { render_box(left_top.x, left_top.y, right_bottom.x, right_bottom.y); }

  // draw rectangle with frame, using optimization tricks
  void solid_frame(real left, real top, real right, real bottom, real thickness, E3DCOLOR background, E3DCOLOR frame);

  // draw rectangle box, using current color & texture (if present)
  void render_rect(real left, real top, real right, real bottom, Point2 left_top_tc = P2::ZERO, Point2 dx_tc = P2::AXIS_X,
    Point2 dy_tc = P2::AXIS_Y);

  void render_rect(Point2 left_top, Point2 right_bottom, Point2 left_top_tc = P2::ZERO, Point2 dx_tc = P2::AXIS_X,
    Point2 dy_tc = P2::AXIS_Y)
  {
    render_rect(left_top.x, left_top.y, right_bottom.x, right_bottom.y, left_top_tc, dx_tc, dy_tc);
  }

  // draw rectangle box, using current color & texture (if present)
  void render_rect_t(real left, real top, real right, real bottom, Point2 left_top_tc = P2::ZERO, Point2 right_bottom_tc = P2::ONE);

  void render_rect_t(Point2 left_top, Point2 right_bottom, Point2 left_top_tc = P2::ZERO, Point2 right_bottom_tc = P2::ONE)
  {
    render_rect_t(left_top.x, left_top.y, right_bottom.x, right_bottom.y, left_top_tc, right_bottom_tc);
  }
  void render_rounded_box(Point2 lt, Point2 rb, E3DCOLOR col, E3DCOLOR border, Point4 rounding,
    float thickness); // optimized box with thickness
  void render_rounded_frame(Point2 lt, Point2 rb, E3DCOLOR col, Point4 rounding, float thickness);
  void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 tc_rightbottom, E3DCOLOR col, Point4 rounding);
  void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 dx_tc, Point2 dy_tc, E3DCOLOR col, Point4 rounding);

  // draw line
  void draw_line(real left, real top, real right, real bottom, real thickness = 1.0f);
  void draw_line(Point2 left_top, Point2 right_bottom, real thickness = 1.0f)
  {
    draw_line(left_top.x, left_top.y, right_bottom.x, right_bottom.y, thickness);
  }

  // pic quad color matrix
  void set_picquad_color_matrix(const TMatrix4 *cm);
  void set_picquad_color_matrix_saturate(float factor);
  void set_picquad_color_matrix_sepia(float factor = 1);
  inline void reset_picquad_color_matrix() { set_picquad_color_matrix(nullptr); }

  //************************************************************************
  //* Anti-aliased primitives
  //************************************************************************

  void render_line_ending_aa(E3DCOLOR color, const Point2 pos, const Point2 fwd, const Point2 left, float radius);
  void render_line_aa(const Point2 *points, int points_count, bool is_closed, float line_width, const Point2 line_indent,
    E3DCOLOR color);

  void render_line_aa(dag::ConstSpan<Point2> points, bool is_closed, float line_width, const Point2 line_indent, E3DCOLOR color)
  {
    render_line_aa(points.data(), points.size(), is_closed, line_width, line_indent, color);
  }

  void render_dashed_line(const Point2 p0, const Point2 p1, const float dash, const float space, const float line_width,
    E3DCOLOR color);

  void render_poly(dag::ConstSpan<Point2> points, E3DCOLOR fill_color);
  void render_inverse_poly(dag::ConstSpan<Point2> points_ccw, E3DCOLOR fill_color, Point2 left_top, Point2 right_bottom);
  void render_ellipse_aa(Point2 pos, Point2 radius, float line_width, E3DCOLOR color, E3DCOLOR fill_color);
  void render_ellipse_aa(Point2 pos, Point2 radius, float line_width, E3DCOLOR color, E3DCOLOR mid_color, E3DCOLOR fill_color);
  void render_sector_aa(Point2 pos, Point2 radius, Point2 angles, float line_width, E3DCOLOR color, E3DCOLOR fill_color);
  void render_sector_aa(Point2 pos, Point2 radius, Point2 angles, float line_width, E3DCOLOR color, E3DCOLOR mid_color,
    E3DCOLOR fill_color);
  void render_rectangle_aa(Point2 lt, Point2 rb, float line_width, E3DCOLOR color, E3DCOLOR fill_color);
  void render_rectangle_aa(Point2 lt, Point2 rb, float line_width, E3DCOLOR color, E3DCOLOR mid_color, E3DCOLOR fill_color);
  void render_smooth_round_rect(Point2 lt, Point2 rb, float corner_inner_radius, float corner_outer_radius, E3DCOLOR color);

  void render_line_gradient_out(Point2 from, Point2 to, E3DCOLOR center_col, float center_width, float outer_width,
    E3DCOLOR outer_col = E3DCOLOR(0, 0, 0, 0));

  // ************************************************************************
  // * text stuff
  // ************************************************************************
  // all of these functions accept logical screen coordinates only
  // ************************************************************************
  // set curent text out position
  void goto_xy(Point2 pos);
  void goto_xy(real x, real y) { goto_xy(Point2(x, y)); }
  // get curent text out position
  Point2 get_text_pos();

  // set font spacing
  void set_spacing(int new_spacing);

  // set font mono width (all glyphs are centered inside with width, text width is fixed)
  void set_mono_width(int mw);

  // get font spacing
  int get_spacing();

  // set current font
  // returns previous font id
  int set_font(int font_id, int font_kern = 0, int mono_w = 0);

  // set current font height
  // returns previous font height
  int set_font_ht(int font_ht);

  //! set text draw attributes
  void set_draw_str_attr(FontFxType t, int ofs_x, int ofs_y, E3DCOLOR col, int factor_x32);
  //! reset text draw attributes to defaults
  void reset_draw_str_attr() { set_draw_str_attr(FFT_NONE, 0, 0, 0, 0); }

  //! set text draw second texture(to be modulated with font texture)
  //! su_x32 and sv_x32 specify fixed point scale for u,v (32 mean texel-to-pixel correspondance, >32 =stretch)
  //! su_x32=0 or sv_x32=0 means stretching texture for font height
  //! bv_ofs specify upward offset in pixels from font baseline for v origin
  void set_draw_str_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, int su_x32 = 32, int sv_x32 = 32, int bv_ofs = 0);
  //! reset text draw second texture to defaults
  void reset_draw_str_texture() { set_draw_str_texture(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, 32, 32, 0); }

  //! render single character using current font (Unicode16)
  void draw_char_u(wchar_t ch);

  //! render text string using current font with scale (UTF-8)
  void draw_str_scaled(real scale, const char *str, int len = -1);
  //! render text string using current font with scale (Unicode16)
  void draw_str_scaled_u(real scale, const wchar_t *str, int len = -1);

  //! render whole inscription using cache RT with current font (UTF-8)
  void draw_inscription_scaled(real scale, const char *str, int len = -1);

  //
  // alternative inscription API
  //

  //! creates inscription for specified text, font and scale; returns handle to be used in other calls;
  static uint32_t create_inscription(int font_id, real scale, const char *str, int len = -1);
  //! renders ready inscription using current position and color
  void draw_inscription(uint32_t inscr_handle, float over_scale = 1);
  //! returns size of ready inscription (or 0 sized box when inscription is not ready)
  static BBox2 get_inscription_size(uint32_t inscr_handle);
  //! touches inscription to prevent eviction from cache
  static void touch_inscription(uint32_t inscr_handle);
  //! purges inscription for cache
  static void purge_inscription(uint32_t inscr_handle);
  //! returns true when inscription is valid and ready
  static bool is_inscription_ready(uint32_t inscr_handle);


  //
  // separate prepare/render text as quads
  //
  //! render scaled text to buffer (fill quad vertices and tex/quad count pairs); returns false if some glyphs to be yet rasterized
  bool draw_str_scaled_u_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
    unsigned dsb_flags, real scale, const wchar_t *str, int len = -1);
  bool draw_str_scaled_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
    unsigned dsb_flags, real scale, const char *str, int len = -1);
  //! render buffer (previously filled with draw_str_scaled_u_buf)
  void render_str_buf(dag::ConstSpan<GuiVertex> qv, dag::ConstSpan<uint16_t> tex_qcnt, dag::ConstSpan<d3d::SamplerHandle> smp,
    unsigned dsb_flags);


  //! render text string with scale=1
  inline void draw_str(const char *str, int len = -1) { draw_str_scaled(1.0f, str, len); }
  inline void draw_str_u(const wchar_t *str, int len = -1) { draw_str_scaled_u(1.0f, str, len); }
  inline void draw_inscription(const char *str, int len = -1) { draw_inscription_scaled(1.0f, str, len); }

  //! render text string by format using current font (UTF-8)
  void draw_strv(float scale, const char *f, const DagorSafeArg *arg, int anum);
  void draw_inscription_v(float scale, const char *f, const DagorSafeArg *arg, int anum);

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS 1.0f,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void draw_strf, draw_strv);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void draw_inscription_fmt, draw_inscription_v);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
#define DSA_OVERLOADS_PARAM_DECL float scale,
#define DSA_OVERLOADS_PARAM_PASS scale,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void draw_strf_scaled, draw_strv);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void draw_inscription_scaled_fmt, draw_inscription_v);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

  // render text string using current font with format: 'hh':'mm':'ss'.'msms'
  void draw_timestr(unsigned int ms);

  // vertex transform

  void resetViewTm();
  void setViewTm(const float m[2][3]);
  void setViewTm(Point2 ax, Point2 ay, Point2 o, bool add = false);
  void setRotViewTm(float x0, float y0, float rot_ang_rad, float skew_ang_rad, bool add = 0);
  void getViewTm(float dest_vtm[2][3], bool pure_trans = false) const;

  const GuiVertexTransform &getVertexTransform() const { return vertexTransform; }
  const GuiVertexTransform &getVertexTransformInverse() const { return vertexTransformInverse; }
  void calcInverseVertexTransform();
  void calcPreTransformViewport(Point2 lt, Point2 rb);
  void calcPreTransformViewport();

private:
  void defaultState();
  void rollStateBlock();
  void flushDrawElem(bool roll_state = false);
  void render_imgui_list();
};

GuiContext *get_stdgui_context();

// ************************************************************************
// * font handling
// ************************************************************************

// init fonts - get font cfg section & codepage filename (same as T_init())
void init_fonts(const DataBlock &blk);

void init_dynamics_fonts(int atlas_count, int atlas_size, const char *path_prefix);

void load_dynamic_font(const DataBlock &font_description, int screen_height);

// close fonts
void close_fonts();

bool get_font_context(StdGuiFontContext &ctx, int font_id, int font_spacing, int mono_w = 0, int ht = 0);

struct FontEntry
{
  const char *fontName;
  int fontId;
};
// returns list of fonts (name and id pairs), includes both real fonts and aliases
dag::ConstSpan<FontEntry> get_fonts_list();

// get font ID for specified name; return -1, if failed
int get_font_id(const char *font_name);

// get font object for specified id; return NULL, if failed
DagorFontBinDump *get_font(int font_id);

// get font name; return NULL, if failed
const char *get_font_name(int font_id);

// return initial font height (as read from font binary/config)
int get_initial_font_ht(int font_id);

// return default font height
int get_def_font_ht(int font_id);

// set default font height (for dynFonts only)
bool set_def_font_ht(int font_id, int ht);

//! prefetch (force prerender) glyphs for given chars of current font
void prefetch_font_glyphs(const wchar_t *chars, int count, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);

//! returns character width, glyph.dx (Unicode16)
float get_char_width_u(wchar_t ch, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);

//! get character bounding box (Unicode16)
BBox2 get_char_bbox_u(wchar_t ch, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);

//! get character visible bounding box (Unicode16)
BBox2 get_char_visible_bbox_u(wchar_t ch, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);

//! get string bounding box (UTF-8)
BBox2 get_str_bbox(const char *str, int len = -1, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
//! get string bounding box (Unicode16)
BBox2 get_str_bbox_u(const wchar_t *str, int len = -1, const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
//! get string bounding box (Unicode16) with clipping
BBox2 get_str_bbox_u_ex(const wchar_t *str, int len, const StdGuiFontContext &fctx, float max_w, bool left_align, int &out_start_idx,
  int &out_count, const wchar_t *break_sym = NULL, float ellipsis_resv_w = 0);

Point2 get_font_cell_size(const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
int get_font_caps_ht(const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
real get_font_line_spacing(const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
int get_font_ascent(const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);
int get_font_descent(const StdGuiFontContext &fctx = get_stdgui_context()->curRenderFont);

// return font cell size
inline Point2 get_font_cell_size(int f, int ht = 0) { return get_font_cell_size(StdGuiFontContext(f, 0, 0, ht)); }

// return font height for capital letters, e.g, 'A'
inline int get_font_caps_ht(int f, int ht = 0) { return get_font_caps_ht(StdGuiFontContext(f, 0, 0, ht)); }

// return font spacing
inline real get_font_line_spacing(int f, int ht = 0) { return get_font_line_spacing(StdGuiFontContext(f, 0, 0, ht)); }

// return font ascent
inline int get_font_ascent(int f, int ht = 0) { return get_font_ascent(StdGuiFontContext(f, 0, 0, ht)); }

// return font descent
inline int get_font_descent(int f, int ht = 0) { return get_font_descent(StdGuiFontContext(f, 0, 0, ht)); }

// return font dynamic property, return false for fonts static sized in pixels
bool is_font_dynamic(int font_id);

// the callback will be set on device reset. If it returns false, the gui renderer will take no action on it
void set_device_reset_filter(DeviceResetFilter callback);

// to be called in d3d reset callback after device was reseted
void after_device_reset();

// to be called from scene::act() to perform pending font rasterisation and other tasks
void update_internals_per_act();


// ************************************************************************
// * single-threaded legacy wrapper over stdgui_context
// ************************************************************************
void acquire();
void release();
struct ScopedAcquire
{
  ScopedAcquire() { acquire(); }
  ~ScopedAcquire() { release(); }
};

// init dynamic VB & IB for lockless rendering of GUI quads;
// quad_num is estimated maximum number of quads rendered per frame
void init_dynamic_buffers(int quad_num = 8192, int extra_indices = 1024);

// init rendering - must called after shaders initilization
void init_render();

// close renderer
void close_render();

// should be called in beforeRender() of each frame to reset positions in GUI dynamic buffers
// (unless target d3d driver natively supports lockless ring buffers)
// returns current buffer position (in quads); it can be slightly greater than real number of rendered quads
// when one or more rounds occured in last frame
int reset_per_frame_dynamic_buffer_pos();

// not supported
// set current shader - show fatal error, if fail; set shader(NULL) - reset to default shader
void set_shader(const char *shader_name);
// set current shader - show fatal error, if fail; set shader(NULL) - reset to default shader
void set_shader(ShaderMaterial *mat);

// set current shader - show fatal error, if fail
void set_shader(GuiShader *shader);
// reset shader to default
void reset_shader();
GuiShader *get_shader();

// set opaque semantic user state
void set_user_state(int state, const float *v, int cnt); // currently no more than 8 floats. to be changed
template <class T>
inline void set_user_state(int state, const T &v)
{
  return set_user_state(state, (const float *)(const char *)&v, sizeof(T));
}

// set current color
void set_color(E3DCOLOR color);
inline void set_color(int r, int g, int b, int a = 255) { set_color(E3DCOLOR(r, g, b, a)); }

// set alpha blending
void set_alpha_blend(BlendMode blend_mode);
BlendMode get_alpha_blend();
inline void set_ablend(bool on) { set_alpha_blend(on ? PREMULTIPLIED : NO_BLEND); }

// set current texture (if texture changed, flush cached data)
void set_textures(const TEXTUREID tex_id, d3d::SamplerHandle smp_id, const TEXTUREID tex_id2, d3d::SamplerHandle smp_id2);
inline void set_texture(const TEXTUREID tex_id, d3d::SamplerHandle smp_id)
{
  set_textures(tex_id, smp_id, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE);
}
inline void reset_textures() { set_textures(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE); }
void set_mask_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, Point3 transform0, Point3 transform1);

inline void start_font_str(float s) { get_stdgui_context()->start_font_str(s); }

// ************************************************************************
// * viewports
// ************************************************************************
// set viewport (add it to stack)
inline void set_viewport(real left, real top, real right, real bottom);
void set_viewport(const GuiViewPort &rt);

// reset to previous viewport (remove it from stack). return false, if no items to remove
bool restore_viewport();

// get current viewport
const GuiViewPort &get_viewport();

// ************************************************************************
// * low-level rendering
// ************************************************************************
// setup render parameters. call it before any GUI rendering.
// set screen logical resolution to current device resolution.
void start_render();

// setup render parameters. call it before any GUI rendering.
// set screen logical resolution in pixels.
// if resolution is zero, use current device resolution.
void start_render(int screen_width, int screen_height);

// continue with current target
void continue_render();

// restore parameters. call it after all GUI rendering
void end_render();

// return true, if start_render already has been called
bool is_render_started();

class ScopeStarter
{
public:
  ScopeStarter() { start_render(); }
  ScopeStarter(int scr_w, int scr_h) { start_render(scr_w, scr_h); }
  ~ScopeStarter() { end_render(); }
};
class ScopeStarterOptional
{
public:
  ScopeStarterOptional() : doStart(!is_render_started()) { doStart ? start_render() : (void)0; }
  ScopeStarterOptional(int scr_w, int scr_h) : doStart(!is_render_started())
  {
    if (doStart)
      start_render(scr_w, scr_h);
  }
  ~ScopeStarterOptional() { doStart ? end_render() : (void)0; }

private:
  bool doStart;
};

// get current logical resolution
Point2 screen_size();
real screen_width();
real screen_height();

// get real screen size
real real_screen_width();
real real_screen_height();

// return logical screen-to-real scale coefficient
Point2 logic2real();

// return real-to-logical screen scale coefficient
Point2 real2logic();

//! begin raw layer (texture must not be changed until call to end_raw_layer())
void start_raw_layer();
//! finishes raw layer and renders all cached layers
void end_raw_layer();

// draw vertices
// function accepts logical screen coordinates only
// !warn! vertices may be changed by this function!
void draw_quads(GuiVertex *verts, int num_quads);
void draw_faces(GuiVertex *verts, int num_verts, const IndexType *indices, int num_faces);

// render current cached data
void flush_data();

// draw two poligons, using current color & texture (if present)
// function accepts logical screen coordinates only
void render_quad(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc0 = P2::ZERO, Point2 tc1 = Point2(0, 1), Point2 tc2 = P2::ONE,
  Point2 tc3 = Point2(1, 0));

void render_quad_color(Point2 p0, Point2 p3, Point2 p2, Point2 p1, Point2 tc0, Point2 tc3, Point2 tc2, Point2 tc1, E3DCOLOR c0,
  E3DCOLOR c3, E3DCOLOR c2, E3DCOLOR c1);

void render_quad_t(Point2 p0, Point2 p1, Point2 p2, Point2 p3, Point2 tc_lefttop = P2::ZERO, Point2 tc_rightbottom = P2::ONE);

// ************************************************************************
// * render primitives
// ************************************************************************
// all of these functions accept logical screen coordinates only
// ************************************************************************
// draw frame, using current color
void render_frame(real left, real top, real right, real bottom, real thickness);

// draw rectangle, using current color
void render_box(real left, real top, real right, real bottom);
inline void render_box(Point2 left_top, Point2 right_bottom);

// draw rectangle with frame, using optimization tricks
void solid_frame(real left, real top, real right, real bottom, real thickness, E3DCOLOR background, E3DCOLOR frame);

// draw rectangle box, using current color & texture (if present)
void render_rect(real left, real top, real right, real bottom, Point2 left_top_tc = P2::ZERO, Point2 dx_tc = P2::AXIS_X,
  Point2 dy_tc = P2::AXIS_Y);

inline void render_rect(Point2 left_top, Point2 right_bottom, Point2 left_top_tc = P2::ZERO, Point2 dx_tc = P2::AXIS_X,
  Point2 dy_tc = P2::AXIS_Y);

// draw rectangle box, using current color & texture (if present)
void render_rect_t(real left, real top, real right, real bottom, Point2 left_top_tc = P2::ZERO, Point2 right_bottom_tc = P2::ONE);

inline void render_rect_t(Point2 left_top, Point2 right_bottom, Point2 left_top_tc = P2::ZERO, Point2 right_bottom_tc = P2::ONE);

void render_rounded_box(Point2 lt, Point2 rb, E3DCOLOR col, E3DCOLOR frame, Point4 rounding, float thickness);
void render_rounded_frame(Point2 lt, Point2 rb, E3DCOLOR col, Point4 rounding, float thickness);
void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 tc_rightbottom, E3DCOLOR col, Point4 rounding);
void render_rounded_image(Point2 lt, Point2 rb, Point2 tc_lefttop, Point2 dx_tc, Point2 dy_tc, E3DCOLOR col, Point4 rounding);
// draw line
void draw_line(real left, real top, real right, real bottom, real thickness = 1.0f);
inline void draw_line(Point2 left_top, Point2 right_bottom, real thickness = 1.0f)
{
  draw_line(left_top.x, left_top.y, right_bottom.x, right_bottom.y, thickness);
}

//! set custom color matrix to render picture quads
void set_picquad_color_matrix(const TMatrix4 *cm);
//! set saturate/desaturate(grey-scale) color matrix to render picture quads; factor=0..1+ (0=full desaturate, 1=no change, >1=over
//! saturate)
void set_picquad_color_matrix_saturate(float factor);
//! set sepia (grey-scale) color matrix to render picture quads; factor=0..1 (0=no change, 1=full sepia)
void set_picquad_color_matrix_sepia(float factor = 1);
//! reset color matrix to defaults
inline void reset_picquad_color_matrix() { set_picquad_color_matrix(nullptr); }

//************************************************************************
//* Anti-aliased primitives
//************************************************************************
inline void render_line_aa(const Point2 from, const Point2 to, bool /*is_closed*/, float line_width, const Point2 line_indent,
  E3DCOLOR color)
{
  const Point2 points[2] = {from, to};
  get_stdgui_context()->render_line_aa(points, 2, false, line_width, line_indent, color);
}


// ************************************************************************
// * text stuff
// ************************************************************************
// all of these functions accept logical screen coordinates only
// ************************************************************************
// set curent text out position
void goto_xy(Point2 pos);
inline void goto_xy(real x, real y) { goto_xy(Point2(x, y)); }

// get curent text out position
Point2 get_text_pos();

// set font spacing
void set_spacing(int new_spacing);

// set font mono width (all glyphs are centered inside width, total text width is fixed)
void set_mono_width(int mw);

// get font spacing
int get_spacing();

// set current font
// returns previous font id
int set_font(int font_id, int font_kern = 0, int mono_w = 0);

// set current font height
// returns previous font height
int set_font_ht(int font_ht);

//! set text draw attributes
void set_draw_str_attr(FontFxType t, int ofs_x, int ofs_y, E3DCOLOR col, int factor_x32);
//! reset text draw attributes to defaults
inline void reset_draw_str_attr() { set_draw_str_attr(FFT_NONE, 0, 0, 0, 0); }

//! set text draw second texture(to be modulated with font texture)
//! su_x32 and sv_x32 specify fixed point scale for u,v (32 mean texel-to-pixel correspondance, >32 =stretch)
//! su_x32=0 or sv_x32=0 means stretching texture for font height
//! bv_ofs specify upward offset in pixels from font baseline for v origin
void set_draw_str_texture(TEXTUREID tex_id, d3d::SamplerHandle smp_id, int su_x32 = 32, int sv_x32 = 32, int bv_ofs = 0);
//! reset text draw second texture to defaults
inline void reset_draw_str_texture() { set_draw_str_texture(BAD_TEXTUREID, d3d::INVALID_SAMPLER_HANDLE, 32, 32, 0); }

//! render single character using current font (Unicode16)
void draw_char_u(wchar_t ch);

//! render text string using current font with scale (UTF-8)
void draw_str_scaled(real scale, const char *str, int len = -1);
//! render text string using current font with scale (Unicode16)
void draw_str_scaled_u(real scale, const wchar_t *str, int len = -1);

//! render whole inscription using cache RT with current font (UTF-8)
void draw_inscription_scaled(float scale, const char *str, int len = -1);

inline uint32_t create_inscription(int font_id, real scale, const char *str, int len = -1)
{
  return GuiContext::create_inscription(font_id, scale, str, len);
}
inline uint32_t create_inscription_ht(int font_id, real font_ht, const char *str, int len = -1)
{
  return GuiContext::create_inscription(font_id, font_ht / float(get_font_caps_ht(font_id)), str, len);
}

void draw_inscription(uint32_t inscr_handle, float over_scale = 1);
inline BBox2 get_inscription_size(uint32_t inscr_handle) { return GuiContext::get_inscription_size(inscr_handle); }
inline void touch_inscription(uint32_t inscr_handle) { GuiContext::touch_inscription(inscr_handle); }
inline void purge_inscription(uint32_t inscr_handle) { GuiContext::purge_inscription(inscr_handle); }
inline bool is_inscription_ready(uint32_t inscr_handle) { return GuiContext::is_inscription_ready(inscr_handle); }

//! render scaled text to buffer (fill quad vertices and tex/quad count pairs); returns false if some glyphs shall be yet rasterized
bool draw_str_scaled_u_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
  unsigned dsb_flags, real scale, const wchar_t *str, int len = -1);
bool draw_str_scaled_buf(SmallTab<GuiVertex> &out_qv, SmallTab<uint16_t> &out_tex_qcnt, SmallTab<d3d::SamplerHandle> &out_smp,
  unsigned dsb_flags, real scale, const char *str, int len = -1);
//! checks whether quad cache buffers (previously filled with draw_str_scaled_u_buf) is valid and up-to-date
inline bool check_str_buf_ready(dag::ConstSpan<uint16_t> tex_qcnt)
{
  return tex_qcnt.size() > 0 && tex_qcnt[0] == dyn_font_atlas_reset_generation;
}
//! render buffer (previously filled with draw_str_scaled_u_buf)
void render_str_buf(dag::ConstSpan<GuiVertex> qv, dag::ConstSpan<uint16_t> tex_qcnt, dag::ConstSpan<d3d::SamplerHandle> smp,
  unsigned dsb_flags);

//! render text string with scale=1
static inline void draw_str(const char *str, int len = -1) { draw_str_scaled(1.0f, str, len); }
static inline void draw_str_u(const wchar_t *str, int len = -1) { draw_str_scaled_u(1.0f, str, len); }
static inline void draw_inscription(const char *str, int len = -1) { draw_inscription_scaled(1.0f, str, len); }

//! render text string by format using current font (UTF-8)
void draw_strv(float scale, const char *f, const DagorSafeArg *arg, int anum);
void draw_inscription_v(float scale, const char *f, const DagorSafeArg *arg, int anum);

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS 1.0f,
DECLARE_DSA_OVERLOADS_FAMILY_LT(static inline void draw_strf, draw_strv);
DECLARE_DSA_OVERLOADS_FAMILY_LT(static inline void draw_inscription_fmt, draw_inscription_v);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
#define DSA_OVERLOADS_PARAM_DECL float scale,
#define DSA_OVERLOADS_PARAM_PASS scale,
DECLARE_DSA_OVERLOADS_FAMILY_LT(static inline void draw_strf_scaled, draw_strv);
DECLARE_DSA_OVERLOADS_FAMILY_LT(static inline void draw_inscription_scaled_fmt, draw_inscription_v);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

// render text string using current font with format: 'hh':'mm':'ss'.'msms'
void draw_timestr(unsigned int ms);

// ************************************************************************
// * private - implement of inline functions
// ************************************************************************

inline void set_viewport(real left, real top, real right, real bottom) { set_viewport(GuiViewPort(left, top, right, bottom)); }

inline void render_box(Point2 left_top, Point2 right_bottom) { render_box(left_top.x, left_top.y, right_bottom.x, right_bottom.y); }

inline void render_rect(Point2 left_top, Point2 right_bottom, Point2 left_top_tc, Point2 dx_tc, Point2 dy_tc)
{
  render_rect(left_top.x, left_top.y, right_bottom.x, right_bottom.y, left_top_tc, dx_tc, dy_tc);
}

inline void render_rect_t(Point2 left_top, Point2 right_bottom, Point2 left_top_tc, Point2 right_bottom_tc)
{
  render_rect_t(left_top.x, left_top.y, right_bottom.x, right_bottom.y, left_top_tc, right_bottom_tc);
}

//! replace tabs (U+0009) with zero-width spaces (U+200B) in zero-terminated unicode string
static inline void replace_tabs_with_zwspaces(wchar_t *u16)
{
  for (wchar_t *p = wcschr(u16, L'\t'); p; p = wcschr(p + 1, L'\t'))
    *p = L'\u200B';
}
}; // namespace StdGuiRender

inline StdGuiFontContext::StdGuiFontContext(int font_id, int s, int mw, int ht) :
  font(StdGuiRender::get_font(font_id)), spacing(s), monoW(mw), fontHt(ht)
{}
