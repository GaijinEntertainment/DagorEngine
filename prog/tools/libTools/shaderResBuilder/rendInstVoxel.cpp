// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/voxel/voxelCache.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <math/dag_mesh.h>
#include <libTools/shaderResBuilder/rendInstResSrc.h>
#include <libTools/shaderResBuilder/matSubst.h>
#include <rendInst/rendInstConsts.h>
#include <image/dag_dxtCompress.h>
#include <image/dag_tga.h>
#include <convert/detex/detex.h>
#if !TEX_CANNOT_USE_ISPC
#include <ispc_texcomp.h>
#endif
#include <generic/dag_fixedVectorSet.h>
#include <util/dag_bitArray.h>
#include <util/dag_radix.h>
#include <math/dag_mathUtils.h>
#include <math/dag_convexHullComputer.h>
#include <gameMath/quantization.h>
#include <dag_noise/dag_uint_noise.h>
#include <osApiWrappers/dag_files.h>


extern bool shadermeshbuilder_strip_d3dres;
#define ISSUE_FATAL(...) log ? log->addMessage(ILogWriter::FATAL, __VA_ARGS__) : DAG_FATAL(__VA_ARGS__)


static vec4f ApplySRGBCurve(vec4f x)
{
  // Approximately pow(x, 1.0 / 2.2)
  return v_sel(v_sub(v_mul(v_splats(1.055f), v_pow_est(x, v_splats(1.0f / 2.4f))), v_splats(0.055f)), v_mul(x, v_splats(12.92f)),
    v_cmp_lt(x, v_splats(0.0031308f)));
}

static vec4f RemoveSRGBCurve(vec4f x)
{
  // Approximately pow(x, 2.2)
  return v_sel(v_pow_est(v_mul(v_add(x, v_splats(0.055f)), v_splats(1.0f / 1.055f)), v_splats(2.4f)),
    v_mul(x, v_splats(1.0f / 12.92f)), v_cmp_lt(x, v_splats(0.04045f)));
}


static float compute_threshold_length(const ShaderMeshData &mesh, float threshold_percent)
{
  Tab<float> unpacked_vert_f3(tmpmem);
  Tab<float> triLen(tmpmem);
  triLen.reserve(mesh.countFaces());

  for (const auto &re : mesh.elems)
  {
    if (!re.vertexData or re.si == RELEM_NO_INDEX_BUFFER)
      continue;
    const auto &vd = *re.vertexData.get();

    int pos_offset = -1, pos_type = -1, coffset = 0;
    for (auto &c : vd.vDesc)
    {
      if (c.streamId != 0)
        continue;
      unsigned chSize = 0;
      if (c.vbu == SCUSAGE_POS && c.vbui == 0)
      {
        pos_offset = coffset;
        pos_type = c.t;
      }
      if (channel_size(c.t, chSize))
        coffset += chSize;
    }

    float *vd_f3 = nullptr;
    int vd_stride_f3 = 0;
    switch (pos_type)
    {
      case SCTYPE_FLOAT3:
        vd_f3 = (float *)&vd.vData[re.sv * vd.stride + pos_offset];
        vd_stride_f3 = vd.stride;
        break;

      case SCTYPE_SHORT4:
        unpacked_vert_f3.resize(re.numv * 3);
        vd_f3 = unpacked_vert_f3.data();
        for (const auto *s = &vd.vData[re.sv * vd.stride + pos_offset], *se = s + re.numv * vd.stride; s < se;
             s += vd.stride, vd_f3 += 3)
          vd_f3[0] = *(int16_t *)(s + 0), vd_f3[1] = *(int16_t *)(s + 2), vd_f3[2] = *(int16_t *)(s + 4);
        vd_f3 = unpacked_vert_f3.data();
        vd_stride_f3 = elem_size(unpacked_vert_f3) * 3;
        break;

      case SCTYPE_HALF4:
        unpacked_vert_f3.resize(re.numv * 3);
        vd_f3 = unpacked_vert_f3.data();
        for (const auto *s = &vd.vData[re.sv * vd.stride + pos_offset], *se = s + re.numv * vd.stride; s < se;
             s += vd.stride, vd_f3 += 3)
          vd_f3[0] = half_to_float(*(uint16_t *)(s + 0)), vd_f3[1] = half_to_float(*(uint16_t *)(s + 2)),
          vd_f3[2] = half_to_float(*(uint16_t *)(s + 4));
        vd_f3 = unpacked_vert_f3.data();
        vd_stride_f3 = elem_size(unpacked_vert_f3) * 3;
        break;

      default: debug("unsupported position type %d in compute_threshold_length()", pos_type); break;
    }

    if (!vd_f3)
      continue;

    for (int i = re.si, ei = re.si + re.numf * 3; i < ei; i += 3)
    {
      uint32_t idx[3];
      if (vd.is32bit())
      {
        idx[0] = vd.iData32[i + 0];
        idx[1] = vd.iData32[i + 1];
        idx[2] = vd.iData32[i + 2];
      }
      else
      {
        idx[0] = vd.iData[i + 0];
        idx[1] = vd.iData[i + 1];
        idx[2] = vd.iData[i + 2];
      }
      vec3f vert[3];
      for (int j = 0; j < 3; ++j)
        vert[j] = v_ldu_p3((const float *)((const uint8_t *)vd_f3 + idx[j] * vd_stride_f3));
      vec3f maxLen = v_length3_sq_x(v_sub(vert[0], vert[1]));
      maxLen = v_max(maxLen, v_length3_sq_x(v_sub(vert[1], vert[2])));
      maxLen = v_max(maxLen, v_length3_sq_x(v_sub(vert[2], vert[0])));
      triLen.push_back(v_extract_x(maxLen));
    }
  }

  if (triLen.empty())
    return -1;

  eastl::sort(triLen.begin(), triLen.end(), eastl::less<float>{});

  int index = int(floorf(threshold_percent * 0.01f * triLen.size()));
  index = clamp<int>(index, 0, triLen.size() - 1);
  float len = sqrtf(triLen[index]);
  debug("threshold edge length for %g%% triangles (%d/%d) is %g", threshold_percent, index, triLen.size(), len);
  return len;
}


RenderableInstanceLodsResSrc::VoxelMip::VoxelMip(const IPoint3 nb, float vs) :
  numBlocks(nb), voxelSize(vs), bitmap(tmpmem), rgbaBlocks(tmpmem), normBlocks(tmpmem)
{
  bitmap.resize(nb.x * nb.y * nb.z);
  for (uint32_t fi = 0; fi < 6; fi++)
    faces[fi].init(nb[voxelcache::CacheDump::FACE_X[fi]], nb[voxelcache::CacheDump::FACE_Y[fi]]);
}


bool RenderableInstanceLodsResSrc::addVoxelLod(int first_mip, int num_mips, real range, LodsEqualMaterialGather &mat_gather,
  DagorMatShaderSubstitute &mat_subst)
{
  G_ASSERT(voxelHull.getFaceCount() > 0);
  Node node;
  node.flags = NODEFLG_RENDERABLE;
  node.tm.identity();
  node.name = "voxel_impostor";
  node.obj = new MeshHolderObj(new Mesh(voxelHull));
  auto md = new MaterialData();
  md->className = "rendinst_voxel";
  node.mat = new MaterialDataList();
  node.mat->addSubMat(md);
  node.calc_wtm();

  if (log)
    if (!mat_subst.checkMatClasses(&node, node.name, *log))
      return false;

  int lodId = append_items(lods, 1);
  Lod &lod = lods[lodId];
  lod.range = range;
  lod.firstVoxelMip = first_mip;
  lod.numVoxelMips = num_mips;
  addMeshNode(lod, &node, &node, mat_gather, nullptr);
  if (shadermeshbuilder_strip_d3dres)
    return true;

  G_ASSERT(lod.rigid.meshData.elems.size() == 1);
  auto &relem = lod.rigid.meshData.elems[0];
  G_ASSERT(relem.vertexData->numv == relem.numv);

  // add mip data to VB
  const uint32_t numLayers = numVoxelLayers;
  int size = 0;
  size += 4;            // number of mips, number of layers
  size += num_mips * 4; // mip offsets
  for (int mi = first_mip, me = first_mip + num_mips; mi < me; mi++)
  {
    const auto &mip = voxelMips[mi];
    size += 4 * 3; // numBlocks
    size += 4;     // voxelSize
    size += 4 * 3; // boxMin
    size += data_size(mip.bitmap);
    for (auto &f : mip.faces)
      size += f.blockIndex.size() * numLayers * 8;
  }

  lod.voxelHullVerts = relem.vertexData->numv;
  auto rawData = relem.vertexData->attachRawData(size);
  relem.numv = relem.vertexData->numv;

  lod.voxelSurface = new (tmpmem) VoxelSurfaceSrc();
  int numRgbaBlocks = 0;
  int numNormBlocks = 0;
  for (int mi = first_mip, me = first_mip + num_mips; mi < me; mi++)
  {
    const auto &mip = voxelMips[mi];
    numRgbaBlocks += mip.rgbaBlocks.size();
    numNormBlocks += mip.normBlocks.size();
  }
  lod.voxelSurface->rgbaBlocks.reserve(numRgbaBlocks);
  lod.voxelSurface->normBlocks.reserve(numNormBlocks);

  auto ptr = (uint32_t *)rawData.data();
  *ptr++ = num_mips | numLayers << 16;
  auto offsets = ptr;
  ptr += num_mips;

  int rgbaIndexOffset = 0;
  int normIndexOffset = 0;
  for (int mi = first_mip, me = first_mip + num_mips; mi < me; mi++)
  {
    const auto &mip = voxelMips[mi];
    offsets[mi - first_mip] = (uint8_t *)ptr - rawData.data();
    append_items(lod.voxelSurface->rgbaBlocks, mip.rgbaBlocks.size(), mip.rgbaBlocks.data());
    append_items(lod.voxelSurface->normBlocks, mip.normBlocks.size(), mip.normBlocks.data());

    *ptr++ = mip.numBlocks.x;
    *ptr++ = mip.numBlocks.y;
    *ptr++ = mip.numBlocks.z;
    *(float *)ptr++ = mip.voxelSize;
    *(float *)ptr++ = mip.boxMin.x;
    *(float *)ptr++ = mip.boxMin.y;
    *(float *)ptr++ = mip.boxMin.z;
    for (auto b : mip.bitmap)
    {
      *ptr++ = uint32_t(b);
      *ptr++ = uint32_t(b >> 32);
    }
    for (auto &f : mip.faces)
    {
      for (auto &layers : f.blockIndex)
        for (uint32_t li = 0; li < numLayers; li++)
        {
          auto &index = layers[li];
          int ri = index.rgba01;
          int ni = index.norm;
          ri += (ri == 0) ? 0 : rgbaIndexOffset;
          ni += (ni == 0) ? 0 : normIndexOffset;
          *ptr++ = index.depth << (32 - VOXEL_DEPTH_BITS / 2) | ri;
          *ptr++ = index.depth >> (VOXEL_DEPTH_BITS / 2) << (32 - VOXEL_DEPTH_BITS / 2) | ni;
        }
    }

    rgbaIndexOffset += mip.rgbaBlocks.size();
    normIndexOffset += mip.normBlocks.size();
  }
  G_ASSERT((uint8_t *)ptr - rawData.data() <= rawData.size());
  memset(ptr, 0, rawData.data() + rawData.size() - (uint8_t *)ptr);

  return true;
}


