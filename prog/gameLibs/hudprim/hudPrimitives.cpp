// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <hudprim/dag_hudPrimitives.h>
#include <gui/dag_stdGuiRender.h>

#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <shaders/dag_shaderBlock.h>
#include <osApiWrappers/dag_unicode.h>
#include <math/dag_Matrix3.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_unicode.h>
#include <shaders/dag_shaderMesh.h>
#include <memory/dag_framemem.h>
#include <generic/dag_smallTab.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <render/whiteTex.h>
#include <wchar.h>
#include <gameMath/screenProjection.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_renderStates.h>

typedef StdGuiRender::IndexType HudIndexType;

HudPrimitives::TextBufferHandle HudPrimitives::HANDLE_STATIC = -1;

// HudPrimitives global variables
carray<E3DCOLOR, ECT_NUM> HudPrimitives::colorCodes;
HudShader HudPrimitives::hudShader;
int HudPrimitives::refCount = 0;

using namespace StdGuiRender;

BBox2 BufferedText::calcBBox() const
{
  BBox2 box;
  for (int i = 0; i < text.size(); ++i)
  {
    box += text[i].posLT;
    box += text[i].posRB;
  }
  return box;
}


static inline float scaleByAspect(float aspect, int width)
{
  return (int(aspect * float(width)) + 1) & ~1; // divisible by 2
}

static ShaderVariableInfo maskMatrixLine0VarId("mask_matrix_line_0", true), maskMatrixLine1VarId("mask_matrix_line_1", true),
  textureVarId("tex", true), maskTexVarId("mask_tex", true), guiTextDepthVarId("gui_text_depth", true),
  writeToZVarId("gui_write_to_z", true), testDepthVarId("gui_test_depth", true), blueToAlphaVarId("blue_to_alpha_mul", true);

static int renderModeVarId = -1;
static int guiAcesObjectBlockId = -1;

static const CompiledShaderChannelId hud_channels[] = {
  {SCTYPE_FLOAT4, SCUSAGE_POS, 0, 0},
  {SCTYPE_E3DCOLOR, SCUSAGE_VCOL, 0, 0},
  {SCTYPE_FLOAT2, SCUSAGE_TC, 0, 0},
};

void HudShader::channels(const CompiledShaderChannelId *&channels, int &num_channels)
{
  channels = hud_channels;
  num_channels = countof(hud_channels);
}

void HudShader::link()
{
  G_STATIC_ASSERT(sizeof(*this) <= sizeof(ExtState));

  renderModeVarId = VariableMap::getVariableId("render_mode");

  guiAcesObjectBlockId = ShaderGlobal::getBlockId("gui_aces_object");
}

