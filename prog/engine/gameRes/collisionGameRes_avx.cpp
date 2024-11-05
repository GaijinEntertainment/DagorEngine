// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <math/dag_traceRayTriangle.h>

#if defined(__AVX__) && defined(_TARGET_64BIT)
template <bool check_bounding>
VECTORCALL bool CollisionResource::traceRayMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm)
{
  int resultIdx = -1;
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  const uint32_t mainBatchSize = 8;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (mainBatchSize * 3 - 1))); i += mainBatchSize * 3)
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[mainBatchSize][3];
    for (uint32_t j = 0; j < mainBatchSize; j++)
    {
      vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x);
      vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x);
      vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x);
    }

    int ret = traceray8TrianglesCullCCW<check_bounding>(v_local_from, v_local_dir, in_out_t, vert);
    if (ret >= 0)
      resultIdx = i + ret * 3;
  }

  const uint32_t finalizeBatchSize = 4;

  while (i < indicesSize)
  {
    uint32_t count = 0;
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[finalizeBatchSize][3];
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < finalizeBatchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    int ret = traceray4TrianglesCullCCW(v_local_from, v_local_dir, in_out_t, vert, count);
    if (ret >= 0)
      resultIdx = i + ret * 3;

    i += finalizeBatchSize * 3;
  }

  if (resultIdx >= 0 && v_out_norm)
  {
    vec4f v0 = v_ld(&vertices[indices[resultIdx + 0]].x);
    vec4f v1 = v_ld(&vertices[indices[resultIdx + 1]].x);
    vec4f v2 = v_ld(&vertices[indices[resultIdx + 2]].x);
    *v_out_norm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
  }
  return resultIdx >= 0;
}

template <bool check_bounding>
VECTORCALL bool CollisionResource::rayHitMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float in_t)
{
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();

  const uint32_t mainBatchSize = 8;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (mainBatchSize * 3 - 1))); i += mainBatchSize * 3)
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[mainBatchSize][3];
    for (uint32_t j = 0; j < mainBatchSize; j++)
    {
      vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x);
      vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x);
      vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x);
    }

    if (rayhit8TrianglesCullCCW<check_bounding>(v_local_from, v_local_dir, in_t, vert))
      return true;
  }

  const uint32_t finalizeBatchSize = 4;

  while (i < indicesSize)
  {
    uint32_t count = 0;
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[finalizeBatchSize][3];
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < finalizeBatchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    if (rayhit4TrianglesCullCCW(v_local_from, v_local_dir, in_t, vert, count))
      return true;

    i += finalizeBatchSize * 3;
  }

  return false;
}

template <bool check_bounding>
VECTORCALL bool CollisionResource::traceRayMeshNodeLocalAllHits_AVX256(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_nodes_ret_t &ret_array)
{
  const uint16_t *__restrict indices = node.indices.data();
  const Point3_vec4 *__restrict vertices = node.vertices.data();
  const uint32_t indicesSize = node.indices.size();
  bool noCull = force_no_cull || node.checkBehaviorFlags(CollisionNode::SOLID);

  constexpr uint32_t mainBatchSize = 8;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (mainBatchSize * 3 - 1))); i += mainBatchSize * 3)
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[mainBatchSize][3];
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(8)
#endif
    for (uint32_t j = 0; j < mainBatchSize; j++)
    {
      vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x);
      vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x);
      vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x);
    }

    alignas(32) float outT[mainBatchSize];
    int ret = traceray8TrianglesMask<check_bounding>(v_local_from, v_local_dir, in_t, outT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < mainBatchSize; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = v_ld(&vertices[indices[i + j * 3 + 0]].x);
            vec4f v1 = v_ld(&vertices[indices[i + j * 3 + 1]].x);
            vec4f v2 = v_ld(&vertices[indices[i + j * 3 + 2]].x);
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
        }
      }
    }
  }

  const uint32_t finalizeBatchSize = 4;

  while (i < indicesSize)
  {
    uint32_t count = 0;
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[finalizeBatchSize][3];
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < finalizeBatchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    vec4f vInOutT = v_splats(in_t);
    int ret = traceray4TrianglesMask(v_local_from, v_local_dir, vInOutT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
      alignas(16) float outT[finalizeBatchSize];
      v_st(outT, vInOutT);
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < count; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = vert[j][0];
            vec4f v1 = vert[j][1];
            vec4f v2 = vert[j][2];
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
        }
      }
    }

    i += finalizeBatchSize * 3;
  }

  return !ret_array.empty();
}

const CollisionResource::TraceMeshNodeLocalApi CollisionResource::traceMeshNodeLocalApi_AVX256 = {
  60 * 3, // check bounding for each 8 if triangles >= 60
  {&CollisionResource::traceRayMeshNodeLocalCullCCW_AVX256<false>, &CollisionResource::rayHitMeshNodeLocalCullCCW_AVX256<false>,
    &CollisionResource::traceRayMeshNodeLocalAllHits_AVX256<false>},
  {&CollisionResource::traceRayMeshNodeLocalCullCCW_AVX256<true>, &CollisionResource::rayHitMeshNodeLocalCullCCW_AVX256<true>,
    &CollisionResource::traceRayMeshNodeLocalAllHits_AVX256<true>},
};

const bool CollisionResource::haveTraceMeshNodeLocalApi_AVX256 = true;

#else // __AVX__

const CollisionResource::TraceMeshNodeLocalApi CollisionResource::traceMeshNodeLocalApi_AVX256 = {};

const bool CollisionResource::haveTraceMeshNodeLocalApi_AVX256 = false;

#endif // __AVX__
