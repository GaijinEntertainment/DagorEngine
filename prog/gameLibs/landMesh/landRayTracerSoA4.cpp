// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/landRayTracerSoA4.h>

#include <math/dag_math2d.h>
#include <math/dag_wooray2d.h>
#include <math/dag_bits.h>
#include <debug/dag_log.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <util/dag_parallelFor.h>
#include <util/dag_threadPool.h>
#include <EASTL/sort.h>

#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_bvhIO.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <daBVH/dag_swBLAS_soa4Queries.h>

// build/load scratch: the record plus owning payload buffers; assemble() concatenates these
// into the single dump and rewrites the record offsets
struct LandRayTracerSoA4::CellTmp
{
  static_assert(sizeof(Cell) % 16 == 0 && alignof(Cell) == 16, "records are copied into the dump as raw bytes");
  Cell meta;
  dag::Vector<uint8_t> soa;   // SoA4 BLAS: [tree][pad][vert21]
  dag::Vector<uint8_t> htMip; // max-height mip chain, box-space u8
};

float LandRayTracerSoA4::Cell::htMaxAt(const uint8_t *ht, float bx, float bz) const
{
  const float invTexel = htDim / 65536.f;
  int tx = clamp((int)(bx * invTexel), 0, htDim - 1), tz = clamp((int)(bz * invTexel), 0, htDim - 1);
  return ht[tz * htDim + tx] * HT_Q_STEP;
}

// consumes/renumbers both arrays in place (leafOrderVertexFetch rewrites idx)
void LandRayTracerSoA4::buildCellGeom(CellTmp &out, dag::Vector<vec4f> &v4, dag::Vector<uint32_t> &idx)
{
  Cell &m = out.meta;
  const int vertCount = (int)v4.size(), idxCount = (int)idx.size();
  if (!vertCount || idxCount < 3)
    return;

  dag::Vector<vec4f> ordered;
  build_bvh::leafOrderVertexFetch(idx.data(), (unsigned)idxCount, v4.data(), (unsigned)vertCount, ordered);
  const vec4f *bverts = ordered.data();
  const int bvertCount = (int)ordered.size();

  Tab<build_bvh::QuadPrim> prims;
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, idx.data(), idxCount / 3, bverts);
  if (prims.empty())
    return;

  dag::Vector<build_bvh::DoubleQuadPrim> dqs;
  build_bvh::buildDoubleQuadPrims(dqs, prims.data(), (int)prims.size(), bverts, nullptr, 1.5f);
  const int dqCount = (int)dqs.size();

  Tab<bbox3f> dqBoxes;
  dqBoxes.resize(dqCount);
  build_bvh::addDoubleQuadPrimitivesAABBList(dqBoxes.data(), dqs.data(), dqCount, bverts);
  Tab<bbox3f> nodes;
  int maxDepth = 0;
  // XZ-only splits: land queries are vertical lines and steep short rays, and terrain is
  // single-valued in Y, so Y splits only ever pair XZ-overlapping children (see SplitAxes).
  int rootNode = build_bvh::create_bvh_node_sah(nodes, dqBoxes.data(), dqCount, 4, maxDepth, build_bvh::SplitAxes::XZ);

  bbox3f worldBox = build_bvh::calcBox(bverts, bvertCount);
  vec3f safeSize = v_max(v_sub(worldBox.bmax, worldBox.bmin), v_splats(0.0001f));
  m.scale = v_and(v_div(v_splats(65535.f), safeSize), v_cast_vec4f(V_CI_MASK1110));
  m.ofs = v_and(v_neg(v_mul(worldBox.bmin, m.scale)), v_cast_vec4f(V_CI_MASK1110));
  m.ofsY = v_extract_y(m.ofs);
  m.invScaleY = 1.f / v_extract_y(m.scale);
  m.minY = v_extract_y(worldBox.bmin);
  m.maxY = v_extract_y(worldBox.bmax);

  // stackless BLAS is build-time scratch: [tree][pad][vert21] (base >= 0 contract, see swBLASLeafDefs)
  const int vertsOfs = (build_bvh::calcBLASTreeBytes((int)nodes.size(), dqCount) + 7) & ~7;
  if ((int64_t)vertsOfs + (int64_t)(bvertCount - 1) * 8 > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    logerr("LandRayTracerSoA4: cell BLAS exceeds 24-bit leaf base range, cell skipped");
    return;
  }
  dag::Vector<uint8_t> stk(vertsOfs + bvertCount * 8, 0);
  int dataOffset = 0;
  build_bvh::writeDoubleQuadBVH2(stk.data(), nodes.data(), dqs.data(), m.scale, m.ofs, vertsOfs, rootNode, rootNode, dataOffset, 8,
    false /*useHalves: UINT16 quantized boxes*/);
  for (int i = 0; i < bvertCount; ++i)
    build_bvh::packVert21(stk.data() + vertsOfs + i * 8, v_madd(bverts[i], m.scale, m.ofs));

  soa4::ConvertResult cr = soa4::buildFromStackless(stk.data(), 0, dataOffset, vertsOfs, bvertCount * 8, out.soa);
  m.root = cr.root;
  m.treeBytes = cr.treeBytes;
  m.vertsOfs = cr.vertsOfs;
  if (!cr.valid())
  {
    logerr("LandRayTracerSoA4: SoA4 conversion failed for a cell with %d tris, cell skipped", idxCount / 3);
    return;
  }
  m.hasGeom = 1;
  finalizeCellAccel(out);
}

// derived acceleration data rebuilt from the SoA4 leaves themselves (shared by build and load):
// the adaptive max-height mip chain and the packet-sharing leaf-extent gate
void LandRayTracerSoA4::finalizeCellAccel(CellTmp &out)
{
  Cell &m = out.meta;
  m.htDim = 2;
  while (m.htDim < 32 && (m.maxY - m.minY) / m.htDim > 20.f)
    m.htDim *= 2;
  m.htLevels = 0;
  int htTotal = 0;
  for (int d = m.htDim; d >= 1; d >>= 1, m.htLevels++)
  {
    m.htLvlOfs[m.htLevels] = htTotal;
    htTotal += d * d;
  }
  out.htMip.clear();
  out.htMip.resize(htTotal, 0);
  const float htInvTexel = m.htDim / 65536.f;
  m.triCount = 0;
  dag::Vector<float> leafExt;
  soa4::iterateLeafRefs(
    out.soa.data(), m.root, [](vec3f, vec3f) { return true; },
    [&](vec3f bmin, vec3f bmax, soa4::LeafRef, const soa4::LeafLoc &loc) {
      m.triCount += (int)quadLeafTriCount(soa4::leafFields(out.soa.data(), loc));
      uint8_t q = (uint8_t)clamp((int)ceilf(v_extract_y(bmax) / HT_Q_STEP), 0, 255);
      int hx0 = clamp((int)(v_extract_x(bmin) * htInvTexel), 0, m.htDim - 1);
      int hx1 = clamp((int)(v_extract_x(bmax) * htInvTexel), 0, m.htDim - 1);
      int hz0 = clamp((int)(v_extract_z(bmin) * htInvTexel), 0, m.htDim - 1);
      int hz1 = clamp((int)(v_extract_z(bmax) * htInvTexel), 0, m.htDim - 1);
      for (int hz = hz0; hz <= hz1; ++hz)
        for (int hx = hx0; hx <= hx1; ++hx)
        {
          uint8_t &d = out.htMip[hz * m.htDim + hx];
          d = max(d, q);
        }
      vec3f e = v_sub(bmax, bmin);
      leafExt.push_back(max(v_extract_x(e), max(v_extract_y(e), v_extract_z(e))));
      return false;
    });
  eastl::sort(leafExt.begin(), leafExt.end());
  m.packetMaxExt = leafExt.empty() ? 0.f : leafExt[leafExt.size() / 20]; // 5th percentile
  for (int l = 1; l < m.htLevels; ++l)                                   // 2x2 max-downsample
  {
    int sd = m.htDim >> (l - 1), dd = m.htDim >> l;
    const uint8_t *src = out.htMip.data() + m.htLvlOfs[l - 1];
    uint8_t *dst = out.htMip.data() + m.htLvlOfs[l];
    for (int z = 0; z < dd; ++z)
      for (int x = 0; x < dd; ++x)
        dst[z * dd + x] = max(max(src[(z * 2) * sd + x * 2], src[(z * 2) * sd + x * 2 + 1]),
          max(src[(z * 2 + 1) * sd + x * 2], src[(z * 2 + 1) * sd + x * 2 + 1]));
  }
}

