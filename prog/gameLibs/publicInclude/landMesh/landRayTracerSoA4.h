//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// SoA4-BLAS-based land tracer: serves the legacy tracer's query API from per-cell daBVH SoA4
// BLASes. Sources are the native LTS4 stream, direct per-cell geometry, or a legacy LTdump
// stream parsed by the built-in bridge into the same per-cell geometry build
// as a per-cell daBVH double-quad BLAS in the SoA4 CPU layout (the CollisionResource format) plus
// an adaptive per-cell max-height mip chain, and serves the tracer's queries with matching cull
// semantics (traceRay/rayHit/getHeightBelow cull backfaces, getHeight is two-sided). Adds packet
// queries: several nearby, roughly-coherent queries share one traversal with results exactly equal
// to the per-query calls.

#include <math/dag_math3d.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>
#include <daBVH/dag_swBLASRootRef.h>
#include <generic/dag_span.h>

class IGenLoad;
class IGenSave;
class GlobalSharedMemStorage;

class LandRayTracerSoA4
{
public:
  LandRayTracerSoA4() = default;
  ~LandRayTracerSoA4(); // drops the claim held on an attached or published shared-memory region
  // the served view points into the dump: copying would alias another instance's storage
  LandRayTracerSoA4(const LandRayTracerSoA4 &) = delete;
  LandRayTracerSoA4 &operator=(const LandRayTracerSoA4 &) = delete;

  // rebuild all cells from a loaded land tracer; cells build in parallel on the threadpool
  // (serial when the pool has no workers)
  // build directly from per-cell triangle geometry (row-major cells_x * cells_y entries; an empty
  // cell has no verts), the exporter-side entry that needs no LandRayTracer
  struct CellSource
  {
    dag::ConstSpan<Point3> verts;
    dag::ConstSpan<int> indices;
  };
  bool build(int cells_x, int cells_y, float cell_size, const Point3 &origin_pos, const BBox3 &land_box,
    dag::ConstSpan<CellSource> src_cells);
  // raw payload cap shared by every stream this class reads: streams larger than this are corrupt
  // by contract, and the bound also limits what a hostile claim can allocate before it fails
  static constexpr int MAX_STREAM_SIZE = 512 << 20;

  // Legacy LTdump stream bridge: parses the old format (grid header, per-cell records, packed u16
  // vertices, cell-local face indices; the grid acceleration sections are skipped) straight into
  // the per-cell geometry build, so the legacy tracer is not needed to read old levels. Empty is a
  // legitimate outcome, not an error: gray-box levels carry a tracer whose cells hold no geometry
  // and must keep loading with heightmap-only collision --
  // the backwards-compatible load path; like save()/load() below, the stream reads may throw
  // IGenLoad::LoadException on truncated input
  enum class LegacyLoadResult
  {
    Ok,
    Empty,
    Failed
  };
  LegacyLoadResult loadStreamToDump(IGenLoad &crd, int dump_sz, IMemAlloc *allocator = nullptr);
  // the native format: header + per-cell daBVH-serialized BLAS (see daBVH/dag_bvhIO.h); load() reads
  // what save() writes, deserializing straight into the SoA4 buffers. Like any IGenLoad consumer,
  // the stream entries may throw IGenLoad::LoadException on truncated input (a false return covers
  // well-formed streams with invalid content); a failed load leaves the instance empty
  bool save(IGenSave &cwr) const;
  bool load(IGenLoad &crd);
  // the content gates load() applies to a parsed stream, run on the in-memory cells: lets the
  // exporter refuse a tracer the runtime would reject without a save/parse round-trip; returns
  // nullptr when consistent, else the reason
  const char *validate() const;

  // Shared-memory path for hosted servers: sibling processes on one host serve one relocatable
  // dump per level instead of each parsing and building its own. publishShared() copies the dump
  // into a record under sm_name (fill, freeze read-only, then mark ready) - a separate step so
  // the caller publishes only a tracer it accepted; when a sibling won the publish race, its
  // record is attached and the private copy dropped, and a full storage keeps the private copy.
  // attach() serves queries from a record another process published - trusted by provenance like
  // the storage's other consumers, only addressing is checked; on success it takes over the
  // caller's findPtr claim and the dtor releases it (the caller releases on failure).
  static GlobalSharedMemStorage *sharedMem;
  static constexpr unsigned SM_DATA_TAG = _MAKE4C('LTS4');
  bool attach(const void *dump_data, int64_t dump_bytes);
  void publishShared(const char *sm_name); // no-op without sharedMem/name, or when nothing owned

