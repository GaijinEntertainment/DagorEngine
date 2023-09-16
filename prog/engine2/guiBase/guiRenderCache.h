/************************************************************************
  GUI rendering cache
************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GUIRENDERCACHE_H
#define __GUIRENDERCACHE_H

#include <gui/dag_stdGuiRender.h>
#include <3d/dag_texMgr.h>
#include <math/dag_color.h>
#include <string.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_carray.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_vector.h>

#define LOG_CACHE 0
#define LOG_DRAWS 0

// number of quads in each layer cache buffer
#define BUF_QUAD_COUNT  512
// max simultaneous vertex streams in single context
#define MAX_BUFFERS     4 /* pow2 */
#define MAX_SUB_BUFFERS 4 /* pow2 */

namespace StdGuiRender
{
class CachedBBox2
{
public:
  IPoint2 lim[2];
  inline CachedBBox2() { setempty(); }

  explicit inline CachedBBox2(const IBBox2 &b)
  {
    lim[0] = b.lim[0];
    lim[1] = b.lim[1];
  }

  inline void setempty()
  {
    lim[0] = IPoint2(32767, 32767);
    lim[1] = IPoint2(-32767, -32767);
  }

  inline bool operator&(const IBBox2 &b) const
  {
    if (b.isEmpty() || (b.lim[0].x >= lim[1].x) || (b.lim[0].y >= lim[1].y) || (b.lim[1].x <= lim[0].x) || (b.lim[1].y <= lim[0].y))
      return 0;
    return 1;
  }
};

typedef Tab<CachedBBox2> BoxList;

// unbuffered linear VB/IB buffer
struct GuiVertexData
{
  char *vertices;
  IndexType *indices;

  int currVertex;
  int currIndex;

  int startVertex;
  int startIndex;

  int verticesTotal;
  int indicesTotal;

  int stride;
  unsigned logFrame;

  Vbuffer *vb;
  Ibuffer *ib;

  Tab<CompiledShaderChannelId> channels;

  GuiVertexData() :
    vb(NULL),
    ib(NULL),
    channels(midmem_ptr()),
    stride(0),
    logFrame(0),
    vertices(NULL),
    indices(NULL),
    verticesTotal(0),
    indicesTotal(0)
  {
    reset();
  }

  void setFormat(const CompiledShaderChannelId *channels, int num_chans);
  bool create(int num_vertices, int num_indices, const char *name);
  void destroy();
  void lock(bool need_discard);
  void unlock();

  void reset()
  {
    startVertex = 0;
    startIndex = 0;
    currVertex = 0;
    currIndex = 0;
  }

  size_t elemSize() const { return stride; }
  size_t indexSize() const { return sizeof(IndexType); }

  char *currVertexPtr() const { return &vertices[currVertex * stride]; }
  IndexType *currIndexPtr() const { return &indices[currIndex]; }

  bool hasSpace(int num_vert, int num_ind) const
  {
    return ((verticesTotal - currVertex) >= num_vert) && ((indicesTotal - currIndex) >= num_ind);
  }
  bool hasSpaceForQuads(int num_quads) const
  {
    if (D3D_HAS_QUADS)
      return hasSpace(num_quads * 4, 0);
    else
      return hasSpace(num_quads * 4, num_quads * 6);
  }

  void advance(int num_vertices, int num_indices)
  {
    currVertex += num_vertices;
    currIndex += num_indices;
  }

  void fillQuadIB(int dst_index, int vertex)
  {
    IndexType *data = &indices[dst_index];
    data[0] = vertex + 0;
    data[1] = vertex + 1;
    data[2] = vertex + 2;
    data[3] = vertex + 3;
    data[4] = vertex + 0;
    data[5] = vertex + 2;
  }

  void updateQuadIB()
  {
    if (!D3D_HAS_QUADS)
    {
      // indexed TRILIST
      fillQuadIB(currIndex, currVertex);
      currIndex += 6;
    }
    currVertex += 4;
  }

  // make sure VB has space
  void logOverflow(int verts, int inds);