bool RenderableInstanceLodsResSrc::buildVoxels(const DataBlock &blk, const DataBlock &default_voxel_params,
  voxelcache::CacheDump *voxel_cache, LodsEqualMaterialGather &mat_gather, DagorMatShaderSubstitute &mat_subst)
{
  int numVoxelLods = rendinst::RI_MAX_LODS - lods.size();
  if (numVoxelLods <= 0)
  {
    ISSUE_FATAL("Can't add voxel LODs in %s - too many regular LODs (%d)", assetName.c_str(), lods.size());
    return false;
  }
  if (!buildVoxelMips(blk, default_voxel_params, voxel_cache))
    return false;
  numVoxelLods = min<int>(numVoxelLods, voxelMips.size());

  if (!buildVoxelHull(blk, default_voxel_params))
    return false;

  const auto vblk = blk.getBlockByNameEx("voxel_impostor");
  const float projScale = vblk->getReal("projScale", default_voxel_params.getReal("projScale", defaults::projScale));
  const float triangleThreshold =
    vblk->getReal("triangleThreshold", default_voxel_params.getReal("triangleThreshold", defaults::triangleThreshold));
  const bool adjustMeshLodRanges =
    vblk->getBool("adjustMeshLodRanges", default_voxel_params.getBool("adjustMeshLodRanges", defaults::adjustMeshLodRanges));

  Lod tmpImpostorLod;
  if (hasImpostor)
  {
    tmpImpostorLod = eastl::move(lods.back());
    lods.pop_back();
  }

  if (adjustMeshLodRanges)
  {
    for (int li = 0; li < lods.size(); li++)
    {
      auto &lod = lods[li];
      if (lod.numVoxelMips > 0)
        break;
      const auto &mesh = lod.rigid.meshData;
      float range = lod.range;
      float len = compute_threshold_length(mesh, triangleThreshold);
      if (len <= 0)
        debug("LOD %d of rendInst %s has no usable triangles", li, assetName.c_str());
      else
      {
        float vsize = len * 0.5f;         // threshold when triangle is about 2 voxels/pixels
        range = projScale * 0.5f * vsize; // 0.5 is from radius/diameter, see defaults::projScale comment
      }

      if (range > lod.range)
        range = lod.range;
      if (li > 0 and range <= lods[li - 1].range)
        range = lods[li - 1].range + 1;

      if (range != lod.range)
      {
        debug("adjusting LOD %d range from %g to %g in %s", li, lod.range, range, assetName.c_str());
        lod.range = range;
      }
    }
  }

  debug("adding %d voxel LODs starting at %d", numVoxelLods, lods.size());
  for (int li = 0; li < numVoxelLods; li++)
  {
    float voxelSize = voxelMips[li == numVoxelLods - 1 ? voxelMips.size() - 1 : li].voxelSize;
    float range = projScale * 0.5f * voxelSize;
    if (range <= lods.back().range)
    {
      if (log)
        log->addMessage(log->WARNING, "LOD #%d range is too large in %s, %g >= %g (for voxel size %g)", lods.size() - 1,
          assetName.c_str(), lods.back().range, range, voxelSize);
      else
        logwarn("LOD #%d range is too large in %s, %g >= %g (for voxel size %g)", lods.size() - 1, assetName.c_str(),
          lods.back().range, range, voxelSize);
      range = lods.back().range + 1;
    }
    if (!addVoxelLod(li, li == numVoxelLods - 1 ? voxelMips.size() - li : 1, range, mat_gather, mat_subst))
      return false;
  }

  if (hasImpostor)
  {
    if (tmpImpostorLod.range <= lods.back().range)
    {
      if (log)
        log->addMessage(log->WARNING, "Impostor LOD range is too small in %s, %g <= %g (for voxel size %g)", assetName.c_str(),
          tmpImpostorLod.range, lods.back().range, voxelMips.back().voxelSize);
      else
        logwarn("Impostor LOD range is too small in %s, %g <= %g (for voxel size %g)", assetName.c_str(), tmpImpostorLod.range,
          lods.back().range, voxelMips.back().voxelSize);
      tmpImpostorLod.range = lods.back().range + 1;
    }
    lods.emplace_back(eastl::move(tmpImpostorLod));
  }

  return true;
}

struct RgbaPixel32
{
  union
  {
    struct
    {
      uint8_t r, g, b, a;
    };
    uint32_t u;
    uint32_t c;
  };
};

template <typename T>
struct ScalarAverager
{
  T sum;
  ScalarAverager(T zero) : sum(zero) {}
  void add(T v) { sum += v; }
  T result(uint32_t num) const { return sum / num; }
};

template <>
struct ScalarAverager<vec4f>
{
  vec4f sum = v_zero();
  void add(vec4f v) { sum = v_add(sum, v); }
  vec4f result(uint32_t num) const { return v_div(sum, v_splats(num)); }
};

struct NormalAverager
{
  vec3f sum = v_zero();
  void add(vec3f v) { sum = v_add(sum, v); }
  vec3f result(uint32_t num) const { return v_div(sum, v_splats(num)); }
};

struct VoxelAverager
{
  ScalarAverager<vec4f> albedoColoring, srmt;
  NormalAverager normal;
  uint32_t num = 0;

  bool empty() const { return num == 0; }

  void flipNormal() { normal.sum = v_neg(normal.sum); }

  void add(const voxelcache::Pixel &p)
  {
    vec4f ac = v_mul(v_make_vec4f(p.albedoR, p.albedoG, p.albedoB, 0), v_splats(1.0f / 255));
    ac = v_insert_w(RemoveSRGBCurve(ac), p.coloring / 255.0f);
    albedoColoring.add(ac);
    srmt.add(v_mul(v_make_vec4f(p.smoothness, p.reflectance, p.metalness, p.translucency), v_splats(1.0f / 255)));
    normal.add(v_sub(v_mul(v_make_vec3f(p.normalX, p.normalY, p.normalZ), v_splats(2.0f / 255)), V_C_ONE));
    num++;
  }

  void result(RgbaPixel32 &rgba0, RgbaPixel32 &rgba1, TexPixelRg<uint8_t> &norm)
  {
    if (num == 0)
    {
      rgba0.u = 0xff00ff;
      rgba1.u = 0;
      norm = {0, 0};
    }
    else
    {
      vec4f ac = albedoColoring.result(num);
      ac = v_perm_xyzd(ApplySRGBCurve(ac), ac);
      ac = v_mul(ac, v_splats(255));
      vec4f mat = v_mul(srmt.result(num), v_splats(255));
      float roughness = 1 - v_extract_x(mat) / 255;
      vec3f n = normal.result(num);
      vec4f nl = v_length3(n);
      n = v_safediv(n, nl);
      roughness = adjust_roughness(roughness, v_extract_x(nl));

      rgba0.r = uint8_t(roundf(v_extract_x(ac)));
      rgba0.g = uint8_t(roundf(v_extract_y(ac)));
      rgba0.b = uint8_t(roundf(v_extract_z(ac)));
      rgba0.a = uint8_t(roundf(v_extract_w(ac)));

      rgba1.r = uint8_t(roundf((1 - roughness) * 255));
      rgba1.g = uint8_t(roundf(v_extract_y(mat)));
      rgba1.b = uint8_t(roundf(v_extract_z(mat)));
      rgba1.a = uint8_t(roundf(v_extract_w(mat)));

      constexpr float HALF255 = 255 * 0.5f;
      Point2 onorm = gamemath::p3_to_oct(Point3(v_extract_x(n), v_extract_y(n), v_extract_z(n))) * HALF255 + Point2(HALF255, HALF255);
      norm.r = uint8_t(roundf(onorm.x));
      norm.g = uint8_t(roundf(onorm.y));
    }
  }
};

struct VoxelSurfaceBlock
{
  carray<RgbaPixel32, 4 * 4> rgba0, rgba1;
  carray<TexPixelRg<uint8_t>, 4 * 4> norm;
  uint16_t mask = 0;
};

template <typename M, typename T>
static int merge_surface(dag::Span<T> surface, dag::Span<int> old_to_new)
{
  G_ASSERT(surface.size() == old_to_new.size());

  // make list of graph nodes
  struct MergeNode
  {
    uint32_t source;
    uint32_t numNeighbors = 0;
  };
  Tab<MergeNode> nodes(tmpmem);

  uint32_t numNodes = surface.size();
  nodes.resize_noinit(numNodes);
  for (uint32_t i = 0; i < numNodes; i++)
    nodes[i] = {.source = i};

  // count neighbors
  for (uint32_t i = 1; i < numNodes; i++)
    for (uint32_t j = 0; j < i; j++)
      if (M(surface[nodes[i].source]).tryMerge(surface[nodes[j].source]))
      {
        nodes[j].numNeighbors++;
        nodes[i].numNeighbors++;
      }

  // sort by number of neighbors, descending
  eastl::sort(nodes.begin(), nodes.end(),
    [](const MergeNode &a, const MergeNode &b) -> bool { return a.numNeighbors > b.numNeighbors; });

  // cut nodes without neighbors
  for (uint32_t i = 0; i < numNodes; i++)
    if (nodes[i].numNeighbors == 0)
    {
      numNodes = i;
      break;
    }

  if (numNodes < 2)
  {
    for (uint32_t i = 0; i < old_to_new.size(); i++)
      old_to_new[i] = i;
    return old_to_new.size();
  }

  // greedy clique algo
  Bitarray isMerged(tmpmem);
  isMerged.resize(numNodes);
  uint32_t numBlocks = 0;
  for (uint32_t i = 0; i < numNodes; i++)
  {
    if (isMerged[i])
      continue;

    M merger(surface[nodes[i].source]);
    old_to_new[nodes[i].source] = numBlocks;
    for (uint32_t j = i + 1; j < numNodes; j++)
    {
      if (isMerged[j] or !merger.tryMerge(surface[nodes[j].source]))
        continue;
      isMerged.set(j);
      old_to_new[nodes[j].source] = numBlocks;
    }
    numBlocks++;
  }

  // add unmerged blocks
  for (uint32_t i = numNodes; i < old_to_new.size(); i++)
    old_to_new[nodes[i].source] = numBlocks++;

  return numBlocks;
}

