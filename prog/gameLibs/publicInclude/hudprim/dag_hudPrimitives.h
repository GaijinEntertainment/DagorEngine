//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_bounds2.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_e3dColor.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <gui/dag_stdGuiRender.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class ShaderElement;

// http://en.wikipedia.org/wiki/ANSI_escape_code#Colors
enum
{
  ECT_BLACK = 0,
  ECT_DARK_RED,
  ECT_DARK_GREEN,
  ECT_DARK_YELLOW,
  ECT_DARK_BLUE,
  ECT_DARK_MAGENTA,
  ECT_DARK_CYAN,
  ECT_GREY,
  ECT_DARK_GREY,
  ECT_RED,
  ECT_GREEN,
  ECT_YELLOW,
  ECT_BLUE,
  ECT_MAGENTA,
  ECT_CYAN,
  ECT_WHITE,

  // extra light colors
  ECT_LIGHT_RED,
  ECT_LIGHT_GREEN,
  ECT_LIGHT_YELLOW,
  ECT_LIGHT_BLUE,
  ECT_LIGHT_MAGENTA,
  ECT_LIGHT_CYAN,
  ECT_NUM
};

struct HudPrimitivesSavedView
{
  int screenWidth;
  int screenHeight;
};

struct BufferedChar
{
  Point2 posLT; // Character relative pos added.
  Point2 posRB;
  Point2 texcoordLT;
  Point2 texcoordRB;
};

struct BufferedText
{
  Tab<BufferedChar> text;
  TEXTUREID textureId;

  BBox2 calcBBox() const;

  BufferedText() : text(midmem), textureId(BAD_TEXTUREID) {}
};

struct HudTexElem
{
  TEXTUREID skinTex;
  // lt, lb, rb, rt
  carray<Point2, 4> tc;

  HudTexElem() : skinTex(BAD_TEXTUREID), tc() {}

  HudTexElem(TEXTUREID skin_tex, const Point2 &tc_lt = Point2(0.f, 0.f), const Point2 &tc_rb = Point2(1.f, 1.f)) : skinTex(skin_tex)
  {
    setTc(tc_lt, tc_rb);
  }

  HudTexElem(TEXTUREID skin_tex, const Point2 &tc_lt, const Point2 &tc_lb, const Point2 &tc_rb, const Point2 &tc_rt) :
    skinTex(skin_tex)
  {
    tc[0] = tc_lt;
    tc[1] = tc_lb;
    tc[2] = tc_rb;
    tc[3] = tc_rt;
  }

  bool isValid() const { return skinTex != BAD_TEXTUREID; }

  void setTc(const Point2 &tc_lt, const Point2 &tc_rb)
  {
    tc[0] = tc_lt;
    tc[1] = Point2(tc_lt.x, tc_rb.y);
    tc[2] = tc_rb;
    tc[3] = Point2(tc_rb.x, tc_lt.y);
  }
};

struct HudShader : StdGuiRender::GuiShader
{
  // aliased to ExtState
  struct ExtStateLocal
  {
    Color4 currentBlueToAlpha;

    TEXTUREID currentTexture;
    TEXTUREID maskTexId;

    Point3 maskMatrix0;
    Point3 maskMatrix1;

    uint8_t renderMode;
    uint8_t writeToZ;
    uint8_t depthTest;
    int16_t stencilRefValue; //-1 = ignore
    bool maskEnable;

    void reset()
    {
      currentBlueToAlpha = Color4(0, 0, 0, 1);
      currentTexture = BAD_TEXTUREID;
      maskTexId = BAD_TEXTUREID;
      maskMatrix0 = Point3(0, 0, 0);
      maskMatrix1 = Point3(0, 0, 0);

      renderMode = 0; // RENDER_MODE_HUD;
      writeToZ = 0;
      depthTest = 0;
      stencilRefValue = -1;
      maskEnable = false;
    }
  };