  void appendQuadsUnsafe(const void *__restrict src, int num_quads)
  {
    memcpy(currVertexPtr(), src, num_quads * 4 * stride);
    for (int i = 0; i < num_quads; i++)
      updateQuadIB();
  }
  bool appendQuads(const void *__restrict src, int num_quads)
  {
    if (!hasSpaceForQuads(num_quads))
    {
      logOverflow(num_quads * 4, 0);
      return false;
    }

    if (!vertices || (!D3D_HAS_QUADS && !indices))
      return false;

    appendQuadsUnsafe(src, num_quads);
    return true;
  }

  void appendFacesUnsafe(const void *__restrict vsrc, int num_verts, const IndexType *isrc, int num_indices)
  {
    memcpy(currVertexPtr(), vsrc, num_verts * stride);
    memcpy(currIndexPtr(), isrc, num_indices * sizeof(IndexType));
    currVertex += num_verts;
    currIndex += num_indices;
  }
  bool appendFaces(const void *__restrict vsrc, int num_verts, const IndexType *isrc, int num_indices)
  {
    if (!hasSpace(num_verts, num_indices))
    {
      logOverflow(num_verts, num_indices);
      return false;
    }

    if (!vertices || !indices)
      return false;

    appendFacesUnsafe(vsrc, num_verts, isrc, num_indices);
    return true;
  }
};

// caches similar quads in system ram
struct QuadBuffer
{
  eastl::unique_ptr<GuiVertex[]> quads;
  uint32_t quadsUsed = 0;
  uint32_t quadsTotal = 0;

  QuadBuffer(int num_quads) : quads(new GuiVertex[num_quads * 4]), quadsTotal(num_quads) {}

  bool hasSpaceForQuads(int num_quads) const { return (quadsUsed + num_quads) <= quadsTotal; }

  void appendQuads(const GuiVertex *__restrict verts, int num_quads)
  {
    G_ASSERT(quads);
    memcpy(&quads.get()[quadsUsed * 4], verts, sizeof(GuiVertex) * num_quads * 4);
    quadsUsed += num_quads;
  }
};

class RenderLayer
{
public:
  LayerParams params;
  uint16_t bufferIdx; // index in 'qBuffers'
  uint16_t boxIdx;    // index in 'boxes'

  RenderLayer(const LayerParams &p, int qbidx, int bidx) : bufferIdx(qbidx), boxIdx(bidx), params(p) {}

  // if it's a valid rect for this layer, return true
  bool checkRect(const IBBox2 &other, dag::ConstSpan<BoxList> boxes) const
  {
    const BoxList &boxList = boxes[boxIdx];
    for (int i = 0; i < boxList.size(); i++)
      if (boxList[i] & other)
        return false;
    return true;
  }
};

struct BufferedRenderer;
struct GuiViewportRect
{
  float l;
  float t;
  float w;
  float h;
};

// for StdGuiMaterial & GuiVertex caching
struct QuadCache
{
  Tab<RenderLayer> layers;
  Tab<QuadBuffer> qBuffers;
  Tab<BoxList> boxes;

  int usedQBuffers = 0;
  int usedBoxes = 0;

  BufferedRenderer *renderer;

  // current state (layers has individual layer params)
  // init by stdGuiRenderer
  // guiState.layerParams copied from last layer after flush()
  GuiState guiState;

  QuadCache(BufferedRenderer *rdr) : renderer(rdr)
  {
    getFreeQuadBuffer(); // preallocate first GUI buffer
    usedQBuffers = 0;

    guiState.params.reset();
    guiState.fontAttr.reset();
  }

  int getFreeQuadBuffer();
  int getFreeBoxList();

  // add vertices to the render cache
  void addQuads(const LayerParams &params, const IBBox2 &rect, const GuiVertex *verts, int num_quads);

  // calc BBox and add
  void addQuads(const LayerParams &params, const GuiVertex *verts, int num_quads);

  // uses local copy of layer params
  void addQuads(const GuiVertex *verts, int num_quads) { addQuads(guiState.params, verts, num_quads); };

  // clear cache (without flushing)
  void clear();
  // flush cached data to buffers and clear cache
  void flush();
};

struct DrawElem
{
  enum Command : uint8_t
  {
#if D3D_HAS_QUADS
    DRAW_QUADS,
    DRAW_INDEXED,
#else
    DRAW_INDEXED,
    DRAW_QUADS,
#endif
    DRAW_FAN,
    DRAW_LINESTRIP,
    EXEC_CMD // startIndex = cmd, extState = CallBackExtState
  };
  Command command;
  uint8_t shaderId_bufferId; // 4:4