void HudShader::cleanup()
{
  if (!material)
    return;
  ShaderGlobal::set_texture(textureVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(maskTexVarId, BAD_TEXTUREID);
}

void HudShader::setStates(const float viewport[5], const GuiState &guiState, const ExtState *extState, bool viewport_changed,
  bool /*guistate_changed*/, bool /*extstate_changed*/, int targetW, int targetH)
{
  G_UNREFERENCED(viewport);
  G_UNREFERENCED(viewport_changed);
  G_UNREFERENCED(targetH);
  G_UNREFERENCED(targetW);
  ShaderMaterial *mat = material;
  ShaderElement *elem = element;

  const FontFxAttr &fontAttr = guiState.fontAttr;
  const ExtStateLocal &st = *(const ExtStateLocal *)extState;

  if (st.maskEnable)
  {
    ShaderGlobal::set_texture(maskTexVarId, st.maskTexId);

    ShaderGlobal::set_color4(maskMatrixLine0VarId, st.maskMatrix0.x, st.maskMatrix0.y, st.maskMatrix0.z, 0.f);

    ShaderGlobal::set_color4(maskMatrixLine1VarId, st.maskMatrix1.x, st.maskMatrix1.y, st.maskMatrix1.z, 0.f);
  }
  else
  {
    ShaderGlobal::set_texture(maskTexVarId, BAD_TEXTUREID);
  }

  ShaderGlobal::set_int(writeToZVarId, fontAttr.writeToZ);
  ShaderGlobal::set_int(testDepthVarId, fontAttr.testDepth);

  ShaderGlobal::set_texture(textureVarId, st.currentTexture);
  ShaderGlobal::set_color4(blueToAlphaVarId, st.currentBlueToAlpha);

  mat->set_int_param(renderModeVarId, st.renderMode);
  if (st.stencilRefValue >= 0)
    shaders::set_stencil_ref(st.stencilRefValue);

  ShaderGlobal::setBlock(guiAcesObjectBlockId, ShaderGlobal::LAYER_OBJECT, false);
  elem->setStates(0, true);
}

HudPrimitives::HudPrimitives(StdGuiRender::GuiContext *gui_context) //-V730
  :
  isInHudRender(false),
  guiContext(NULL),
  tm2d(TMatrix::IDENT),
  tm3d(TMatrix::IDENT),
  globTm2d(TMatrix4::IDENT),
  globTm3d(TMatrix4::IDENT),
  globTm3dRelative(TMatrix4::IDENT),
  depth2d(0.0f),
  projTm2d(TMatrix4::IDENT),
  useRelativeGlobTm(false)
{
  ownContext = gui_context == NULL;
  guiContext = ownContext ? (new StdGuiRender::GuiContext()) : gui_context;

  guiContext->setExtStateLocation(1, &extStateStorage);

  colorMul = E3DCOLOR(255, 255, 255, 255);
  state().reset();

  if (refCount == 0)
  {
    hudShader.init("gui_aces");
    get_white_on_demand();
  }

  // init these vars with 1x1 (non-real resolution) to avoid garbage in members
  screenWidth = screenHeight = 1;
  screenWidthRcp2 = 2.0f / screenWidth;
  screenHeightRcp2 = 2.0f / screenHeight;
  viewY = 0;
  viewH = 1;
  float sx = 0;
  float sw = 1;
  viewX = int(sx);
  viewW = int(sw);
  viewWidthRcp2 = 2.0f / sw;
  viewHeightRcp2 = 2.0f / viewH;
  resetTransform2d();
  refCount++;
}

HudPrimitives::~HudPrimitives()
{
  refCount--;

  if (refCount == 0)
  {
    hudShader.close();
  }

  if (ownContext)
    del_it(guiContext);
}

void HudPrimitives::loadFromBlk(const DataBlock *blk)
{
  const DataBlock *colorTableBlk = blk->getBlockByNameEx("colorTable");
  static const char *colorTableNames[ECT_NUM] = {
    "black",
    "dark_red",
    "dark_green",
    "dark_yellow",
    "dark_blue",
    "dark_magenta",
    "dark_cyan",
    "grey",
    "dark_grey",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white",
    "light_red",
    "light_green",
    "light_yellow",
    "light_blue",
    "light_magenta",
    "light_cyan",
  };

  for (int i = 0; i < ECT_NUM; ++i)
    colorCodes[i] = colorTableBlk->getE3dcolor(colorTableNames[i], E3DCOLOR_MAKE(0, 0, 0, 255));
}

E3DCOLOR *HudPrimitives::getColorForModify(int color_id) { return safe_at(colorCodes, color_id); }
TEXTUREID HudPrimitives::getWhiteTexId() { return get_white_on_demand().getTexId(); }
Texture *HudPrimitives::getWhiteTex() { return get_white_on_demand().getTex2D(); }

void HudPrimitives::saveViewport()
{
  savedView.screenWidth = screenWidth;
  savedView.screenHeight = screenHeight;
}

void HudPrimitives::restoreViewport()
{
  screenWidth = savedView.screenWidth;
  screenHeight = savedView.screenHeight;

  screenWidthRcp2 = 2.0f / screenWidth;
  screenHeightRcp2 = 2.0f / screenHeight;
}

void HudPrimitives::setViewport(int width, int height)
{
  screenHeight = height;
  screenWidth = width;
  screenWidthRcp2 = 2.0f / screenWidth;
  screenHeightRcp2 = 2.0f / screenHeight;
}

int HudPrimitives::extractColorFromText(const char *text)
{
  char colorText[4];
  colorText[0] = *(text + 1);
  colorText[1] = *(text + 2);
  colorText[2] = *(text + 3);
  colorText[3] = 0;
  char color_id = atoi(colorText);
  G_ASSERT(color_id < uint32_t(ECT_NUM));
  return color_id;
}
int HudPrimitives::extractColorFromText(const wchar_t *text)
{
  wchar_t colorText[4];
  colorText[0] = *(text + 1);
  colorText[1] = *(text + 2);
  colorText[2] = *(text + 3);
  colorText[3] = 0;
  wchar_t *endPointer;
  char color_id = wcstol(colorText, &endPointer, 10);
  G_ASSERT(color_id < uint32_t(ECT_NUM));
  return color_id;
}

bool HudPrimitives::createGuiBuffer(int num_quads, int num_extra_indices)
{
  return guiContext->createBuffer(0, NULL, num_quads, num_extra_indices, "hudprim.guibuf");
}

bool HudPrimitives::createHudBuffer(int num_quads, int num_extra_indices)
{
  return guiContext->createBuffer(1, &hudShader, num_quads, num_extra_indices, "hudprim.hudbuf");
}

void HudPrimitives::resetFrame()
{
  G_ASSERT(!isInHudRender);
  colorMul = E3DCOLOR(255, 255, 255, 255);
  state().reset();
  G_ASSERT(guiContext);
  guiContext->resetFrame();
}

void HudPrimitives::flush() { guiContext->flushData(); }

bool HudPrimitives::updateTargetIfNecessary()
{
  if (screenWidth == 1 || screenHeight == 1)
    return false;

  updateTarget();
  return true;
}

void HudPrimitives::updateTarget()
{
  guiContext->setTarget();
  updateViewFromContext();
}

void HudPrimitives::updateTarget(int width, int height, int left, int top)
{
  guiContext->setTarget(width, height, left, top);
  updateViewFromContext();
}

void HudPrimitives::updateViewFromContext()
{
  screenWidth = guiContext->screenWidth;
  screenHeight = guiContext->screenHeight;
  screenWidthRcp2 = 2.0f / screenWidth;
  screenHeightRcp2 = 2.0f / screenHeight;

  viewY = guiContext->viewY;
  viewH = guiContext->viewH;
  float sx = guiContext->viewX;
  float sw = guiContext->viewW;
  viewX = int(sx);
  viewW = int(sw);
  viewWidthRcp2 = 2.0f / sw;
  viewHeightRcp2 = 2.0f / viewH;
  setTransform2d(depth2d, tm2d, projTm2d);
}

int HudPrimitives::beginChunk() { return guiContext->beginChunk(); }

void HudPrimitives::endChunk() { guiContext->endChunk(); }

void HudPrimitives::beginRender()
{
  G_ASSERT(!isInHudRender);
  isInHudRender = true;
  updateViewFromContext();
  guiContext->start_raw_layer();
  state().currentTexture = BAD_TEXTUREID;
  state().currentBlueToAlpha = Color4(0.f, 0.f, 0.f, 1.f);
  setState();
  guiContext->setBuffer(1);
}

void HudPrimitives::endRender()
{
  G_ASSERT(isInHudRender);
  flush();
  guiContext->setBuffer(0);
  guiContext->end_raw_layer();
  isInHudRender = false;
}

void HudPrimitives::beginRenderImm()
{
  G_ASSERT(!isInHudRender);
  isInHudRender = true;
  updateTarget();
  guiContext->beginChunkImm();
  guiContext->start_raw_layer();
  guiContext->setBuffer(1);
}

void HudPrimitives::endRenderImm()
{
  G_ASSERT(isInHudRender);
  flush();
  guiContext->end_raw_layer();
  guiContext->endChunkImm();
  isInHudRender = false;
}

void HudPrimitives::renderChunk(int chunk_id)
{
  G_ASSERT(!isInHudRender);
  guiContext->renderChunk(chunk_id);
}

HudPrimitives::RenderMode HudPrimitives::setRenderMode(HudPrimitives::RenderMode mode)
{
  RenderMode old = (RenderMode)state().renderMode;
  if (old != mode)
  {
    state().renderMode = mode;
    setState();
  }
  return old;
}

void HudPrimitives::setStencil(int stencil_ref_value)
{
  int old = state().stencilRefValue;
  if (old != stencil_ref_value)
  {
    state().stencilRefValue = stencil_ref_value;
    setState();
  }
}

void HudPrimitives::updateState(TEXTUREID texture_id)
{
  if (texture_id != state().currentTexture)
  {
    if (texture_id == BAD_TEXTUREID)
      texture_id = getWhiteTexId();

    if (texture_id != state().currentTexture)
      state().currentBlueToAlpha = Color4(0.f, 0.f, 0.f, 1.f);

    state().currentTexture = texture_id;
    setState();
  }
}


void HudPrimitives::beforeReset()
{
  if (ownContext)
    del_it(guiContext);

  guiContext = NULL;
}


void HudPrimitives::afterReset() {}

// stereo_depth for driver-side 3D

#define PREPARE_FILL_PARAMS()                             \
  float viewOfsX = (float(viewX)) * -viewWidthRcp2 - 1.f; \
  float viewOfsY = (float(viewY)) * viewHeightRcp2 + 1.f;

#define FILL_GUIVTX(data, idx, xx, yy, zz, ww, tcx, tcy, stereo_depth, cc)  \
  data[idx].pos.x = stereo_depth * (xx * viewWidthRcp2 + viewOfsX);         \
  data[idx].pos.y = stereo_depth * (viewOfsY - yy * viewHeightRcp2) * 1.0f; \
  data[idx].pos.z = stereo_depth * zz;                                      \
  data[idx].pos.w = stereo_depth * ww;                                      \
  data[idx].tuv.x = tcx;                                                    \
  data[idx].tuv.y = tcy;                                                    \
  data[idx].color = cc;

void HudPrimitives::updateProjectedQuadVB(HudVertex *data, const E3DCOLOR &color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
  const Point4 &p3, const Point2 &tc0, const Point2 &tc1, const Point2 &tc2, const Point2 &tc3, const E3DCOLOR *vertex_colors)
{
#define SET_GUI_VTX(data, idx, p, tc, cc)                                                               \
  data[idx].pos = Point4(p.x - (viewX)*viewWidthRcp2, (p.y + (viewY)*viewHeightRcp2) * 1.0f, p.z, p.w); \
  data[idx].tuv = tc;                                                                                   \
  data[idx].color = cc;

  SET_GUI_VTX(data, 0, p0, tc0, vertex_colors ? vertex_colors[0] : color);
  SET_GUI_VTX(data, 1, p1, tc1, vertex_colors ? vertex_colors[1] : color);
  SET_GUI_VTX(data, 2, p2, tc2, vertex_colors ? vertex_colors[2] : color);
  SET_GUI_VTX(data, 3, p3, tc3, vertex_colors ? vertex_colors[3] : color);

#undef SET_GUI_VTX
}

void HudPrimitives::renderQuadScreenSpaceDeprecated(TEXTUREID texture_id, E3DCOLOR color, const Point4 &p0, const Point4 &p1,
  const Point4 &p2, const Point4 &p3, const Point2 &tc0, const Point2 &tc1, const Point2 &tc2, const Point2 &tc3,
  const E3DCOLOR *vertex_colors)
{
  G_ASSERT(isInHudRender);

  updateState(texture_id);
  HudVertex *vtx = guiContext->qCacheAllocT<HudVertex>(1);

#define FILL_GUIVTX42(data, idx, p, t, stereo_depth, cc) FILL_GUIVTX(data, idx, p.x, p.y, p.z, p.w, t.x, t.y, stereo_depth, cc)
  PREPARE_FILL_PARAMS();
  FILL_GUIVTX42(vtx, 0, p0, tc0, 1, vertex_colors ? vertex_colors[0] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 1, p1, tc1, 1, vertex_colors ? vertex_colors[1] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 2, p2, tc2, 1, vertex_colors ? vertex_colors[2] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 3, p3, tc3, 1, vertex_colors ? vertex_colors[3] : e3dcolor_mul(color, colorMul));
#undef FILL_GUIVTX42
}

void HudPrimitives::renderQuadScreenSpace3d(TEXTUREID texture_id, E3DCOLOR color, const Point4 &p0, const Point4 &p1, const Point4 &p2,
  const Point4 &p3, const Point2 &tc0, const Point2 &tc1, const Point2 &tc2, const Point2 &tc3, const E3DCOLOR *vertex_colors)
{
  G_ASSERT(isInHudRender);

  updateState(texture_id);
  HudVertex *vtx = guiContext->qCacheAllocT<HudVertex>(1);

#define FILL_GUIVTX42(data, idx, p, t, cc)                          \
  data[idx].pos.x = p.w * (p.x * viewWidthRcp2 + viewOfsX);         \
  data[idx].pos.y = p.w * (viewOfsY - p.y * viewHeightRcp2) * 1.0f; \
  data[idx].pos.z = p.z;                                            \
  data[idx].pos.w = p.w;                                            \
  data[idx].tuv.x = t.x;                                            \
  data[idx].tuv.y = t.y;                                            \
  data[idx].color = cc;

  PREPARE_FILL_PARAMS();
  FILL_GUIVTX42(vtx, 0, p0, tc0, vertex_colors ? vertex_colors[0] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 1, p1, tc1, vertex_colors ? vertex_colors[1] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 2, p2, tc2, vertex_colors ? vertex_colors[2] : e3dcolor_mul(color, colorMul));
  FILL_GUIVTX42(vtx, 3, p3, tc3, vertex_colors ? vertex_colors[3] : e3dcolor_mul(color, colorMul));
#undef FILL_GUIVTX42
}

void HudPrimitives::renderQuad(const HudTexElem &elem, const E3DCOLOR color, dag::ConstSpan<float> pos, dag::ConstSpan<E3DCOLOR> col,
  const Coords coords, const float depth_2d)
{
  G_ASSERT(isInHudRender);
  updateState(elem.skinTex);
  HudVertex *vtx = guiContext->qCacheAllocT<HudVertex>(1);

  Point4 p[4];
  for (int pNo = 0; pNo < 4; ++pNo)
  {
    switch (coords)
    {
      case COORDS_2D: p[pNo] = Point4(pos[pNo * 2 + 0], pos[pNo * 2 + 1], depth_2d, 1.0f) * globTm2d; break;
      case COORDS_3D: p[pNo] = Point4(pos[pNo * 3 + 0], pos[pNo * 3 + 1], pos[pNo * 3 + 2], 1.0f) * getGlobTm3d(); break;
      case COORDS_2D_SCREEN_SPACE:
        p[pNo] = Point4(pos[pNo * 2 + 0] / width * 2.0f - 1.0f, -(pos[pNo * 2 + 1] / height * 2.0f - 1.0f), 0.0f, 1.0f);
        break;
      case COORDS_4D_CLIP_SPACE: p[pNo] = Point4(pos[pNo * 4 + 0], pos[pNo * 4 + 1], pos[pNo * 4 + 2], pos[pNo * 4 + 3]); break;
    };
  }

  updateProjectedQuadVB(vtx, e3dcolor_mul(color, colorMul), p[0], p[1], p[2], p[3], elem.tc[0], elem.tc[1], elem.tc[2], elem.tc[3],
    col.data());
}

template <typename T>
static double count_poly_area(dag::ConstSpan<T> points)
{
  int n = points.size();
  double ret = 0.0f;
  for (int p = n - 1, q = 0; q < n; p = q++)
  {
    Point2 pval = Point2::xy(points[p]);
    Point2 qval = Point2::xy(points[q]);
    ret += (pval.x * qval.y - qval.x * pval.y);
  }
  return (ret * 0.5f);
}

static bool is_inside(const Point2 &A, const Point2 &B, const Point2 &C, const Point2 &P)
{
  double ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  double cCROSSap, bCROSScp, aCROSSbp;

  ax = C.x - B.x;
  ay = C.y - B.y;
  bx = A.x - C.x;
  by = A.y - C.y;
  cx = B.x - A.x;
  cy = B.y - A.y;
  apx = P.x - A.x;
  apy = P.y - A.y;
  bpx = P.x - B.x;
  bpy = P.y - B.y;
  cpx = P.x - C.x;
  cpy = P.y - C.y;

  aCROSSbp = ax * bpy - ay * bpx;
  cCROSSap = cx * apy - cy * apx;
  bCROSScp = bx * cpy - by * cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
}

template <typename T>
static bool snip(dag::ConstSpan<T> points, int u, int v, int w, int n, int *V)
{
  int p;
  Point2 A = Point2::xy(points[V[u]]);
  Point2 B = Point2::xy(points[V[v]]);
  Point2 C = Point2::xy(points[V[w]]);
  if ((((B.x - A.x) * (C.y - A.y)) - ((B.y - A.y) * (C.x - A.x))) < 0)
    return false;
  for (p = 0; p < n; p++)
  {
    if ((p == u) || (p == v) || (p == w))
      continue;
    Point2 P = Point2::xy(points[V[p]]);
    if (is_inside(A, B, C, P))
      return false;
  }
  return true;
}

template <typename T>
static void triangulateT(dag::ConstSpan<T> p, Tab<HudIndexType> &indices)
{
  const int n = p.size();
  if (n < 3)
    return;
  int *V = new int[n];
  if (0.0f < count_poly_area(p))
  {
    for (int v = 0; v < n; v++)
      V[v] = v;
  }
  else
  {
    for (int v = 0; v < n; v++)
      V[v] = (n - 1) - v;
  }
  int nv = n;
  int count = 2 * nv;
  for (int m = 0, v = nv - 1; nv > 2;)
  {
    if (0 >= (count--))
    {
      delete[] V;
      return;
    }

    int u = v;
    if (nv <= u)
      u = 0;
    v = u + 1;
    if (nv <= v)
      v = 0;
    int w = v + 1;
    if (nv <= w)
      w = 0;

    if (snip(p, u, v, w, nv, V))
    {
      int a, b, c, s, t;
      a = V[u];
      b = V[v];
      c = V[w];
      indices.push_back(HudIndexType(a));
      indices.push_back(HudIndexType(b));
      indices.push_back(HudIndexType(c));
      m++;
      for (s = v, t = v + 1; t < nv; s++, t++)
        V[s] = V[t];
      nv--;
      count = 2 * nv;
    }
  }

  delete[] V;
}

static void triangulate(dag::ConstSpan<Point4> p, Tab<HudIndexType> &indices) { triangulateT(p, indices); }

static void triangulate(dag::ConstSpan<Point2> p, Tab<HudIndexType> &indices) { triangulateT(p, indices); }

void HudPrimitives::renderPoly(TEXTUREID texture_id, E3DCOLOR color, dag::ConstSpan<Point2> p, dag::ConstSpan<Point2> tc)
{
  G_ASSERT(isInHudRender);
  G_ASSERT(p.size() == tc.size());

  updateState(texture_id);
  flush();

  Tab<HudPrimitives::HudVertex> vtx(framemem_ptr());
  Tab<HudIndexType> ind(framemem_ptr());
  vtx.resize(p.size());

  triangulate(p, ind);
  if (ind.empty())
    return;
  G_ASSERT((ind.size() % 3) == 0);

  PREPARE_FILL_PARAMS();

  color = e3dcolor_mul(color, colorMul);
  HudPrimitives::HudVertex *data = &vtx[0];
  for (int i = 0; i < p.size(); i++)
  {
    FILL_GUIVTX(data, i, p[i].x, p[i].y, 0.f, 1.f, tc[i].x, tc[i].y, 1.0f, color);
  }

  guiContext->draw_faces(&vtx[0], p.size(), &ind[0], ind.size() / 3);
}

void HudPrimitives::renderTriFan(TEXTUREID texture_id, E3DCOLOR color, dag::ConstSpan<Point4> p, dag::ConstSpan<Point2> tc)
{
  G_ASSERT(isInHudRender);
  G_ASSERT(p.size() == tc.size());
  G_ASSERTF(p.size() >= 3, "Trifan is built of at least three vertices");

  updateState(texture_id);
  flush();

  int numTris = p.size() - 2;

  Tab<HudVertex> vtx(framemem_ptr());
  Tab<HudIndexType> ind(framemem_ptr());
  vtx.resize(p.size());
  ind.resize(numTris * 3);

  PREPARE_FILL_PARAMS();

  color = e3dcolor_mul(color, colorMul);
  HudPrimitives::HudVertex *data = &vtx[0];
  for (int i = 0; i < p.size(); i++)
  {
    FILL_GUIVTX(data, i, p[i].x, p[i].y, p[i].z, p[i].w, tc[i].x, tc[i].y, 1.0f, color);
  }

  for (int i = 0; i < numTris; i++)
  {
    ind[i * 3 + 0] = 0;
    ind[i * 3 + 1] = i + 1;
    ind[i * 3 + 2] = i + 2;
  }

  guiContext->draw_faces(&vtx[0], p.size(), &ind[0], numTris);
}

Point2 HudPrimitives::getColoredTextBBox(const char *text, int font_no)
{
  Point2 result;
  renderOrMeasureColoredTextFx(0, 0, font_no, E3DCOLOR_MAKE(255, 255, 255, 255), text, 0, -1, 0, 0, 0,
    E3DCOLOR_MAKE(255, 255, 255, 255), 1.f, NULL, &result);

  return result;
}

void HudPrimitives::renderColoredTextFx(int x, int y, int font_no, E3DCOLOR color, const char *text, float pixelLen, int len,
  int align, int fx_type, int fx_offset, E3DCOLOR fx_color, int fx_scale, const char *font_tex_2 /*= NULL*/)
{
  renderOrMeasureColoredTextFx(x, y, font_no, color, text, pixelLen, len, align, fx_type, fx_offset, fx_color, fx_scale, font_tex_2);
}

void HudPrimitives::renderOrMeasureColoredTextFx(int x, int y, int font_no, E3DCOLOR color, const char *text, float pixelLen, int len,
  int align, int fx_type, int fx_offset, E3DCOLOR fx_color, int fx_scale, const char *font_tex_2 /*= NULL*/,
  Point2 *out_size /*= NULL*/) // uses out_size to return text size or NULL to render it
{
  G_UNREFERENCED(len);
  StdGuiFontContext c(font_no);
  float curX = int(align < 0 ? x : align ? floor(x - pixelLen) : floor(x - pixelLen * 0.5f));
  const char *subStr = text;
  const char *escPtr = strchr(subStr, '\x1b');
  E3DCOLOR curColor = color;
  bool inEsc = false;
  if (out_size)
    out_size->x = out_size->y = 0.f;

  do
  {
    if (escPtr != subStr && subStr[0] != 0) // non empty string
    {
      int len_ = escPtr ? (escPtr - subStr) : int(strlen(subStr));
      // access rfont
      Point2 size = get_str_bbox(subStr, len_, c).width();
      if (out_size)
      {
        // without render
        out_size->x += size.x;
        out_size->y = max(out_size->y, size.y);
      }
      else
        renderTextFx(int(curX), y, font_no, curColor, subStr, len_, -1, fx_type, fx_offset, fx_color, fx_scale, font_tex_2);
      curX += size.x;
    }

    if (!escPtr)
      break;

    if (!inEsc)
    {
      char color_id = extractColorFromText(escPtr);
      G_ASSERTF(color_id < uint32_t(ECT_NUM), "%s trying to render bad escaped text '%s'", __FUNCTION__, text);
      if (color_id >= uint32_t(ECT_NUM))
        break;
      E3DCOLOR new_color = colorCodes[color_id];
      float alpha = float(color.a) / 255;
      curColor = E3DCOLOR_MAKE(int(new_color.r * alpha), int(new_color.g * alpha), int(new_color.b * alpha), color.a);
      subStr = escPtr + 4;
      escPtr = strchr(subStr, '\x1b');
      G_ASSERT(escPtr != NULL); // should be closing esc
      inEsc = true;
    }
    else
    {
      inEsc = false;
      subStr = escPtr + 1;
      escPtr = strchr(subStr, '\x1b');
      curColor = color;
    }
  } while (1);
}

static uint32_t build_hash(const char *p, int font_id, float scale = 1.f)
{
  uint32_t result = fnv1_step<32>(font_id);
  result = fnv1_step<32>(unsigned(scale * 64), result);
  return str_hash_fnv1<32>(p, result);
}

static uint32_t build_hash(const char *p, int font_id, float scale, int fx_type, int fx_offset, E3DCOLOR fx_color, int fx_scale,
  const char *font_tex_2)
{
  uint32_t result = build_hash(p, font_id, scale);
  result = fnv1_step<32>(fx_type, result);
  result = fnv1_step<32>(fx_offset, result);
  result = fnv1_step<32>(fx_color.u, result);
  result = fnv1_step<32>(fx_scale, result);
  if (font_tex_2)
    result = str_hash_fnv1<32>(font_tex_2, result);
  return result;
}

void HudPrimitives::renderTextEx(int x, int y, E3DCOLOR color, int font_id,
  int align, // -1 - left, 0 - center, 1 - right
  const char *text, int len, TextBufferHandle *inout_handle)
{
  G_ASSERT(isInHudRender);

  if (font_id < 0)
    return;

  if (!text || !(*text))
    return;

  if (align == 0 || align == 1)
  {
    guiContext->set_font(font_id);
    BBox2 bbox = get_str_bbox(text, len);
    float pixelLen = bbox.lim[1].x - bbox.lim[0].x;
    x = int(align ? floor(x - pixelLen) : floor(x - pixelLen / 2));
  }
  else if (align != -1)
    DAG_FATAL("Unknown align value '%d'", align);

  renderText(x, y, font_id, color, text, len, inout_handle);
}

void HudPrimitives::renderText(int x, int y, int font_no, E3DCOLOR color, const char *text, int len, TextBufferHandle *inout_handle)
{
  G_ASSERT(isInHudRender);

  if (!text[0])
    return;

  TextBuffer tmp, *buffer = &tmp;
  if (inout_handle)
  {
    uint32_t hash = build_hash(text, font_no);
    if (inout_handle != &HANDLE_STATIC)
    {
      if (*inout_handle != hash)
        textBuffers.erase(*inout_handle);
      *inout_handle = hash;
    }
    buffer = &textBuffers[hash];
  }

  if (!buffer->isReady || !StdGuiRender::check_str_buf_ready(buffer->tex_qcnt))
  {
    Tab<wchar_t> wbuf(framemem_ptr());
    const wchar_t *text_u = convert_utf8_to_u16_buf(wbuf, text, len);
    replace_tabs_with_zwspaces(wbuf.data());
    uint32_t textLen = (int)wcslen(text_u);
    Point2 posOffset = Point2(x, y);

    StdGuiRender::set_font(font_no);
    buffer->isReady = StdGuiRender::draw_str_scaled_u_buf(buffer->qv, buffer->tex_qcnt, DSBFLAG_rel, 1.f, text_u, textLen);
  }

  if (!buffer->isReady || buffer->tex_qcnt.size() < 2)
    return;

  const GuiVertex *cur_qv = buffer->qv.data();
  for (const uint16_t *tq = &buffer->tex_qcnt[1], *tq_end = buffer->tex_qcnt.data() + buffer->tex_qcnt.size(); tq < tq_end; tq += 2)
  {
    int full_qnum = tq[1];
    if (!full_qnum)
      continue;

    TEXTUREID texture_id = D3DRESID::fromIndex(tq[0]);
    if (texture_id != state().currentTexture)
    {
      state().currentTexture = texture_id;
      state().currentBlueToAlpha = Color4(1.f, 0.f, 0.f, 0.f);
      setState();
    }

    for (; full_qnum > 0; cur_qv += 4, full_qnum--)
    {
      Point2 pos_lt(cur_qv[0].px * GUI_POS_SCALE_INV + x, cur_qv[0].py * GUI_POS_SCALE_INV + y);
      Point2 pos_rb(cur_qv[1].px * GUI_POS_SCALE_INV + x, cur_qv[2].py * GUI_POS_SCALE_INV + y);
      Point2 texcoord_lt(cur_qv[0].tc0u / 4096.f, cur_qv[0].tc0v / 4096.f);
      Point2 texcoord_rb(cur_qv[1].tc0u / 4096.f, cur_qv[2].tc0v / 4096.f);

      HudVertex *vtx = guiContext->qCacheAllocT<HudVertex>(1);
      updateProjectedQuadVB(vtx, e3dcolor_mul(color, colorMul), Point4(pos_lt.x, pos_lt.y, 0.0f, 1.0f) * globTm2d,
        Point4(pos_lt.x, pos_rb.y, 0.0f, 1.0f) * globTm2d, Point4(pos_rb.x, pos_rb.y, 0.0f, 1.0f) * globTm2d,
        Point4(pos_rb.x, pos_lt.y, 0.0f, 1.0f) * globTm2d, Point2(texcoord_lt.x, texcoord_lt.y), Point2(texcoord_lt.x, texcoord_rb.y),
        Point2(texcoord_rb.x, texcoord_rb.y), Point2(texcoord_rb.x, texcoord_lt.y), NULL);
    }
  }
}

void HudPrimitives::getBufferedText(BufferedText &out_buffered_text, int font_no, int align, const char *text)
{
  G_UNREFERENCED(align);
  G_UNREFERENCED(text);
  G_UNREFERENCED(font_no);
  G_UNREFERENCED(out_buffered_text);
}

void HudPrimitives::renderBufferedText(int x, int y, E3DCOLOR color, int font_no, int align, const char *text, float z, float w)
{
  G_UNREFERENCED(x);
  G_UNREFERENCED(y);
  G_UNREFERENCED(font_no);
  G_UNREFERENCED(align);
  G_UNREFERENCED(text);
  G_UNREFERENCED(z);
  G_UNREFERENCED(w);
  color = color;
}

void HudPrimitives::renderBufferedText(int x, int y, E3DCOLOR color, const BufferedText *buffered_text, float z, float w)
{
  G_UNREFERENCED(x);
  G_UNREFERENCED(y);
  G_UNREFERENCED(buffered_text);
  G_UNREFERENCED(z);
  G_UNREFERENCED(w);
  color = color;
}

void HudPrimitives::clear() { textBuffers.clear(); }

void HudPrimitives::renderTextFx(float x, float y, int font_no, E3DCOLOR color, const char *text, int len, int align, int fx_type,
  int fx_offset, E3DCOLOR fx_color, int fx_scale, const char *font_tex_2, float scale, int valign, bool round_coords, Pivot pivot,
  TextBufferHandle *inout_handle)
{
  G_ASSERT(isInHudRender);

  if (!text[0] || color.a == 0)
    return;

  if (fx_type == 0 && (scale <= 0.f || scale == 1.0f))
  {
    renderTextEx(x, y, color, font_no, align, text, -1, inout_handle);
    return;
  }

  TextBuffer tmp, *buffer = &tmp;
  if (inout_handle)
  {
    uint32_t hash = build_hash(text, font_no, scale, fx_type, fx_offset, fx_color, fx_scale, font_tex_2);
    if (inout_handle != &HANDLE_STATIC)
    {
      if (*inout_handle != hash)
        textBuffers.erase(*inout_handle);
      *inout_handle = hash;
    }
    buffer = &textBuffers[hash];
  }

  // ZMode is set externally to text rendering and we MUST restore it after changing buffers:
  // otherwise calls to setZMode become useless each time we renderTextFx in the middle of the frame,
  // e.g. some parts of depth-tested HUD will be rendered with depth test and some not, depending on call order.
  bool oldWriteToZ = guiContext->guiState.fontAttr.writeToZ;
  int oldTestDepth = guiContext->guiState.fontAttr.testDepth;
  // flush();
  // FIXME: lazy buffer change
  guiContext->setBuffer(0);
  guiContext->setZMode(oldWriteToZ, oldTestDepth);

  fx_color.a = (int)fx_color.a * color.a / 255;
  guiContext->set_draw_str_attr((FontFxType)fx_type, fx_offset, fx_offset, fx_color, fx_scale);

  if (!buffer->isReady || !StdGuiRender::check_str_buf_ready(buffer->tex_qcnt))
  {
    Tab<wchar_t> wbuf(framemem_ptr());
    const wchar_t *text_u = convert_utf8_to_u16_buf(wbuf, text, len);
    replace_tabs_with_zwspaces(wbuf.data());
    if (text_u && len > 0)
      len = (int)wcslen(text_u);

    if (font_tex_2)
    {
      TEXTUREID fontTex2 = guiContext->guiState.fontAttr.getTex2();
      TEXTUREID t2 = add_managed_texture(font_tex_2);
      d3d::SamplerHandle fontSampler2 = guiContext->guiState.fontAttr.getTex2Sampler();
      if (t2 != fontTex2)
      {
        if (fontTex2 != BAD_TEXTUREID)
          release_managed_tex(fontTex2);
        fontTex2 = t2;
        fontSampler2 = get_texture_separate_sampler(fontTex2);
        acquire_managed_tex(fontTex2);
        guiContext->guiState.fontAttr.setTex2(fontTex2, fontSampler2);
      }
      guiContext->set_draw_str_texture(fontTex2, fontSampler2);
    }

    guiContext->set_font(font_no);

    G_ASSERT(guiContext->curRenderFont.font);

    float locScale = (scale <= 0) ? 1 : scale;
    buffer->isReady = guiContext->draw_str_scaled_u_buf(buffer->qv, buffer->tex_qcnt, DSBFLAG_rel, locScale, text_u, len);
  }

  if (align == 0 || align == 1 || valign == 0 || valign == 1)
  {
    guiContext->set_font(font_no);
    Point2 textSize = get_str_bbox(text, len).width() * (scale > 0.f ? scale : 1.f);
    if (align == 0 || align == 1)
      x = (align ? (x - textSize.x) : (x - textSize.x / 2));
    if (valign == 0 || valign == 1)
      y = (valign ? (y - textSize.y) : (y + textSize.y / 2));

    if (round_coords)
    {
      x = floorf(x);
      y = floorf(y);
    }
  }

  if (buffer->isReady)
  {
    guiContext->set_color(color);
    guiContext->goto_xy(x - viewX, y - viewY);
    guiContext->setRotViewTm(pivot.origin.x - viewX, pivot.origin.y - viewY, pivot.angleRadians, 0);
    guiContext->render_str_buf(buffer->qv, buffer->tex_qcnt, DSBFLAG_rel | DSBFLAG_allQuads | DSBFLAG_curColor);
  }

  guiContext->setBuffer(1);
  guiContext->setZMode(oldWriteToZ, oldTestDepth);
  guiContext->reset_draw_str_texture();
}

void HudPrimitives::setMask(TEXTUREID texture_id, const Point3 &matrix_line_0, const Point3 &matrix_line_1, const Point2 &tc0,
  const Point2 &tc1)
{
  state().maskTexId = texture_id;

  Point3 matrixLine0 = matrix_line_0 * (tc1.x - tc0.x);
  Point3 matrixLine1 = matrix_line_1 * (tc1.y - tc0.y);
  matrixLine0.z += tc0.x;
  matrixLine1.z += tc0.y;

  state().maskMatrix0 = Point3(matrixLine0.x, matrixLine0.y, matrixLine0.z); // 0.
  state().maskMatrix1 = Point3(matrixLine1.x, matrixLine1.y, matrixLine1.z); // 0.
  state().maskEnable = true;
  setState();
}

void HudPrimitives::setMask(TEXTUREID texture_id, const Point2 &center_pos, float angle, const Point2 &scale, const Point2 &tc0,
  const Point2 &tc1)
{
  state().maskTexId = texture_id;

  Point2 centerPos((center_pos.x - viewX) * viewWidthRcp2 - 1.f, 1.f - (center_pos.y - viewY) * viewHeightRcp2);

  Matrix3 sacaleAndOffset;
  sacaleAndOffset.setcol(0, 1.f, 0.f, 0.f);
  sacaleAndOffset.setcol(1, 0.f, 1.f, 0.f);
  sacaleAndOffset.setcol(2, -centerPos.x, -centerPos.y, 1.f);

  Matrix3 rotation;
  rotation.setcol(0, cos(angle), sin(angle), 0.f);
  rotation.setcol(1, -sin(angle), cos(angle), 0.f);
  rotation.setcol(2, 0.f, 0.f, 1.f);

  Matrix3 texCenter;
  texCenter.setcol(0, 1.f / scale.x, 0.f, 0.f);
  texCenter.setcol(1, 0.f, -1.f / scale.y, 0.f);
  texCenter.setcol(2, 0.5f, 0.5f, 1.f);

  Matrix3 matrix = texCenter * rotation * sacaleAndOffset;

  setMask(texture_id, Point3(matrix[0][0], matrix[1][0], matrix[2][0]), Point3(matrix[0][1], matrix[1][1], matrix[2][1]), tc0, tc1);
}

void HudPrimitives::clearMask()
{
  state().maskEnable = false;
  setState();
}

void HudPrimitives::setColorMul(E3DCOLOR color_mul) { colorMul = color_mul; }

const TMatrix &HudPrimitives::getTransform2d(float /*stereo_depth*/) const { return tm2d; }

const TMatrix4 &HudPrimitives::getGlobTm2d(float /*stereo_depth*/) const { return globTm2d; }

void HudPrimitives::setTransform2d(float depth, const TMatrix &in_tm, const TMatrix4 &proj_tm)
{
  if (depth <= 0)
  {
    resetTransform2d();
    return;
  }

  depth2d = depth;
  tm2d = in_tm;
  projTm2d = proj_tm;
  globTm2d = TMatrix4(tm2d * make_ui3d_local_tm(depth, width, height, proj_tm[0][0], proj_tm[1][1])) * proj_tm;
}

void HudPrimitives::resetTransform2d()
{
  depth2d = 0.0f;
  tm2d = TMatrix::IDENT;
  projTm2d = TMatrix4::IDENT;
  globTm2d = TMatrix4(make_ui3d_local_tm(0.0f, width, height, 1.0f, 1.0f));
}

const TMatrix &HudPrimitives::getTransform3d() const { return tm3d; }

const TMatrix4 &HudPrimitives::getGlobTm3d() const { return useRelativeGlobTm ? globTm3dRelative : globTm3d; }

void HudPrimitives::setTransform3d(const TMatrix &in_tm, const TMatrix4 &proj_tm)
{
  tm3d = in_tm;
  globTm3d = TMatrix4(in_tm) * proj_tm;

  TMatrix tm_relative = in_tm;
  tm_relative.setcol(3, 0, 0, 0);
  globTm3dRelative = TMatrix4(tm_relative) * proj_tm;
}

void HudPrimitives::setUseRelativeGlobTm(bool use) { useRelativeGlobTm = use; }

void HudPrimitives::renderQuad(const HudTexElem &elem, const E3DCOLOR color, const Point2 &p0, const Point2 &p1, const Point2 &p2,
  const Point2 &p3, float depth_2d, const E3DCOLOR *vertex_colors)
{
  Point2 p[4] = {p0, p1, p2, p3};
  renderQuad(elem, color, dag::ConstSpan<float>(&p[0].x, 8), dag::ConstSpan<E3DCOLOR>(vertex_colors, vertex_colors ? 4 : 0), COORDS_2D,
    depth_2d);
}

void HudPrimitives::renderLine(E3DCOLOR color, const Point2 &p0, const Point2 &p1, float width)
{
  G_ASSERT(isInHudRender);

  Point2 ofs = Point2(-(p1.y - p0.y), p1.x - p0.x);
  ofs.normalize();
  ofs *= width;

  renderQuad(HudTexElem(getWhiteTexId()), color, Point2(p0.x - ofs.x, p0.y - ofs.y), Point2(p1.x - ofs.x, p1.y - ofs.y),
    Point2(p1.x + ofs.x, p1.y + ofs.y), Point2(p0.x + ofs.x, p0.y + ofs.y));
}

bool HudPrimitives::getScreenPos(const Point3 &world_pos, IPoint2 &screen_pos) const
{
  Point4 pos = Point4::xyz1(world_pos) * getGlobTm3d();
  bool visible = pos.z > 0 && pos.z < pos.w;
  ;
  bool negative = pos.w < 0;
  if (!is_equal_float(pos.w, 0.f))
    pos *= safeinv(pos.w);
  if (negative)
  {
    const Point2 backPos = -normalize(Point2(pos.x, pos.y)) * 10000.f;
    pos.x = backPos.x;
    pos.y = backPos.y;
  }
  screen_pos.set(width * (0.5f * pos.x + 0.5f), height * (-0.5f * pos.y + 0.5f));
  return visible;
}