  virtual void channels(const CompiledShaderChannelId *&channels, int &num_channels);
  virtual void link();
  virtual void cleanup();

  virtual void setStates(const float viewport[5], const StdGuiRender::GuiState &guiState, const StdGuiRender::ExtState *extState,
    bool viewport_changed, bool guistate_changed, bool extstate_changed, int targetW, int targetH);
};

class HudPrimitives
{
public:
  struct HudVertex
  {
    Point4 pos;
    E3DCOLOR color;
    Point2 tuv;
  };

  struct Pivot
  {
    Point2 origin = ZERO<Point2>();
    float angleRadians = 0.f;
  };

  enum RenderMode
  {
    RENDER_MODE_HUD = 0,
    RENDER_MODE_SCENE = 1
  };

  enum Coords
  {
    COORDS_2D,
    COORDS_3D,
    COORDS_2D_SCREEN_SPACE,
    COORDS_4D_CLIP_SPACE,
  };

  StdGuiRender::GuiContext *guiContext;

  // virtual screen coordinate system with square pixels
  int viewX, viewY, viewW, viewH;
  float viewWidthRcp2;
  float viewHeightRcp2;

  // adapt to different naming conventions
  union
  {
    int width;
    int screenWidth;
  };
  union
  {
    int height;
    int screenHeight;
  };

  union
  {
    float widthRcp2; // 1/x * 2.
    float screenWidthRcp2;
  };

  union
  {
    float heightRcp2;
    float screenHeightRcp2;
  };

  float minz, maxz;

  bool ownContext;
  bool isInHudRender;

  // current state for quads
  E3DCOLOR colorMul;
  StdGuiRender::ExtState extStateStorage;

  HudShader::ExtStateLocal &state()
  {
    return *(HudShader::ExtStateLocal *)&extStateStorage; //-V641
  }

  HudPrimitivesSavedView savedView;

  // create new gui_context or from already existing gui_context
  HudPrimitives(StdGuiRender::GuiContext *gui_context = NULL);
  ~HudPrimitives();

  E3DCOLOR *getColorForModify(int color_id);
  TEXTUREID getWhiteTexId();
  static Texture *getWhiteTex();
  void loadFromBlk(const DataBlock *blk);

  // hud_quads aces_gui shader elements (quads, TextEx)
  // gui_quads aces_default stdGuiRender elements (TextFx)
  // extra_indices - used for render polygons/fans
  bool createGuiBuffer(int gui_quads, int hud_extra_indices);
  bool createHudBuffer(int hud_quads, int gui_extra_indices);

  bool updateTargetIfNecessary();
  void updateTarget();
  void updateTarget(int width, int height, int left = 0, int top = 0);
  void updateViewFromContext();

  void beforeReset();
  void afterReset();
  void resetFrame();
  int beginChunk(); // return chunk_id 0-based
  void endChunk();
  void renderChunk(int chunk_id);

  // switch to hud buffer
  void beginRender();
  // switch back to stdgui buffer
  void endRender();

  // special version of for immediate rendering (replaces begin/end/renderChunk)
  // in different contexts - set minimum states
  void beginRenderImm();
  // finish and draw everything
  void endRenderImm();

  void saveViewport();
  void restoreViewport();
  void setViewport(int width, int height);

  void flush();
  void setState()
  {
    G_ASSERT(guiContext->isInRender);
    guiContext->setRollState(StdGuiRender::GuiContext::ROLL_EXT_STATE);
  }
  // shader custom variant select (returns old mode)
  RenderMode setRenderMode(RenderMode mode);

  void setStencil(int stencil_ref_value);
  void updateState(TEXTUREID texture_id);

  // Deprected drawing method without clipping, 3d, and stereo support so
  // do not use it anymore
  void renderQuadScreenSpaceDeprecated(TEXTUREID texture_id, E3DCOLOR color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
    const Point4 &p3, const Point2 &tc0 = Point2(0.f, 0.f), const Point2 &tc1 = Point2(0.f, 1.f), const Point2 &tc2 = Point2(1.f, 1.f),
    const Point2 &tc3 = Point2(1.f, 0.f), const E3DCOLOR *vertex_colors = NULL);

