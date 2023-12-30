#pragma once

#include <string.h>

//-----------------------------------------------------------------------------
//  OptimizeFaces
//-----------------------------------------------------------------------------
//  Parameters:
//      indexList
//          input index list
//      indexCount
//          the number of indices in the list
//      vertexCount
//          the largest index value in indexList
//      newIndexList
//          a pointer to a preallocated buffer the same size as indexList to
//          hold the optimized index list
//      lruCacheSize
//          the size of the simulated post-transform cache (max:64)
//-----------------------------------------------------------------------------
template <class Indices>
class CacheOptimizer
{
public:
  static const unsigned int kMaxVertexCacheSize = 32;
  static const unsigned int kMaxPrecomputedVertexValenceScores = 32;
  typedef unsigned int uint;
  typedef unsigned short uint16;
  typedef unsigned char byte;
  // code for computing vertex score was taken, as much as possible
  // directly from the original publication.
  static float ComputeVertexCacheScore(int cachePosition, uint vertexCacheSize)
  {
    const double FindVertexScore_CacheDecayPower = 1.5;
    const double FindVertexScore_LastTriScore = 0.75;

    double score = 0.0;
    if (cachePosition < 0)
    {
      // Vertex is not in FIFO cache - no score.
    }
    else
    {
      if (cachePosition < 4)
      {
        // This vertex was used in the last triangle,
        // so it has a fixed score, whichever of the three
        // it's in. Otherwise, you can get very different
        // answers depending on whether you add
        // the triangle 1,2,3 or 3,1,2 - which is silly.
        score = FindVertexScore_LastTriScore;
      }
      else
      {
        assert(cachePosition < vertexCacheSize);
        // Points for being high in the cache.
        const double scaler = 1.0f / static_cast<double>(vertexCacheSize - 3);
        score = 1.0 - static_cast<double>(cachePosition - 3) * scaler;
        score = pow(score, FindVertexScore_CacheDecayPower);
      }
    }

    return score;
  }

  static float ComputeVertexValenceScore(uint numActiveFaces)
  {
    const double FindVertexScore_ValenceBoostScale = 2.0;
    const double FindVertexScore_ValenceBoostPower = 0.5;

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    double valenceBoost = pow(static_cast<double>(numActiveFaces), -FindVertexScore_ValenceBoostPower);
    return FindVertexScore_ValenceBoostScale * valenceBoost;
  }
  float s_vertexCacheScores[kMaxVertexCacheSize + 1][kMaxVertexCacheSize];
  float s_vertexValenceScores[kMaxPrecomputedVertexValenceScores];

  bool ComputeVertexScores()
  {
    for (uint cacheSize = 0; cacheSize <= kMaxVertexCacheSize; ++cacheSize)
    {
      for (uint cachePos = 0; cachePos < cacheSize; ++cachePos)
      {
        s_vertexCacheScores[cacheSize][cachePos] = ComputeVertexCacheScore(cachePos, cacheSize);
      }
      memset(&s_vertexCacheScores[cacheSize][cacheSize], 0, (kMaxVertexCacheSize - cacheSize) * sizeof(float));
    }

    for (uint valence = 0; valence < kMaxPrecomputedVertexValenceScores; ++valence)
    {
      s_vertexValenceScores[valence] = ComputeVertexValenceScore(valence);
    }

    return true;
  }
  bool s_vertexScoresComputed;

  CacheOptimizer() : s_vertexScoresComputed(ComputeVertexScores()) {}

  inline float FindVertexCacheScore(uint cachePosition, uint maxSizeVertexCache)
  {
    return s_vertexCacheScores[maxSizeVertexCache][cachePosition];
  }

  inline float FindVertexValenceScore(uint numActiveTris) { return s_vertexValenceScores[numActiveTris]; }

  float FindVertexScore(uint numActiveFaces, uint cachePosition, uint vertexCacheSize)
  {
    assert(s_vertexScoresComputed);

    if (numActiveFaces == 0)
    {
      // No tri needs this vertex!
      return -1.0f;
    }

    float score = 0.f;
    if (cachePosition < vertexCacheSize)
    {
      score += s_vertexCacheScores[vertexCacheSize][cachePosition];
    }

    if (numActiveFaces < kMaxPrecomputedVertexValenceScores)
    {
      score += s_vertexValenceScores[numActiveFaces];
    }
    else
    {
      score += ComputeVertexValenceScore(numActiveFaces);
    }

    return floorf(score * 2097152.f) / 2097152.f; // round to 21 most significant bits of mantissa
  }

  struct OptimizeVertexData
  {
    float score;
    uint activeFaceListStart;
    uint activeFaceListSize;
    Indices cachePos0;
    Indices cachePos1;
    OptimizeVertexData() : score(0.f), activeFaceListStart(0), activeFaceListSize(0), cachePos0(0), cachePos1(0) {}
  };