const LandRayTracerSoA4::DumpHeader LandRayTracerSoA4::emptyHdr = {};
GlobalSharedMemStorage *LandRayTracerSoA4::sharedMem = nullptr;

// drop the claim findPtr/allocPtr took on the shared region; the storage then frees or keeps the
// record by its own policy (local storages free at refcount 0, file-backed ones persist)
void LandRayTracerSoA4::releaseSmRegion()
{
  if (dumpData && sharedMem && sharedMem->doesPtrBelong((void *)dumpData))
    sharedMem->releasePtr(SM_DATA_TAG, (void *)dumpData);
}

LandRayTracerSoA4::~LandRayTracerSoA4() { releaseSmRegion(); }

// run one build callback per cell on the threadpool (serial without workers)
template <class BuildOne>
static void for_each_cell_parallel(int cell_count, const BuildOne &build_one)
{
  struct Ctx
  {
    const BuildOne *buildOne;
  } ctx = {&build_one};
  auto range = [](Ctx &c, uint32_t b, uint32_t e) {
    for (uint32_t i = b; i < e; ++i)
      (*c.buildOne)(i);
  };
  if (threadpool::get_num_workers() > 0)
    threadpool::parallel_for(0, cell_count, 1, [&ctx, &range](uint32_t b, uint32_t e, uint32_t) { range(ctx, b, e); });
  else
    range(ctx, 0, cell_count);
}

// concatenate built cells into the single relocatable dump; false (and an empty dump, so every
// query misses) if no cell has geometry
bool LandRayTracerSoA4::assemble(DumpHeader h, dag::Span<CellTmp> tmp_cells)
{
  resetDump();
  uint64_t ofs = sizeof(DumpHeader) + (uint64_t)tmp_cells.size() * sizeof(Cell);
  bool any = false;
  for (CellTmp &t : tmp_cells)
    if (t.meta.hasGeom)
    {
      ofs = (ofs + 15) & ~15ull; // SoA4 blobs need 16-byte alignment; the u8 mips do not
      t.meta.dataOfs = (uint32_t)ofs;
      t.meta.dataBytes = (uint32_t)t.soa.size();
      ofs += t.soa.size();
      t.meta.htOfs = (uint32_t)ofs;
      ofs += t.htMip.size();
      any = true;
    }
  if (!any || ofs > 0xFFFFFFFFull)
    return false;
  for (CellTmp &t : tmp_cells)
    if (t.meta.hasGeom)
    {
      alignas(16) float scl[4], ofw[4];
      v_st(scl, t.meta.scale);
      v_st(ofw, t.meta.ofs);
      t.meta.wx0 = (0.f - ofw[0]) / scl[0], t.meta.wx1 = (65535.f - ofw[0]) / scl[0];
      t.meta.wz0 = (0.f - ofw[2]) / scl[2], t.meta.wz1 = (65535.f - ofw[2]) / scl[2];
    }
  dump.resize((size_t)ofs, 0);
  h.cellCount = (int)tmp_cells.size();
  h.invCellSize = h.cellSize > 0.f ? 1.f / h.cellSize : 0.f;
  memcpy(dump.data(), &h, sizeof(h));
  Cell *tab = (Cell *)(dump.data() + sizeof(DumpHeader));
  for (int i = 0; i < (int)tmp_cells.size(); ++i)
  {
    CellTmp &t = tmp_cells[i];
    memcpy(&tab[i], &t.meta, sizeof(Cell));
    if (t.meta.hasGeom)
    {
      memcpy(dump.data() + t.meta.dataOfs, t.soa.data(), t.soa.size());
      memcpy(dump.data() + t.meta.htOfs, t.htMip.data(), t.htMip.size());
      // each copied cell releases its scratch immediately: the transient peak tapers instead of
      // holding the full payload beside the assembled dump
      t.soa = {};
      t.htMip = {};
    }
  }
  dumpData = dump.data();
  dumpBytes = (int64_t)ofs;
  return true;
}

// publishing is a separate step, not a side effect of assemble(): only the caller knows the
// stream was accepted, and a ready record is what every sibling process serves
void LandRayTracerSoA4::publishShared(const char *sm_name)
{
  if (!sharedMem || !sm_name || !*sm_name || !dump.size())
    return; // nothing owned to publish: empty, or already serving from a shared record
  bool smNew = false;
  void *p = sharedMem->findOrAlloc(sm_name, SM_DATA_TAG, dump.size(), smNew);
  if (!p)
  {
    logmessage(_MAKE4C('SHMM'), "failed to publish SoA4 land tracer dump '%s' (%dK; mem %lluK/%lluK, rec=%d)", sm_name,
      int(dump.size() >> 10), ((uint64_t)sharedMem->getMemUsed()) >> 10, ((uint64_t)sharedMem->getMemSize()) >> 10,
      sharedMem->getRecUsed());
    return;
  }
  if (!smNew)
  {
    // a sibling published this record while we parsed: serve its copy and drop ours
    dag::Vector<uint8_t> own;
    own.swap(dump); // attach() resets the instance, so keep our dump alive until it took over
    const int64_t bytes = dumpBytes;
    if (attach(p, bytes)) // the record's declared size is gated equal to ours
    {
      logmessage(_MAKE4C('SHMM'), "attached SoA4 land tracer dump published by a sibling: %p, %dK, '%s'", p, int(bytes >> 10),
        sm_name);
      return;
    }
    sharedMem->releasePtr(SM_DATA_TAG, p); // addressing refusal cannot happen for a same-build record; keep the private copy
    dump.swap(own);
    dumpData = dump.data();
    dumpBytes = bytes;
    return;
  }
  G_ASSERT(!(uintptr_t(p) & 15)); // the storage hands out page-aligned records; SoA4 needs 16
  memcpy(p, dump.data(), dump.size());
  mark_global_shared_mem_readonly(p, dump.size(), true); // frozen before attachers can see it
  sharedMem->markPtrDataReady(p);                        // findPtr blocks attachers on this flag until the copy is done
  logmessage(_MAKE4C('SHMM'), "published SoA4 land tracer dump to shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d)", p,
    int(dump.size() >> 10), sm_name, ((uint64_t)sharedMem->getMemUsed()) >> 10, ((uint64_t)sharedMem->getMemSize()) >> 10,
    sharedMem->getRecUsed());
  dump = {};
  dumpData = (const uint8_t *)p; // dumpBytes is unchanged; queries now read the shared record
}

bool LandRayTracerSoA4::build(int cells_x, int cells_y, float cell_size, const Point3 &origin_pos, const BBox3 &land_box,
  dag::ConstSpan<CellSource> src_cells)
{
  if (cells_x <= 0 || cells_y <= 0 || cells_x > 4096 || cells_y > 4096 || !check_finite(cell_size) || cell_size <= 0.f)
    return false;
  if (!check_finite(origin_pos.x + origin_pos.y + origin_pos.z) ||
      !check_finite(land_box[0].x + land_box[0].y + land_box[0].z + land_box[1].x + land_box[1].y + land_box[1].z))
    return false;                            // same contract as load(): a poisoned grid must fail, not silently miss everywhere
  if (src_cells.size() != cells_x * cells_y) // cells_x * cells_y >= 1 is guaranteed above
    return false;
  DumpHeader h;
  h.numCellsX = cells_x;
  h.numCellsY = cells_y;
  h.cellSize = cell_size;
  h.origin = origin_pos;
  h.bbox = land_box;
  h.maxY = land_box[1].y;
  dag::Vector<CellTmp> tmp(src_cells.size());
  for_each_cell_parallel((int)src_cells.size(), [&](uint32_t i) {
    const CellSource &s = src_cells[i];
    const int nv = s.verts.size();
    dag::Vector<vec4f> v4(nv);
    for (int v = 0; v < nv - 1; ++v)
      v4[v] = v_and(v_ldu(&s.verts[v].x), v_cast_vec4f(V_CI_MASK1110));
    if (nv)
      v4[nv - 1] = v_ldu_p3_safe(&s.verts[nv - 1].x);
    dag::Vector<uint32_t> idx(s.indices.size());
    for (int v = 0; v < s.indices.size(); ++v)
      idx[v] = (uint32_t)s.indices[v];
    buildCellGeom(tmp[i], v4, idx);
  });
  return assemble(h, make_span(tmp));
}