template <bool use0, bool use1>
struct RgbaMerger
{
  static constexpr int MAX_DIFF = 2;
  static inline bool rgba_eq(RgbaPixel32 a, RgbaPixel32 b)
  {
    return abs(int(a.r) - b.r) <= MAX_DIFF and abs(int(a.g) - b.g) <= MAX_DIFF and abs(int(a.b) - b.b) <= MAX_DIFF and
           abs(int(a.a) - b.a) <= MAX_DIFF;
  }

  carray<RgbaPixel32, 4 * 4> rgba0, rgba1;
  uint16_t mask;
  RgbaMerger(const VoxelSurfaceBlock &b) : rgba0(b.rgba0), rgba1(b.rgba1), mask(b.mask) {}

  bool tryMerge(const VoxelSurfaceBlock &s)
  {
    for (uint32_t i = 0, overlap = s.mask & mask; overlap != 0; i++, overlap >>= 1)
    {
      if ((overlap & 1) == 0)
        continue;
      if (use0 and !rgba_eq(rgba0[i], s.rgba0[i]))
        return false;
      if (use1 and !rgba_eq(rgba1[i], s.rgba1[i]))
        return false;
    }
    for (uint32_t i = 0, m = s.mask & ~mask; m != 0; i++, m >>= 1)
    {
      if ((m & 1) == 0)
        continue;
      rgba0[i] = s.rgba0[i];
      rgba1[i] = s.rgba1[i];
    }
    mask |= s.mask;
    return true;
  }
};

struct NormMerger
{
  static constexpr int MAX_DIFF = 2;
  static inline bool rg_eq(TexPixelRg<uint8_t> a, TexPixelRg<uint8_t> b)
  {
    return abs(int(a.r) - b.r) <= MAX_DIFF and abs(int(a.g) - b.g) <= MAX_DIFF;
  }

  carray<TexPixelRg<uint8_t>, 4 * 4> norm;
  uint16_t mask;
  NormMerger(const VoxelSurfaceBlock &b) : norm(b.norm), mask(b.mask) {}

  bool tryMerge(const VoxelSurfaceBlock &s)
  {
    for (uint32_t i = 0, overlap = s.mask & mask; overlap != 0; i++, overlap >>= 1)
    {
      if ((overlap & 1) == 0)
        continue;
      if (!rg_eq(norm[i], s.norm[i]))
        return false;
    }
    for (uint32_t i = 0, m = s.mask & ~mask; m != 0; i++, m >>= 1)
    {
      if ((m & 1) == 0)
        continue;
      norm[i] = s.norm[i];
    }
    mask |= s.mask;
    return true;
  }
};

template <typename T>
struct BinaryMerger
{
  T data;
  BinaryMerger(const T &d) : data(d) {}
  bool tryMerge(const T &s) { return memcmp(&data, &s, sizeof(T)) == 0; }
};

template <typename T>
static void fill_unused_pixels(T &block)
{
  uint32_t mask = block.mask;
  while (mask != 0xffff)
  {
    uint32_t nextMask = mask;
    for (uint32_t y = 0, m = 1, i = 0; y < 4; y++)
      for (uint32_t x = 0; x < 4; x++, i++, m <<= 1)
      {
        if ((mask & m) != 0)
          continue;
        if (y > 0 and (mask & (m >> 4)) != 0)
          block.copyPixel(i - 4, i);
        else if (x > 0 and (mask & (m >> 1)) != 0)
          block.copyPixel(i - 1, i);
        else if (x < 3 and (mask & (m << 1)) != 0)
          block.copyPixel(i + 1, i);
        else if (y < 3 and (mask & (m << 4)) != 0)
          block.copyPixel(i + 4, i);
        else
          continue;
        nextMask |= m;
      }
    mask = nextMask;
  }
}

static void remap_norm_indices(carray<RenderableInstanceLodsResSrc::VoxelFace, 6> &faces, uint32_t max_layers,
  dag::ConstSpan<int> old_to_new)
{
  for (auto &face : faces)
    for (auto &layers : face.blockIndex)
      for (uint32_t li = 0; li < max_layers; li++)
      {
        uint32_t &index = layers[li].norm;
        if (index == 0)
          continue;
        index = old_to_new[index - 1] + 1;
      }
}

static void remap_rgba_indices(carray<RenderableInstanceLodsResSrc::VoxelFace, 6> &faces, uint32_t max_layers,
  dag::ConstSpan<int> old_to_new)
{
  for (auto &face : faces)
    for (auto &layers : face.blockIndex)
      for (uint32_t li = 0; li < max_layers; li++)
      {
        uint32_t &index = layers[li].rgba01;
        if (index == 0)
          continue;
        index = old_to_new[index - 1] + 1;
      }
}

