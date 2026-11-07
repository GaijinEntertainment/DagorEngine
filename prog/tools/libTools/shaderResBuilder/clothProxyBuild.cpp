// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "clothProxyBuild.h"

#include <libTools/shaderResBuilder/dynSceneResSrc.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/util/iLogWriter.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_clothSimChannels.h>
#include <math/dag_bits.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>

// Fails the asset but lets build() finish, so AV still gets a loadable resource; dabuild keys on log.hasErrors().
#define ISSUE_ASSET_ERROR(...)                         \
  do                                                   \
  {                                                    \
    if (log)                                           \
      log->addMessage(ILogWriter::ERROR, __VA_ARGS__); \
    else                                               \
      logerr(__VA_ARGS__);                             \
  } while (0)

// WARNING/NOTE reach AV's console only through the writer (a bare logwarn/debug never does); neither fails the build.
#define ISSUE_ASSET_WARNING(...)                         \
  do                                                     \
  {                                                      \
    if (log)                                             \
      log->addMessage(ILogWriter::WARNING, __VA_ARGS__); \
    else                                                 \
      logwarn(__VA_ARGS__);                              \
  } while (0)

#define ISSUE_ASSET_NOTE(...)                         \
  do                                                  \
  {                                                   \
    if (log)                                          \
      log->addMessage(ILogWriter::NOTE, __VA_ARGS__); \
    else                                              \
      debug(__VA_ARGS__);                             \
  } while (0)

struct ClothChannelOffsets
{
  int index = -1;
  int weight = -1;
  bool valid() const { return index >= 0 && weight >= 0; }
};

struct ExtraChannelDest
{
  int vbu = -1, vbui = -1;
  bool declared() const { return vbu >= 0; }
};

// vDesc records where a channel LANDS, not its source: searching it for extra[N] never matches, ask the material.
// "Not declared" is also the runtime's opt-out test (get_skin_channel_offsets).
static ExtraChannelDest find_extra_channel_dest(const ShaderMaterial &mat, int usage_index)
{
  class DestCB : public ShaderChannelsEnumCB
  {
  public:
    const int wantUsageIndex;
    ExtraChannelDest dest;
    DestCB(int want_usage_index) : wantUsageIndex(want_usage_index) {}
    void enum_shader_channel(int u, int ui, int, int vbu, int vbui, ChannelModifier, int) override
    {
      if (u == SCUSAGE_EXTRA && ui == wantUsageIndex)
        dest = ExtraChannelDest{vbu, vbui};
    }
  } cb(usage_index);
  int flags = 0;
  mat.enum_channels(cb, flags);
  return cb.dest;
}

static int find_extra_channel_offset(const ShaderMeshData::RElem &elem, int usage_index)
{
  if (!elem.vertexData || !elem.mat)
    return -1;
  const ExtraChannelDest dest = find_extra_channel_dest(*elem.mat, usage_index);
  return dest.declared() ? find_vertex_channel_offset(*elem.vertexData, dest.vbu, dest.vbui) : -1;
}

static ClothChannelOffsets get_cloth_channel_offsets(const ShaderMeshData::RElem &elem)
{
  ClothChannelOffsets ofs;
  ofs.index = find_extra_channel_offset(elem, CLOTH_SKINNING_INDEX_EXTRA_CHANNEL);
  ofs.weight = find_extra_channel_offset(elem, CLOTH_SKINNING_WEIGHT_EXTRA_CHANNEL);
  return ofs;
}

template <typename Callable>
static void for_each_skin_elem(DynamicRenderableSceneLodsResSrc::Lod &lod, Callable cb)
{
  for (DynamicRenderableSceneLodsResSrc::SkinnedObj &skin : lod.skins)
    for (ShaderSkinnedMeshData *md : skin.meshData)
      if (md)
        for (ShaderMeshData::RElem &elem : md->getShaderMeshData().elems)
          cb(elem);
}

// Must match gather_cloth_mesh_es, or the solver and the baked bindings pick different meshes.
static ShaderMeshData::RElem *find_cloth_proxy_elem(DynamicRenderableSceneLodsResSrc::Lod &proxy_lod, int *matches)
{
  ShaderMeshData::RElem *found = nullptr;
  int count = 0;
  for_each_skin_elem(proxy_lod, [&](ShaderMeshData::RElem &elem) {
    if (!elem.vertexData || !elem.mat)
      return;
    if (!find_extra_channel_dest(*elem.mat, CLOTH_SIM_WEIGHT_EXTRA_CHANNEL).declared())
      return;
    count++;
    if (!found)
      found = &elem;
  });
  if (matches)
    *matches = count;
  return found;
}