  const BBox3 &getBBox() const { return hdr().bbox; }
  int getCellCount() const { return hdr().cellCount; }
  // world-space XZ footprint of a cell from the grid (cells are row-major, X-aligned); an
  // out-of-range index returns an empty box, like the other per-cell accessors' miss behavior
  BBox2 getCellBBox2D(int cell_idx) const
  {
    const DumpHeader &h = hdr();
    if ((unsigned)cell_idx >= (unsigned)h.cellCount)
      return BBox2();
    Point2 mn(h.origin.x + (cell_idx % h.numCellsX) * h.cellSize, h.origin.z + (cell_idx / h.numCellsX) * h.cellSize);
    return BBox2(mn, mn + Point2(h.cellSize, h.cellSize));
  }
  int64_t dataSize() const;

  bool traceRay(const Point3 &p, const Point3 &dir, float &t, Point3 *out_norm = nullptr) const;
  bool rayHit(const Point3 &p, const Point3 &dir, float t) const;
  bool getHeight(const Point2 &pos, float &ht, Point3 *out_norm = nullptr) const;
  // surfaces within [ht, pos.y] like the tracer: pass the lower bound in ht (or below the terrain);
  // on success the resolved surface height is written back to ht (in/out, the legacy contract)
  bool getHeightBelow(const Point3 &pos, float &ht, Point3 *out_norm = nullptr) const;
  // conservative upper bound of the surface height at pos (the gridHt analog)
  bool getHeightBounding(const Point2 &pos, float &ht) const;

  // cell indices intersecting the radius-rad square around (px, pz); the tracer's contract
  int getCellIdxNear(int *out_idx, int idx_count, float px, float pz, float rad) const;
  // triangles overlapping world_box as {vert0 (w = -0.0 marker), edge1, edge2}, decrementing `left`
  // per triangle; false when the box misses the map, spans more than 2x2 cells or `left` runs out
  // (the TraceMeshFaces contract)
  bool getFaces(bbox3f_cref world_box, vec4f *__restrict triangles, int &left) const;
  // conservative sine of the highest terrain elevation seen from pos, meant to skip rays with
  // dir.y > result: the legacy tracer's scan (near cells contribute face edges, far cells
  // vertices, per-cell boxes as the early-out) with the same 1.1 safety factor -- a tuned
  // heuristic, not a strict bound
  float calcHighestHorizon(const Point3 &pos) const;
  // Cell geometry enumeration, mirroring BaseLandRayTracer's surface shape: world-space vertices
  // in a stable order plus triangle index triples into that order, decoded from the BLAS leaves.
  // The order is implementation-defined (not index-compatible with the legacy tracer for the same
  // cell); the triangle set and windings match the source. Out-of-range or empty cells enumerate
  // nothing and count 0.
  int getCellVertCount(int cell_idx) const;
  int getCellTriCount(int cell_idx) const;
  template <class CB>
  void iterateCellVertices(int cell_idx, CB cb) const // cb(const Point3 &world_pos)
  {
    iterateCellVerticesImpl(cell_idx, &cb, [](void *c, const Point3 &p) { (*(CB *)c)(p); });
  }
  template <class CB>
  void iterateCellFaces(int cell_idx, CB cb) const // cb(int v0, int v1, int v2), indices into iterateCellVertices' order
  {
    iterateCellFacesImpl(cell_idx, &cb, [](void *c, int v0, int v1, int v2) { (*(CB *)c)(v0, v1, v2); });
  }

