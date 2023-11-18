// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include "guiRenderCache.h"
#include <shaders/dag_dynShaderBuf.h>
#include <3d/dag_ringDynBuf.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_TMatrix4more.h>
#include <debug/dag_debug.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_globalSettings.h>

#define LOG_WRITES_FRAMES_RATE (60) // once per sec on 60FPS

namespace StdGuiRender
{
void GuiVertexData::setFormat(const CompiledShaderChannelId *chans, int num_chans)
{
  // copy channels
  clear_and_shrink(channels);
  append_items(channels, num_chans);
  for (int i = 0; i < num_chans; i++)
    channels[i] = chans[i];
  stride = dynrender::getStride(chans, num_chans);
}

bool GuiVertexData::create(int num_vertices, int num_indices, const char *name)
{
  G_ASSERTF(!vb && !ib, "vb=%p ib=%p, destroy() not called?", ib, vb);
  int minIndices = max(num_indices, 6); // dummy minimal index buffer
  int flag32 = (indexSize() > 2) ? SBCF_INDEX32 : 0;
  vb = d3d::create_vb(int(num_vertices * elemSize()), SBCF_DYNAMIC, name);
  ib = d3d::create_ib(int(minIndices * indexSize()), SBCF_DYNAMIC | flag32);
  if (vb && ib)
  {
    verticesTotal = num_vertices;
    indicesTotal = minIndices;
    return true;
  }

  destroy();
  return false;
}

void GuiVertexData::destroy()
{
  destroy_it(vb);
  destroy_it(ib);
}

void GuiVertexData::lock(bool need_discard)
{
  if (vb && ib)
  {
    int flag = need_discard ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE;
    if (vb->lock(0, 0, (void **)&vertices, flag | VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK))
    {
      if (ib->lock(0, 0, (void **)&indices, flag | VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK))
        return;

      vb->unlock();
    }
  }

  vertices = NULL;
  indices = NULL;
}

void GuiVertexData::unlock()
{
  if (vb && ib)
  {
    if (vertices)
      vb->unlock();
    if (indices)
      ib->unlock();
  }
}

void GuiVertexData::logOverflow(int verts, int inds)
{
  if (::dagor_frame_no() >= logFrame)
  {
    logwarn("StdGuiRender: vertex buffer fullfilled v:%d i:%d +v:%d +i:%d", currVertex, currIndex, verts, inds);
    logFrame = ::dagor_frame_no() + LOG_WRITES_FRAMES_RATE;
  }
  (void)verts;
  (void)inds;
}

// get free buffer. if buffer is not found - add new
int QuadCache::getFreeQuadBuffer()
{
  if (usedQBuffers >= qBuffers.size())
    qBuffers.push_back(QuadBuffer(BUF_QUAD_COUNT));

  QuadBuffer &qc = qBuffers[usedQBuffers++];
  qc.quadsUsed = 0;
  return &qc - qBuffers.data();
}

// get free box list. if list is not found - add new
int QuadCache::getFreeBoxList()
{
  if (usedBoxes >= boxes.size())
    boxes.push_back(BoxList{});
  BoxList &boxList = boxes[usedBoxes++];
  boxList.clear();
  return &boxList - boxes.data();
}

// add vertices to the render cache
// when rendering raw layer, cache is completely bypassed
void QuadCache::addQuads(const LayerParams &params, const IBBox2 &rect, const GuiVertex *verts, int num_quads)
{
#if LOG_CACHE
  debug("QC:addQuads (%d %d %d) %.4f %.4f - %.4f %.4f, [%d]", params.alphaBlend, params.texId, params.fontTexId, rect.lim[0].x,
    rect.lim[0].y, rect.lim[1].x, rect.lim[1].y, num_quads);
#endif

  const RenderLayer *layer;
  for (;;)
  {
    layer = NULL;
    // find the layer with compatible params
    if (layers.size() > 0 && layers.back().params.cmpEq(params))
    {
      layer = &layers.back();
    }
    else
    {
      for (int s = layers.size() - 1, i = s; i >= 0; i--)
        if (layers[i].params.cmpEq(params))
        {
          // check overlapping
          for (; s > i; s--)
            if (!layers[s].checkRect(rect, boxes))
              break;
          if (s > i)
            break;
          layer = &layers[i];
        }
    }

    if (!layer)
    {
      layer = &layers.push_back(RenderLayer(params, getFreeQuadBuffer(), getFreeBoxList()));
#if LOG_CACHE
      debug("QC:newLayer %d", layers.size() - 1);
#endif
    }

    QuadBuffer &layerBuffer = qBuffers[layer->bufferIdx];
    if (!layerBuffer.hasSpaceForQuads(num_quads))
    {
      flush();
      // FIXME: no layers
      while (num_quads > BUF_QUAD_COUNT)
      {
        layerBuffer.appendQuads(verts, BUF_QUAD_COUNT);
        verts += BUF_QUAD_COUNT;
        num_quads -= BUF_QUAD_COUNT;
        flush();
      }
      continue;
    }
    break;
  };

  // layer is valid
  qBuffers[layer->bufferIdx].appendQuads(verts, num_quads);
  boxes[layer->boxIdx].push_back(CachedBBox2(rect));
  //    debug("placed in layer: %X", layer);
}

void QuadCache::addQuads(const LayerParams &params, const GuiVertex *verts, int num_quads)
{
  IBBox2 bbox;
  for (int i = 0; i < num_quads * 4; i++)
    bbox += IPoint2(verts[i].px, verts[i].py);

  addQuads(params, bbox, verts, num_quads);
}

void QuadCache::flush()
{
  if (layers.size() == 0)
    return;

  G_ASSERT(renderer->currentBufferId >= 0);

#if LOG_CACHE
  debug("QC:flush frame %5d: render cache: %d layers:", dagor_frame_no(), layers.size());
#endif

  const LayerParams oldParams = guiState.params; // renderer->guiState;
  GuiState guiStateTmp;
  guiStateTmp.fontAttr = guiState.fontAttr;

  for (int i = 0; i < layers.size(); i++)
  {
    const RenderLayer &layer = layers[i];
    const QuadBuffer &layerBuffer = qBuffers[layer.bufferIdx];
    if (layerBuffer.quadsUsed == 0)
      continue;

#if LOG_CACHE
    debug("QC:drawlayer %2d: alpha=%d tex=%2d, fontTex=%2d fontFx=%d; qnum=%3d rects.count=%2d", i, layer.params.alphaBlend,
      layer.params.texId, layer.params.fontTexId, layer.params.fontFx, layerBuffer.quadsUsed, boxes[layer.boxIdx].size());
#endif

    guiStateTmp.params = layer.params;
    renderer->pushGuiState(&guiStateTmp);

    if (renderer->copyQuads(layerBuffer.quads.get(), layerBuffer.quadsUsed))
      renderer->drawQuads(layerBuffer.quadsUsed);
  }
  if (!guiStateTmp.params.cmpEq(oldParams))
  {
    guiStateTmp.params = oldParams;
    renderer->pushGuiState(&guiStateTmp);
  }

  clear();
}

// clear cache without rendering cached data
void QuadCache::clear()
{
  layers.clear();
  usedQBuffers = 0;
  usedBoxes = 0;
  //  currShader->set_texture_param(textureVarId, BAD_TEXTUREID);
}


BufferedRenderer::BufferedRenderer() :
  quadCache(this), shaders(midmem), viewports(midmem), extStates(midmem), guiStates(midmem), drawElems(midmem)
{
  callback = NULL;
  callbackData = 0;

  isInChunk = false;
  isInLock = false;
  currentShaderId = -1;
  currentBufferId = -1;

  screenOrig = Point2(0, 0);
  screenScale = Point2(1, 1);

  for (int i = 0; i < MAX_BUFFERS; i++)
  {
    buffers[i].reset();
    currentShaders[i] = NULL;
    defaultShaders[i] = NULL;
    extStatePtrs[i] = NULL;
    buffersDiscard[i] = false;
  }
}

BufferedRenderer::~BufferedRenderer()
{
  G_ASSERT(!isInLock && !isInChunk);

  for (int i = 0; i < buffers.size(); i++)
    buffers[i].destroy();
}

bool BufferedRenderer::createBuffer(int id, const CompiledShaderChannelId *channels, int num_chans, int gui_quads, int extra_indices,
  const char *name)
{
  if (id >= buffers.size())
    return false;
  GuiVertexData &buf = buffers[id];
  buf.destroy();
  buf.setFormat(channels, num_chans);
  buffersDiscard[id] = false;
  return buf.create(gui_quads * 4, gui_quads * 6 + extra_indices, name);
}

void BufferedRenderer::validateShader(int buffer, GuiShader *mat)
{
  GuiVertexData *buf = &buffers[buffer];
  if (!mat->material->checkChannels(buf->channels.data(), buf->channels.size()))
    fatal("StdGuiRender - invalid channels for shader '%s'!", (char *)mat->material->getShaderClassName());
}

void BufferedRenderer::setBufAndShader(int buffer, GuiShader *mat)
{
  if (currentBufferId == buffer && (currentShaderId >= 0 && shaders[currentShaderId] == mat))
    return;

  if (mat == NULL || buffer < 0 || buffer >= buffers.size())
  {
    currentShaderId = -1;
    currentBufferId = -1;
    return;
  }

  currentBufferId = buffer;

  for (int matId = 0; matId < shaders.size(); matId++)
    if (shaders[matId] == mat)
    {
      currentShaderId = matId;
      return;
    }

  shaders.push_back(mat);
  currentShaderId = shaders.size() - 1;
}

int block_on_dp = -1;

void BufferedRenderer::drawPrims(DrawElem::Command cmd, int num_prims)
{
  G_ASSERT(currentShaderId >= 0 && currentBufferId >= 0);

  DrawElem &de = drawElems.push_back();

  de.command = cmd;
  de.shaderId_bufferId = (currentShaderId << 4) | currentBufferId;

  int currentViewport = viewports.size() - 1;
  int currentGuiState = guiStates.size() - 1;
  int currentExtStat = extStates.size() - 1;

  G_ASSERT(currentViewport >= 0 && currentGuiState >= 0 && currentExtStat >= 0);
  de.view = currentViewport;
  de.guiState = currentGuiState;
  de.extState = currentExtStat;

  de.primCount = num_prims;
  de.startIndex = buffers[currentBufferId].startIndex;
  de.startVertex = buffers[currentBufferId].startVertex;
#if !D3D_HAS_QUADS
  if (de.command == DrawElem::DRAW_QUADS) // convert to draw_indexed
  {
    de.command = DrawElem::DRAW_INDEXED;
    de.startVertex = 0;
    de.primCount = de.primCount * 2;
  }
#endif

  buffers[currentBufferId].startIndex = buffers[currentBufferId].currIndex;
  buffers[currentBufferId].startVertex = buffers[currentBufferId].currVertex;
}

void BufferedRenderer::execCommand(int cmd, const Point2 &pos, const Point2 &size, RenderCallBack cb, uintptr_t data)
{
  DrawElem &de = drawElems.push_back();
  de.command = DrawElem::EXEC_CMD;

  int currentViewport = viewports.size() - 1;
  int currentGuiState = guiStates.size() - 1;
  int currentExtStat = extStates.size() - 1;

  ExtState old = extStates[currentExtStat];
  pushExtState(&old); // overwrite this for call
  pushExtState(&old); // restore old one
  CallBackExtState *cbes = (CallBackExtState *)&extStates[currentExtStat + 1];
  cbes->pos = pos;
  cbes->size = size;
  cbes->cb = cb;
  cbes->data = data;

  G_ASSERT(currentViewport >= 0 && currentGuiState >= 0 && currentExtStat >= 0);
  de.view = currentViewport;
  de.guiState = currentGuiState;
  de.extState = currentExtStat + 1;
  de.startIndex = cmd;
}

uintptr_t BufferedRenderer::execCommandImmediate(int cmd, const Point2 &pos, const Point2 &size)
{
  if (callback != NULL)
  {
    CallBackState cbs;
    cbs.pos = pos;
    cbs.size = size;
    cbs.data = callbackData;
    cbs.command = cmd;
    cbs.guiState = NULL;
    callback(cbs);
    if (cbs.data != callbackData) // Some data returned from callback.
      return cbs.data;
  }

  return -1;
}

void BufferedRenderer::lock()
{
  for (int i = 0; i < buffers.size(); i++)
  {
    buffers[i].lock(buffersDiscard[i]);
    buffersDiscard[i] = false;
  }
  isInLock = true;
}

void BufferedRenderer::unlock()
{
  isInLock = false;
  for (int i = 0; i < buffers.size(); i++)
    buffers[i].unlock();
}

void BufferedRenderer::resetBuffers()
{
  G_ASSERT(!isInChunk && !isInLock);

  quadCache.clear();
  viewports.clear();
  extStates.clear();
  guiStates.clear();
  drawElems.clear();
  chunks.clear();

  for (int i = 0; i < buffers.size(); i++)
  {
    GuiVertexData &buf = buffers[i];
    // If data from a single frame takes up less than half of the buffer, discard it less frequently, if more than half, discard each
    // frame.
    buffersDiscard[i] = (buf.startVertex > buf.verticesTotal / 2 || buf.startIndex > buf.indicesTotal / 2);
    if (buffersDiscard[i])
      buf.reset();
  }
}

int BufferedRenderer::beginChunk() // return chunk_id 0-base
{
  G_ASSERT(!isInChunk);
  G_ASSERT(!isInLock);
#if LOG_CACHE
  debug("BR:beginChunk");
#endif
  isInChunk = true;
  lock();
  return chunks.size();
}

void BufferedRenderer::endChunk()
{
  G_ASSERT(isInChunk);
  isInChunk = false;
  if (isInLock)
    unlock();
  chunks.push_back(drawElems.size());
#if LOG_CACHE
  debug("BR:endChunk");
#endif
}

void BufferedRenderer::renderChunk(int chunk_id, int targetW, int targetH)
{
  G_ASSERT(!isInChunk);

  if (chunk_id < 0 || chunk_id >= chunks.size())
    return;

  int drawElemBeg = chunk_id ? chunks[chunk_id - 1] : 0;
  int drawElemEnd = chunks[chunk_id];
  if (drawElemEnd <= drawElemBeg) // chunk is empty
    return;

#if LOG_CACHE
  debug("BR:renderChunk");
#endif

  const GuiViewportRect *oldvp = nullptr;
  int oldShaderId = -1;
  int oldViewport = -1;
  int oldGuiState = -1;
  int oldExtState = -1;
  for (int drawElemId = drawElemBeg; drawElemId < drawElemEnd; drawElemId++)
  {
    const DrawElem &de = drawElems[drawElemId];

    if (oldViewport != de.view)
    {
      const GuiViewportRect &vp = viewports[de.view];
      oldvp = &vp;
      d3d::setview(int((screenOrig.x + vp.l * screenScale.x) / screenPixelAR), int(screenOrig.y + vp.t * screenScale.y),
        int(vp.w * screenScale.x / screenPixelAR), int(vp.h * screenScale.y), 0, 1);
    }

    int shaderId = de.shaderId_bufferId >> 4;
    int bufferId = de.shaderId_bufferId & (MAX_BUFFERS - 1);
    DrawElem::Command command = de.command;

    if (EASTL_UNLIKELY(command == DrawElem::EXEC_CMD))
    {
      oldShaderId = -1;
      oldViewport = -1;
      oldGuiState = -1;
      oldExtState = -1;

      CallBackExtState *es = (CallBackExtState *)&extStates[de.extState];
      if ((callback != NULL) || (es->cb != NULL))
      {
        CallBackState cbs;
        cbs.pos = es->pos;
        cbs.size = es->size;
        cbs.command = de.startIndex;
        cbs.guiState = &guiStates[de.guiState];
        if (es->cb == nullptr)
        {
          cbs.data = callbackData;
          callback(cbs);
        }
        else
        {
          cbs.data = es->data;
          es->cb(cbs);
        }
      }
      continue;
    }

    if (oldShaderId != shaderId)
    {
      if (oldShaderId >= 0)
        shaders[oldShaderId]->cleanup();

      // new shader requires full variables update
      oldShaderId = shaderId;
      oldViewport = -1;
      oldGuiState = -1;
      oldExtState = -1;
    }

    if ((oldViewport != de.view) || (oldGuiState != de.guiState) || (oldExtState != de.extState))
    {
#if LOG_DRAWS
      debug("BR:SetStates #%d", drawElemId);
#endif
      G_FAST_ASSERT(oldvp >= viewports.data() && oldvp < (viewports.data() + viewports.size()));
      shaders[shaderId]->setStates(&oldvp->l, guiStates[de.guiState], &extStates[de.extState], oldViewport != de.view,
        oldGuiState != de.guiState, oldExtState != de.extState, NULL, NULL, targetW, targetH);
      oldViewport = de.view;
      oldGuiState = de.guiState;
      oldExtState = de.extState;
    }

    GuiVertexData &buf = buffers[bufferId];

#if D3D_HAS_QUADS
    if (command == DrawElem::DRAW_QUADS)
    {
#if LOG_DRAWS
      debug("BR:DRAW_QUADS %d, %d", de.startVertex, de.primCount);
#endif
      d3d::setvsrc(0, buf.vb, (int)buf.elemSize());
      d3d::draw(PRIM_QUADLIST, de.startVertex, de.primCount);
    }
    else
#endif
      if (command == DrawElem::DRAW_INDEXED)
    {
#if LOG_DRAWS
      debug("BR:DRAW_INDEXED %d, %d", de.startIndex, de.primCount);
#endif
      d3d::setvsrc(0, buf.vb, (int)buf.elemSize());
      // indexed indices are relative to start of prim
      d3d::setind(buf.ib);
      d3d::drawind(PRIM_TRILIST, de.startIndex, de.primCount, de.startVertex);
    }
#if D3D_HAS_QUADS // no quads, no fan
    else if (command == DrawElem::DRAW_FAN)
    {
      d3d::setvsrc(0, buf.vb, (int)buf.elemSize());
      d3d::draw(PRIM_TRIFAN, de.startVertex, de.primCount);
    }
#endif
    else if (command == DrawElem::DRAW_LINESTRIP)
    {
      d3d::setvsrc(0, buf.vb, (int)buf.elemSize());
      d3d::draw(PRIM_LINESTRIP, de.startVertex, de.primCount);
    }
    else
    {
      G_ASSERT(0);
    }
  }

  if (oldShaderId >= 0)
    shaders[oldShaderId]->cleanup();
}
} // namespace StdGuiRender