static bool lod_uses_cloth_skinning(DynamicRenderableSceneLodsResSrc::Lod &lod)
{
  bool anyCloth = false;
  for_each_skin_elem(lod, [&](ShaderMeshData::RElem &elem) { anyCloth |= get_cloth_channel_offsets(elem).valid(); });
  return anyCloth;
}

// Indices rebased from vdata-global to elem-local; empty when any index leaves the elem's vertex range.
// Faces dedup by unordered vertex triple, first copy kept - must be consistent with gather_cloth_mesh_es
// (cloth sim runtime change), or a two-sided cage would hide winding flips and skew the paint census.
static Tab<int> gather_cage_faces(const ShaderMeshData::RElem &elem)
{
  Tab<int> out(tmpmem);
  const bool is32 = elem.vertexData->iData32.size() > 0;
  const int rebase = max(elem.sv, 0);
  out.reserve(elem.numf * 3);
  eastl::hash_set<uint64_t> seenFaces;
  seenFaces.reserve(elem.numf);
  for (int i = 0; i < elem.numf; i++)
  {
    int v[3];
    for (int k = 0; k < 3; k++)
    {
      const int at = elem.si + i * 3 + k;
      v[k] = (is32 ? (int)elem.vertexData->iData32[at] : (int)elem.vertexData->iData[at]) - rebase;
      if (v[k] < 0 || v[k] >= elem.numv)
      {
        out.clear();
        return out;
      }
    }
    const int lo = min(min(v[0], v[1]), v[2]), hi = max(max(v[0], v[1]), v[2]);
    const uint64_t key = (uint64_t(lo) << 42) | (uint64_t(v[0] + v[1] + v[2] - lo - hi) << 21) | uint64_t(hi);
    if (!seenFaces.insert(key).second)
      continue;
    out.push_back(v[0]);
    out.push_back(v[1]);
    out.push_back(v[2]);
  }
  return out;
}

// Undirected key high, direction low, so sorting groups both halves of an edge.
static void validate_cage_winding(const Tab<int> &faces, const char *fname, ILogWriter *log)
{
  Tab<uint64_t> dirEdges(tmpmem);
  dirEdges.reserve(faces.size());
  for (int f = 0; f < faces.size() / 3; f++)
    for (int e = 0; e < 3; e++)
    {
      const int a = faces[f * 3 + e], b = faces[f * 3 + (e + 1) % 3];
      if (a == b)
        continue;
      const uint64_t key = ((uint64_t)min(a, b) << 32) | (uint64_t)max(a, b);
      dirEdges.push_back((key << 1) | (a < b ? 0u : 1u));
    }
  eastl::sort(dirEdges.begin(), dirEdges.end());
  int flipped = 0, edges = 0;
  for (int i = 0; i < dirEdges.size();)
  {
    const uint64_t key = dirEdges[i] >> 1;
    int forward = 0, reverse = 0;
    while (i < dirEdges.size() && (dirEdges[i] >> 1) == key)
      (dirEdges[i++] & 1) ? reverse++ : forward++;
    edges++;
    if (forward + reverse == 2 && (forward == 2 || reverse == 2))
      flipped++;
  }
  // Only a warning: computeBindNormals (cloth sim runtime change) re-orients each component, so only a
  // non-orientable weld really hurts.
  if (flipped)
    ISSUE_ASSET_WARNING("cloth proxy lod '%s': %d of %d edges with inconsistent triangle winding", fname, flipped, edges);
}

// Greedy first-fit in face order: order dependent, so it estimates the runtime's colour count, not bounds it.
static int count_cage_edge_colours(const Tab<int> &faces, int num_verts, int cap)
{
  const int maskWords = (cap + 31) / 32;
  eastl::hash_set<uint64_t> seenEdges;
  seenEdges.reserve(faces.size());
  Tab<uint32_t> vertMask(tmpmem);
  vertMask.resize(num_verts * maskWords);
  mem_set_0(vertMask);
  int colorCount = 0;
  for (int f = 0; f < faces.size() / 3; f++)
    for (int e = 0; e < 3; e++)
    {
      const int a = faces[f * 3 + e], b = faces[f * 3 + (e + 1) % 3];
      if (a == b || !seenEdges.insert((uint64_t(min(a, b)) << 32) | uint64_t(max(a, b))).second)
        continue;
      const uint32_t *m0 = &vertMask[a * maskWords], *m1 = &vertMask[b * maskWords];
      int c = cap;
      for (int w = 0; w < maskWords; w++)
        if (const uint32_t freeBits = ~(m0[w] | m1[w]))
        {
          c = w * 32 + (int)__bsf_unsafe(freeBits);
          break;
        }
      if (c >= cap)
        return cap + 1;
      vertMask[a * maskWords + c / 32] |= 1u << (c & 31);
      vertMask[b * maskWords + c / 32] |= 1u << (c & 31);
      colorCount = max(colorCount, c + 1);
    }
  return colorCount;
}