// one legacy cell record as it sits on the wire (the pack mapping is stored as raw floats)
struct LegacyCellRec
{
  float ofs[4], scale[4];
  float maxHt;
  uint32_t gridHtStart, gridStart, fistartGridsize, facesStart, vertsStart, resv[2];
};
static_assert(sizeof(LegacyCellRec) == 64, "must match the LTdump on-disk cell record");
static constexpr int LEGACY_HEADER_SIZE = 78; // sign, grid dims, cell size, offset, box, six section counts

LandRayTracerSoA4::LegacyLoadResult LandRayTracerSoA4::loadStreamToDump(IGenLoad &crd, int dump_sz, IMemAlloc *allocator)
{
  // dump_sz is level data and bounds every section below; real streams are two orders below the
  // shared cap, so a larger claim is corrupt and the cap bounds what it can allocate meanwhile
  if (dump_sz < LEGACY_HEADER_SIZE || dump_sz > MAX_STREAM_SIZE)
    return LegacyLoadResult::Failed;
  IMemAlloc *mem = allocator ? allocator : midmem;

  char sign[6];
  crd.read(sign, 6);
  if (memcmp(sign, "LTdump", 6) != 0)
    return LegacyLoadResult::Failed;
  const int cx = crd.readInt(), cy = crd.readInt();
  const float cs = crd.readReal();
  Point3 org;
  BBox3 bb;
  crd.read(&org, sizeof(org));
  crd.read(&bb, sizeof(bb));
  // the whole header is validated before a single section is read, so a malformed grid cannot buy
  // itself the section allocations first
  if (cx < 0 || cy < 0 || cx > 4096 || cy > 4096 || !check_finite(cs) || cs <= 0.f)
    return LegacyLoadResult::Failed;
  if (!check_finite(org.x + org.y + org.z + bb[0].x + bb[0].y + bb[0].z + bb[1].x + bb[1].y + bb[1].z))
    return LegacyLoadResult::Failed;
  if (int64_t(cx) * cy * (int64_t)sizeof(CellTmp) > (256 << 20)) // scratch a huge claimed grid would take
    return LegacyLoadResult::Failed;

  // one cumulative budget: sections capped independently could sum far past the declared size
  // while the earlier tables are still live
  int64_t budget = dump_sz - LEGACY_HEADER_SIZE;
  auto takeSection = [&](int64_t elem_sz) {
    const int cnt = crd.readInt();
    const int64_t bytes = int64_t(cnt) * elem_sz;
    if (cnt < 0 || bytes > budget)
      return -1;
    budget -= bytes;
    return cnt;
  };

  const int cellCnt = takeSection(sizeof(LegacyCellRec));
  if (cellCnt != cx * cy)
    return LegacyLoadResult::Failed;
  Tab<LegacyCellRec> recs(mem);
  recs.resize(cellCnt);
  crd.read(recs.data(), cellCnt * (int)sizeof(LegacyCellRec));
  // the writer serialized whole arrays with the first cell based at their starts: a nonzero first
  // start would silently drop the leading geometry from every span derived below
  if (cellCnt && (recs[0].vertsStart != 0 || recs[0].facesStart != 0))
    return LegacyLoadResult::Failed;

  for (int sec = 0; sec < 2; ++sec) // the grid acceleration sections serve the legacy queries only
  {
    const int cnt = takeSection(4);
    if (cnt < 0)
      return LegacyLoadResult::Failed;
    crd.seekrel(cnt * 4);
  }

  const int faceCnt = takeSection(2);
  if (faceCnt < 0)
    return LegacyLoadResult::Failed;
  Tab<uint16_t> wireFaces(mem);
  wireFaces.resize(faceCnt);
  crd.read(wireFaces.data(), faceCnt * 2);

  const int vertCnt = takeSection(8); // 4 u16 lanes per vertex
  if (vertCnt < 0)
    return LegacyLoadResult::Failed;
  // both tables widen on decode (u16 lanes -> Point3, u16 indices -> int): budget that side too,
  // or a maximal compressible claim drives a multi-GB transient
  if (int64_t(vertCnt) * (int64_t)sizeof(Point3) + int64_t(faceCnt) * (int64_t)sizeof(int) > MAX_STREAM_SIZE)
    return LegacyLoadResult::Failed;
  Tab<uint16_t> wireVerts(mem);
  wireVerts.resize(vertCnt * 4);
  crd.read(wireVerts.data(), vertCnt * 8);
  // the trailing faceIndices section is legacy-query-only: intentionally unread

  // decode into flat arrays and hand the builder spans over them: a cell's vertices and indices
  // are contiguous, so no per-cell containers are needed
  Tab<Point3> vtx(mem);
  Tab<int> idx(mem);
  vtx.resize(vertCnt);
  idx.resize(faceCnt);
  dag::Vector<CellSource> src(cellCnt);
  int64_t tris = 0;
  for (int i = 0; i < cellCnt; ++i)
  {
    const LegacyCellRec &c = recs[i];
    const uint32_t v0 = c.vertsStart, v1 = i + 1 < cellCnt ? recs[i + 1].vertsStart : (uint32_t)vertCnt;
    const uint32_t f0 = c.facesStart, f1 = i + 1 < cellCnt ? recs[i + 1].facesStart : (uint32_t)faceCnt;
    if (v0 > v1 || v1 > (uint32_t)vertCnt || f0 > f1 || f1 > (uint32_t)faceCnt)
      return LegacyLoadResult::Failed;
    const uint32_t nv = v1 - v0, nf = f1 - f0;
    if (nf && (!nv || nf % 3 != 0))
      return LegacyLoadResult::Failed;
    // a geometry-less cell (det-hmap-stripped areas) carries whatever the empty box packed, which
    // is not finite: the legacy runtime never read those records, so skip before validating them
    if (!nv || nf < 3)
      continue;
    // linear decode: if the extreme packed lane stays finite, every vertex does
    if (!check_finite(c.scale[0] * 65535.f + c.ofs[0] + c.scale[1] * 65535.f + c.ofs[1] + c.scale[2] * 65535.f + c.ofs[2]))
      return LegacyLoadResult::Failed;
    for (uint32_t v = 0; v < nv; ++v)
    {
      const uint16_t *q = &wireVerts[(v0 + v) * 4];
      vtx[v0 + v] = Point3(q[0] * c.scale[0] + c.ofs[0], q[1] * c.scale[1] + c.ofs[1], q[2] * c.scale[2] + c.ofs[2]);
    }
    for (uint32_t f = 0; f < nf; ++f)
    {
      const uint16_t vi = wireFaces[f0 + f];
      if (vi >= nv) // indices are cell-local: the builder would read past the cell's vertices
        return LegacyLoadResult::Failed;
      idx[f0 + f] = (int)vi;
    }
    src[i].verts = make_span_const(vtx.data() + v0, nv);
    src[i].indices = make_span_const(idx.data() + f0, nf);
    tris += nf / 3;
  }
  clear_and_shrink(wireFaces); // fully decoded: the wire copies must not outlive into the build
  clear_and_shrink(wireVerts);
  clear_and_shrink(recs);
  if (!tris)
    return LegacyLoadResult::Empty;
  return build(cx, cy, cs, org, bb, make_span_const(src)) ? LegacyLoadResult::Ok : LegacyLoadResult::Failed;
}

static constexpr int LTS4_MAGIC = _MAKE4C('LTS4');
// gates the ENTIRE layout, per-cell records included: any future change to any field, per-cell
// shape included, bumps this; readers reject unknown versions, which leaves LTS4-only levels
// tracer-less (only transitional dual-stream files can still rebuild from their LTdump)
static constexpr int LTS4_VERSION = 1;