  void renderQuadScreenSpaceDeprecated(const HudTexElem &elem, E3DCOLOR color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
    const Point4 &p3, const E3DCOLOR *vertex_colors = NULL)
  {
    renderQuadScreenSpaceDeprecated(elem.skinTex, color, p0, p1, p2, p3, elem.tc[0], elem.tc[1], elem.tc[2], elem.tc[3],
      vertex_colors);
  }

  void renderQuadScreenSpace3d(TEXTUREID texture_id, E3DCOLOR color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
    const Point4 &p3, const Point2 &tc0 = Point2(0.f, 0.f), const Point2 &tc1 = Point2(0.f, 1.f), const Point2 &tc2 = Point2(1.f, 1.f),
    const Point2 &tc3 = Point2(1.f, 0.f), const E3DCOLOR *vertex_colors = NULL);

  void renderQuad(const HudTexElem &elem, const E3DCOLOR color, dag::ConstSpan<float> pos, dag::ConstSpan<E3DCOLOR> col,
    const Coords coords, const float depth_2d = 0.0f);

  void renderPoly(TEXTUREID texture_id, E3DCOLOR color, dag::ConstSpan<Point2> p, dag::ConstSpan<Point2> tc);

  void renderTriFan(TEXTUREID texture_id, E3DCOLOR color, dag::ConstSpan<Point4> p, dag::ConstSpan<Point2> tc);

  void drawPrims(int d3d_prim, int num_prim, const HudVertex *ptr)
  {
    guiContext->draw_prims(d3d_prim, num_prim, (const void *)ptr, NULL);
  }

  typedef uint32_t TextBufferHandle;
  static TextBufferHandle HANDLE_STATIC;

  void renderText(int x, int y, int font_no, E3DCOLOR color, const char *text, int len = -1, TextBufferHandle *inout_handle = nullptr);

  void renderTextEx(int x, int y, E3DCOLOR color, int font_id,
    int align, // -1 - left, 0 - center, 1 - right
    const char *texts, int len = -1, TextBufferHandle *inout_handle = nullptr);

  void renderTextFx(float x, float y, int font_no, E3DCOLOR color, const char *text, int len = -1, int align = -1, int fx_type = 0,
    int fx_offset = 1, E3DCOLOR fx_color = E3DCOLOR_MAKE(0, 0, 0, 255), int fx_scale = 42, const char *font_tex_2 = NULL,
    float scale = -1.f, int valign = -1, bool round_coords = true, Pivot pivot = Pivot{ZERO<Point2>(), 0.f},
    TextBufferHandle *inout_handle = nullptr);

  void renderColoredTextFx(int x, int y, int font_no, E3DCOLOR color, const char *text, float pixelLen, int len = -1, int align = -1,
    int fx_type = 0, int fx_offset = 1, E3DCOLOR fx_color = E3DCOLOR_MAKE(0, 0, 0, 255), int fx_scale = 42,
    const char *font_tex_2 = NULL);

  Point2 getColoredTextBBox(const char *text, int font_no);

  void setMask(TEXTUREID texture_id, const Point3 &matrix_line_0, const Point3 &matrix_line_1, const Point2 &tc0 = Point2(0.f, 0.f),
    const Point2 &tc1 = Point2(1.f, 1.f));

  void setMask(TEXTUREID texture_id, const Point2 &center_pos, float angle, const Point2 &scale, const Point2 &tc0 = Point2(0.f, 0.f),
    const Point2 &tc1 = Point2(1.f, 1.f));

  void clearMask();

  void getBufferedText(BufferedText &out_buffered_text, int font_no,
    int align, // -1 - left, 0 - center, 1 - right
    const char *text);