// Byte decode must be consistent with the cloth_authored_* helpers (cloth sim runtime change; E3DCOLOR
// word: R=w>>16, G=w>>8, B=w).
static void validate_cage_paint(const ShaderMeshData::RElem &elem, const Tab<int> &faces, const char *fname, ILogWriter *log)
{
  const int weightOfs = find_extra_channel_offset(elem, CLOTH_SIM_WEIGHT_EXTRA_CHANNEL);
  const int posOfs = find_vertex_channel_offset(*elem.vertexData, SCUSAGE_POS, 0);
  if (weightOfs < 0 || posOfs < 0)
  {
    ISSUE_ASSET_ERROR("cloth proxy lod '%s': the built cage vertex layout is missing the %s channel, so the paint "
                      "and island audits cannot run and the cage cannot simulate",
      fname, weightOfs < 0 ? "sim weight" : "position");
    return;
  }
  const int numVerts = elem.numv;
  const int stride = elem.vertexData->stride;
  const uint8_t *vbase = elem.vertexData->vData.data() + max(elem.sv, 0) * stride;
  const auto weightOf = [&](int v) { return *(const uint32_t *)(vbase + v * stride + weightOfs); };
  const auto isKinematic = [](uint32_t w) { return ((w >> 16) & 0xFF) == 0 || ((w >> 8) & 0xFF) == 0; };
  const auto isUnleashed = [](uint32_t w) { return ((w >> 8) & 0xFF) == 255; };

  // Alpha==255 marks a painted vertex. First, because an unpainted cage zero-fills and then reads as all pinned.
  {
    int painted = 0;
    for (int i = 0; i < numVerts; i++)
      painted += (weightOf(i) >> 24) == 255;
    if (painted == 0)
    {
      ISSUE_ASSET_ERROR("cloth proxy lod '%s': the sim cage carries no painted vertex colour at all (R inverse "
                        "mass, G max distance, B backstop) - paint it and re-export",
        fname);
      return;
    }
  }

  Tab<int> root(tmpmem);
  root.resize(numVerts);
  for (int i = 0; i < numVerts; i++)
    root[i] = i;
  auto find = [&root](int x) {
    while (root[x] != x)
      x = root[x] = root[root[x]];
    return x;
  };
  for (int f = 0; f < faces.size() / 3; f++)
  {
    const int a = faces[f * 3], b = faces[f * 3 + 1], c = faces[f * 3 + 2];
    const int r0 = find(a);
    root[find(b)] = r0;
    root[find(c)] = r0;
  }

  // -1 marks an anchored component; >= 0 counts the component's unanchored verts.
  Tab<int> orphanVertsOfRoot(tmpmem);
  orphanVertsOfRoot.resize(numVerts);
  mem_set_0(orphanVertsOfRoot);
  int pins = 0, leashed = 0;
  for (int i = 0; i < numVerts; i++)
  {
    const uint32_t w = weightOf(i);
    const bool kinematic = isKinematic(w), unleashed = isUnleashed(w);
    pins += kinematic;
    leashed += !kinematic && !unleashed;
    int &n = orphanVertsOfRoot[find(i)];
    if (kinematic || !unleashed) // anchored: pinned, or leashed to its skinned pose
      n = -1;
    else if (n >= 0)
      n++;
  }
  int islands = 0, orphanVerts = 0;
  BBox3 orphanBox;
  for (int i = 0; i < numVerts; i++)
  {
    const int r = find(i);
    if (orphanVertsOfRoot[r] <= 0)
      continue;
    if (r == i)
    {
      islands++;
      orphanVerts += orphanVertsOfRoot[r];
    }
    orphanBox += *(const Point3 *)(vbase + i * stride + posOfs);
  }

  const int distanceColours = count_cage_edge_colours(faces, numVerts, CLOTH_MAX_EDGE_COLORS);

  // The greedy count stops at the cap (count_cage_edge_colours returns cap + 1), so over-cap is a bound, not a count.
  ISSUE_ASSET_NOTE("cloth proxy lod '%s': paint census: %d pinned, %d leashed, %d unleashed of %d verts; %d unanchored "
                   "island(s); %s%d edge colours (cap %d)",
    fname, pins, leashed, numVerts - pins - leashed, numVerts, islands, distanceColours > CLOTH_MAX_EDGE_COLORS ? "over " : "",
    min(distanceColours, (int)CLOTH_MAX_EDGE_COLORS), CLOTH_MAX_EDGE_COLORS);

  if (islands)
    ISSUE_ASSET_ERROR("cloth proxy lod '%s': %d disconnected island(s) with no pinned or leashed painted vertex "
                      "(%d of %d verts, bbox y[%.2f..%.2f]) - nothing anchors them in game. Weld the cage, or pin "
                      "or leash every island.",
      fname, islands, orphanVerts, numVerts, orphanBox[0].y, orphanBox[1].y);
  // Only a warning: the runtime colours a superset of these edges, but in its own order.
  if (distanceColours > CLOTH_MAX_EDGE_COLORS)
    ISSUE_ASSET_WARNING("cloth proxy lod '%s': the distance constraints alone greedy-colour to over %d edge colours - "
                        "the cage is unlikely to fit the solver. Reduce vertex valence.",
      fname, CLOTH_MAX_EDGE_COLORS);
}