bool RenderableInstanceLodsResSrc::buildVoxelMips(const DataBlock &blk, const DataBlock &default_params, voxelcache::CacheDump *cache)
{
  const IPoint3 srcSize = cache->mapSize;
  const IPoint3 srcBitMul(1, srcSize.z * srcSize.x, srcSize.x);
  const IPoint3 srcMin = cache->ibbox[0];

  const auto vblk = blk.getBlockByNameEx("voxel_impostor");
  const uint32_t maxLayers = clamp<int>(vblk->getInt("maxLayers", default_params.getInt("maxLayers", 2)), 1, MAX_VOXEL_LAYERS);
  numVoxelLayers = maxLayers;

  // build bitmap of solid source voxels
  Bitarray srcSolid;
  srcSolid.setPtr(cache->insideBits.data(), srcSize.x * srcSize.y * srcSize.z);

  auto isSrcSolid = [&srcSolid, srcSize](const IPoint3 &p) -> int {
    if (uint32_t(p.x) >= uint32_t(srcSize.x))
      return 0;
    if (uint32_t(p.y) >= uint32_t(srcSize.y))
      return 0;
    if (uint32_t(p.z) >= uint32_t(srcSize.z))
      return 0;
    return srcSolid.get(((p.y * srcSize.z) + p.z) * srcSize.x + p.x);
  };

  for (uint32_t fi = 0; fi < 6; fi++)
  {
    const auto &f = *cache->surface[fi].get();
    const uint32_t mx = srcBitMul[cache->FACE_X[fi]];
    const uint32_t my = srcBitMul[cache->FACE_Y[fi]];
    const uint32_t mz = srcBitMul[cache->FACE_Z[fi]];
    const uint32_t sz = srcSize[cache->FACE_Z[fi]];

    for (uint32_t fy = 0, pi = 0; fy < f.ny; fy++)
    {
      uint32_t bxy = fy * my;
      for (uint32_t fx = 0; fx < f.nx; fx++, pi++, bxy += mx)
      {
        for (uint32_t di = f.pixelIndex[pi], ei = f.pixelIndex[pi + 1]; di < ei; di++)
        {
          uint32_t fz = uint32_t(floorf(f.depth[di]));
          if (fz < sz)
            srcSolid.set(bxy + fz * mz);
        }
      }
    }
  }

  Bitarray coverBits;

  struct SemiVoxel
  {
    uint32_t x, y, z;
    uint32_t cover;
  };
  Tab<SemiVoxel> semiVoxels(tmpmem);
  Tab<SemiVoxel> semiVoxelsBuf(tmpmem);

  double voxelSize = cache->targetVoxelSize;
  for (int iterations = 32; iterations > 0; iterations--)
  {
    const double srcScale = voxelSize / cache->voxelSize;
    const IPoint3 rawSize = ipoint3(ceil(point3(DPoint3::xyz(cache->ibbox.width()) / srcScale)));
    if (!voxelMips.empty() and max(rawSize.x, max(rawSize.y, rawSize.z)) < 4)
      break;

    const IPoint3 numBlocks = (rawSize + 3) >> 2;
    const IPoint3 mapSize = numBlocks << 2;
    const uint32_t srcPerVoxel = uint32_t(ceil(srcScale));
    const uint32_t srcPerVoxelSq = srcPerVoxel * srcPerVoxel;
    auto &mip = voxelMips.emplace_back(numBlocks, float(voxelSize));
    mip.boxMin = point3(DPoint3::xyz(srcMin) * cache->voxelSize) + cache->boxMin;
    mip.rawSize = rawSize;
    debug("voxel mip %dx%dx%d", P3D(mapSize));

    if (int maxBlocks = max(max(numBlocks.x, numBlocks.y), numBlocks.z); maxBlocks > (1 << VOXEL_DEPTH_BITS))
    {
      ISSUE_FATAL("Voxel map is too big (%d > %d)", maxBlocks, 1 << VOXEL_DEPTH_BITS);
      return false;
    }

    // compute coverage
    {
      coverBits.resize(srcPerVoxelSq * 3);
      semiVoxels.clear();

      // walk blocks and voxels in Morton order
      uint32_t bx = 0, by = 0, bz = 0;
      while (bx < numBlocks.x or by < numBlocks.y or bz < numBlocks.z)
      {
        if (bx < numBlocks.x and by < numBlocks.y and bz < numBlocks.z)
        {
          uint64_t blockBits = 0;
          uint32_t lx = 0, ly = 0, lz = 0;
          while (lx < 4)
          {
            IPoint3 mapPos(bx * 4 + lx, by * 4 + ly, bz * 4 + lz);
            IPoint3 srcPos = ipoint3(round(DPoint3::xyz(mapPos) * srcScale)) + srcMin;
            IPoint3 cover = IPoint3::ZERO;
            coverBits.reset();

            for (uint32_t fy = 0; fy < srcPerVoxel; fy++)
              for (uint32_t fz = 0; fz < srcPerVoxel; fz++)
                for (uint32_t fx = 0; fx < srcPerVoxel; fx++)
                  if (isSrcSolid(IPoint3(fx, fy, fz) + srcPos))
                  {
                    if (!coverBits.exchange(fy * srcPerVoxel + fz, true))
                      cover.x++;
                    if (!coverBits.exchange(fz * srcPerVoxel + fx + srcPerVoxelSq, true))
                      cover.y++;
                    if (!coverBits.exchange(fx * srcPerVoxel + fy + srcPerVoxelSq * 2, true))
                      cover.z++;
                  }

            uint32_t maxCover = max(max(cover.x, cover.y), cover.z);
            if (maxCover == 0) {}
            else if (maxCover == srcPerVoxelSq)
              blockBits |= uint64_t(1) << (ly * 4 * 4 + lz * 4 + lx);
            else
              semiVoxels.emplace_back(P3D(mapPos), maxCover);

            uint32_t c = lx & ly & lz;
            uint32_t cx = c ^ (c + 1);
            uint32_t cy = cx & lx;
            uint32_t cz = cy & ly;
            lx ^= cx;
            ly ^= cy;
            lz ^= cz;
          }

          mip.bitmap[(by * numBlocks.z + bz) * numBlocks.x + bx] = blockBits;
        }
        uint32_t c = bx & by & bz;
        uint32_t cx = c ^ (c + 1);
        uint32_t cy = cx & bx;
        uint32_t cz = cy & by;
        bx ^= cx;
        by ^= cy;
        bz ^= cz;
      }
    }

    if (!semiVoxels.empty())
    {
      // add solid voxels
      uint64_t totalCover = 0;
      uint32_t accum = srcPerVoxelSq >> 1; // round to nearest
      uint32_t numSolidAdded = 0;
      for (auto &v : semiVoxels)
      {
        totalCover += v.cover;
        accum += v.cover;
        if (accum < srcPerVoxelSq)
          continue;

        accum -= srcPerVoxelSq;
        numSolidAdded++;
        auto mapPos = IPoint3::xyz(v);
        auto blockPos = mapPos >> 2;
        auto bi = (blockPos.y * numBlocks.z + blockPos.z) * numBlocks.x + blockPos.x;
        mip.bitmap[bi] |= uint64_t(1) << ((mapPos.y & 3) << 4 | (mapPos.z & 3) << 2 | (mapPos.x & 3));
      }
      debug("%d semi-solid voxels, %.2f total coverage, added %d solid voxels", semiVoxels.size(), double(totalCover) / srcPerVoxelSq,
        numSolidAdded);
    }

    // build surface blocks
    Tab<VoxelSurfaceBlock> surface(tmpmem);
    for (uint32_t fi = 0; fi < 6; fi++)
    {
      auto &cf = *cache->surface[fi].get();
      auto &mf = mip.faces[fi];
      const uint32_t nx = numBlocks[cache->FACE_X[fi]];
      const uint32_t ny = numBlocks[cache->FACE_Y[fi]];
      const uint32_t nz = numBlocks[cache->FACE_Z[fi]];

      for (uint32_t by = 0, bi = 0; by < ny; by++)
        for (uint32_t bx = 0; bx < nx; bx++, bi++)
        {
          // find layer depths
          // debug("block %d %d / %d %d", bx, by, nx, ny);
          uint32_t layerDepth[MAX_VOXEL_LAYERS];
          uint32_t layerMask[MAX_VOXEL_LAYERS];
          uint32_t numLayers = 0;
          for (int dz = fi & 1 ? -1 : 1, bz = fi & 1 ? nz - 1 : 0, ez = fi & 1 ? -1 : nz; bz != ez; bz += dz)
          {
            // find surface pixels in this 3d block
            uint32_t mask = 0;
            for (uint32_t ly = 0, lm = 1; ly < 4; ly++)
              for (uint32_t lx = 0; lx < 4; lx++, lm <<= 1)
                for (uint32_t lz = 0; lz < 4; lz++)
                {
                  IPoint3 mapPos;
                  mapPos[cache->FACE_X[fi]] = bx * 4 + lx;
                  mapPos[cache->FACE_Y[fi]] = by * 4 + ly;
                  mapPos[cache->FACE_Z[fi]] = bz * 4 + lz;
                  IPoint3 dp = IPoint3::ZERO;
                  dp[cache->FACE_Z[fi]] = dz;
                  if (mip.isSolid(mapPos) and !mip.isSolid(mapPos - dp))
                    mask |= lm;
                }

            if (mask == 0)
              continue;

            // find covered pixels, add others to previous layers
            uint32_t overlap = numLayers > 0 ? 0 : mask;
            for (int layer = numLayers - 1; layer >= 0; layer--)
            {
              uint32_t covered = mask & layerMask[layer];
              if (layer + 1 < numLayers)
                layerMask[layer + 1] |= covered;
              overlap |= covered;
              mask ^= covered;
              if (layer == 0)
                layerMask[0] |= mask;
            }

            // add new layer
            if (overlap != 0 and numLayers < maxLayers)
            {
              layerDepth[numLayers] = bz;
              layerMask[numLayers] = overlap;
              numLayers++;
            }
          }

          if (numLayers == 0)
            continue;

          // add surface blocks
          uint32_t surfIndex = surface.size();
          surface.append_default(numLayers);

          uint32_t layer = 0;
          for (; layer < numLayers; layer++)
          {
            mf.blockIndex[bi][layer] = {layerDepth[layer], surfIndex + layer + 1};
            surface[surfIndex + layer].mask = layerMask[layer];
          }
          for (; layer < MAX_VOXEL_LAYERS; layer++)
            mf.blockIndex[bi][layer] = {layerDepth[numLayers - 1], surfIndex + numLayers};

          // merge pixels for each layer
          for (layer = 0; layer < numLayers; layer++)
          {
            auto &s = surface[surfIndex + layer];
            uint32_t mask = layerMask[layer];
            const float depth = (layerDepth[layer] * 4 + (fi & 1 ? 4 : 0)) * srcScale + srcMin[cache->FACE_Z[fi]];

            for (uint32_t ly = 0, li = 0, lm = 1; ly < 4; ly++)
              for (uint32_t lx = 0; lx < 4; lx++, li++, lm <<= 1)
              {
                if ((mask & lm) == 0)
                {
                  s.rgba0[li].u = 0;
                  s.rgba1[li].u = 0;
                  s.norm[li] = {0, 0};
                  continue;
                }

                IPoint3 mapPos;
                mapPos[cache->FACE_X[fi]] = bx * 4 + lx;
                mapPos[cache->FACE_Y[fi]] = by * 4 + ly;
                mapPos[cache->FACE_Z[fi]] = 0;
                IPoint3 srcPos = ipoint3(round(DPoint3::xyz(mapPos) * srcScale)) + srcMin;
                uint32_t sx0 = min<uint32_t>(srcPos[cache->FACE_X[fi]], cf.nx);
                uint32_t sy0 = min<uint32_t>(srcPos[cache->FACE_Y[fi]], cf.ny);
                uint32_t sx1 = min(sx0 + srcPerVoxel, cf.nx);
                uint32_t sy1 = min(sy0 + srcPerVoxel, cf.ny);

                VoxelAverager avg;

                auto tracePos = [&avg](voxelcache::BakedFace const &f, uint32_t sx, uint32_t sy, float zcut) {
                  uint32_t si = sy * f.nx + sx;
                  uint32_t pi = f.pixelIndex[si], ei = f.pixelIndex[si + 1];
                  for (; pi < ei; pi++)
                    if (f.depth[pi] >= zcut)
                    {
                      avg.add(f.pixel[pi]);
                      break;
                    }
                };

                auto traceNeg = [&avg](voxelcache::BakedFace const &f, uint32_t sx, uint32_t sy, float zcut) {
                  uint32_t si = sy * f.nx + sx;
                  uint32_t pi = f.pixelIndex[si], ei = f.pixelIndex[si + 1];
                  eastl::swap(pi, ei);
                  for (ei--, pi--; pi != ei; pi--)
                    if (f.depth[pi] <= zcut)
                    {
                      avg.add(f.pixel[pi]);
                      break;
                    }
                };

                // find nearest surface after the depth cut
                if (fi & 1)
                {
                  for (uint32_t sy = sy0; sy < sy1; sy++)
                    for (uint32_t sx = sx0; sx < sx1; sx++)
                      traceNeg(cf, sx, sy, depth);
                }
                else
                {
                  for (uint32_t sy = sy0; sy < sy1; sy++)
                    for (uint32_t sx = sx0; sx < sx1; sx++)
                      tracePos(cf, sx, sy, depth);
                }

                if (avg.empty())
                {
                  // check side faces for surface data at the same voxel
                  coverBits.resize(srcPerVoxel);
                  srcPos[cache->FACE_X[fi]] = sx0;
                  srcPos[cache->FACE_Y[fi]] = sy0;
                  srcPos[cache->FACE_Z[fi]] = uint32_t(depth);
                  IPoint3 axes;
                  axes[cache->FACE_X[fi]] = 0;
                  axes[cache->FACE_Y[fi]] = 1;
                  axes[cache->FACE_Z[fi]] = 2;
                  for (uint32_t ofi = 0; ofi < 6; ofi++)
                  {
                    if (ofi == fi or ofi == (fi ^ 1))
                      continue;
                    const auto &of = *cache->surface[ofi].get();
                    const bool xisu = axes[cache->FACE_X[ofi]] == 2;
                    const uint32_t nu = xisu ? of.nx : of.ny;
                    const uint32_t nv = xisu ? of.ny : of.nx;

                    // U is original depth axis
                    const int dfu = fi & 1 ? -1 : 1;
                    const int fu1 = fi & 1 ? -1 : nu;
                    const int fu0 = fi & 1 ? clamp<int>(int(ceilf(depth)) - 1, -1, nu - 1) : clamp<int>(int(depth), 0, nu);
                    const uint32_t mu = xisu ? 1 : of.nx;

                    // V is face axis orthogonal to U
                    const int fv0 = srcPos[xisu ? cache->FACE_Y[ofi] : cache->FACE_X[ofi]];
                    const int fv1 = min(fv0 + srcPerVoxel, nv);
                    const uint32_t mv = xisu ? of.nx : 1;

                    // depth range from this side face
                    const int z0 = srcPos[cache->FACE_Z[ofi]];

                    for (uint32_t fv = fv0, vi = fv0 * mv; fv < fv1; fv++, vi += mv)
                    {
                      coverBits.reset();
                      for (uint32_t fu = fu0; fu != fu1; fu += dfu)
                      {
                        uint32_t si = vi + fu * mu;
                        uint32_t pi = of.pixelIndex[si], ei = of.pixelIndex[si + 1];
                        for (; pi < ei; pi++)
                        {
                          int z = int(of.depth[pi]) - z0;
                          if (z >= 0 and z < srcPerVoxel and !coverBits.exchange(z, true))
                            avg.add(of.pixel[pi]);
                        }
                      }
                    }
                  }
                }

                if (avg.empty())
                {
                  // check back face, flip normal
                  auto &backFace = *cache->surface[fi ^ 1].get();
                  if (fi & 1)
                  {
                    for (uint32_t sy = sy0; sy < sy1; sy++)
                      for (uint32_t sx = sx0; sx < sx1; sx++)
                        tracePos(backFace, sx, sy, depth - 4 * srcScale);
                  }
                  else
                  {
                    for (uint32_t sy = sy0; sy < sy1; sy++)
                      for (uint32_t sx = sx0; sx < sx1; sx++)
                        traceNeg(backFace, sx, sy, depth + 4 * srcScale);
                  }
                  avg.flipNormal();
                }

                if (avg.empty())
                {
                  // this is bad, check front and back faces ignoring the depth range (better than no data)
                  for (uint32_t sy = sy0; sy < sy1; sy++)
                    for (uint32_t sx = sx0; sx < sx1; sx++)
                      tracePos(cf, sx, sy, 0);
                  auto &backFace = *cache->surface[fi ^ 1].get();
                  for (uint32_t sy = sy0; sy < sy1; sy++)
                    for (uint32_t sx = sx0; sx < sx1; sx++)
                      tracePos(backFace, sx, sy, 0);
                }

                avg.result(s.rgba0[li], s.rgba1[li], s.norm[li]);
              }
          }
        }

      if (0)
      {
        // dump for debugging
        eastl::unique_ptr<TexImage32, tmpmemDeleter> albedo(TexImage32::create(nx * 4, ny * 4, tmpmem)),
          normal(TexImage32::create(nx * 4, ny * 4, tmpmem));
        for (uint32_t layer = 0; layer < maxLayers; layer++)
        {
          TexPixel32 *aptr = albedo->getPixels();
          TexPixel32 *nptr = normal->getPixels();
          for (uint32_t by = 0, bi = 0; by < ny; by++, aptr += nx * 12, nptr += nx * 12)
            for (uint32_t bx = 0; bx < nx; bx++, bi++, aptr += 4, nptr += 4)
            {
              if (uint32_t si = mf.blockIndex[bi][layer].rgba01; si != 0)
              {
                G_ASSERT(si - 1 < surface.size());
                auto &s = surface[si - 1];
                for (uint32_t ly = 0, li = 0, lm = 1; ly < 4; ly++)
                  for (uint32_t lx = 0; lx < 4; lx++, li++, lm <<= 1)
                  {
                    auto &p = aptr[ly * nx * 4 + lx];
                    p.r = s.rgba0[li].r;
                    p.g = s.rgba0[li].g;
                    p.b = s.rgba0[li].b;
                    p.a = (s.mask & lm) ? 255 : 0;
                  }
              }
              else
              {
                for (uint32_t ly = 0; ly < 4; ly++)
                  memset(aptr + ly * nx * 4, 0, 4 * 4);
              }

              if (uint32_t si = mf.blockIndex[bi][layer].norm; si != 0)
              {
                G_ASSERT(si - 1 < surface.size());
                auto &s = surface[si - 1];
                for (uint32_t ly = 0, li = 0, lm = 1; ly < 4; ly++)
                  for (uint32_t lx = 0; lx < 4; lx++, li++, lm <<= 1)
                  {
                    auto &p = nptr[ly * nx * 4 + lx];
                    Point2 on = Point2(s.norm[li].r, s.norm[li].g) * (2.0f / 255) - Point2(1, 1);
                    Point3 n = gamemath::oct_to_p3(on);
                    p.r = uint8_t(roundf((n.x * 0.5f + 0.5f) * 255));
                    p.g = uint8_t(roundf((n.y * 0.5f + 0.5f) * 255));
                    p.b = uint8_t(roundf((n.z * 0.5f + 0.5f) * 255));
                    p.a = (s.mask & lm) ? 255 : 0;
                  }
              }
              else
              {
                for (uint32_t ly = 0, li = 0; ly < 4; ly++)
                  memset(nptr + ly * nx * 4, 0, 4 * 4);
              }
            }

          save_tga32(String(0, "%s_%d_a_%u_%u.tga", assetName.c_str(), voxelMips.size(), fi, layer), albedo->getPixels(), albedo->w,
            albedo->h, albedo->w * 4);
          save_tga32(String(0, "%s_%d_n_%u_%u.tga", assetName.c_str(), voxelMips.size(), fi, layer), normal->getPixels(), normal->w,
            normal->h, normal->w * 4);
        }
      }
    }

    // merge surface blocks
    Tab<int> oldToNew(tmpmem);

    // rgba pixels
    {
      oldToNew.resize(surface.size());
      const int surfNum = merge_surface<RgbaMerger<true, true>>(make_span_const(surface), make_span(oldToNew));
      struct RgbaSurface
      {
        carray<RgbaPixel32, 4 * 4> rgba0, rgba1;
        uint16_t mask;
        void copyPixel(uint32_t from, uint32_t to)
        {
          rgba0[to] = rgba0[from];
          rgba1[to] = rgba1[from];
        }
      };
      Tab<RgbaSurface> surf(tmpmem);
      surf.resize_noinit(surfNum);
      for (auto &d : surf)
        d.mask = 0;
      for (uint32_t i = 0, n = surface.size(); i < n; i++)
      {
        auto &s = surface[i];
        auto &d = surf[oldToNew[i]];
        if (d.mask == 0)
        {
          d.rgba0 = s.rgba0;
          d.rgba1 = s.rgba1;
          d.mask = s.mask;
        }
        else
        {
          for (uint32_t j = 0, m = s.mask & ~d.mask; m != 0; j++, m >>= 1)
          {
            if ((m & 1) == 0)
              continue;
            d.rgba0[j] = s.rgba0[j];
            d.rgba1[j] = s.rgba1[j];
          }
          d.mask |= s.mask;
        }
      }

      remap_rgba_indices(mip.faces, maxLayers, make_span_const(oldToNew));

      // compress blocks and build binary data
      Tab<carray<uint8_t, 4 * 4 * 2>> rgbaBlocks;
      rgbaBlocks.resize_noinit(surfNum);

#if TEX_CANNOT_USE_ISPC
      ISSUE_FATAL("Can't compress voxel surface to BC7 because ISPC is not supported - don't use voxel LODs");
      return false;
#else
      bc7_enc_settings bc7Settings;
      GetProfile_alpha_basic(&bc7Settings);

      for (uint32_t bi = 0; bi < surfNum; bi++)
      {
        auto &s = surf[bi];
        auto &d = rgbaBlocks[bi];
        fill_unused_pixels(s);

        carray<RgbaPixel32, 4 * 4 * 2> pixels;
        for (int y = 0; y < 4; y++)
        {
          memcpy(&pixels[y * 4 + 0 * 4], &s.rgba0[(y) * 4], 4 * 4);
          memcpy(&pixels[y * 4 + 4 * 4], &s.rgba1[(y) * 4], 4 * 4);
        }

        rgba_surface cs{.ptr = (uint8_t *)&pixels[0], .width = 4, .height = 4, .stride = 4 * 4};
        CompressBlocksBC7(&cs, d.data(), &bc7Settings);
        cs.ptr = (uint8_t *)&pixels[4 * 4];
        CompressBlocksBC7(&cs, d.data() + 4 * 4, &bc7Settings);
      }
#endif

      oldToNew.resize(surfNum);
      int num = merge_surface<BinaryMerger<decltype(rgbaBlocks)::value_type>>(make_span_const(rgbaBlocks), make_span(oldToNew));
      mip.rgbaBlocks.resize_noinit(num);
      for (uint32_t i = 0; i < surfNum; i++)
        mip.rgbaBlocks[oldToNew[i]] = rgbaBlocks[i];

      remap_rgba_indices(mip.faces, maxLayers, make_span_const(oldToNew));

      debug("%d rgba blocks", num);
      if (num >= (1 << (32 - VOXEL_DEPTH_BITS / 2)))
      {
        ISSUE_FATAL("Too many voxel surface blocks (%d > %d)", num, 1 << (32 - VOXEL_DEPTH_BITS / 2));
        return false;
      }
    }

    // normals
    {
      oldToNew.resize(surface.size());
      const int surfNum = merge_surface<NormMerger>(make_span_const(surface), make_span(oldToNew));
      struct NormSurface
      {
        carray<TexPixelRg<uint8_t>, 4 * 4> norm;
        uint16_t mask;
        void copyPixel(uint32_t from, uint32_t to) { norm[to] = norm[from]; }
      };
      Tab<NormSurface> surf(tmpmem);
      surf.resize_noinit(surfNum);
      for (auto &d : surf)
        d.mask = 0;
      for (uint32_t i = 0, n = surface.size(); i < n; i++)
      {
        auto &s = surface[i];
        auto &d = surf[oldToNew[i]];
        if (d.mask == 0)
        {
          d.norm = s.norm;
          d.mask = s.mask;
        }
        else
        {
          for (uint32_t j = 0, m = s.mask & ~d.mask; m != 0; j++, m >>= 1)
          {
            if ((m & 1) == 0)
              continue;
            d.norm[j] = s.norm[j];
          }
          d.mask |= s.mask;
        }
      }

      remap_norm_indices(mip.faces, maxLayers, make_span_const(oldToNew));

      // compress blocks
      Tab<carray<uint8_t, 4 * 4>> normBlocks(tmpmem);
      normBlocks.resize_noinit(surfNum);
      for (uint32_t bi = 0; bi < surfNum; bi++)
      {
        auto &s = surf[bi];
        auto &d = normBlocks[bi];
        fill_unused_pixels(s);
        carray<TexPixelRg<uint8_t>, 4 * 4> pixels;
        for (int y = 0; y < 4; y++)
          memcpy(&pixels[y * 4], &s.norm[(y) * 4], 4 * 2);
        CompressBC5(&pixels[0].r, 4, 4, 16, (char *)d.data(), 4 * 2, 2);
      }

      oldToNew.resize(surfNum);
      int num = merge_surface<BinaryMerger<decltype(normBlocks)::value_type>>(make_span_const(normBlocks), make_span(oldToNew));
      mip.normBlocks.resize_noinit(num);
      for (uint32_t i = 0; i < surfNum; i++)
        mip.normBlocks[oldToNew[i]] = normBlocks[i];

      remap_norm_indices(mip.faces, maxLayers, make_span_const(oldToNew));

      debug("%d normal blocks", num);
      if (num >= (1 << (32 - VOXEL_DEPTH_BITS / 2)))
      {
        ISSUE_FATAL("Too many voxel surface blocks (%d > %d)", num, 1 << (32 - VOXEL_DEPTH_BITS / 2));
        return false;
      }
    }

    if (0)
    {
      // dump all compressed blocks for debugging
      int nx = max(int(sqrtf(mip.normBlocks.size())), 1);
      int ny = (mip.normBlocks.size() + nx - 1) / nx;
      eastl::unique_ptr<TexImage32, tmpmemDeleter> normal(TexImage32::create(nx * 4, ny * 4, tmpmem));
      auto ptr = normal->getPixels();
      for (int by = 0, bi = 0; by < ny; by++, ptr += nx * 4 * 3)
        for (int bx = 0; bx < nx; bx++, bi++, ptr += 4)
        {
          carray<TexPixelRg<uint8_t>, 4 * 4> buffer;
          if (bi >= mip.normBlocks.size() or
              !detexDecompressBlockRGTC2(mip.normBlocks[bi].data(), DETEX_MODE_MASK_ALL, DETEX_DECOMPRESS_FLAG_ENCODE, &buffer[0].r))
          {
            for (int y = 0; y < 4; y++)
              memset(ptr + y * nx * 4, 0, 4 * 4);
            continue;
          }
          auto p = ptr;
          for (int y = 0, i = 0; y < 4; y++, p += nx * 4)
            for (int x = 0; x < 4; x++, i++)
            {
              Point2 on = Point2(buffer[i].r, buffer[i].g) * (2.0f / 255) - Point2::ONE;
              Point3 n = gamemath::oct_to_p3(on);
              p[x].r = uint8_t(roundf((n.x * 0.5f + 0.5f) * 255));
              p[x].g = uint8_t(roundf((n.y * 0.5f + 0.5f) * 255));
              p[x].b = uint8_t(roundf((n.z * 0.5f + 0.5f) * 255));
              p[x].a = 255;
            }
        }

      nx = max(int(sqrtf(mip.rgbaBlocks.size())), 1);
      ny = (mip.rgbaBlocks.size() + nx - 1) / nx;
      eastl::unique_ptr<TexImage32, tmpmemDeleter> albedo(TexImage32::create(nx * 4, ny * 4, tmpmem));
      ptr = albedo->getPixels();
      for (int by = 0, bi = 0; by < ny; by++, ptr += nx * 4 * 3)
        for (int bx = 0; bx < nx; bx++, bi++, ptr += 4)
        {
          carray<RgbaPixel32, 4 * 4> buffer;
          if (bi >= mip.rgbaBlocks.size() or !detexDecompressBlockBPTC(mip.rgbaBlocks[bi].data(), DETEX_MODE_MASK_ALL_MODES_BPTC,
                                               DETEX_DECOMPRESS_FLAG_ENCODE, (uint8_t *)buffer.data()))
          {
            for (int y = 0; y < 4; y++)
              memset(ptr + y * nx * 4, 0, 4 * 4);
            continue;
          }
          auto p = ptr;
          for (int y = 0, i = 0; y < 4; y++, p += nx * 4)
            for (int x = 0; x < 4; x++, i++)
            {
              p[x].r = buffer[i].r;
              p[x].g = buffer[i].g;
              p[x].b = buffer[i].b;
              p[x].a = buffer[i].a;
            }
        }

      save_tga32(String(0, "%s_%d_cn.tga", assetName.c_str(), voxelMips.size()), normal->getPixels(), normal->w, normal->h,
        normal->w * 4);
      save_tga32(String(0, "%s_%d_ca.tga", assetName.c_str(), voxelMips.size()), albedo->getPixels(), albedo->w, albedo->h,
        albedo->w * 4);
    }

    if (0)
    {
      // dump top layer compressed blocks for each face, for debugging
      uint32_t fi = 0;
      for (auto &mf : mip.faces)
      {
        const uint32_t nx = numBlocks[cache->FACE_X[fi]];
        const uint32_t ny = numBlocks[cache->FACE_Y[fi]];
        const uint32_t nz = numBlocks[cache->FACE_Z[fi]];
        G_ASSERT(mf.blockIndex.size() == nx * ny);

        eastl::unique_ptr<TexImage32, tmpmemDeleter> albedo(TexImage32::create(nx * 4, ny * 4, tmpmem));
        auto ptr = albedo->getPixels();
        for (int by = 0, bi = 0; by < ny; by++, ptr += nx * 4 * 3)
          for (int bx = 0; bx < nx; bx++, bi++, ptr += 4)
          {
            carray<RgbaPixel32, 4 * 4> buffer;
            uint32_t ci = mf.blockIndex[bi][0].rgba01 - 1;
            if (ci >= mip.rgbaBlocks.size() or !detexDecompressBlockBPTC(mip.rgbaBlocks[ci].data(), DETEX_MODE_MASK_ALL_MODES_BPTC,
                                                 DETEX_DECOMPRESS_FLAG_ENCODE, (uint8_t *)buffer.data()))
            {
              for (int y = 0; y < 4; y++)
                memset(ptr + y * nx * 4, 0, 4 * 4);
              continue;
            }
            auto p = ptr;
            for (int y = 0, i = 0; y < 4; y++, p += nx * 4)
              for (int x = 0; x < 4; x++, i++)
              {
                p[x].r = buffer[i].r;
                p[x].g = buffer[i].g;
                p[x].b = buffer[i].b;
                p[x].a = buffer[i].a;
                IPoint3 ip;
                ip[cache->FACE_X[fi]] = bx * 4 + x;
                ip[cache->FACE_Y[fi]] = by * 4 + y;
                auto zc = cache->FACE_Z[fi];
                bool solid = false;
                for (int z = 0; z < nz * 4; z++)
                {
                  ip[zc] = z;
                  if (mip.isSolid(ip))
                  {
                    solid = true;
                    break;
                  }
                }
                p[x].a = solid ? 255 : 0;
              }
          }

        save_tga32(String(0, "%s_%d_ca_%d.tga", assetName.c_str(), voxelMips.size(), fi), albedo->getPixels(), albedo->w, albedo->h,
          albedo->w * 4);
        fi++;
      }
    }

    voxelSize *= 2;
  }

  debug("%d voxel mips", voxelMips.size());
  return true;
}