  void beginRenderBufferedText(){};
  void renderBufferedText(int x, int y, E3DCOLOR color, const BufferedText *buffered_text, float z = 0.f,
    float w = 1.f); // buffered_text must be valid until endRenderBufferedText.

  void renderBufferedText(int x, int y, E3DCOLOR color, int font_no, int align, const char *text, float z = 0.f, float w = 1.f);
  void endRenderBufferedText(){};

  void setColorMul(E3DCOLOR color_mul);
  void clear();

  void setZMode(bool write_to_z, int test_depth_mode) { guiContext->setZMode(write_to_z, test_depth_mode); }

  void setShader(HudShader *shader) { guiContext->setShaderToBuffer(1, shader); }

  void enableZWrite(bool write, bool test) { setZMode(write, test ? 1 : 0); }

  int set_font(int font_id, int font_kern = 0) { return guiContext->set_font(font_id, font_kern); }
  BBox2 get_str_bbox(const char *str, int len, const StdGuiFontContext &fctx) { return StdGuiRender::get_str_bbox(str, len, fctx); }
  BBox2 get_str_bbox(const char *str, int len = -1) { return get_str_bbox(str, len, guiContext->curRenderFont); }
  BBox2 get_char_visible_bbox_u(wchar_t c, const StdGuiFontContext &fctx) { return StdGuiRender::get_char_visible_bbox_u(c, fctx); }

  const TMatrix &getTransform2d(float stereo_depth = 0.0f) const;
  const TMatrix4 &getGlobTm2d(float stereo_depth = 0.0f) const;
  void setTransform2d(float depth, const TMatrix &in_tm, const TMatrix4 &proj_tm);
  void resetTransform2d();

  const TMatrix &getTransform3d() const;
  const TMatrix4 &getGlobTm3d() const;
  void setTransform3d(const TMatrix &in_tm, const TMatrix4 &proj_tm);

  void setUseRelativeGlobTm(bool use);

  static int extractColorFromText(const char *text);
  static int extractColorFromText(const wchar_t *text);

  // Helper methods kept for compatibility with old game lib code
  // Now all helper methods must be shipped in separate module, as hudtools for example

  void renderQuad(const HudTexElem &elem, const E3DCOLOR color, const Point2 &p0, const Point2 &p1, const Point2 &p2, const Point2 &p3,
    float depth_2d = 0, const E3DCOLOR *vertex_colors = NULL);

  void renderLine(E3DCOLOR color, const Point2 &p0, const Point2 &p1, float width);

  bool getScreenPos(const Point3 &world_pos, IPoint2 &screen_pos) const;

protected:
  TMatrix tm2d;
  TMatrix tm3d;
  TMatrix4 globTm2d;
  TMatrix4 globTm3d;
  TMatrix4 globTm3dRelative;
  TMatrix4 projTm2d;
  float depth2d;

  bool useRelativeGlobTm;

  struct TextBuffer
  {
    bool isReady = false;
    SmallTab<GuiVertex> qv;
    SmallTab<uint16_t> tex_qcnt;
  };
  ska::flat_hash_map<uint32_t, TextBuffer> textBuffers;

  inline void updateProjectedQuadVB(HudVertex *data, const E3DCOLOR &color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
    const Point4 &p3, const Point2 &tc0, const Point2 &tc1, const Point2 &tc2, const Point2 &tc3, const E3DCOLOR *vertex_colors);

  void renderOrMeasureColoredTextFx(int x, int y, int font_no, E3DCOLOR color, const char *text, float pixelLen, int len = -1,
    int align = -1, int fx_type = 0, int fx_offset = 1, E3DCOLOR fx_color = E3DCOLOR_MAKE(0, 0, 0, 255), int fx_scale = 42,
    const char *font_tex_2 = NULL,
    Point2 *out_size = NULL); // uses out_size to return text size or NULL to render it

private:
  // global resources
  static carray<E3DCOLOR, ECT_NUM> colorCodes;
  static HudShader hudShader;
  static int refCount;
};