static void validateClothProxyLod(DynamicRenderableSceneLodsResSrc::Lod &proxy_lod, ILogWriter *log)
{
  int cageMatches = 0;
  const ShaderMeshData::RElem *proxyElemPtr = find_cloth_proxy_elem(proxy_lod, &cageMatches);
  if (!proxyElemPtr)
  {
    ISSUE_ASSET_ERROR("cloth proxy lod '%s' has no material carrying the sim weight channel (extra[%d]); the sim "
                      "cage must use dynamic_cloth_sim_proxy",
      proxy_lod.fileName.str(), (int)CLOTH_SIM_WEIGHT_EXTRA_CHANNEL);
    return;
  }
  if (cageMatches > 1)
    ISSUE_ASSET_ERROR("cloth proxy lod '%s' has %d elems with the sim weight channel; the sim cage must be one "
                      "welded mesh with one material",
      proxy_lod.fileName.str(), cageMatches);
  const ShaderMeshData::RElem &proxyElem = *proxyElemPtr;
  const char *fname = proxy_lod.fileName.str();

  if (proxyElem.numv > CLOTH_MAX_VERTICES)
    ISSUE_ASSET_ERROR("cloth proxy lod '%s' has %d vertices, over the solver's limit of %d", fname, proxyElem.numv,
      CLOTH_MAX_VERTICES);

  const Tab<int> faces = gather_cage_faces(proxyElem);
  if (faces.empty())
  {
    if (proxyElem.numf <= 0)
    {
      ISSUE_ASSET_WARNING("cloth proxy lod '%s': the cage has no faces - winding and paint audits skipped", fname);
    }
    else
    {
      ISSUE_ASSET_ERROR("cloth proxy lod '%s': the cage index buffer references vertices outside the elem's own "
                        "vertex range (%d faces) - the runtime refuses such a cage and it never simulates",
        fname, proxyElem.numf);
    }
    return;
  }
  validate_cage_winding(faces, fname, log);
  validate_cage_paint(proxyElem, faces, fname, log);
}