VECTORCALL VECMATH_FINLINE vec4i v_cross3i(vec4i a, vec4i b)
{
  // (a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
  vec4i yzxw = v_permi_yzxw(a);
  vec4i bcad = v_permi_yzxw(b);
  return v_permi_yzxy(v_subi(v_muli(a, bcad), v_muli(yzxw, b)));
}

VECTORCALL VECMATH_FINLINE vec4i v_dot3i(vec4i a, vec4i b)
{
  vec4i m = v_muli(a, b);
  return v_addi(v_addi(v_splat_xi(m), v_splat_yi(m)), v_splat_zi(m));
}

VECTORCALL VECMATH_FINLINE vec4i v_dot4i_x(vec4i a, vec4i b)
{
  vec4i m = v_muli(a, b);
  m = v_addi(m, v_permi_yyww(m));
  return v_addi(m, v_splat_zi(m));
}

VECTORCALL VECMATH_FINLINE vec4i v_striple3i(vec4i a, vec4i b, vec4i c) { return v_dot3i(v_cross3i(a, b), c); }

struct VoxelHullMesh
{
  struct HalfEdge
  {
    int vertex, face, reverse, next;
    HalfEdge() = default;
    HalfEdge(int v, int f, int r, int n) { set(v, f, r, n); }
    void set(int v, int f, int r, int n)
    {
      vertex = v;
      face = f;
      reverse = r;
      next = n;
    }
    void setFree() { vertex = -1; }
    bool isFree() const { return vertex < 0; }
  };