  enum TraverseResult
  {
    TRAVERSE_RESULT_SKIP,
    TRAVERSE_RESULT_CONTINUE,
    TRAVERSE_RESULT_FINISH
  };
  // 2D shape sweep over the max-height grids, the legacy tracer's traverse contract: the shape's
  // plane is world XZ (getBoundsY = the second axis = world Z), Visitor::visit(cell_min,
  // cell_size, max_ht, is_final_grid) returns SKIP (drop the rect), CONTINUE (descend / accept)
  // or FINISH (stop the walk, return true). The coarse pass sees whole cells with their
  // world-space top (exact when built, the cell box top when loaded); accepted cells descend to
  // their height mip texels, u8-quantized ceilings, as the final grid. Either way heights only
  // over-state, so pruning by them stays conservative. Returns true when any final-grid rect was
  // accepted or the visitor finished.
  template <class Shape, class Visitor>
  bool traverse(const Shape &shape, Visitor &visitor) const
  {
    const DumpHeader &h = hdr();
    bool result = false;
    float zMin, zMax;
    shape.getBoundsY(zMin, zMax);
    const int czMin = max((int)floorf((zMin - h.origin.z) * h.invCellSize), 0);
    const int czMax = min((int)floorf((zMax - h.origin.z) * h.invCellSize), h.numCellsY - 1);
    for (int cz = czMin; cz <= czMax; ++cz)
    {
      const float rowZ0 = h.origin.z + cz * h.cellSize, rowZ1 = rowZ0 + h.cellSize;
      float xMin, xMax;
      if (!shape.getBoundsX(rowZ0, rowZ1, xMin, xMax))
        continue;
      const int cxMin = max((int)floorf((xMin - h.origin.x) * h.invCellSize), 0);
      const int cxMax = min((int)floorf((xMax - h.origin.x) * h.invCellSize), h.numCellsX - 1);
      for (int cx = cxMin; cx <= cxMax; ++cx)
      {
        const Cell &c = cellsTab()[cz * h.numCellsX + cx];
        if (!c.hasGeom)
          continue;
        const Point2 cellMin(h.origin.x + cx * h.cellSize, rowZ0);
        const int r = (int)visitor.visit(cellMin, Point2(h.cellSize, h.cellSize), c.maxY, false);
        if (r == TRAVERSE_RESULT_SKIP)
          continue;
        if (r == TRAVERSE_RESULT_FINISH)
          return true;
        if (traverseCellHt(c, cellMin, shape, visitor, result))
          return true;
      }
    }
    return result;
  }

  // Packet queries: results are exactly equal to the equivalent per-query calls; sharing engages
  // per cell only while the packet's union box stays smaller than (almost) every leaf. A packet
  // containing NaN input degrades to per-query calls, so healthy entries keep their answers.
  enum
  {
    MAX_PACKET = 8
  };
  // nrays/npts must be in [0, MAX_PACKET]: 0 is a no-op, larger packets return 0 having touched
  // nothing (dev builds assert). t_io[k] in/out per ray; returns hit count (hit iff t_io[k] shrank
  // below its own input). max_t only bounds the shared segment; per-ray caps live in t_io.
  int traceRayPacket(const Point3_vec4 *pos, const Point3_vec4 *dirs, int nrays, float max_t, float *t_io) const;
  // ht_out[k] <= -1e30f on miss; returns hit count
  int getHeightBelowPacket(const Point3_vec4 *pos, int npts, float *ht_out) const;

private:
  // The whole tracer is one relocatable dump: [DumpHeader][Cell records][per-cell payloads],
  // where payloads are the SoA4 BLAS blobs (internally offset-based already) and the u8 height
  // mip chains, addressed by offsets from the dump start. No pointers inside means the dump can
  // later be placed in shared memory and attached by other processes as-is.
  struct DumpHeader
  {
    int numCellsX = 0, numCellsY = 0;
    float cellSize = 0.f, invCellSize = 0.f; // invCellSize derived by assemble(), stored so queries need no side state
    float maxY = -1e30f;                     // scene top; the default makes every query on an empty tracer miss
    Point3 origin = Point3(0, 0, 0);
    BBox3 bbox;
    int cellCount = 0;
    int _pad = 0; // keeps sizeof(DumpHeader) a 16B multiple so the Cell records after it stay aligned
  };

  struct alignas(16) Cell // POD record in the dump; payload locations are dump offsets
  {
    vec4f scale = v_zero(), ofs = v_zero(); // world -> box space: b = w * scale + ofs
    soa4::RootRef root;
    int treeBytes = 0, vertsOfs = 0;                // within the BLAS blob
    uint32_t dataOfs = 0, dataBytes = 0, htOfs = 0; // dump offsets: SoA4 [tree][pad][vert21], ht mips
    int hasGeom = 0;
    float ofsY = 0.f, invScaleY = 1.f;
    float minY = 0.f, maxY = 0.f;
    float packetMaxExt = 0.f; // 5th-percentile leaf extent (box space): packet-sharing gate
    int triCount = 0;         // triangles in the BLAS leaves (the enumeration API's count)
    int htDim = 0, htLevels = 0;
    int htLvlOfs[8] = {};
    // world XZ footprint of the cell's geometry, derived once from scale/ofs (horizon scans it
    // per call otherwise); also keeps the record's alignment tail deterministic in the dump
    float wx0 = 0.f, wx1 = 0.f, wz0 = 0.f, wz1 = 0.f;
    uint32_t _padding[3] = {};

    float htMaxAt(const uint8_t *ht, float bx, float bz) const;
  };

  // ht mip texels store box-space Y ceil-quantized to u8: q = ceil(y / HT_Q_STEP), decode
  // q * HT_Q_STEP >= true max; 65535 / 255 is exactly 257, so the full box range fits
  static constexpr float HT_Q_STEP = 257.f;