// Only the proxy's positions are read, so a pure cage needs no cloth channels of its own.
static void skinProxyLod(DynamicRenderableSceneLodsResSrc::Lod &lod, DynamicRenderableSceneLodsResSrc::Lod &proxy_lod, ILogWriter *log)
{
  // Indices written below are cage-elem-local: the solver's own particle index space.
  const ShaderMeshData::RElem *proxyElemPtr = find_cloth_proxy_elem(proxy_lod, nullptr);
  if (!proxyElemPtr)
    return;
  const ShaderMeshData::RElem &proxyElem = *proxyElemPtr;
  const int proxyStride = proxyElem.vertexData->stride;
  const int proxyPosOffset = find_vertex_channel_offset(*proxyElem.vertexData, SCUSAGE_POS, 0);
  const int proxyNumVerts = proxyElem.numv;
  if (proxyPosOffset < 0 || proxyNumVerts <= 0)
  {
    ISSUE_ASSET_ERROR("cloth proxy lod '%s' has no usable position channel", proxy_lod.fileName.str());
    return;
  }
  // Copied: in the single-mesh case proxy_lod IS lod, and the loop below writes into that same buffer.
  const Tab<uint8_t> proxyVertexData = proxyElem.vertexData->vData;
  const int proxyBase = max(proxyElem.sv, 0) * proxyStride + proxyPosOffset;

  for_each_skin_elem(lod, [&](ShaderMeshData::RElem &elem) {
    const ClothChannelOffsets ofs = get_cloth_channel_offsets(elem);
    if (!ofs.valid())
      return;
    const int posOffset = elem.vertexData ? find_vertex_channel_offset(*elem.vertexData, SCUSAGE_POS, 0) : -1;
    if (posOffset < 0)
    {
      ISSUE_ASSET_ERROR("cloth lod '%s' has no usable position channel", lod.fileName.str());
      return;
    }

    Tab<uint8_t> &vertexData = elem.vertexData->vData;
    const int stride = elem.vertexData->stride;
    const int elemBase = max(elem.sv, 0) * stride;

    const float FAR_VERT_DIST = 0.5f;
    int farVerts = 0;
    float worstDistSq = 0.0f;

    for (int i = 0; i < elem.numv; i++)
    {
      const Point3 &lodPos = *(const Point3 *)&vertexData[elemBase + i * stride + posOffset];

      // By d^2, not by weight: same order, but the weights can all underflow to zero.
      float vertDistSq[4] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
      uint32_t vertIndeces[4] = {0, 0, 0, 0};
      for (int j = 0; j < proxyNumVerts; j++)
      {
        const Point3 &proxyPos = *(const Point3 *)&proxyVertexData[proxyBase + j * proxyStride];
        const float distSq = (lodPos - proxyPos).lengthSq();
        for (int k = 0; k < 4; k++)
          if (distSq < vertDistSq[k])
          {
            for (int l = 3; l > k; l--)
            {
              vertDistSq[l] = vertDistSq[l - 1];
              vertIndeces[l] = vertIndeces[l - 1];
            }
            vertDistSq[k] = distSq;
            vertIndeces[k] = j;
            break;
          }
      }

      float vertWeights[4];
      float sum = 0;
      for (int j = 0; j < 4; j++)
        sum += vertWeights[j] = expf(-vertDistSq[j] / lod.proxy.clothBindFalloffSq);
      if (sum > 1e-16f && sum <= FLT_MAX) // a negative authored falloff overflows the exp to +inf
        for (int j = 0; j < 4; j++)
          vertWeights[j] /= sum;
      else // hard-bind to the nearest instead of 0/0 = NaN
      {
        vertWeights[0] = 1.0f;
        vertWeights[1] = vertWeights[2] = vertWeights[3] = 0.0f;
      }
      if (vertDistSq[0] > FAR_VERT_DIST * FAR_VERT_DIST)
      {
        farVerts++;
        worstDistSq = max(worstDistSq, vertDistSq[0]);
      }

      float indices[4];
      for (int j = 0; j < 4; j++)
        indices[j] = (float)vertIndeces[j];
      memcpy(&vertexData[elemBase + i * stride + ofs.index], indices, sizeof(indices));
      // color8 = VSDT_E3DCOLOR (B8G8R8A8): via E3DCOLOR, or the shader sees .x and .z swapped.
      *(E3DCOLOR *)&vertexData[elemBase + i * stride + ofs.weight] = E3DCOLOR((uint8_t)(vertWeights[0] * 255.0f),
        (uint8_t)(vertWeights[1] * 255.0f), (uint8_t)(vertWeights[2] * 255.0f), (uint8_t)(vertWeights[3] * 255.0f));
    }
    if (farVerts)
      ISSUE_ASSET_WARNING("cloth lod '%s': %d of %d cloth verts are over %.2fm from the nearest sim cage vertex "
                          "(worst %.2fm) - the cage does not cover the garment there",
        lod.fileName.str(), farVerts, elem.numv, FAR_VERT_DIST, sqrtf(worstDistSq));
  });
}
bool validate_cloth_proxy_type(int proxy_int, const char *lod_file_name, ILogWriter *log)
{
  using ProxyType = DynamicRenderableSceneLodsResSrc::ProxyType;
  if (proxy_int == (int)ProxyType::None || proxy_int == (int)ProxyType::Skin)
    return true;
  ISSUE_ASSET_ERROR("lod '%s' has proxy:i=%d, which is not a proxy type; only %d (cloth sim cage) is defined",
    lod_file_name ? lod_file_name : "?", proxy_int, (int)ProxyType::Skin);
  return false;
}