bool LandRayTracerSoA4::save(IGenSave &cwr) const
{
  const DumpHeader &h = hdr();
  cwr.writeInt(LTS4_MAGIC);
  cwr.writeInt(LTS4_VERSION);
  cwr.writeInt(h.numCellsX);
  cwr.writeInt(h.numCellsY);
  cwr.writeReal(h.cellSize);
  cwr.writeReal(h.origin.x), cwr.writeReal(h.origin.y), cwr.writeReal(h.origin.z);
  for (int i = 0; i < 2; ++i)
    cwr.writeReal(h.bbox[i].x), cwr.writeReal(h.bbox[i].y), cwr.writeReal(h.bbox[i].z);
  for (int ci = 0; ci < h.cellCount; ++ci)
  {
    const Cell &c = cellsTab()[ci];
    cwr.writeInt(c.hasGeom ? 1 : 0);
    if (!c.hasGeom)
      continue;
    // the exact world->box mapping, so loaded queries are bit-equal to the built instance
    cwr.write(&c.scale, sizeof(c.scale));
    cwr.write(&c.ofs, sizeof(c.ofs));
    // the daBVH serializer walks stackless skip words: round-trip the SoA4 tree back
    dag::Vector<uint8_t> stk;
    const int vertBytes = (int)c.dataBytes - c.vertsOfs;
    soa4::StacklessResult sr = soa4::buildStackless(cellData(c), c.root, c.vertsOfs, vertBytes, stk);
    if (!sr.valid())
      return false;
    bbox3f lb;
    const vec4f safeScale = v_sel(V_C_ONE, c.scale, v_cast_vec4f(V_CI_MASK1110));
    lb.bmin = v_div(v_neg(c.ofs), safeScale);
    lb.bmax = v_div(v_sub(v_splats(65535.f), c.ofs), safeScale);
    if (!build_bvh::serializeQuadBLAS(cwr, stk.data(), sr.treeBytes, sr.vertsOfs, vertBytes / 8, lb))
      return false;
  }
  return true;
}

// the packed world footprint must sit in this cell's own grid slot: landmesh terrain fills each
// cell, so a legit packed center sits well inside half a cell of the slot center and the 0.5*cs
// window rejects any whole-cell grid shift; sub-cell drift waits for the integrity framing
static const char *check_cell_mapping(const float sv[4], const float ov[4], int ci, int cells_x, float cs, const Point3 &org)
{
  if (!check_finite(sv[0] + sv[1] + sv[2] + ov[0] + ov[1] + ov[2]) || sv[0] <= 0.f || sv[1] <= 0.f || sv[2] <= 0.f)
    return "non-finite or non-positive cell mapping"; // would silently poison every query of this cell
  const float wcx = (0.5f * 65535.f - ov[0]) / sv[0], wcz = (0.5f * 65535.f - ov[2]) / sv[2];
  if (!check_finite(wcx + wcz)) // a denormal-tiny scale makes the derived footprint infinite
    return "cell footprint overflows";
  if (fabsf(wcx - (org.x + (float(ci % cells_x) + 0.5f) * cs)) > 0.5f * cs ||
      fabsf(wcz - (org.z + (float(ci / cells_x) + 0.5f) * cs)) > 0.5f * cs)
    return "cell footprint outside its grid slot";
  return nullptr;
}

// the header box must contain the accumulated representable extents within packing epsilon: a
// shrunk box would publish wrong bounds and cull valid queries (an oversized one only over-includes)
static const char *check_extents_in_box(const Point2 &cmin, const Point2 &cmax, float min_y, float max_y, const BBox3 &bb)
{
  if (cmin.x > cmax.x)
    return nullptr; // no geometry-bearing cells
  const float xztol = max(0.1f, 1e-4f * max(cmax.x - cmin.x, cmax.y - cmin.y));
  if (cmin.x < bb[0].x - xztol || cmin.y < bb[0].z - xztol || cmax.x > bb[1].x + xztol || cmax.y > bb[1].z + xztol)
    return "cell extents escape the header box";
  const float ytol = max(0.1f, 1e-4f * (max_y - min_y));
  if (bb[0].y > min_y + ytol || bb[1].y < max_y - ytol)
    return "cell height range escapes the header box";
  return nullptr;
}

const char *LandRayTracerSoA4::validate() const
{
  const DumpHeader &h = hdr();
  Point2 cellsMin(1e30f, 1e30f), cellsMax(-1e30f, -1e30f);
  float cellsMinY = 1e30f, cellsMaxY = -1e30f;
  for (int ci = 0; ci < h.cellCount; ++ci)
  {
    const Cell &m = cellsTab()[ci];
    if (!m.hasGeom)
      continue;
    alignas(16) float sv[4], ov[4];
    v_st(sv, m.scale);
    v_st(ov, m.ofs);
    if (const char *why = check_cell_mapping(sv, ov, ci, h.numCellsX, h.cellSize, h.origin))
      return why;
    if (!check_finite(m.invScaleY + m.minY + m.maxY))
      return "derived cell heights overflow";
    cellsMinY = min(cellsMinY, m.minY), cellsMaxY = max(cellsMaxY, m.maxY);
    cellsMin.x = min(cellsMin.x, (-ov[0]) / sv[0]), cellsMin.y = min(cellsMin.y, (-ov[2]) / sv[2]);
    cellsMax.x = max(cellsMax.x, (65535.f - ov[0]) / sv[0]), cellsMax.y = max(cellsMax.y, (65535.f - ov[2]) / sv[2]);
  }
  return check_extents_in_box(cellsMin, cellsMax, cellsMinY, cellsMaxY, h.bbox);
}