  struct CellTmp; // build/load scratch: Cell meta + owning payload buffers, concatenated by assemble()

  dag::Vector<uint8_t> dump; // owns the payload unless it lives in a shared-memory record
  // the served view: the owned vector's storage, or a shared-memory record after attach() or
  // publishShared() - a storage claim is held exactly when dumpData points into sharedMem
  const uint8_t *dumpData = nullptr;
  int64_t dumpBytes = 0;
  void releaseSmRegion();
  void resetDump()
  {
    releaseSmRegion();
    dump.clear();
    dumpData = nullptr;
    dumpBytes = 0;
  }

  // the header inside the dump is the only grid state; an empty tracer serves the all-zero
  // static header, so every query misses without a special case
  static const DumpHeader emptyHdr;
  const DumpHeader &hdr() const { return dumpData ? *(const DumpHeader *)dumpData : emptyHdr; }
  const Cell *cellsTab() const { return (const Cell *)(dumpData + sizeof(DumpHeader)); }

  const uint8_t *cellData(const Cell &c) const { return dumpData + c.dataOfs; }
  const uint8_t *cellHt(const Cell &c) const { return dumpData + c.htOfs; }

  int cellIdxAt(float x, float z) const
  {
    const DumpHeader &h = hdr();
    int cx = (int)floorf((x - h.origin.x) * h.invCellSize);
    int cz = (int)floorf((z - h.origin.z) * h.invCellSize);
    if (cx < 0 || cz < 0 || cx >= h.numCellsX || cz >= h.numCellsY)
      return -1;
    return cz * h.numCellsX + cx;
  }

  // concatenate built cells into the single dump under the passed grid header (cellCount and
  // invCellSize are derived inside)
  bool assemble(DumpHeader h, dag::Span<CellTmp> tmp_cells);
  static void buildCellGeom(CellTmp &out, dag::Vector<vec4f> &verts4, dag::Vector<uint32_t> &idx);
  static void finalizeCellAccel(CellTmp &out);
  template <bool ANY_HIT>
  bool traceCell(const Cell &c, vec3f p, vec3f d, const Point3 &pos, const Point3 &dir, float t_in, float t_out, float &t,
    const Cell **hit_cell, uint32_t *hit_ref, int *hit_sub) const;
  template <bool CULL_DOWN> // compile-time: getHeight is two-sided, getHeightBelow culls down-facing surfaces
  bool heightQuery(float x, float z, float limit_y, float &ht, Point3 *out_norm) const;
  template <class Shape, class Visitor>
  bool traverseCellHt(const Cell &c, const Point2 &cell_min, const Shape &shape, Visitor &visitor, bool &result) const
  {
    const uint8_t *ht = cellHt(c);
    const float texel = hdr().cellSize / c.htDim;
    float zMin, zMax;
    shape.getBoundsY(zMin, zMax);
    const int tzMin = max((int)floorf((zMin - cell_min.y) / texel), 0);
    const int tzMax = min((int)floorf((zMax - cell_min.y) / texel), c.htDim - 1);
    for (int tz = tzMin; tz <= tzMax; ++tz)
    {
      const float rowZ0 = cell_min.y + tz * texel, rowZ1 = rowZ0 + texel;
      float xMin, xMax;
      if (!shape.getBoundsX(rowZ0, rowZ1, xMin, xMax))
        continue;
      const int txMin = max((int)floorf((xMin - cell_min.x) / texel), 0);
      const int txMax = min((int)floorf((xMax - cell_min.x) / texel), c.htDim - 1);
      for (int tx = txMin; tx <= txMax; ++tx)
      {
        const float h = (ht[tz * c.htDim + tx] * HT_Q_STEP - c.ofsY) * c.invScaleY;
        const int r = (int)visitor.visit(Point2(cell_min.x + tx * texel, rowZ0), Point2(texel, texel), h, true);
        if (r == TRAVERSE_RESULT_SKIP)
          continue;
        if (r == TRAVERSE_RESULT_FINISH)
          return true;
        result = true;
      }
    }
    return false;
  }
  template <bool ANY_HIT>
  bool traceImpl(const Point3 &pos, const Point3 &dir, float &t, const Cell **hit_cell, uint32_t *hit_ref, int *hit_sub) const;
  void iterateCellVerticesImpl(int cell_idx, void *ctx, void (*cb)(void *, const Point3 &)) const;
  void iterateCellFacesImpl(int cell_idx, void *ctx, void (*cb)(void *, int, int, int)) const;
};
