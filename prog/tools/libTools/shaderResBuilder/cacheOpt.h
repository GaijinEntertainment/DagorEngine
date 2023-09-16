#pragma once

#include "forsythTriangleReorderopt.h"
#include <meshoptimizer/include/meshoptimizer.h>
#include <EASTL/vector.h>

// function rearranges both indices and vertex array,returns false if not enough memory or internal error
template <class Indices>
inline void remap_indices(const Indices *indices, const int numIndices, Indices *out_indices, const unsigned int startVert,
  const unsigned int numVerts, int *const vertind)
{
  // caches oldIndex --> newIndex conversion
  int *const indexCache = vertind;
  memset(indexCache, -1, sizeof(int) * numVerts);

  // loop over primitive groups
  unsigned int indexCtr = startVert;

  for (int j = 0; j < numIndices; j++)
  {
    int in_index = indices[j] - startVert;
    int cachedIndex = indexCache[in_index];
    if (cachedIndex == -1) // we haven't seen this index before
    {
      // point to "last" vertex in VB
      out_indices[j] = indexCtr;

      // add to index cache, increment
      indexCache[in_index] = indexCtr++;
    }
    else
    {
      // we've seen this index before
      out_indices[j] = cachedIndex;
    }
  }
}

template <class Indices>
inline bool optimize_list_4cache(unsigned char *verts, int vertSize, int stv, int numv, Indices *inds, int numtri, int cache_size,
  const float *verts_f3, int vertSize_f3)
{
  unsigned numIndices = numtri * 3;
  Indices *newInds = new Indices[numIndices];
  Indices *id = inds;
  Indices max_index = *id, min_index = *id;
  for (int j = 0; j < numIndices; j++, id++)
  {
    min_index = min(*id, min_index);
    max_index = max(*id, max_index);
  }

  if (max_index - min_index + 1 > 3)
  {
    const unsigned WARP_SIZE = 32;
    const unsigned GROUP_SIZE = 256;

    eastl::vector<Indices> shiftedIndices(numIndices);
    for (int i = 0; i < numIndices; ++i)
      shiftedIndices[i] = inds[i] - stv;

    eastl::vector<Indices> forsythNewInds(numIndices);
    Forsyth::OptimizeFaces(shiftedIndices.data(), numIndices, max_index + 1, forsythNewInds.data(), cache_size);
    const float forsythStat =
      meshopt_analyzeVertexCache(forsythNewInds.data(), numIndices, numv, cache_size, WARP_SIZE, GROUP_SIZE).atvr;

    eastl::vector<Indices> meshoptNewInds(numIndices);
    meshopt_optimizeVertexCache(meshoptNewInds.data(), shiftedIndices.data(), numIndices, max_index + 1);
    const float meshoptStat =
      meshopt_analyzeVertexCache(meshoptNewInds.data(), numIndices, numv, cache_size, WARP_SIZE, GROUP_SIZE).atvr;

    eastl::vector<Indices> &bestNewIndices = (meshoptStat < forsythStat) ? meshoptNewInds : forsythNewInds;

    if (verts_f3)
      meshopt_optimizeOverdraw(newInds, bestNewIndices.data(), numIndices, verts_f3, numv, vertSize_f3, 1.05f);
    else
      memcpy(newInds, bestNewIndices.data(), numIndices * sizeof(Indices));
    for (int i = 0; i < numIndices; ++i)
      newInds[i] += stv;
  }
  else
    memcpy(newInds, inds, numIndices * sizeof(Indices));
  if (numv > 0)
  {
    Tab<unsigned char> vertices(tmpmem);
    Tab<int> vertind(tmpmem);
    vertind.resize(numv);
    vertices.resize(numv * vertSize);
    int numv2 = numv;
    remap_indices(newInds, numIndices, inds, stv, numv, &vertind[0]);
    memcpy(&vertices[0], verts, numv * vertSize);
    int *in = &vertind[0];
    unsigned char *vert = &vertices[0];
    for (int i = 0; i < numv; ++i, ++in, vert += vertSize)
    {
      if (*in == -1)
        continue;

      memcpy(&verts[(*in - stv) * vertSize], vert, vertSize);
    }
  }
  else
    memcpy(inds, newInds, numIndices * sizeof(*inds));
  delete[] newInds;
  return true;
}