  struct Vert
  {
    vec4i p;
    Vert() {}
    Vert(vec4i v) : p(v) {}
    void setFree() { p = v_zeroi(); }
    bool isFree() const { return v_extract_wi(p) != 1; }
  };

  struct Face
  {
    vec4i plane;
    int e[3];
    int _padding;

    Face() = default;
    Face(int a, int b, int c) : plane{}, _padding{} { set(a, b, c); }
    void set(int a, int b, int c)
    {
      e[0] = a;
      e[1] = b;
      e[2] = c;
    }
    void setFree() { e[0] = -1; }
    bool isFree() const { return e[0] < 0; }
  };

  Tab<Vert> verts;
  Tab<Face> faces;
  Tab<HalfEdge> edges;

  VoxelHullMesh() : verts(tmpmem), faces(tmpmem), edges(tmpmem) {}

  void swap(VoxelHullMesh &m)
  {
    verts.swap(m.verts);
    faces.swap(m.faces);
    edges.swap(m.edges);
  }

  int addEdge(int v0, int v1, int fi)
  {
    int ei = edges.size() - 1;
    for (; ei >= 0; ei--)
      if (edges[ei].isFree())
        break;
    if (ei < 0)
    {
      ei = edges.size();
      edges.push_back();
    }

    edges[ei].set(v0, fi, -1, -1);

    for (auto &e : edges)
    {
      if (e.isFree())
        continue;
      if (e.vertex == v0 and e.next >= 0 and edges[e.next].vertex == v1)
      {
        debug("duplicate edge %d (%d %d) when adding %d", &e - edges.data(), v0, v1, ei);
        G_ASSERT(false);
      }
      if (e.vertex == v1 and e.next >= 0 and edges[e.next].vertex == v0)
      {
        if (e.reverse != -1)
        {
          debug("edge %d already has reverse edge %d (when adding edge %d)", &e - edges.data(), e.reverse, ei);
          G_ASSERT(false);
        }
        e.reverse = ei;
        edges[ei].reverse = &e - edges.data();
      }
    }
    return ei;
  }

  int addFace(int v0, int v1, int v2)
  {
    int fi = faces.size() - 1;
    for (; fi >= 0; fi--)
      if (faces[fi].isFree())
        break;
    if (fi < 0)
    {
      fi = faces.size();
      faces.push_back();
    }

    int e0 = addEdge(v0, v1, fi);
    int e1 = addEdge(v1, v2, fi);
    int e2 = addEdge(v2, v0, fi);
    faces[fi].set(e0, e1, e2);
    edges[e0].next = e1;
    edges[e1].next = e2;
    edges[e2].next = e0;
    return fi;
  }

  void initOctahedron(IPoint3 size)
  {
    verts.reserve(6);
    faces.reserve(8);
    edges.reserve(2 * 12);

    verts.emplace_back(v_make_vec4i(0, 0, 0, 1));
    verts.emplace_back(v_make_vec4i(0, 0, size.z * 2, 1));
    verts.emplace_back(v_make_vec4i(size.x * 2, 0, 0, 1));
    verts.emplace_back(v_make_vec4i(size.x, size.y, size.z, 1));
    verts.emplace_back(v_make_vec4i(size.x, size.y, -size.z, 1));
    verts.emplace_back(v_make_vec4i(-size.x, size.y, size.z, 1));

    addFace(0, 1, 2);
    addFace(3, 5, 4);
    addFace(0, 4, 5);
    addFace(1, 0, 5);
    addFace(1, 5, 3);
    addFace(2, 1, 3);
    addFace(2, 3, 4);
    addFace(0, 2, 4);

#if DAGOR_DBGLEVEL > 0
    verify();
#endif
  }