  uint16_t view;
  uint16_t guiState;
  uint16_t extState;

  uint16_t primCount;
  IndexType startVertex;
  int startIndex; // 65535*1.5
};

// aliased to ExtState
struct CallBackExtState
{
  Point2 pos;
  Point2 size;
  RenderCallBack cb;
  uintptr_t data;
};

struct BufferedRenderer
{
  Tab<GuiShader *> shaders;

  //(current state is always last)
  Tab<GuiViewportRect> viewports;
  Tab<ExtState> extStates;
  Tab<GuiState> guiStates;
  Tab<DrawElem> drawElems;
  eastl::fixed_vector<int, 4, /*bOverflow*/ true> chunks; // drawElems chunks[N]..chunks[N+1]

  bool isInChunk;
  bool isInLock;

  int currentShaderId;
  int currentBufferId;

  // target params
  Point2 screenOrig;
  Point2 screenScale;
  static constexpr float screenPixelAR = 1.f;
  int targetWd = 0, targetHt = 0;

  QuadCache quadCache;

  carray<GuiVertexData, MAX_BUFFERS> buffers; //-V730_NOINIT
  carray<bool, MAX_BUFFERS> buffersDiscard;   //-V730_NOINIT
  carray<GuiShader *, MAX_BUFFERS> currentShaders;
  carray<GuiShader *, MAX_BUFFERS> defaultShaders;
  carray<ExtState *, MAX_BUFFERS> extStatePtrs;

  RenderCallBack callback;
  uintptr_t callbackData;

  BufferedRenderer();
  ~BufferedRenderer();

  bool createBuffer(int id, const CompiledShaderChannelId *channels, int num_chans, int gui_quads, int extra_indices,
    const char *name);

  void lock();
  void unlock();

  void validateShader(int buffer, GuiShader *mat);
  void setBufAndShader(int buffer, GuiShader *mat);

  void pushGuiState(GuiState *state) { guiStates.push_back(*state); }
  void pushExtState(ExtState *state) { extStates.push_back(*state); }
  void pushViewport(GuiViewportRect *viewport) { viewports.push_back(*viewport); }

  // set viewport rect to shader
  void pushViewportRect(float l, float t, float w, float h)
  {
    if (viewports.size() > 0)
    {
      GuiViewportRect &v = viewports.back();
      if (v.l == l && v.t == t && v.w == w && v.h == h)
        return;
    }

    GuiViewportRect viewport;
    viewport.l = l;
    viewport.t = t;
    viewport.w = w;
    viewport.h = h;
    pushViewport(&viewport);
  }

  bool copyQuads(const void *__restrict ptr, int num_quads)
  {
    G_ASSERT(currentBufferId >= 0);
    G_ASSERT(isInLock);
    return buffers[currentBufferId].appendQuads(ptr, num_quads);
  }

  bool copyFaces(const void *vertices, int num_verts, const IndexType *indices, int num_inds)
  {
    G_ASSERT(currentBufferId >= 0);
    G_ASSERT(isInLock);
    return buffers[currentBufferId].appendFaces(vertices, num_verts, indices, num_inds);
  }

  void execCommand(int command, const Point2 &pos, const Point2 &size, RenderCallBack cb = nullptr, uintptr_t data = 0);
  uintptr_t execCommandImmediate(int command, const Point2 &pos, const Point2 &size);

  void drawPrims(DrawElem::Command cmd, int num_prims);

  void drawQuads(int num_quads) { drawPrims(DrawElem::DRAW_QUADS, num_quads); }
  void drawIndexed(int num_faces) { drawPrims(DrawElem::DRAW_INDEXED, num_faces); }
  void drawFan(int num_faces) { drawPrims(DrawElem::DRAW_FAN, num_faces); }

  int beginChunk(); // return chunk_id 0-based
  void endChunk();
  void renderChunk(int chunk_id, int targetW, int targetH);
  void renderChunkCustom(int chunk_id, int targetW, int targetH, int replace_buffer, Ptr<ShaderElement> shaderElem);

  void resetBuffers();
};
} // namespace StdGuiRender


#endif //__GUIRENDERCACHE_H