bool LandRayTracerSoA4::load(IGenLoad &crd)
{
  // any failure - a mid-parse exception included - leaves the instance consistently empty: the
  // dump materializes only in the final assemble()
  resetDump();
  if (crd.readInt() != LTS4_MAGIC || crd.readInt() != LTS4_VERSION)
    return false;
  int cx = crd.readInt(), cy = crd.readInt();
  float cs = crd.readReal();
  Point3 org;
  org.x = crd.readReal(), org.y = crd.readReal(), org.z = crd.readReal();
  BBox3 bb;
  for (int i = 0; i < 2; ++i)
    bb[i].x = crd.readReal(), bb[i].y = crd.readReal(), bb[i].z = crd.readReal();
  if (cx <= 0 || cy <= 0 || cx > 4096 || cy > 4096 || !check_finite(cs) || cs <= 0.f)
    return false;
  // budget the scratch record array in bytes, not a raw cell product: this line is what keeps a
  // tiny crafted header from preallocating hundreds of MB before any payload is read
  if ((int64_t)cx * cy * (int64_t)sizeof(CellTmp) > (256 << 20))
    return false;
  if (!check_finite(org.x + org.y + org.z) || !check_finite(bb[0].x + bb[0].y + bb[0].z + bb[1].x + bb[1].y + bb[1].z))
    return false;
  // a finite but inverted box is the setempty sentinel: geometry-bearing streams never carry it,
  // and consumers cull against the published box, so reject rather than repair
  if (bb[0].x > bb[1].x || bb[0].y > bb[1].y || bb[0].z > bb[1].z)
    return false;
  dag::Vector<CellTmp> tmp(cx * cy);
  // the size contract covers the whole decompressed stream: header and per-cell metadata count
  // toward the cap alongside each BLAS's serialized bytes
  int64_t serializedTotal = 5 * 4 + 12 + 24, allocTotal = 0;
  // accumulated per-cell representable extents, reconciled against the header box after the loop
  Point2 cellsMin(1e30f, 1e30f), cellsMax(-1e30f, -1e30f);
  float cellsMinY = 1e30f, cellsMaxY = -1e30f;
  for (int ci = 0; ci < (int)tmp.size(); ++ci)
  {
    CellTmp &t = tmp[ci];
    const int hasGeom = crd.readInt();
    serializedTotal += 4 + (hasGeom == 1 ? 32 : 0);
    if (hasGeom != 0 && hasGeom != 1) // the writer emits exactly 0/1: anything else is corruption
      return false;
    if (!hasGeom)
      continue;
    Cell &m = t.meta;
    crd.read(&m.scale, sizeof(m.scale));
    crd.read(&m.ofs, sizeof(m.ofs));
    alignas(16) float sv[4], ov[4];
    v_st(sv, m.scale);
    v_st(ov, m.ofs);
    if (check_cell_mapping(sv, ov, ci, cx, cs, org))
      return false;
    build_bvh::Soa4DeserializeResult r = build_bvh::deserializeQuadBLASToSoA4(crd, t.soa);
    if (!r.root.valid())
      return false;
    // the exporter<->loader cap is a contract on SERIALIZED bytes (reported by the deserializer:
    // decompressor streams have no tell); the reconstructed SoA layout is budgeted separately
    serializedTotal += r.serializedBytes;
    if (serializedTotal > MAX_STREAM_SIZE)
      return false;
    // the serialized box and the world->box mapping are redundant (the writer derives the box
    // from the mapping): reconciling them catches a finite mutation of either, Y axis included
    alignas(16) float bmin[4], bmax[4];
    v_st(bmin, r.box.bmin);
    v_st(bmax, r.box.bmax);
    // NaN compares false through the tolerance test below, so non-finite values need their own
    // gate; the derived extents can also overflow to inf under a tiny positive scale
    if (!check_finite(bmin[0] + bmin[1] + bmin[2] + bmax[0] + bmax[1] + bmax[2]))
      return false;
    bool boxBad = false;
    for (int a = 0; a < 3; ++a)
    {
      const float dmin = -ov[a] / sv[a], dmax = (65535.f - ov[a]) / sv[a];
      if (!check_finite(dmin + dmax))
        return false;
      const float tol = max(1.f, 1e-4f * (fabsf(dmin) + fabsf(dmax)));
      boxBad |= fabsf(bmin[a] - dmin) > tol || fabsf(bmax[a] - dmax) > tol;
    }
    if (boxBad)
      return false;
    m.root = r.root;
    m.treeBytes = r.treeBytes;
    m.vertsOfs = r.vertsOfs;
    m.ofsY = v_extract_y(m.ofs);
    m.invScaleY = 1.f / v_extract_y(m.scale);
    m.minY = -m.ofsY * m.invScaleY;
    m.maxY = (65535.f - m.ofsY) * m.invScaleY;
    if (!check_finite(m.invScaleY + m.minY + m.maxY))
      return false; // a denormal-tiny scale overflows the derived inverse
    m.hasGeom = 1;
    cellsMinY = min(cellsMinY, m.minY);
    cellsMaxY = max(cellsMaxY, m.maxY);
    cellsMin.x = min(cellsMin.x, (-ov[0]) / sv[0]), cellsMin.y = min(cellsMin.y, (-ov[2]) / sv[2]);
    cellsMax.x = max(cellsMax.x, (65535.f - ov[0]) / sv[0]), cellsMax.y = max(cellsMax.y, (65535.f - ov[2]) / sv[2]);
    finalizeCellAccel(t);
    // runtime allocation budget on per-cell claims; assemble() adds the final dump while releasing
    // each copied cell, so this cap also bounds the whole-load transient peak at roughly 2x
    allocTotal += (int64_t)t.soa.size() + (int64_t)t.htMip.size();
    if (allocTotal > (int64_t)MAX_STREAM_SIZE)
      return false;
  }
  // a valid stream is consumed exactly: a decodable prefix (e.g. a shrunk cell count over the
  // same payload) must not load as a smaller grid with the dropped rows silently missing
  char excess = 0;
  if (crd.tryRead(&excess, 1) != 0)
    return false;
  if (check_extents_in_box(cellsMin, cellsMax, cellsMinY, cellsMaxY, bb))
    return false;
  DumpHeader h;
  h.numCellsX = cx;
  h.numCellsY = cy;
  h.cellSize = cs;
  h.origin = org;
  h.bbox = bb;
  // cell box tops only over-state, so raising the prune top above an understating header top is
  // over-inclusion only (a corrupt lower top would false-negative every query above it); the
  // public bbox keeps the header value
  h.maxY = max(bb[1].y, cellsMaxY);
  return assemble(h, make_span(tmp));
}

int64_t LandRayTracerSoA4::dataSize() const { return dumpBytes; }

// The record's provenance makes content checks redundant, as for the storage's other consumers
// (heightmap, vromfs): the name binds it to this exact level file and stream offset, the storage
// namespace is per build, and ready is set only after the publisher - who parsed and validated
// the stream - froze the pages read-only. Only addressing is checked.
bool LandRayTracerSoA4::attach(const void *dump_data, int64_t dump_bytes)
{
  resetDump();
  const uint8_t *base = (const uint8_t *)dump_data;
  // dump offsets are u32, and SoA4 blobs need a 16-aligned base
  if (!base || (uintptr_t(base) & 15) || dump_bytes < (int64_t)sizeof(DumpHeader) || dump_bytes > (int64_t)0xFFFFFFFFll)
    return false;
  const DumpHeader &h = *(const DumpHeader *)base;
  if (h.cellCount < 0 || (int64_t)sizeof(DumpHeader) + (int64_t)h.cellCount * (int64_t)sizeof(Cell) > dump_bytes)
    return false; // cannot even hold its own record table
  // the caller's storage claim is now this instance's, dropped in the dtor
  dumpData = base;
  dumpBytes = dump_bytes;
  return true;
}

// world normal of the hit sub-triangle: positions map world->box by a per-axis scale, so the box
// normal transforms by the same diagonal (covariance of a diagonal matrix)
static __forceinline void ref_normal_to_world(const uint8_t *data, uint32_t ref, int sub_tri, vec4f scale, Point3 &out_norm)
{
  vec3f a, b, c;
  soa4::fetchLeafTri(data, (soa4::LeafRef)ref, sub_tri, soa4::Vert21Loader{}, a, b, c);
  vec3f n = v_cross3(v_sub(b, a), v_sub(c, a));
  v_stu_p3(&out_norm.x, v_norm3(v_mul(n, scale)));
}

template <bool ANY_HIT>
bool LandRayTracerSoA4::traceCell(const Cell &c, vec3f p, vec3f d, const Point3 &pos, const Point3 &dir, float t_in, float t_out,
  float &t, const Cell **hit_cell, uint32_t *hit_ref, int *hit_sub) const
{
  const float cellMinY = pos.y + dir.y * (dir.y < 0.f ? t_out : t_in);
  if (cellMinY > c.maxY)
    return false;
  const float cellMaxY = pos.y + dir.y * (dir.y < 0.f ? t_in : t_out);
  if (cellMaxY < c.minY)
    return false;
  RayData r;
  r.rayOrigin = v_madd(p, c.scale, c.ofs);
  r.rayDir = v_mul(d, c.scale); // affine map preserves the ray parameter, so r.t stays in world units
  r.calc();
  r.t = t;
  r.bestTriOffset = 0;
  r.data = cellData(c);
  bool hit; // land traces always cull backfaces, like the legacy tracer
  if (ANY_HIT)
    hit = soa4::rayAnyHit<true>(r, c.root);
  else
    hit = soa4::rayClosest<true>(r, c.root);
  if (!hit)
    return false;
  t = r.t;
  if (hit_cell)
  {
    *hit_cell = &c;
    *hit_ref = (uint32_t)r.bestTriOffset;
    *hit_sub = r.bestSubTri;
  }
  return true;
}