  void OptimizeFaces(const Indices *indexList, uint indexCount, uint vertexCount, Indices *newIndexList, uint16 lruCacheSize)
  {
    std::vector<OptimizeVertexData> vertexDataList;
    vertexDataList.resize(vertexCount);

    // compute face count per vertex
    for (uint i = 0; i < indexCount; ++i)
    {
      Indices index = indexList[i];
      assert(index < vertexCount);
      OptimizeVertexData &vertexData = vertexDataList[index];
      vertexData.activeFaceListSize++;
    }

    std::vector<uint> activeFaceList;

    const Indices kEvictedCacheIndex = std::numeric_limits<Indices>::max();

    {
      // allocate face list per vertex
      uint curActiveFaceListPos = 0;
      for (uint i = 0; i < vertexCount; ++i)
      {
        OptimizeVertexData &vertexData = vertexDataList[i];
        vertexData.cachePos0 = kEvictedCacheIndex;
        vertexData.cachePos1 = kEvictedCacheIndex;
        vertexData.activeFaceListStart = curActiveFaceListPos;
        curActiveFaceListPos += vertexData.activeFaceListSize;
        vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos0, lruCacheSize);
        vertexData.activeFaceListSize = 0;
      }
      activeFaceList.resize(curActiveFaceListPos);
    }

    // fill out face list per vertex
    for (uint i = 0; i < indexCount; i += 3)
    {
      for (uint j = 0; j < 3; ++j)
      {
        Indices index = indexList[i + j];
        OptimizeVertexData &vertexData = vertexDataList[index];
        activeFaceList[vertexData.activeFaceListStart + vertexData.activeFaceListSize] = i;
        vertexData.activeFaceListSize++;
      }
    }

    std::vector<byte> processedFaceList;
    processedFaceList.resize(indexCount);
    memset(&processedFaceList[0], 0, indexCount);

    Indices vertexCacheBuffer[(kMaxVertexCacheSize + 3) * 2];
    memset(vertexCacheBuffer, 0, (kMaxVertexCacheSize + 3) * 2 * sizeof(Indices));
    Indices *cache0 = vertexCacheBuffer;
    Indices *cache1 = vertexCacheBuffer + (kMaxVertexCacheSize + 3);
    Indices entriesInCache0 = 0;

    uint bestFace = 0;
    float bestScore = -1.f;

    const float maxValenceScore = FindVertexScore(1, kEvictedCacheIndex, lruCacheSize) * 3.f;

    for (uint i = 0; i < indexCount; i += 3)
    {
      if (bestScore < 0.f)
      {
        // no verts in the cache are used by any unprocessed faces so
        // search all unprocessed faces for a new starting point
        for (uint j = 0; j < indexCount; j += 3)
        {
          if (processedFaceList[j] == 0)
          {
            uint face = j;
            float faceScore = 0.f;
            for (uint k = 0; k < 3; ++k)
            {
              Indices index = indexList[face + k];
              OptimizeVertexData &vertexData = vertexDataList[index];
              assert(vertexData.activeFaceListSize > 0);
              assert(vertexData.cachePos0 >= lruCacheSize);
              faceScore += vertexData.score;
            }

            if (faceScore > bestScore)
            {
              bestScore = faceScore;
              bestFace = face;

              assert(bestScore <= maxValenceScore);
              if (bestScore >= maxValenceScore)
              {
                break;
              }
            }
          }
        }
        assert(bestScore >= 0.f);
      }

      processedFaceList[bestFace] = 1;
      Indices entriesInCache1 = 0;

      // add bestFace to LRU cache and to newIndexList
      for (uint v = 0; v < 3; ++v)
      {
        Indices index = indexList[bestFace + v];
        newIndexList[i + v] = index;

        OptimizeVertexData &vertexData = vertexDataList[index];

        if (vertexData.cachePos1 >= entriesInCache1)
        {
          vertexData.cachePos1 = entriesInCache1;
          cache1[entriesInCache1++] = index;

          if (vertexData.activeFaceListSize == 1)
          {
            --vertexData.activeFaceListSize;
            continue;
          }
        }

        assert(vertexData.activeFaceListSize > 0);
        uint *begin = &activeFaceList[vertexData.activeFaceListStart];
        uint *end = begin + vertexData.activeFaceListSize;
        uint *it = std::find(begin, end, bestFace);
        assert(it != end);
        std::swap(*it, *(end - 1));
        --vertexData.activeFaceListSize;
        vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);
      }

      // move the rest of the old verts in the cache down and compute their new scores
      for (uint c0 = 0; c0 < entriesInCache0; ++c0)
      {
        Indices index = cache0[c0];
        OptimizeVertexData &vertexData = vertexDataList[index];

        if (vertexData.cachePos1 >= entriesInCache1)
        {
          vertexData.cachePos1 = entriesInCache1;
          cache1[entriesInCache1++] = index;
          vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);
        }
      }

      // find the best scoring triangle in the current cache (including up to 3 that were just evicted)
      bestScore = -1.f;
      for (uint c1 = 0; c1 < entriesInCache1; ++c1)
      {
        Indices index = cache1[c1];
        OptimizeVertexData &vertexData = vertexDataList[index];
        vertexData.cachePos0 = vertexData.cachePos1;
        vertexData.cachePos1 = kEvictedCacheIndex;
        for (uint j = 0; j < vertexData.activeFaceListSize; ++j)
        {
          uint face = activeFaceList[vertexData.activeFaceListStart + j];
          float faceScore = 0.f;
          for (uint v = 0; v < 3; v++)
          {
            Indices faceIndex = indexList[face + v];
            OptimizeVertexData &faceVertexData = vertexDataList[faceIndex];
            faceScore += faceVertexData.score;
          }
          if (faceScore > bestScore)
          {
            bestScore = faceScore;
            bestFace = face;
          }
        }
      }

      std::swap(cache0, cache1);
      entriesInCache0 = std::min(entriesInCache1, (Indices)lruCacheSize);
    }
  }
};