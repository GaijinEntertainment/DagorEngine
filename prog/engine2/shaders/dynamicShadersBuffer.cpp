// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <shaders/dag_DynamicShadersBuffer.h>
#include <3d/autoLockBuffer.h>
// #include <debug/dag_debug.h>


class DynamicShadersBuffer::VertexBuffer : public AutoLockVBuffer<true, ANY_VERTEX>
{
public:
  VertexBuffer(int vertex_count, VDECL init_vdecl, int _stride) : AutoLockVBuffer<true, ANY_VERTEX>(vertex_count, init_vdecl, _stride)
  {}
};

class DynamicShadersBuffer::IndexBuffer : public AutoLockIBuffer<true>
{
public:
  IndexBuffer(int ind_count) : AutoLockIBuffer<true>(ind_count) {}
};


DynamicShadersBuffer::DynamicShadersBuffer() : currentShader(NULL), vBuf(NULL), iBuf(NULL) { close(); }


// Calls init().
DynamicShadersBuffer::DynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts, int max_faces) :
  currentShader(NULL), vBuf(NULL), iBuf(NULL)
{
  G_ASSERTF(max_verts < 0x100000 && max_faces * 3 < 0x100000, "max_verts=%d max_faces=%d", max_verts, max_faces);
  init(channels, channel_count, max_verts, max_faces);
}


// Create dynamic vertex and index buffers, get VDECL and stride.
void DynamicShadersBuffer::init(CompiledShaderChannelId *channels, int channel_count, int max_verts, int max_faces)
{
  G_ASSERT(channels);
  close();

  maxVerts = max_verts;
  maxFaces = max_faces;

  VDECL vdecl = dynrender::addShaderVdecl(channels, channel_count);
  int stride = dynrender::getStride(channels, channel_count);

  vBuf = new VertexBuffer(max_verts, vdecl, stride);
  iBuf = new IndexBuffer(3 * max_faces);
}


// Calls close().
DynamicShadersBuffer::~DynamicShadersBuffer() { close(); }


// Destroy buffers, etc.
void DynamicShadersBuffer::close()
{
  del_it(vBuf);
  del_it(iBuf);
  startVerts = startFaces = 0;

  maxVerts = maxFaces = 0;
  usedVerts = usedFaces = 0;
  currentShader = NULL;
}


// Flushes buffer before setting different shader.
void DynamicShadersBuffer::setCurrentShader(ShaderElement *shader)
{
  if (currentShader != shader)
  {
    flush();
    currentShader = shader;
    currentShader->replaceVdecl(vBuf->getVDecl());
  }
}


// Adds specified number of vertices and faces, zero index value refers to
// first added vertex.
// Flushes buffer if necessary. Number of added vertices and faces
// should not exceed maximum.
//== NOTE: Buffer is locked for writing with VBLOCK_NOOVERWRITE flag when appending
//== or with VBLOCK_DISCARD flag when starting from the beginning.
void DynamicShadersBuffer::addFaces(const void *vertex_data, int num_verts, const int *indices, int num_faces)
{
  if (!vBuf)
    return;
  G_ASSERT(num_verts && num_faces);

  if (num_verts > maxVerts)
    fatal("num_verts > maxVerts!\nnum_verts=%d maxVerts=%d num_faces=%d maxFaces=%d", num_verts, maxVerts, num_faces, maxFaces);

  if (num_faces > maxFaces)
    fatal("num_faces > maxFaces!\nnum_verts=%d maxVerts=%d num_faces=%d maxFaces=%d", num_verts, maxVerts, num_faces, maxFaces);

  if ((usedVerts + num_verts > maxVerts) || (usedFaces + num_faces > maxFaces))
  {
    flush(true);
  }


  // fill vertices
  vBuf->addData(vertex_data, num_verts);

  // fill indices
  uint32_t *iBuffer = iBuf->reserveData(3 * num_faces);
  G_ASSERT(iBuffer);
  if (!iBuffer)
    return;

  for (int i = 0; i < num_faces * 3; i++)
  {
    iBuffer[i] = (uint32_t)(indices[i] + usedVerts);
  }

  usedVerts += num_verts;
  usedFaces += num_faces;
}


void DynamicShadersBuffer::fillRawFaces(void **vertex_data, int num_verts, uint32_t **indices, int num_faces, int &used_verts)
{
  G_ASSERT(vBuf);

  if (num_verts > maxVerts)
    fatal("num_verts > maxVerts!\nnum_verts=%d maxVerts=%d num_faces=%d maxFaces=%d", num_verts, maxVerts, num_faces, maxFaces);

  if (num_faces > maxFaces)
    fatal("num_faces > maxFaces!\nnum_verts=%d maxVerts=%d num_faces=%d maxFaces=%d", num_verts, maxVerts, num_faces, maxFaces);

  if ((usedVerts + num_verts > maxVerts) || (usedFaces + num_faces > maxFaces))
  {
    flush(true);
  }

  *vertex_data = vBuf->reserveData(num_verts);
  G_ASSERT(*vertex_data);
  if (!*vertex_data)
    return;

  *indices = iBuf->reserveData(num_faces * 3);
  G_ASSERT(*indices);
  if (!*indices)
    return;

  used_verts = usedVerts;

  usedVerts += num_verts;
  usedFaces += num_faces;
}


// Render current buffer contents using current shader (if set) and discard
// them, so that next addFaces() will start from the beginning.
//== NOTE: Call swap() on buffers to switch to another buffer after rendering.
void DynamicShadersBuffer::flush(bool reset)
{
  if (!usedVerts || !usedFaces)
    return;
  if (!currentShader)
    return;

  if (!vBuf)
    return;
  if (usedFaces == startFaces)
  {
    if (reset)
    {
      vBuf->setToDriver(true);
      iBuf->setToDriver(true);
      iBuf->processDynamic();
      vBuf->processDynamic();
      startVerts = startFaces = usedVerts = usedFaces = 0;
    }
    return;
  }

  // debug_ctx("flush buffer: %d verts %d indices", usedVerts, usedFaces);

  vBuf->setVertexFormat();
  vBuf->setToDriver(reset);
  iBuf->setToDriver(reset);

  currentShader->render(startVerts, usedVerts - startVerts, startFaces * 3, usedFaces - startFaces);

  if (reset)
  {
    iBuf->processDynamic();
    vBuf->processDynamic();
    startVerts = startFaces = usedVerts = usedFaces = 0;
  }
  else
  {
    startVerts = usedVerts;
    startFaces = usedFaces;
  }
}