// Shared cell traversal for closest-hit and any-hit rays: XZ bounds, then the direct <=2x2
// cell loop or a 2D DDA over the crossed cells in near-to-far order. ANY_HIT returns on the
// first hit anywhere (t stays each cell's untouched budget); closest-hit shrinks t across
// cells and early-breaks the DDA once the best hit precedes the next cell entry.
template <bool ANY_HIT>
bool LandRayTracerSoA4::traceImpl(const Point3 &pos, const Point3 &dir, float &t, const Cell **hit_cell, uint32_t *hit_ref,
  int *hit_sub) const
{
  if (check_nan(pos.x + pos.y + pos.z + dir.x + dir.y + dir.z + t))
    return false;
  t = min(t, 1073741824.f); // the legacy tracer's distance cap; also keeps +inf out of the cell math
  const float segMinY = pos.y + min(0.f, dir.y * t);
  const DumpHeader &h = hdr();
  if (segMinY > h.maxY) // whole segment above everything
    return false;
  vec4f xz01 = v_madd(v_make_vec4f(dir.x, dir.z, dir.x, dir.z), v_make_vec4f(0, 0, t, t), v_make_vec4f(pos.x, pos.z, pos.x, pos.z));
  vec4f mnmx = v_perm_xyab(v_min(xz01, v_perm_zwxy(xz01)), v_max(xz01, v_perm_zwxy(xz01))); // (minx, minz, maxx, maxz)
  alignas(16) int cxz[4];
  v_sti(cxz, v_cvt_floori(v_mul(v_sub(mnmx, v_make_vec4f(h.origin.x, h.origin.z, h.origin.x, h.origin.z)), v_splats(h.invCellSize))));
  const int cx0 = max(cxz[0], 0), cz0 = max(cxz[1], 0);
  const int cx1 = min(cxz[2], h.numCellsX - 1), cz1 = min(cxz[3], h.numCellsY - 1);
  if (cx1 < cx0 || cz1 < cz0)
    return false;
  vec3f p = v_ldu(&pos.x), d = v_ldu(&dir.x);
  bool hit = false;
  if (cx1 - cx0 <= 1 && cz1 - cz0 <= 1)
  {
    for (int cz = cz0; cz <= cz1; ++cz)
      for (int cx = cx0; cx <= cx1; ++cx)
      {
        const Cell &c = cellsTab()[cz * h.numCellsX + cx];
        float tq = t;
        if (c.hasGeom && traceCell<ANY_HIT>(c, p, d, pos, dir, 0.f, t, ANY_HIT ? tq : t, hit_cell, hit_ref, hit_sub))
        {
          if (ANY_HIT)
            return true;
          hit = true;
        }
      }
    return hit;
  }
  IBBox2 limits(IPoint2(0, 0), IPoint2(h.numCellsX - 1, h.numCellsY - 1));
  WooRay2d woo(Point2(pos.x - h.origin.x, pos.z - h.origin.z), Point2(dir.x, dir.z), t, Point2(h.cellSize, h.cellSize), limits);
  IPoint2 diff = woo.getEndCell() - woo.currentCell();
  int n = 2 * (abs(diff.x) + abs(diff.y)) + 1;
  float curt = 0.f;
  double t_ = 0.0;
  for (; n; n--)
  {
    IPoint2 cell = woo.currentCell();
    const bool nextCell = woo.nextCell(t_);
    const float cellExit = nextCell ? min((float)t_, t) : t;
    if (limits & cell)
    {
      const Cell &c = cellsTab()[cell.y * h.numCellsX + cell.x];
      float tq = t;
      if (c.hasGeom && traceCell<ANY_HIT>(c, p, d, pos, dir, curt, cellExit, ANY_HIT ? tq : t, hit_cell, hit_ref, hit_sub))
      {
        if (ANY_HIT)
          return true;
        hit = true;
      }
    }
    if (!nextCell || (!ANY_HIT && t <= cellExit))
      break;
    curt = cellExit;
  }
  return hit;
}

bool LandRayTracerSoA4::traceRay(const Point3 &pos, const Point3 &dir, float &t, Point3 *out_norm) const
{
  const Cell *hitCell = nullptr;
  uint32_t hitRef = 0;
  int hitSub = 0;
  if (!traceImpl<false>(pos, dir, t, &hitCell, &hitRef, &hitSub))
    return false;
  if (out_norm && hitCell)
    ref_normal_to_world(cellData(*hitCell), hitRef, hitSub, hitCell->scale, *out_norm);
  return true;
}

bool LandRayTracerSoA4::rayHit(const Point3 &pos, const Point3 &dir, float t) const
{
  return traceImpl<true>(pos, dir, t, nullptr, nullptr, nullptr);
}

template <bool CULL_DOWN>
bool LandRayTracerSoA4::heightQuery(float x, float z, float limit_y, float &ht, Point3 *out_norm) const
{
  int ci = cellIdxAt(x, z);
  if (ci < 0)
    return false;
  const Cell &c = cellsTab()[ci];
  if (!c.hasGeom)
    return false;
  vec3f p = v_madd(v_make_vec4f(x, limit_y, z, 0), c.scale, c.ofs);
  soa4::HeightAcc acc;
  if (limit_y < 1e29f)
    acc.limit = v_extract_y(p);
  acc.wantNormal = out_norm != nullptr;
  soa4::heightWalk<CULL_DOWN>(cellData(c), (uint32_t)c.root.v, p, acc);
  if (!acc.hit)
    return false;
  ht = (acc.maxHt - c.ofsY) * c.invScaleY;
  if (out_norm)
    v_stu_p3(&out_norm->x, v_norm3(v_mul(acc.normalBox, c.scale)));
  return true;
}

bool LandRayTracerSoA4::getHeight(const Point2 &pos, float &ht, Point3 *out_norm) const
{
  if (check_nan(pos.x + pos.y))
    return false;
  return heightQuery<false>(pos.x, pos.y, 1e30f, ht, out_norm);
}

bool LandRayTracerSoA4::getHeightBelow(const Point3 &pos, float &ht, Point3 *out_norm) const
{
  if (check_nan(pos.x + pos.y + pos.z + ht)) // a NaN window bound would bypass the res < ht contract check
    return false;
  float res;
  if (!heightQuery<true>(pos.x, pos.z, pos.y, res, out_norm))
    return false;
  if (res < ht) // the tracer's contract: only surfaces within [ht, pos.y]
    return false;
  ht = res;
  return true;
}

bool LandRayTracerSoA4::getHeightBounding(const Point2 &pos, float &ht) const
{
  if (check_nan(pos.x + pos.y))
    return false;
  int ci = cellIdxAt(pos.x, pos.y);
  if (ci < 0)
    return false;
  const Cell &c = cellsTab()[ci];
  if (!c.hasGeom)
    return false;
  vec4f bp = v_madd(v_make_vec4f(pos.x, 0, pos.y, 0), c.scale, c.ofs);
  ht = (c.htMaxAt(cellHt(c), v_extract_x(bp), v_extract_z(bp)) - c.ofsY) * c.invScaleY;
  return true;
}

int LandRayTracerSoA4::traceRayPacket(const Point3_vec4 *pos, const Point3_vec4 *dirs, int nrays, float max_t, float *t_io) const
{
  G_ASSERT_RETURN(nrays <= MAX_PACKET, 0);
  max_t = min(max_t, 1073741824.f); // the legacy tracer's distance cap
  // NaN inputs are detected per ray, not via the union sums: scalar min/max wash a NaN out when a
  // later healthy ray follows it, and a poisoned union would silently starve the healthy rays
  bool anyNan = false;
  for (int k = 0; k < nrays; ++k)
    anyNan |= check_nan(pos[k].x + pos[k].y + pos[k].z + dirs[k].x + dirs[k].y + dirs[k].z);
  if (anyNan)
  {
    // degrade to per-ray traces so the healthy rays keep the exactly-equal-to-per-query contract
    int hits = 0;
    for (int k = 0; k < nrays; ++k)
      if (!check_nan(pos[k].x + pos[k].y + pos[k].z + dirs[k].x + dirs[k].y + dirs[k].z))
        hits += traceRay(Point3::xyz(pos[k]), Point3::xyz(dirs[k]), t_io[k], nullptr);
    return hits;
  }
  float tIn[MAX_PACKET];
  for (int k = 0; k < nrays; ++k)
    tIn[k] = t_io[k];
  float minx = 1e30f, maxx = -1e30f, minz = 1e30f, maxz = -1e30f, segMinY = 1e30f, segMaxY = -1e30f;
  for (int k = 0; k < nrays; ++k)
  {
    const float ex = pos[k].x + dirs[k].x * max_t, ey = pos[k].y + dirs[k].y * max_t, ez = pos[k].z + dirs[k].z * max_t;
    minx = min(minx, min(pos[k].x, ex)), maxx = max(maxx, max(pos[k].x, ex));
    minz = min(minz, min(pos[k].z, ez)), maxz = max(maxz, max(pos[k].z, ez));
    segMinY = min(segMinY, min(pos[k].y, ey)), segMaxY = max(segMaxY, max(pos[k].y, ey));
  }
  if (check_nan(minx + maxx + minz + maxz + segMinY + segMaxY)) // inf inputs can still produce inf - inf
    return 0;
  const DumpHeader &h = hdr();
  if (segMinY > h.maxY)
    return 0;
  int cx0 = max((int)floorf((minx - h.origin.x) * h.invCellSize), 0);
  int cx1 = min((int)floorf((maxx - h.origin.x) * h.invCellSize), h.numCellsX - 1);
  int cz0 = max((int)floorf((minz - h.origin.z) * h.invCellSize), 0);
  int cz1 = min((int)floorf((maxz - h.origin.z) * h.invCellSize), h.numCellsY - 1);
  if (cx1 < cx0 || cz1 < cz0)
    return 0;
  RayData r[MAX_PACKET];
  for (int cz = cz0; cz <= cz1; ++cz)
    for (int cx = cx0; cx <= cx1; ++cx)
    {
      const Cell &c = cellsTab()[cz * h.numCellsX + cx];
      if (!c.hasGeom || segMinY > c.maxY)
        continue;
      bbox3f ub;
      ub.bmin = v_madd(v_make_vec4f(minx, segMinY, minz, 0), c.scale, c.ofs);
      ub.bmax = v_madd(v_make_vec4f(maxx, segMaxY, maxz, 0), c.scale, c.ofs);
      for (int k = 0; k < nrays; ++k)
      {
        r[k].rayOrigin = v_madd(v_ld(&pos[k].x), c.scale, c.ofs);
        r[k].rayDir = v_mul(v_ld(&dirs[k].x), c.scale);
        r[k].calc();
        r[k].t = t_io[k];
        r[k].bestTriOffset = 0;
        r[k].data = cellData(c);
      }
      // packet-bounds gate: sharing pays only while the packet box is smaller than (almost) every
      // leaf; a spread-out packet falls back to independent traversals in the shared cell setup
      vec3f ubExt = v_sub(ub.bmax, ub.bmin);
      const float packetExt = max(v_extract_x(ubExt), max(v_extract_y(ubExt), v_extract_z(ubExt)));
      if (packetExt <= c.packetMaxExt)
        soa4::rayPacketWalk<true>(cellData(c), (uint32_t)c.root.v, ub, r, nrays);
      else
        for (int k = 0; k < nrays; ++k)
          soa4::rayClosest<true>(r[k], c.root);
      for (int k = 0; k < nrays; ++k)
        t_io[k] = r[k].t;
    }
  int hits = 0;
  for (int k = 0; k < nrays; ++k)
    hits += t_io[k] < tIn[k]; // hit iff the ray's own budget shrank
  return hits;
}

