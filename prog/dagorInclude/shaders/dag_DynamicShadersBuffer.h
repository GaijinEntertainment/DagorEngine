//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_shaders.h>
#include <generic/dag_smallTab.h>
#include <util/dag_stdint.h>

class DynamicShadersBuffer
{
public:
  DynamicShadersBuffer();

  // Calls init().
  DynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts, int max_faces);

  // Create dynamic vertex and index buffers, get VDECL and stride.
  void init(CompiledShaderChannelId *channels, int channel_count, int max_verts, int max_faces);


  // Calls close().
  ~DynamicShadersBuffer();

  // Destroy buffers, etc.
  void close();


  inline ShaderElement *getCurrentShader() { return currentShader; }

  // Flushes buffer before setting different shader.
  void setCurrentShader(ShaderElement *shader);


  // Adds specified number of vertices and faces, zero index value refers to
  // first added vertex.
  // Flushes buffer if necessary. Number of added vertices and faces
  // should not exceed maximum.
  void addFaces(const void *vertex_data, int num_verts, const int *indices, int num_faces);


  void fillRawFaces(void **vertex_data, int num_verts, uint32_t **indices, int num_faces, int &used_verts);


  // Render current buffer contents using current shader (if set) and discard
  // them, so that next addFaces() will start from the beginning.
  void flush(bool reset = false);

  inline int getUsedVertices() { return usedVerts; }

  inline int getUsedFaces() { return usedFaces; }


private:
  int maxVerts, maxFaces;

  class VertexBuffer;
  class IndexBuffer;

  VertexBuffer *vBuf;
  IndexBuffer *iBuf;
  int startVerts, startFaces;

  Ptr<ShaderElement> currentShader;

  int usedVerts, usedFaces;
};