  void verify() const
  {
    for (int fi = 0; fi < faces.size(); fi++)
    {
      if (faces[fi].isFree())
        continue;
      for (int i = 0; i < 3; i++)
      {
        int ei = faces[fi].e[i];
        if (ei < 0 or ei >= edges.size() or edges[ei].isFree() or edges[ei].face != fi)
        {
          debug("invalid face[%d].e[%d] = %d/%d", fi, i, ei, edges.size());
          G_ASSERT(false);
        }
        if (edges[ei].face != fi)
        {
          debug("mislinked face[%d].e[%d]->%d.face=%d", fi, i, ei, edges[ei].face);
          G_ASSERT(false);
        }
      }
    }

    for (int ei = 0; ei < edges.size(); ei++)
    {
      auto &e = edges[ei];
      if (e.isFree())
        continue;
      if (e.vertex < 0 or e.vertex >= verts.size() or e.face < 0 or e.face >= faces.size() or e.reverse < 0 or
          e.reverse >= edges.size() or e.next < 0 or e.next >= edges.size())
      {
        debug("invalid edge[%d]: v=%d/%d f=%d/%d r=%d/%d n=%d/%d", ei, e.vertex, verts.size(), e.face, faces.size(), e.reverse,
          edges.size(), e.next, edges.size());
        G_ASSERT(false);
      }
      G_ASSERT(!verts[e.vertex].isFree());
      G_ASSERT(!faces[e.face].isFree());
      G_ASSERT(!edges[e.reverse].isFree());
      G_ASSERT(!edges[e.next].isFree());
      G_ASSERT(e.reverse != ei);
      G_ASSERT(e.next != ei);
      if (e.vertex == edges[e.reverse].vertex)
      {
        debug("invalid edge[%d].reverse=%d same vertex %d", ei, e.reverse, e.vertex);
        G_ASSERT(false);
      }
      if (edges[e.reverse].face == e.face)
      {
        debug("invalid edge[%d].reverse=%d same face %d", ei, e.reverse, e.face);
        G_ASSERT(false);
      }
      G_ASSERT(edges[e.next].face == e.face);
      auto &f = faces[e.face];
      int i = 0;
      for (; i < 3; i++)
        if (f.e[i] == ei)
        {
          G_ASSERT(f.e[(i + 1) % 3] == e.next);
          break;
        }
      G_ASSERT(i < 3);
      for (i = 0; i < ei; i++)
        if (edges[i].vertex == e.vertex and edges[edges[i].reverse].vertex == edges[e.reverse].vertex)
        {
          debug("duplicate edges %d and %d v %d %d r %d %d n %d %d f %d %d", ei, i, e.vertex, edges[e.reverse].vertex, e.reverse,
            edges[i].reverse, e.next, edges[i].next, e.face, edges[i].face);
          G_ASSERT(false);
        }
    }
  }

  bool rotateEdge(uint32_t di)
  {
    G_ASSERT(!edges[di].isFree());
    HalfEdge de = edges[di];
    int ri = de.reverse;
    HalfEdge re = edges[ri];

    // check for duplicate edge issue
    for (auto &e : edges)
      if (!e.isFree() and e.vertex == de.vertex and edges[e.reverse].vertex == re.vertex)
        return false;

    int d1 = de.next;
    int d2 = edges[d1].next;
    int r1 = re.next;
    int r2 = edges[r1].next;

    faces[de.face].set(di, r2, d1);
    faces[re.face].set(ri, d2, r1);

    edges[di].vertex = edges[d2].vertex;
    edges[di].next = r2;
    edges[r2].face = de.face;
    edges[r2].next = d1;
    edges[d1].next = di;

    edges[ri].vertex = edges[r2].vertex;
    edges[ri].next = d2;
    edges[d2].face = re.face;
    edges[d2].next = r1;
    edges[r1].next = ri;

    verify();
    return true;
  }

  void splitEdge(uint32_t ei)
  {
    G_ASSERT(!edges[ei].isFree());
    HalfEdge e = edges[ei];
    int v0 = e.vertex;
    int v1 = edges[e.next].vertex;
    int er = edges[edges[e.reverse].next].next;
    int v0s = edges[edges[e.next].next].vertex;
    int v1s = edges[er].vertex;

    int vi = verts.size() - 1;
    for (; vi >= 0; vi--)
      if (verts[vi].isFree())
        break;
    if (vi < 0)
    {
      vi = verts.size();
      verts.push_back();
    }
    verts[vi].p = v_srai(v_addi(verts[v0].p, verts[v1].p), 1);

    edges[e.next].vertex = vi;
    edges[e.reverse].vertex = vi;

    int nr = edges[e.next].reverse;
    int err = edges[er].reverse;

    edges[nr].reverse = -1;
    edges[e.next].reverse = -1;

    edges[err].reverse = -1;
    edges[er].reverse = -1;

    addFace(vi, v1, v0s);
    addFace(v1, vi, v1s);

    // verify();
  }

  void collapseHalfEdge(uint32_t ei)
  {
    G_ASSERT(ei < edges.size());
    G_ASSERT(!edges[ei].isFree());

    HalfEdge e0 = edges[ei];
    HalfEdge e1 = edges[e0.next];
    HalfEdge e2 = edges[e1.next];
    G_ASSERT(e2.next == ei);

    faces[e0.face].setFree();
    edges[ei].setFree();
    edges[e0.next].setFree();
    edges[e1.next].setFree();
    edges[e1.reverse].reverse = e2.reverse;
    edges[e2.reverse].reverse = e1.reverse;
  }

  bool collapseEdge(uint32_t ei)
  {
    G_ASSERT(ei < edges.size());
    G_ASSERT(!edges[ei].isFree());
    HalfEdge e = edges[ei];
    int v1 = edges[e.reverse].vertex;
    int va = edges[edges[e.next].next].vertex;
    int vb = edges[edges[edges[e.reverse].next].next].vertex;

    dag::FixedVectorSet<int, 16, true, eastl::use_self<int>, TmpmemAlloc> outVerts;
    for (uint32_t ni = e.reverse;;)
    {
      ni = edges[edges[ni].reverse].next;
      if (ni == e.reverse)
        break;
      G_ASSERT(edges[ni].vertex == v1);
      int ov = edges[edges[ni].next].vertex;
      if (ov != va and ov != vb)
        G_VERIFY(outVerts.insert(ov).second);
    }

    verts[e.vertex].setFree();
    for (uint32_t ni = ei;;)
    {
      ni = edges[edges[ni].reverse].next;
      if (ni == ei)
        break;
      G_ASSERT(edges[ni].vertex == e.vertex);
      edges[ni].vertex = v1;

      // check if collapsing will create a duplicate edge
      int ov = edges[edges[ni].next].vertex;
      if (ov != va and ov != vb and outVerts.contains(ov))
        return false;
    }

    collapseHalfEdge(ei);
    collapseHalfEdge(e.reverse);

    // verify();
    return true;
  }

  bool computePlanes()
  {
    for (auto &f : faces)
    {
      if (f.isFree())
        continue;
      vec4i p0 = verts[edges[f.e[0]].vertex].p;
      vec4i p1 = verts[edges[f.e[1]].vertex].p;
      vec4i p2 = verts[edges[f.e[2]].vertex].p;
      vec4i n = v_cross3i(v_subi(p2, p0), v_subi(p1, p0));
      if (v_test_all_bits_zeros(v_cast_vec4f(n)))
        return false;
      vec4i d = v_negi(v_dot3i(n, p0));
      f.plane = v_permi_xyzd(n, d);
    }
    return true;
  }

  // expecting p.w == 1
  bool isInside(vec4i p) const
  {
    for (auto &f : faces)
    {
      if (f.isFree())
        continue;
      vec4i d = v_dot4i_x(f.plane, p);
      if (v_extract_xi(v_cmp_gti(d, v_zeroi())))
        return false;
    }
    return true;
  }

  bool isConvex() const
  {
    for (auto &v : verts)
      if (!v.isFree() and !isInside(v.p))
        return false;
    return true;
  }

  int computeVolume() const
  {
    vec4i volume = v_zeroi();
    for (auto &f : faces)
    {
      if (f.isFree())
        continue;
      volume =
        v_addi(volume, v_striple3i(verts[edges[f.e[0]].vertex].p, verts[edges[f.e[1]].vertex].p, verts[edges[f.e[2]].vertex].p));
    }
    return -v_extract_xi(volume);
  }

  double computeScore() const
  {
    double sum = 0, sum2 = 0;
    uint32_t num = 0;
    for (auto &f : faces)
    {
      if (f.isFree())
        continue;
      vec4i p0 = verts[edges[f.e[0]].vertex].p;
      vec4i p1 = verts[edges[f.e[1]].vertex].p;
      vec4i p2 = verts[edges[f.e[2]].vertex].p;
      vec3f n = v_cross3(v_cvt_vec4f(v_subi(p2, p0)), v_cvt_vec4f(v_subi(p1, p0)));
      float area = v_extract_x(v_length3(n));
      sum += area;
      sum2 += sqr<double>(area);
      num++;
    }

    double len = 0;
    for (auto &e : edges)
    {
      if (e.isFree())
        continue;
      len += v_extract_x(v_length3_x(v_cvt_vec4f(v_subi(verts[edges[e.next].vertex].p, verts[e.vertex].p))));
    }

    // total area + face area deviation + sum of edge length^2
    return sum + sqrt(sum2 / num - sqr(sum / num)) + len;
  }

  uint32_t countFaces() const
  {
    uint32_t numFaces = 0;
    for (auto &f : faces)
      if (!f.isFree())
        numFaces++;
    return numFaces;
  }

  bool dump(const char *filename) const
  {
    eastl::unique_ptr<void, DagorFileCloser> file(df_open(filename, DF_CREATE | DF_WRITE));
    if (!file)
      return false;
    uint8_t header[80];
    memset(header, 0, sizeof(header));
    df_write(file.get(), header, sizeof(header));
    uint32_t numFaces = countFaces();
    df_write(file.get(), &numFaces, 4);

    Point3 v[4];
    v[0] = Point3::ZERO;
    uint16_t attr = 0;

    for (auto &f : faces)
    {
      if (f.isFree())
        continue;
      for (int i = 0; i < 3; i++)
      {
        v_stu_p3(&v[i + 1].x, v_cvti_vec4f(verts[edges[f.e[i]].vertex].p));
        eastl::swap(v[i + 1].y, v[i + 1].z);
      }
      df_write(file.get(), v, 12 * 4);
      df_write(file.get(), &attr, 2);
    }

    return true;
  }
};