int LandRayTracerSoA4::getHeightBelowPacket(const Point3_vec4 *pos, int npts, float *ht_out) const
{
  G_ASSERT_RETURN(npts <= MAX_PACKET, 0);
  if (npts <= 0) // empty packet is a legitimate no-op call
    return 0;
  float posSum = 0.f;
  for (int k = 0; k < npts; ++k)
    posSum += pos[k].x + pos[k].y + pos[k].z;
  if (check_nan(posSum))
  {
    // degrade to per-point queries: a NaN point misses alone instead of failing the whole packet
    int hits = 0;
    for (int k = 0; k < npts; ++k)
    {
      ht_out[k] = -1e30f;
      if (!check_nan(pos[k].x + pos[k].y + pos[k].z))
        hits += heightQuery<true>(pos[k].x, pos[k].z, pos[k].y, ht_out[k], nullptr);
    }
    return hits;
  }
  int ci = cellIdxAt(pos[0].x, pos[0].z);
  bool sameCell = ci >= 0;
  for (int k = 1; k < npts && sameCell; ++k)
    sameCell = cellIdxAt(pos[k].x, pos[k].z) == ci;
  if (!sameCell) // points straddle cells: independent queries (rare for few-meter packets)
  {
    int hits = 0;
    for (int k = 0; k < npts; ++k)
    {
      ht_out[k] = -1e30f;
      hits += heightQuery<true>(pos[k].x, pos[k].z, pos[k].y, ht_out[k], nullptr);
    }
    return hits;
  }
  const Cell &c = cellsTab()[ci];
  for (int k = 0; k < npts; ++k)
    ht_out[k] = -1e30f;
  if (!c.hasGeom)
    return 0;
  vec3f p[MAX_PACKET];
  soa4::HeightAcc acc[MAX_PACKET];
  for (int k = 0; k < npts; ++k)
  {
    p[k] = v_madd(v_ld(&pos[k].x), c.scale, c.ofs);
    acc[k].limit = v_extract_y(p[k]);
  }
  soa4::heightWalkMulti<true>(cellData(c), (uint32_t)c.root.v, p, acc, npts);
  int hits = 0;
  for (int k = 0; k < npts; ++k)
    if (acc[k].hit)
    {
      ht_out[k] = (acc[k].maxHt - c.ofsY) * c.invScaleY;
      hits++;
    }
  return hits;
}

int LandRayTracerSoA4::getCellIdxNear(int *out_idx, int idx_count, float px, float pz, float rad) const
{
  if (check_nan(px + pz + rad))
    return 0;
  const DumpHeader &h = hdr();
  int x0 = (int)floorf((px - rad - h.origin.x) * h.invCellSize), x1 = (int)floorf((px + rad - h.origin.x) * h.invCellSize);
  int z0 = (int)floorf((pz - rad - h.origin.z) * h.invCellSize), z1 = (int)floorf((pz + rad - h.origin.z) * h.invCellSize);
  x0 = max(x0, 0), z0 = max(z0, 0);
  x1 = min(x1, h.numCellsX - 1), z1 = min(z1, h.numCellsY - 1);
  int cnt = 0;
  for (int z = z0; z <= z1; ++z)
    for (int x = x0; x <= x1 && cnt < idx_count; ++x, ++cnt)
      out_idx[cnt] = z * h.numCellsX + x;
  G_ASSERTF(cnt == 0 || (z1 - z0 + 1) * (x1 - x0 + 1) <= idx_count, "Output array is small, idx_count=%d, needed_size=%d", idx_count,
    (z1 - z0 + 1) * (x1 - x0 + 1));
  return cnt;
}

bool LandRayTracerSoA4::getFaces(bbox3f_cref world_box, vec4f *__restrict triangles, int &left) const
{
  const DumpHeader &h = hdr();
  bbox3f boxV = v_ldu_bbox3(h.bbox);
  if (!v_bbox3_test_box_intersect(world_box, boxV))
    return false;
  alignas(16) float bmn[4], bmx[4];
  v_st(bmn, world_box.bmin);
  v_st(bmx, world_box.bmax);
  if (check_nan(bmn[0] + bmn[1] + bmn[2] + bmx[0] + bmx[1] + bmx[2]))
    return false;
  int cx0 = clamp((int)floorf((bmn[0] - h.origin.x) * h.invCellSize), 0, h.numCellsX - 1);
  int cx1 = clamp((int)floorf((bmx[0] - h.origin.x) * h.invCellSize), 0, h.numCellsX - 1);
  int cz0 = clamp((int)floorf((bmn[2] - h.origin.z) * h.invCellSize), 0, h.numCellsY - 1);
  int cz1 = clamp((int)floorf((bmx[2] - h.origin.z) * h.invCellSize), 0, h.numCellsY - 1);
  if (cx1 > cx0 + 1 || cz1 > cz0 + 1) // the tracer handles at most 2x2 cells; keep its contract
    return false;
  const vec4f w0flag = v_cast_vec4f(v_splatsi(0x80000000)); // -0.0 marker in w, detectable by consumers
  for (int cz = cz0; cz <= cz1; ++cz)
    for (int cx = cx0; cx <= cx1; ++cx)
    {
      const Cell &c = cellsTab()[cz * h.numCellsX + cx];
      if (!c.hasGeom)
        continue;
      bbox3f qb; // query box in this cell's box space
      qb.bmin = v_madd(world_box.bmin, c.scale, c.ofs);
      qb.bmax = v_madd(world_box.bmax, c.scale, c.ofs);
      const vec4f invScale = v_div(V_C_ONE, v_sel(V_C_ONE, c.scale, v_cast_vec4f(V_CI_MASK1110)));
      const bool overflow = soa4::boxTriWalk(cellData(c), (uint32_t)c.root.v, qb, [&](vec3f a, vec3f b, vec3f cc) {
        if (left <= 0) // the caller may pass an exhausted budget
          return true;
        const vec3f aw = v_mul(v_sub(a, c.ofs), invScale);
        triangles[0] = v_perm_xyzd(aw, w0flag);
        triangles[1] = v_mul(v_sub(b, a), invScale); // edges need no offset, only the scale
        triangles[2] = v_mul(v_sub(cc, a), invScale);
        triangles += 3;
        return --left <= 0;
      });
      if (overflow)
        return false;
    }
  return true;
}