void build_cloth_proxy_bindings(DynamicRenderableSceneLodsResSrc &res, ILogWriter *log)
{
  using ProxyType = DynamicRenderableSceneLodsResSrc::ProxyType;
  Tab<DynamicRenderableSceneLodsResSrc::Lod> &lods = res.lods;
  // The runtime takes the sim cage from the LAST lod (getLastLodResource).
  DynamicRenderableSceneLodsResSrc::Lod *proxyLod = nullptr;
  int proxyLodIdx = -1;
  for (int i = 0; i < lods.size(); i++)
    if (lods[i].proxy.type == ProxyType::Skin)
    {
      if (proxyLod)
        ISSUE_ASSET_ERROR("the dynmodel declares two cloth proxy lods ('%s' and '%s'); only the last lod may be proxy:i=%d",
          proxyLod->fileName.str(), lods[i].fileName.str(), (int)ProxyType::Skin);
      else if (i != lods.size() - 1)
        ISSUE_ASSET_ERROR("cloth proxy lod '%s' (proxy:i=%d) is not the last lod, where the runtime looks for the sim cage",
          lods[i].fileName.str(), (int)ProxyType::Skin);
      proxyLod = &lods[i];
      proxyLodIdx = i;
    }
  // A proxy lod carrying cloth materials is a render lod too (single-mesh cloth); a pure cage never renders.
  const bool proxyIsRenderMesh = proxyLod && lod_uses_cloth_skinning(*proxyLod);
  // getLodNo picks the FIRST lod whose range covers the distance, so an equal range makes the cage unreachable
  // and keeps getMaxDist at the render range. The authored range of a pure cage is ignored.
  if (proxyLod && !proxyIsRenderMesh && proxyLodIdx > 0)
    proxyLod->range = lods[proxyLodIdx - 1].range;
  int clothRenderLods = 0, renderLods = 0;
  for (int i = 0; i < lods.size(); i++)
    if (lods[i].proxy.type != ProxyType::Skin || proxyIsRenderMesh)
    {
      renderLods++;
      if (lod_uses_cloth_skinning(lods[i]))
        clothRenderLods++;
    }
  if (clothRenderLods != 0 && clothRenderLods != renderLods)
    for (int i = 0; i < lods.size(); i++)
      if (lods[i].proxy.type != ProxyType::Skin && !lod_uses_cloth_skinning(lods[i]))
        ISSUE_ASSET_ERROR("lod '%s' has no cloth_skinning=1 material while %d of the %d render lods do; every "
                          "render lod of a cloth model must opt in",
          lods[i].fileName.str(), clothRenderLods, renderLods);

  if (proxyLod)
  {
    if (clothRenderLods == 0)
      ISSUE_ASSET_ERROR("the dynmodel declares a cloth proxy lod ('%s', proxy:i=%d) but no render lod has a "
                        "cloth_skinning=1 material, so nothing would be skinned to it",
        proxyLod->fileName.str(), (int)ProxyType::Skin);

    validateClothProxyLod(*proxyLod, log);
    // Cage vertex slots are both the baked indices and the solver's particle ids, so the numbering must not
    // move: connectData dedups byte-identical vertices and remaps index buffers only.
    if (ShaderMeshData::RElem *cageElem = find_cloth_proxy_elem(*proxyLod, nullptr))
      cageElem->vertexData->allowVertexMerge = false;
    for (int i = 0; i < lods.size(); i++)
    {
      if (lods[i].proxy.type != ProxyType::Skin && lod_uses_cloth_skinning(lods[i]))
        skinProxyLod(lods[i], *proxyLod, log);
    }
    if (proxyIsRenderMesh)
      skinProxyLod(*proxyLod, *proxyLod, log);
  }
  else if (clothRenderLods != 0)
  {
    for (int i = 0; i < lods.size(); i++)
      if (lod_uses_cloth_skinning(lods[i]))
        ISSUE_ASSET_ERROR("lod '%s' has a cloth_skinning=1 material but the dynmodel declares no 'proxy:i=%d' lod "
                          "to skin the cloth to",
          lods[i].fileName.str(), (int)ProxyType::Skin);
  }
}