bool RenderableInstanceLodsResSrc::buildVoxelHull(const DataBlock &blk, const DataBlock &default_voxel_params)
{
  if (voxelMips.empty())
  {
    ISSUE_FATAL("No voxel mips - can't build convex hull for voxels");
    return false;
  }
  const auto &mip = voxelMips[0];

  Tab<vec4i> hullPoints(tmpmem);
  {
    Tab<Point3> points(tmpmem);
    points.reserve(((mip.numBlocks.x + mip.numBlocks.z) * mip.numBlocks.y + mip.numBlocks.x * mip.numBlocks.z) * 4 * 4);
    const IPoint3 nv = mip.numBlocks << 2;
    uint32_t prevSolid = 0;
    for (int y = 0; y <= nv.y; y++)
      for (int z = 0; z <= nv.z; z++)
        for (int x = 0; x <= nv.x; x++)
        {
          uint32_t solid = 0;
          if (mip.isSolid(IPoint3(x, y - 1, z - 1)))
            solid++;
          if (mip.isSolid(IPoint3(x, y - 1, z + 0)))
            solid++;
          if (mip.isSolid(IPoint3(x, y + 0, z - 1)))
            solid++;
          if (mip.isSolid(IPoint3(x, y + 0, z + 0)))
            solid++;
          if (solid + prevSolid > 0 and solid + prevSolid < 8)
            points.emplace_back(x, y, z);
          prevSolid = solid;
        }

    ConvexHullComputer hull;
    hull.compute(&points.data()->x, sizeof(Point3), points.size(), 0, 0);
    debug("Computed %d convex hull points from %d voxel points", hull.vertices.size(), points.size());
    hullPoints.resize(hull.vertices.size());
    for (uint32_t i = 0, n = hullPoints.size(); i < n; i++)
      v_sti(&hullPoints[i], v_cvt_roundi(v_insert_w(hull.vertices[i], 1)));

    if (0)
    {
      // dump hull for debugging
      eastl::unique_ptr<void, DagorFileCloser> file(df_open(String(260, "%s_hull.stl", assetName.c_str()), DF_CREATE | DF_WRITE));
      if (file)
      {
        uint8_t header[80];
        memset(header, 0, sizeof(header));
        df_write(file.get(), header, sizeof(header));
        uint32_t numFaces = 0;
        df_write(file.get(), &numFaces, 4);

        Point3 v[4];
        v[0] = Point3::ZERO;
        uint16_t attr = 0;

        for (auto startIndex : hull.faces)
        {
          const auto *edge = &hull.edges[startIndex];
          const auto targetVert = edge->getTargetVertex();
          v_stu_p3(&v[1].x, hull.vertices[targetVert]);
          eastl::swap(v[1].y, v[1].z);
          edge = edge->getNextEdgeOfFace();
          v_stu_p3(&v[2].x, hull.vertices[edge->getTargetVertex()]);
          eastl::swap(v[2].y, v[2].z);
          while (true)
          {
            auto vert = edge->getTargetVertex();
            if (vert == targetVert)
              break;
            v_stu_p3(&v[3].x, hull.vertices[vert]);
            eastl::swap(v[3].y, v[3].z);
            df_write(file.get(), v, 12 * 4);
            df_write(file.get(), &attr, 2);
            numFaces++;
            v[2] = v[3];
            edge = edge->getNextEdgeOfFace();
          }
        }

        df_seek_to(file.get(), sizeof(header));
        df_write(file.get(), &numFaces, 4);
      }
    }
  }

  if (0)
  {
    // dump voxels for debugging
    eastl::unique_ptr<void, DagorFileCloser> file(df_open(String(260, "%s_vox.stl", assetName.c_str()), DF_CREATE | DF_WRITE));
    if (file)
    {
      uint8_t header[80];
      memset(header, 0, sizeof(header));
      df_write(file.get(), header, sizeof(header));
      uint32_t numFaces = 0;
      df_write(file.get(), &numFaces, 4);

      Point3 v[4];
      v[0] = Point3::ZERO;
      uint16_t attr = 0;

      const IPoint3 nv = mip.numBlocks << 2;
      uint32_t prev = 0;
      for (int y = 0; y <= nv.y; y++)
        for (int z = 0; z <= nv.z; z++)
          for (int x = 0; x <= nv.x; x++)
          {
            uint32_t cur = mip.isSolid(IPoint3(x, y, z));

            auto writeQuad = [&](Point3 vx, Point3 vy) {
              eastl::swap(vx.y, vx.z);
              eastl::swap(vy.y, vy.z);
              v[1] = Point3(x, z, y);
              v[2] = v[1] + vx;
              v[3] = v[2] + vy;
              if (cur == 0)
                eastl::swap(v[2], v[3]);
              df_write(file.get(), v, 12 * 4);
              df_write(file.get(), &attr, 2);
              numFaces++;

              if (cur == 0)
                eastl::swap(v[2], v[3]);
              v[2] = v[3];
              v[3] = v[1] + vy;
              if (cur == 0)
                eastl::swap(v[2], v[3]);
              df_write(file.get(), v, 12 * 4);
              df_write(file.get(), &attr, 2);
              numFaces++;
            };

            if (cur != prev)
              writeQuad(Point3(0, 1, 0), Point3(0, 0, 1));
            if (cur != mip.isSolid(IPoint3(x, y - 1, z)))
              writeQuad(Point3(0, 0, 1), Point3(1, 0, 0));
            if (cur != mip.isSolid(IPoint3(x, y, z - 1)))
              writeQuad(Point3(1, 0, 0), Point3(0, 1, 0));

            prev = cur;
          }

      df_seek_to(file.get(), sizeof(header));
      df_write(file.get(), &numFaces, 4);
    }
  }

  // start with bounding octahedron
  VoxelHullMesh mesh;
  mesh.initOctahedron(mip.rawSize + IPoint3::ONE);
  G_VERIFY(mesh.computePlanes());

  auto curScore = mesh.computeScore();
  auto bestScore = curScore;
#if DAGOR_DBGLEVEL > 0
  // sanity check
  G_ASSERT(curScore > 0);
  G_ASSERT(mesh.isConvex());
  for (vec4i p : hullPoints)
    G_ASSERT(mesh.isInside(p));
#endif

  // simulated annealing with mesh perturbation
  constexpr uint32_t maxTriangles = 20;
  const int numIter = 300'000;
  VoxelHullMesh curMesh = mesh;
  VoxelHullMesh pmesh;

  auto testMesh = [&](int iter) {
    if (!pmesh.computePlanes())
      return;
    if (!pmesh.isConvex())
      return;
    if (!eastl::all_of(hullPoints.begin(), hullPoints.end(), [&pmesh](vec4i p) { return pmesh.isInside(p); }))
      return;
    auto score = pmesh.computeScore();
    if (score - iter * bestScore / (numIter * 10) > curScore)
      return;
    curScore = score;
    curMesh.swap(pmesh);
    if (score <= bestScore)
    {
      bestScore = score;
      mesh = curMesh;
    }
  };

  for (int iter = numIter; iter >= 0; iter--)
  {
    // move a vertex
    {
      pmesh = curMesh;
      uint32_t vi;
      for (int t = 0;; t++)
      {
        vi = (uint64_t(uint_noise2D(0, t, iter)) * pmesh.verts.size()) >> 32;
        if (!pmesh.verts[vi].isFree())
          break;
      }
      uint32_t noise = uint_noise1D(1, iter);
      int dx = int((noise >> 0) & 7) + (iter & 1) - 4;
      int dy = int((noise >> 4) & 7) + (iter & 1) - 4;
      int dz = int((noise >> 8) & 7) + (iter & 1) - 4;
      vec4i dp = v_make_vec4i(dx, dy, dz, 0);
      pmesh.verts[vi].p = v_addi(pmesh.verts[vi].p, dp);
      testMesh(iter);
    }

    // rotate edge
    {
      pmesh = curMesh;
      uint32_t ei;
      for (int t = 0;; t++)
      {
        ei = (uint64_t(uint_noise2D(2, t, iter)) * pmesh.edges.size()) >> 32;
        if (!pmesh.edges[ei].isFree())
          break;
      }
      if (pmesh.rotateEdge(ei))
        testMesh(iter);
    }

    // add new vertex
    if (curMesh.countFaces() + 2 <= maxTriangles + iter * maxTriangles / numIter)
    {
      pmesh = curMesh;
      uint32_t ei;
      for (int t = 0;; t++)
      {
        ei = (uint64_t(uint_noise2D(2, t, iter)) * pmesh.edges.size()) >> 32;
        if (!pmesh.edges[ei].isFree())
          break;
      }
      pmesh.splitEdge(ei);
      testMesh(iter);
    }

    // remove vertex
    if (curMesh.countFaces() >= 6)
    {
      pmesh = curMesh;
      uint32_t ei;
      for (int t = 0;; t++)
      {
        ei = (uint64_t(uint_noise2D(3, t, iter)) * pmesh.edges.size()) >> 32;
        if (!pmesh.edges[ei].isFree())
          break;
      }
      if (uint_noise1D(4, iter) & 1)
        ei = pmesh.edges[ei].reverse;
      if (pmesh.collapseEdge(ei))
        testMesh(iter);
    }
  }

  // final pass
  curMesh = mesh;
  for (int ei = curMesh.edges.size() - 1; ei >= 0; ei--)
  {
    if (curMesh.edges[ei].isFree())
      continue;
    pmesh = curMesh;
    if (pmesh.collapseEdge(ei))
      testMesh(0);
  }

  // store hull mesh
  {
    int numFaces = mesh.countFaces();
    vec4f voxelSize = v_splats(mip.voxelSize);
    vec3f boxMin = v_ldu_p3(&mip.boxMin.x);

    voxelHull.vert.clear();
    voxelHull.vert.reserve(mesh.verts.size());
    Tab<int> vertMap(tmpmem);
    vertMap.resize(mesh.verts.size());
    for (int i = 0, j = 0, n = mesh.verts.size(); i < n; i++)
      if (!mesh.verts[i].isFree())
      {
        vertMap[i] = j++;
        vec3f p = v_madd(v_cvt_vec4f(mesh.verts[i].p), voxelSize, boxMin);
        v_stu_p3(&voxelHull.vert.push_back_noinit().x, p);
      }

    voxelHull.face.resize(numFaces);
    int fi = 0;
    for (const auto &f : mesh.faces)
    {
      if (f.isFree())
        continue;
      auto &mf = voxelHull.face[fi];
      for (int i = 0; i < 3; i++)
        mf.v[i] = vertMap[mesh.edges[f.e[i]].vertex];
      mf.smgr = 1;
      mf.mat = 0;
      fi++;
    }
    G_ASSERT(fi == numFaces);

    debug("optimized voxel hull has %d triangles", numFaces);
    if (0)
      mesh.dump(String(260, "%s_vh.stl", assetName.c_str()));
  }

  return true;
}