// The tracer's algorithm on the SoA4 data: near cells contribute via face edges (closest point on
// each edge to pos), far cells via vertices only, with a conservative per-cell box bound as the
// early-out; the same 1.1 fudge covers the vert-only approximation of far cells. Unlike the tracer,
// the far-cell vert scan really walks that cell (the original rescans the start cell).
float LandRayTracerSoA4::calcHighestHorizon(const Point3 &pos) const
{
  if (check_nan(pos.x + pos.y + pos.z))
    return 1.f; // conservative: no direction can be ruled out
  const DumpHeader &h = hdr();
  if (!h.cellCount)
    return -1.f;
  const vec3f vpos = v_ldu(&pos.x);
  const int scx = clamp((int)floorf((pos.x - h.origin.x) * h.invCellSize), 0, h.numCellsX - 1);
  const int scz = clamp((int)floorf((pos.z - h.origin.z) * h.invCellSize), 0, h.numCellsY - 1);
  float high = -1.f;

  auto vertHorizon = [&](const Cell &c) {
    const vec4f invScale = v_div(V_C_ONE, v_sel(V_C_ONE, c.scale, v_cast_vec4f(V_CI_MASK1110)));
    const uint8_t *data = cellData(c);
    const int vertCount = (int)(c.dataBytes - c.vertsOfs) / 8;
    vec4f hi = v_splats(high);
    for (int i = 0; i < vertCount; ++i)
    {
      vec3f w = v_mul(v_sub(RayData::unpackVert21(data + c.vertsOfs + i * 8), c.ofs), invScale);
      vec3f d = v_sub(w, vpos);
      // a vertex coinciding with pos has no direction (norm3 would be NaN): count it as sine 1
      hi = v_max(hi, v_sel(V_C_ONE, v_splat_y(v_norm3(d)), v_cmp_gt(v_dot3(d, d), v_zero())));
    }
    high = v_extract_x(hi);
  };
  auto faceHorizon = [&](const Cell &c) {
    const vec4f invScale = v_div(V_C_ONE, v_sel(V_C_ONE, c.scale, v_cast_vec4f(V_CI_MASK1110)));
    const uint8_t *data = cellData(c);
    vec4f hi = v_splats(high);
    soa4::iterateLeafRefs(
      data, c.root, [](vec3f, vec3f) { return true; },
      [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) {
        const QuadLeafFields f = soa4::leafFields(data, l);
        const int nTri = (int)quadLeafTriCount(f);
        for (int st = 0, emitted = 0; emitted < nTri; ++st)
        {
          if (st == 1 && f.isSingle)
            continue;
          vec3f a, b, cc;
          soa4::fetchLeafTri(data, l, st, soa4::Vert21Loader{}, a, b, cc);
          emitted++;
          a = v_mul(v_sub(a, c.ofs), invScale);
          b = v_mul(v_sub(b, c.ofs), invScale);
          cc = v_mul(v_sub(cc, c.ofs), invScale);
          // an edge passing through pos has no direction (norm3 would be NaN): count it as sine 1
          auto edgeSine = [&](vec3f p0, vec3f p1) {
            vec3f d = v_sub(v_closest_point_on_segment(vpos, p0, p1), vpos);
            return v_sel(V_C_ONE, v_splat_y(v_norm3(d)), v_cmp_gt(v_dot3(d, d), v_zero()));
          };
          hi = v_max(hi, edgeSine(a, b));
          hi = v_max(hi, edgeSine(b, cc));
          hi = v_max(hi, edgeSine(a, cc));
        }
        return false;
      });
    high = v_extract_x(hi);
  };
  // conservative bound of any elevation seen inside an XZ box topping at top_y
  auto boxElev = [&](float wx0, float wx1, float wz0, float wz1, float top_y) -> float {
    const float dy = top_y - pos.y;
    float dx, dz;
    if (dy >= 0.f)
    {
      dx = pos.x < wx0 ? wx0 - pos.x : (pos.x > wx1 ? pos.x - wx1 : 0.f);
      dz = pos.z < wz0 ? wz0 - pos.z : (pos.z > wz1 ? pos.z - wz1 : 0.f);
      if (dx == 0.f && dz == 0.f)
        return dy > 0.f ? 1.f : 0.f;
    }
    else
    {
      dx = max(fabsf(wx0 - pos.x), fabsf(wx1 - pos.x));
      dz = max(fabsf(wz0 - pos.z), fabsf(wz1 - pos.z));
    }
    return dy / sqrtf(dy * dy + dx * dx + dz * dz);
  };

  const Cell &sc = cellsTab()[scz * h.numCellsX + scx];
  if (sc.hasGeom)
  {
    faceHorizon(sc);
    vertHorizon(sc);
  }
  for (int cz = 0, ci = 0; cz < h.numCellsY; ++cz)
    for (int cx = 0; cx < h.numCellsX; ++cx, ++ci)
    {
      const Cell &c = cellsTab()[ci];
      if ((cx == scx && cz == scz) || !c.hasGeom)
        continue;
      if (boxElev(c.wx0, c.wx1, c.wz0, c.wz1, c.maxY) <= high)
        continue;
      vertHorizon(c);
      if (abs(cx - scx) + abs(cz - scz) <= 2) // face precision only matters nearby, as in the tracer
        faceHorizon(c);
    }
  // the far-field vert-only scan can underestimate either way: push the bound away from the
  // terrain (up for positive sines, toward zero for negative ones), never below the raw estimate
  return min(1.f, high > 0.f ? high * 1.1f : high * (1.f / 1.1f));
}

int LandRayTracerSoA4::getCellVertCount(int cell_idx) const
{
  if (uint32_t(cell_idx) >= uint32_t(hdr().cellCount) || !cellsTab()[cell_idx].hasGeom)
    return 0;
  // vertsOfs <= dataBytes holds by construction (the deserializer builds the blob itself); the
  // clamp keeps a violated invariant at zero enumeration instead of a negative count
  return max(0, (int)(cellsTab()[cell_idx].dataBytes - cellsTab()[cell_idx].vertsOfs) / 8);
}

int LandRayTracerSoA4::getCellTriCount(int cell_idx) const
{
  if (uint32_t(cell_idx) >= uint32_t(hdr().cellCount) || !cellsTab()[cell_idx].hasGeom)
    return 0;
  return cellsTab()[cell_idx].triCount;
}

void LandRayTracerSoA4::iterateCellVerticesImpl(int cell_idx, void *ctx, void (*cb)(void *, const Point3 &)) const
{
  const int vertCount = getCellVertCount(cell_idx);
  if (!vertCount)
    return;
  const Cell &c = cellsTab()[cell_idx];
  const uint8_t *data = cellData(c);
  const vec4f invScale = v_div(V_C_ONE, v_sel(V_C_ONE, c.scale, v_cast_vec4f(V_CI_MASK1110)));
  for (int i = 0; i < vertCount; ++i)
  {
    vec3f w = v_mul(v_sub(RayData::unpackVert21(data + c.vertsOfs + i * 8), c.ofs), invScale);
    Point3 p;
    v_stu_p3(&p.x, w);
    cb(ctx, p);
  }
}

void LandRayTracerSoA4::iterateCellFacesImpl(int cell_idx, void *ctx, void (*cb)(void *, int, int, int)) const
{
  if (uint32_t(cell_idx) >= uint32_t(hdr().cellCount) || !cellsTab()[cell_idx].hasGeom)
    return;
  const Cell &c = cellsTab()[cell_idx];
  const uint8_t *data = cellData(c);
  soa4::iterateLeafRefs(
    data, c.root, [](vec3f, vec3f) { return true; },
    [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) {
      const QuadLeafFields f = soa4::leafFields(data, l);
      const int apexBytes = l.bodyOfs + (int)f.relBaseBytes - c.vertsOfs;
      if (apexBytes >= 0 && apexBytes < (int)(c.dataBytes - c.vertsOfs)) // corrupt stream: skip, never index wild
        expandQuadLeafTris(f, (uint32_t)apexBytes / 8,
          [&](uint32_t i0, uint32_t i1, uint32_t i2) { cb(ctx, (int)i0, (int)i1, (int)i2); });
      return false;
    });
}
