// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_bvhIO.h>
#include <daBVH/dag_quadBLASBuilder.h> // writeQuadBox
#include <daBVH/dag_swBLAS_leaf.h>     // RayData::unpackVert21, decode/expand quad leaf
#include <daBVH/dag_swBLAS_soa4.h>     // SoA4 tree layout constants + RootRef
#include <daBVH/swBLASLeafDefs.hlsli>
#include <ioSys/dag_genIo.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_assert.h>
#include <string.h>
#include <stdlib.h>

namespace build_bvh
{

// Child-count byte in the serialized forest stream: 0 == full leaf (16-byte body follows, W1 holds a
// vertex index), SHORT_LEAF_*_MARKER == single-quad leaf (8-byte W0+W1 body; the canonical W2/W3 are
// implied by the marker), RAW_LEAF_MARKER == degenerate no-hit leaf (body kept verbatim, box rebuilt
// empty), 1..MAX_CHILD_COUNT == internal node with that many children. Every byte value is defined, so
// no stream byte is "invalid" and a corrupt count cannot fall through to undefined handling.
static constexpr uint8_t RAW_LEAF_MARKER = 0xFFu;
static constexpr uint8_t SHORT_LEAF_MARKER = 0xFEu;                    // single-quad leaf, 2nd-tri flip clear
static constexpr uint8_t SHORT_LEAF_FLIP_MARKER = 0xFDu;               // single-quad leaf, QUAD_FLIPA_FLAG set
static constexpr uint8_t MAX_CHILD_COUNT = SHORT_LEAF_FLIP_MARKER - 1; // 252; production SAH caps at 4

static inline uint32_t rd32(const uint8_t *p, int o)
{
  uint32_t v;
  memcpy(&v, p + o, 4);
  return v;
}

// Bytes spanned by the subtree rooted at `o` in a runtime [tree] region (leaf or internal+children).
static inline int srcSubtreeBytes(const uint8_t *d, int o, int leaf_size)
{
  const uint32_t skip = rd32(d, o + 12);
  return (skip & QUAD_LEAF_FLAG) ? leaf_size : BVH_BLAS_NODE_SIZE + int(skip);
}

// The count byte cannot encode 0 (the full-leaf marker) or more than MAX_CHILD_COUNT children (the
// leaf-marker range), so serialize validates every fanout up front, before a single byte is written.
// The depth cap matches the deserialize side and bounds writeSubtree too (it walks the same tree).
static bool subtreeCountsFit(const uint8_t *blas, int o, int leaf_size, int depth)
{
  if (depth > BVH_IO_MAX_TREE_DEPTH)
    return false;
  const uint32_t skip = rd32(blas, o + 12);
  if (skip & QUAD_LEAF_FLAG)
    return true;
  const int childEnd = o + BVH_BLAS_NODE_SIZE + int(skip);
  int n = 0;
  for (int c = o + BVH_BLAS_NODE_SIZE; c < childEnd; c += srcSubtreeBytes(blas, c, leaf_size))
    if (++n > MAX_CHILD_COUNT || !subtreeCountsFit(blas, c, leaf_size, depth + 1))
      return false;
  return n > 0;
}

// ============================================================================
// Serialize: walk the runtime forest, drop boxes + skips, keep child counts + leaf topology.
// ============================================================================

static void writeSubtree(IGenSave &cwr, const uint8_t *blas, int o, int verts_ofs, int vert_count, int vert_stride, int leaf_size,
  int depth)
{
  G_ASSERT(depth <= BVH_IO_MAX_TREE_DEPTH); // the subtreeCountsFit pre-pass rejected deeper trees
  const uint32_t skip = rd32(blas, o + 12);
  if (skip & QUAD_LEAF_FLAG)
  {
    const uint32_t w1 = rd32(blas, o + 16), w2 = rd32(blas, o + 20), w3 = rd32(blas, o + 24);
    const int leafOfs = o + BVH_BLAS_NODE_SIZE;
    const int relBaseBytes = int((w1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT);
    const int apexByte = leafOfs + relBaseBytes - verts_ofs;
    // A normal leaf's apex is a real vert21 vertex, so apexByte is a non-negative multiple of the stride
    // that lands inside the vert region. A degenerate no-hit leaf (writeDoubleQuadLeaf overflow: W1..W3
    // == 0) instead points its apex back into the tree body; keep such a leaf verbatim so deserialize
    // never derives -- or indexes verts by -- a bogus vertex.
    if (apexByte < 0 || (apexByte % vert_stride) != 0 || apexByte / vert_stride >= vert_count)
    {
      const uint8_t cc = RAW_LEAF_MARKER;
      cwr.write(&cc, 1);
      const uint32_t body[4] = {skip, w1, w2, w3};
      cwr.write(body, 16);
      return;
    }
    // Store the quad-A apex as a stride-independent VERTEX INDEX in W1's low 24 bits; the o3A-high bits
    // (W1[24:31]) ride along unchanged. deserialize recomputes the byte base for its own layout.
    const uint32_t vertIdx = uint32_t(apexByte / vert_stride);
    const uint32_t w1p = (vertIdx & QUAD_BASE_MASK) | (w1 & ~QUAD_BASE_MASK);
    // A single-quad leaf is canonical (W2 = 2nd-tri flip only, W3 = 0, as packQuadA writes it), so both
    // words are implied by the marker byte: store just W0+W1 and halve the leaf body on disk.
    if ((w2 & ~QUAD_FLIPA_FLAG) == 0 && w3 == 0)
    {
      const uint8_t cc = (w2 & QUAD_FLIPA_FLAG) ? SHORT_LEAF_FLIP_MARKER : SHORT_LEAF_MARKER;
      cwr.write(&cc, 1);
      const uint32_t body[2] = {skip, w1p};
      cwr.write(body, 8);
      return;
    }
    const uint8_t cc = 0; // 0 children == full leaf marker, followed by the 16-byte body
    cwr.write(&cc, 1);
    const uint32_t body[4] = {skip, w1p, w2, w3};
    cwr.write(body, 16);
    return;
  }
  const int childEnd = o + BVH_BLAS_NODE_SIZE + int(skip);
  int n = 0;
  for (int c = o + BVH_BLAS_NODE_SIZE; c < childEnd; c += srcSubtreeBytes(blas, c, leaf_size))
    n++;
  const uint8_t cc = uint8_t(n); // 1..MAX_CHILD_COUNT, guaranteed by the subtreeCountsFit pre-pass
  cwr.write(&cc, 1);
  for (int c = o + BVH_BLAS_NODE_SIZE; c < childEnd; c += srcSubtreeBytes(blas, c, leaf_size))
    writeSubtree(cwr, blas, c, verts_ofs, vert_count, vert_stride, leaf_size, depth + 1);
}

bool serializeQuadBLAS(IGenSave &cwr, const uint8_t *blas, int tree_bytes, int verts_ofs, int vert_count, bbox3f local_box,
  int leaf_size, int vert_stride)
{
  G_ASSERT_RETURN(blas != nullptr && tree_bytes > 0 && vert_count > 0, false);
  // Checked in release too: past a debug-only assert an oversized fanout would narrow into a leaf
  // marker and commit a stream the reader misparses.
  for (int o = 0; o < tree_bytes; o += srcSubtreeBytes(blas, o, leaf_size))
    G_ASSERT_RETURN(subtreeCountsFit(blas, o, leaf_size, 0), false);
  // A no-hit leaf as the whole BLAS (a one-prim tree after writeDoubleQuadLeaf's encode-overflow
  // logerr) is refused by both deserializers, so never commit such a stream.
  if (tree_bytes == leaf_size && (rd32(blas, 12) & QUAD_LEAF_FLAG))
  {
    const uint32_t w1 = rd32(blas, BVH_BLAS_NODE_SIZE);
    const int apexByte = BVH_BLAS_NODE_SIZE + int((w1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT) - verts_ofs;
    if (apexByte < 0 || (apexByte % vert_stride) != 0 || apexByte / vert_stride >= vert_count)
      return false;
  }

  BlasIoHeader h = {};
  h.magic = BVH_IO_MAGIC;
  h.version = BVH_IO_VERSION;
  h.vertStride = uint8_t(vert_stride);
  h.leafSize = uint8_t(leaf_size);
  h.treeBytes = uint32_t(tree_bytes);
  h.vertCount = uint32_t(vert_count);
  alignas(16) float t[4];
  v_st(t, local_box.bmin);
  h.bmin[0] = t[0];
  h.bmin[1] = t[1];
  h.bmin[2] = t[2];
  v_st(t, local_box.bmax);
  h.bmax[0] = t[0];
  h.bmax[1] = t[1];
  h.bmax[2] = t[2];
  cwr.write(&h, sizeof(h));

  // Verts first (verbatim vert21) so deserialize can rebuild leaf boxes during the single tree pass.
  cwr.write(blas + verts_ofs, vert_count * vert_stride);

  // Tree as a pre-order forest of the suppressed root's children (matches writeDoubleQuadBVH2's layout).
  for (int o = 0; o < tree_bytes; o += srcSubtreeBytes(blas, o, leaf_size))
    writeSubtree(cwr, blas, o, verts_ofs, vert_count, vert_stride, leaf_size, 0);
  return true;
}

// ============================================================================
// Shared read/rebuild helpers (used by both the stackless Rebuilder and the direct SoA4 path). They
// carry the untrusted-input hardening so the two deserializers reject the same corrupt streams.
// ============================================================================

// A crafted stream is untrusted: any bounds/structure violation throws instead of over-reading.
[[noreturn]] static void bvhIoBail(IGenLoad &crd, const char *what)
{
  DAGOR_THROW(IGenLoad::LoadException(what, crd.tell()));
  // Without exceptions DAGOR_THROW is DAG_FATAL and does not evaluate its argument; keep the noreturn
  // contract (and the arguments used) by halting here.
  G_UNUSED(crd);
  G_UNUSED(what);
  abort();
}

static inline bool bvhIoIsLeafMarker(uint8_t cc)
{
  return cc == 0 || cc == RAW_LEAF_MARKER || cc == SHORT_LEAF_MARKER || cc == SHORT_LEAF_FLIP_MARKER;
}

// Read one leaf record body into `body` = {W0/skip, W1, W2, W3}. A short leaf stores only W0+W1 on
// disk; its canonical W2 (flip flag only) and W3 (zero) are implied by the marker. W1 holds the apex
// as a stride-independent VERTEX INDEX in its low 24 bits (see the format banner).
static void bvhIoReadLeafRecord(IGenLoad &crd, uint8_t cc, uint32_t body[4])
{
  if (cc == SHORT_LEAF_MARKER || cc == SHORT_LEAF_FLIP_MARKER)
  {
    crd.read(body, 8);
    body[2] = cc == SHORT_LEAF_FLIP_MARKER ? QUAD_FLIPA_FLAG : 0u;
    body[3] = 0;
  }
  else
    crd.read(body, 16);
  // W0 becomes the runtime skip word verbatim; without the leaf bit the traversal would descend into
  // the leaf body as if it were child nodes.
  if (!(body[0] & QUAD_LEAF_FLAG))
    bvhIoBail(crd, "bvhIO: leaf record missing leaf flag");
  // The engine's only degenerate no-hit body is W0 == QUAD_LEAF_FLAG with W1..W3 all zero
  // (writeDoubleQuadLeaf); any other payload is a crafted stream whose offset fields must never reach a
  // vertex decoder. W0's o-bits count: they are copied into the SoA4 child word and feed the quad-A
  // decode, so the converter (soa4Convert.cpp) already refuses them -- reject them here to match.
  if (cc == RAW_LEAF_MARKER && ((body[0] & ~QUAD_LEAF_FLAG) | body[1] | body[2] | body[3]))
    bvhIoBail(crd, "bvhIO: raw no-hit leaf body not all-zero");
}

// Box-space AABB of a non-RAW leaf, from the very vert21 verts the traversal will read (so the rebuilt
// box bounds exactly that triangle set), plus the decoded base vertex index. Bounds-checks the base and
// every triangle index. `w1in`'s low 24 bits are the base vertex index; the high 8 bits (o3A high) ride
// along unchanged.
struct BvhIoLeafBox
{
  bbox3f box;
  uint32_t vertIdx;
};
static BvhIoLeafBox bvhIoLeafBox(IGenLoad &crd, const uint8_t *verts, uint32_t vert_count, int vert_stride, uint32_t w0, uint32_t w1in,
  uint32_t w2, uint32_t w3)
{
  const uint32_t vertIdx = w1in & QUAD_BASE_MASK;
  const uint32_t o3Ahi = w1in & ~QUAD_BASE_MASK;
  if (vertIdx >= vert_count)
    bvhIoBail(crd, "bvhIO: leaf base vertex out of range");
  // o3Ahi carries W1's high 8 bits in place (base bits zero), so decodeQuadLeafFields reads o3A
  // correctly and its relBaseBytes comes out 0 -- unused here, we index by vertIdx directly.
  const QuadLeafFields f = decodeQuadLeafFields(w0, o3Ahi, w2, w3);
  // Validate first (the shared bound authority), then rebuild the box from indices known good.
  if (!validateQuadLeafVertexIndices(f, vertIdx, vert_count))
    bvhIoBail(crd, "bvhIO: leaf triangle vertex out of range");
  bbox3f box;
  v_bbox3_init_empty(box);
  expandQuadLeafTris(f, vertIdx, [&](uint32_t a, uint32_t b, uint32_t c) {
    v_bbox3_add_pt(box, RayData::unpackVert21(verts + size_t(a) * vert_stride));
    v_bbox3_add_pt(box, RayData::unpackVert21(verts + size_t(b) * vert_stride));
    v_bbox3_add_pt(box, RayData::unpackVert21(verts + size_t(c) * vert_stride));
  });
  return {box, vertIdx};
}

// ============================================================================
// Deserialize: stream the tree into one final buffer, recompute boxes + skips in place.
// ============================================================================

namespace
{
struct Rebuilder
{
  IGenLoad &crd;
  uint8_t *dst = nullptr;
  const uint8_t *verts = nullptr; // vert21 region inside dst
  int vertRegionStart = 0;
  int treeBytes = 0;
  int vertStride = 8;
  int leafSize = BVH_BLAS_LEAF_SIZE;
  uint32_t vertCount = 0;
  int dstO = 0;
  // Set by node() to the just-parsed subtree: true if it is a RAW no-hit leaf, or a chain of 1-child
  // internals terminating in one. The top-level loop uses it to refuse such a shape as the whole BLAS.
  bool subtreeIsRawChain = false;

  // Reads one node from the stream, writes it (and its subtree) into dst, returns its box-space AABB.
  bbox3f node(int depth)
  {
    // Untrusted input: a crafted single-child chain recurses far deeper than any real BLAS, so bail
    // past the cap instead of overflowing the native stack.
    if (depth > BVH_IO_MAX_TREE_DEPTH)
      bvhIoBail(crd, "bvhIO: tree depth exceeds limit");
    uint8_t cc;
    crd.read(&cc, 1);

    if (bvhIoIsLeafMarker(cc)) // ---- leaf ----
    {
      if (dstO + leafSize > treeBytes)
        bvhIoBail(crd, "bvhIO: leaf overruns tree region");
      uint32_t body[4];
      bvhIoReadLeafRecord(crd, cc, body);
      subtreeIsRawChain = (cc == RAW_LEAF_MARKER);
      const uint32_t w0 = body[0], w2 = body[2], w3 = body[3];
      const int leafOfs = dstO + BVH_BLAS_NODE_SIZE;

      bbox3f box;
      v_bbox3_init_empty(box);
      uint32_t w1out = body[1]; // RAW_LEAF_MARKER leaves keep W1 verbatim and an empty (never-entered) box
      if (cc != RAW_LEAF_MARKER)
      {
        const BvhIoLeafBox lb = bvhIoLeafBox(crd, verts, vertCount, vertStride, w0, body[1], w2, w3);
        box = lb.box;
        const int relBase = vertRegionStart + int(lb.vertIdx) * vertStride - leafOfs;
        G_ASSERT(relBase >= 0 && relBase <= QUAD_BASE_BYTE_MAX); // total <= QUAD_BASE_BYTE_MAX (the size cap) keeps it in reach
        w1out = (uint32_t(relBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK) | (body[1] & ~QUAD_BASE_MASK);
      }

      writeQuadBox(dst, dstO, box.bmin, box.bmax, v_splats(1.f), v_zero(), w0, /*useHalves*/ false);
      memcpy(dst + leafOfs + 0, &w1out, 4);
      memcpy(dst + leafOfs + 4, &w2, 4);
      memcpy(dst + leafOfs + 8, &w3, 4);
      dstO += leafSize;
      return box;
    }

    // ---- internal ----
    if (dstO + BVH_BLAS_NODE_SIZE > treeBytes)
      bvhIoBail(crd, "bvhIO: node overruns tree region");
    const int hdr = dstO;
    dstO += BVH_BLAS_NODE_SIZE;
    const int childrenStart = dstO;
    bbox3f box;
    v_bbox3_init_empty(box);
    for (int i = 0; i < cc; i++)
    {
      bbox3f cb = node(depth + 1);
      v_bbox3_add_box(box, cb);
    }
    // After the loop subtreeIsRawChain holds the last child's value; a 1-child node inherits it (a RAW
    // chain stays a RAW chain), a >=2-child node breaks it (its box unions real geometry).
    subtreeIsRawChain = (cc == 1) && subtreeIsRawChain;
    writeQuadBox(dst, hdr, box.bmin, box.bmax, v_splats(1.f), v_zero(), uint32_t(dstO - childrenStart), false);
    return box;
  }
};
} // namespace

BlasDeserializeResult deserializeQuadBLAS(IGenLoad &crd, dag::Vector<uint8_t> &out)
{
  BlasIoHeader h;
  crd.read(&h, sizeof(h));
  if (h.magic != BVH_IO_MAGIC || h.version != BVH_IO_VERSION)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: bad BLAS magic/version", -1)); // decompressor streams fatal on tell()
  if (h.vertStride != 8 || h.leafSize != BVH_BLAS_LEAF_SIZE)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: unsupported BLAS layout", -1));

  const int vertStride = h.vertStride;
  const int leafSize = h.leafSize;

  // Size the one output buffer in 64-bit and reject anything absurd BEFORE allocating: a corrupt header
  // must throw, never overflow the size math or drive a wild resize()/read(). The disk verts are tight;
  // the reader re-aligns the vert21 region to align8(treeBytes) (the runtime canonical layout).
  const int64_t treeBytes = h.treeBytes;
  const int64_t vertsOfs = (treeBytes + 7) & ~int64_t(7);
  const int64_t vertBytes = int64_t(h.vertCount) * vertStride;
  const int64_t total = vertsOfs + vertBytes;
  if (treeBytes <= 0 || h.vertCount == 0 || total > BVH_IO_MAX_BLAS_BYTES)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: BLAS size out of range", -1));

  out.resize(size_t(total));
  memset(out.data() + treeBytes, 0, size_t(vertsOfs - treeBytes)); // zero the up-to-7 re-align pad bytes

  crd.read(out.data() + size_t(vertsOfs), int(vertBytes)); // verts verbatim (tight on disk); fits int by the cap

  Rebuilder rb{crd};
  rb.dst = out.data();
  rb.verts = out.data() + size_t(vertsOfs);
  rb.vertRegionStart = int(vertsOfs);
  rb.treeBytes = int(treeBytes);
  rb.vertStride = vertStride;
  rb.leafSize = leafSize;
  rb.vertCount = h.vertCount;
  int topCount = 0;
  bool rootRaw = false;
  while (rb.dstO < rb.treeBytes) // forest of top-level subtrees, laid out contiguously
  {
    rb.node(0);
    ++topCount;
    rootRaw = rb.subtreeIsRawChain;
  }
  if (rb.dstO != rb.treeBytes)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: tree stream inconsistent with header", crd.tell()));
  // A no-hit leaf reached as the whole-BLAS effective root -- bare, or through a chain of 1-child
  // internals -- rebuilds a tree that traces nothing; the SoA4 side stores such a root boxless and
  // refuses it, so refuse it here too (matches deserializeQuadBLASToSoA4's degenerate-root rejection).
  if (topCount == 1 && rootRaw)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: no-hit leaf as whole-BLAS root", crd.tell()));

  BlasDeserializeResult res;
  res.treeBytes = int(treeBytes);
  res.vertsOfs = int(vertsOfs);
  res.vertCount = int(h.vertCount);
  res.box.bmin = v_make_vec4f(h.bmin[0], h.bmin[1], h.bmin[2], 0.f);
  res.box.bmax = v_make_vec4f(h.bmax[0], h.bmax[1], h.bmax[2], 0.f);
  return res;
}

// ============================================================================
// Direct compact -> SoA4 deserialize. CollisionResource CPU trees are SoA4, so a tool/load path should
// not pay the deserializeQuadBLAS (stackless) + soa4::buildFromStackless detour, nor its transient
// stackless peak memory. This produces exactly the buffer that two-step path would (both are
// deterministic, so byte-equality proves it): the stream is parsed once into a small node table with
// the same hardening the stackless Rebuilder applies (same violations throw the same LoadException),
// each node's float box is accumulated bottom-up exactly as Rebuilder::node does, then quantized
// through the same writeQuadBox; the SoA4 tree is sized and emitted like soa4::Builder, with leaf apex
// bases re-pointed straight from the disk vertex index to the final SoA4 body offset.
// The emit/size mirror of soa4::Builder is DELIBERATELY independent (no shared core): the
// byte-equality gate compares the two paths as separate implementations, which is what lets it
// catch an emit or size slip in either; a shared core would make any bug identical on both sides.
// ============================================================================

namespace
{
// One entry per stream node. Children are consecutive in pre-order, so a node's k-th child is reached
// by walking subtreeEnd from firstChild -- no separate child-index arena.
struct Soa4Node // -V730 box is set before every read
{
  bbox3f box;                      // pre-quantization float box: leaf = its vert21 AABB, internal = union of child boxes
  int subtreeEnd = 0;              // exclusive index one past this node's last descendant (== idx+1 for a leaf)
  int childCount = 0;              // stream fanout: 0 leaf, >= 1 internal
  uint32_t w0 = 0;                 // leaf skip word (bit 31 = QUAD_LEAF_FLAG)
  uint32_t w1 = 0, w2 = 0, w3 = 0; // leaf words as on disk: w1 low 24 = base VERTEX INDEX, high 8 = o3A high
  uint32_t vertIdx = 0;            // decoded base vertex (leaf, non-RAW)
  uint8_t marker = 0;              // 0 full / SHORT_LEAF_MARKER / SHORT_LEAF_FLIP_MARKER / RAW_LEAF_MARKER (leaf only)
  bool isLeaf = false;
};

struct Soa4Deserializer
{
  IGenLoad &crd;
  const uint8_t *verts = nullptr; // buffered disk verts (they arrive before the tree)
  uint32_t vertCount = 0;
  int vertStride = 8;
  int stacklessTreeBytes = 0; // h.treeBytes: the equivalent stackless tree size; bounds the parse
  int stacklessOfs = 0;       // running virtual stackless offset for the same overrun/consistency checks
  dag::Vector<Soa4Node> nodes;
  dag::Vector<int> topLevel; // the root's children (the boxless forest = the SoA4 root node's children)
  // counts for the SoA4 size formula (mirror soa4::Builder::sizeTree)
  int leafCount = 0, shortMarkerCount = 0, internalCount = 0, oneChildInternalCount = 0;

  // emit state
  dag::Vector<uint8_t> *out = nullptr;
  int vertsOfs = 0;
  int treeBytesLimit = 0;   // sized SoA4 tree region; every emitted block must stay inside it
  bool shortEnabled = true; // short bodies need the [tree][verts] span within the 23-bit base reach
  bool failed = false;

  Soa4Deserializer(IGenLoad &c) : crd(c) {}

  // Parse one stream node (and its subtree) into `nodes`, returning its index. Mirrors Rebuilder::node's
  // read order, hardening and box accumulation; the virtual stacklessOfs reproduces its overrun checks.
  int parseNode(int depth)
  {
    if (depth > BVH_IO_MAX_TREE_DEPTH)
      bvhIoBail(crd, "bvhIO: tree depth exceeds limit");
    uint8_t cc;
    crd.read(&cc, 1);

    if (bvhIoIsLeafMarker(cc)) // ---- leaf ----
    {
      if (stacklessOfs + BVH_BLAS_LEAF_SIZE > stacklessTreeBytes)
        bvhIoBail(crd, "bvhIO: leaf overruns tree region");
      uint32_t body[4];
      bvhIoReadLeafRecord(crd, cc, body);
      const int idx = (int)nodes.size();
      Soa4Node nd;
      nd.isLeaf = true;
      nd.marker = cc;
      nd.subtreeEnd = idx + 1;
      nd.w0 = body[0];
      nd.w1 = body[1];
      nd.w2 = body[2];
      nd.w3 = body[3];
      if (cc == RAW_LEAF_MARKER) // degenerate no-hit leaf: no valid apex, empty box, carried verbatim
        v_bbox3_init_empty(nd.box);
      else
      {
        const BvhIoLeafBox lb = bvhIoLeafBox(crd, verts, vertCount, vertStride, body[0], body[1], body[2], body[3]);
        nd.box = lb.box;
        nd.vertIdx = lb.vertIdx;
        if (cc == SHORT_LEAF_MARKER || cc == SHORT_LEAF_FLIP_MARKER)
          ++shortMarkerCount;
      }
      ++leafCount;
      stacklessOfs += BVH_BLAS_LEAF_SIZE;
      nodes.push_back(nd);
      return idx;
    }

    // ---- internal ----
    if (stacklessOfs + BVH_BLAS_NODE_SIZE > stacklessTreeBytes)
      bvhIoBail(crd, "bvhIO: node overruns tree region");
    stacklessOfs += BVH_BLAS_NODE_SIZE;
    const int n = cc; // 1..MAX_CHILD_COUNT; a fanout > 4 is un-representable and rejected at emit
    const int idx = (int)nodes.size();
    {
      Soa4Node nd;
      nd.childCount = n;
      v_bbox3_init_empty(nd.box);
      nodes.push_back(nd); // reserve; recursion may realloc `nodes`, so hold the index, not a pointer
    }
    ++internalCount;
    if (n == 1)
      ++oneChildInternalCount;
    bbox3f box;
    v_bbox3_init_empty(box);
    for (int i = 0; i < n; ++i)
    {
      const int c = parseNode(depth + 1);
      v_bbox3_add_box(box, nodes[c].box);
    }
    nodes[idx].box = box;
    nodes[idx].subtreeEnd = (int)nodes.size();
    return idx;
  }

  // A short leaf writes a 4-byte body iff short encoding is enabled globally; the marker (not W2/W3
  // bits) is the decisive test, since serialize picks the short marker exactly for the canonical case.
  bool leafIsShort(int idx) const
  {
    const uint8_t m = nodes[idx].marker;
    return shortEnabled && (m == SHORT_LEAF_MARKER || m == SHORT_LEAF_FLIP_MARKER);
  }

  // Resolve a child through 1-child chains to the leaf or >= 2-child node behind it (SAH never emits
  // such chains; kept so any input converts identically to soa4::Builder::resolveChild). The promotion
  // is bounded by soa4::MAX_TREE_DEPTH -- the same cap soa4Convert.cpp resolveChild uses -- so a chain
  // that collapses past it soft-fails on both paths alike (an invalid root, no throw) rather than the
  // direct path accepting a tree the converter rejects.
  int resolveChildIdx(int idx)
  {
    for (int guard = 0; guard <= soa4::MAX_TREE_DEPTH; ++guard)
    {
      const Soa4Node &nd = nodes[idx];
      if (nd.isLeaf || nd.childCount >= 2)
        return idx;
      idx = idx + 1; // lone child sits right after its parent in pre-order
    }
    failed = true;
    return -1;
  }

  // Collect the child indices of internal node `idx` (fanout <= 4). Returns false on a wider node, which
  // the SoA4 child-ref encoding cannot represent (soa4::Builder caps every span at 4).
  bool collectChildren(int idx, int out4[4], int &out_n)
  {
    const int n = nodes[idx].childCount;
    if (n > 4)
      return false;
    int c = idx + 1;
    for (int i = 0; i < n; ++i)
    {
      out4[i] = c;
      c = nodes[c].subtreeEnd;
    }
    out_n = n;
    return true;
  }

  // Quantize a float box through writeQuadBox (the same path the stackless rebuild uses) and scatter its
  // uint16s into child lane `i` of SoA node `p` (axis stride s2), exactly as soa4::Builder::emitSpan does.
  static void writeSoaBoxLane(uint8_t *p, int s2, int i, bbox3f box)
  {
    alignas(16) uint8_t hdr[BVH_BLAS_NODE_SIZE]; // [min.x|max.x][min.y|max.y][min.z|max.z][skip]
    writeQuadBox(hdr, 0, box.bmin, box.bmax, v_splats(1.f), v_zero(), 0u, /*useHalves*/ false);
    *(uint16_t *)(p + 0 * s2 + i * 2) = *(const uint16_t *)(hdr + 0);
    *(uint16_t *)(p + 1 * s2 + i * 2) = *(const uint16_t *)(hdr + 4);
    *(uint16_t *)(p + 2 * s2 + i * 2) = *(const uint16_t *)(hdr + 8);
    *(uint16_t *)(p + 3 * s2 + i * 2) = *(const uint16_t *)(hdr + 2);
    *(uint16_t *)(p + 4 * s2 + i * 2) = *(const uint16_t *)(hdr + 6);
    *(uint16_t *)(p + 5 * s2 + i * 2) = *(const uint16_t *)(hdr + 10);
  }

  // Emit leaf `idx`'s inline body at `body_ofs`, re-pointing the quad-A apex base to the final vert
  // region. Full/short/degenerate handling matches soa4::Builder::emitSpan.
  void emitLeafBody(int body_ofs, int idx, bool is_short)
  {
    const Soa4Node &nd = nodes[idx];
    uint32_t *b = (uint32_t *)(out->data() + body_ofs);
    if (nd.marker == RAW_LEAF_MARKER) // degenerate no-hit leaf: full body, verbatim, never re-pointed
    {
      b[0] = nd.w1;
      b[1] = nd.w2;
      b[2] = nd.w3;
      return;
    }
    const uint32_t o3Ahi = nd.w1 & ~QUAD_BASE_MASK;
    const int newRelBase = (vertsOfs + (int)nd.vertIdx * vertStride) - body_ofs;
    if (is_short) // 4B body: W1 with bit 23 = flipA, base truncated to 23 bits (shortEnabled guarantees reach)
    {
      if (DAGOR_UNLIKELY((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) >= soa4::SHORT_W1_FLIP))
      {
        failed = true;
        return;
      }
      const uint32_t flip = (nd.w2 & QUAD_FLIPA_FLAG) ? soa4::SHORT_W1_FLIP : 0u;
      b[0] = o3Ahi | flip | (uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT);
    }
    else
    {
      b[0] = o3Ahi | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK);
      b[1] = nd.w2;
      b[2] = nd.w3;
    }
  }

  // Emit the SoA4 node for a span of sibling nodes (top-level forest, or one internal node's children):
  // n child boxes (SoA), n child words (leaf -> W0, internal -> tagged offset), then inline leaf bodies.
  // Returns its tagged pointer, or QUAD_LEAF_FLAG | nodeIdx to bubble a lone leaf up. Mirrors
  // soa4::Builder::emitSpan (pre-order, running offset), reading the parsed table instead of a stackless buffer.
  uint32_t emitSpan(const int *span, int span_count, int &cur, int depth)
  {
    if (DAGOR_UNLIKELY(failed))
      return 0;
    // A SoA4 tree deeper than the walker stack bound cannot be safely traversed, so reject it at the
    // trust boundary like any other bad-stream condition. The check lives on the EMITTED depth (not the
    // looser BVH_IO_MAX_TREE_DEPTH parse cap): 1-child promotion can shrink emit depth below the stream's.
    if (DAGOR_UNLIKELY(depth > soa4::MAX_TREE_DEPTH))
      bvhIoBail(crd, "bvhIO: SoA4 tree depth exceeds walker stack bound");
    int childRes[4];
    unsigned shortMask = 0;
    int n = 0, fullK = 0, shortK = 0;
    for (int ci = 0; ci < span_count; ++ci, ++n)
    {
      if (DAGOR_UNLIKELY(n == 4)) // create_bvh_node_sah caps every span fanout at 4; a wider one cannot be encoded
      {
        failed = true;
        return 0;
      }
      const int r = resolveChildIdx(span[ci]);
      if (DAGOR_UNLIKELY(r < 0))
        return 0;
      childRes[n] = r;
      if (nodes[r].isLeaf)
      {
        if (leafIsShort(r))
        {
          shortMask |= 1u << n;
          ++shortK;
        }
        else
          ++fullK;
      }
    }
    if (n == 1)
    {
      if (nodes[childRes[0]].isLeaf)
        return QUAD_LEAF_FLAG | (uint32_t)childRes[0]; // lone leaf: bubble to the parent / degenerate root
      int cbuf[4], cn;
      if (DAGOR_UNLIKELY(!collectChildren(childRes[0], cbuf, cn)))
      {
        failed = true;
        return 0;
      }
      return emitSpan(cbuf, cn, cur, depth + 1);
    }
    const int size = 16 * n + 12 * fullK + 4 * shortK;
    const int nodeOfs = cur;
    // Node offsets must fit the child-ref [25:2] field and the 23-bit LeafRef parent field, and the
    // whole block must land inside the sized tree region -- every count/offset in the stream is data,
    // so never write past the buffer even if the analytic size disagreed (the cur != treeBytes
    // reconciliation in build() only runs after the fact).
    if (DAGOR_UNLIKELY((uint32_t)nodeOfs >= (1u << 25) || nodeOfs + size > treeBytesLimit))
    {
      failed = true;
      return 0;
    }
    const int s2 = n * 2; // bytes per SoA axis-array
    cur += size;
    int bodyOfs = nodeOfs + 16 * n; // inline leaf bodies follow the child words, in lane order
    uint8_t *p = out->data() + nodeOfs;
    for (int i = 0; i < n; ++i)
    {
      const int ri = childRes[i];
      writeSoaBoxLane(p, s2, i, nodes[ri].box);
      uint32_t word;
      if (nodes[ri].isLeaf)
      {
        word = nodes[ri].w0; // leaf child: the word IS its W0 (bit 31 set)
        const bool isShort = (shortMask >> i) & 1;
        emitLeafBody(bodyOfs, ri, isShort);
        bodyOfs += isShort ? 4 : 12;
      }
      else
      {
        int cbuf[4], cn;
        if (DAGOR_UNLIKELY(!collectChildren(ri, cbuf, cn)))
        {
          failed = true;
          return 0;
        }
        word = emitSpan(cbuf, cn, cur, depth + 1);
      }
      *(uint32_t *)(p + 6 * s2 + i * 4) = word;
    }
    return (uint32_t)nodeOfs | (uint32_t)(n - 1) | (shortMask << soa4::PTR_SHORT_SHIFT);
  }

  // Two passes over the parsed table: analytic size (mirrors Builder::sizeTree), then one pre-order emit.
  Soa4DeserializeResult build()
  {
    Soa4DeserializeResult res; // res.root defaults to invalid()

    stacklessOfs = 0;
    // Every parsed node advances stacklessOfs (overrun bails), so nodes <= treeBytes/16; reserve
    // for the legitimate leaf-dominated case (28 stackless bytes each) to avoid doubling churn.
    nodes.reserve(stacklessTreeBytes / BVH_BLAS_LEAF_SIZE + 1);
    while (stacklessOfs < stacklessTreeBytes) // forest of top-level subtrees, laid out contiguously
      topLevel.push_back(parseNode(0));
    if (stacklessOfs != stacklessTreeBytes)
      bvhIoBail(crd, "bvhIO: tree stream inconsistent with header");

    // The LeafRef encoding carries a 23-bit (32MB) parent node offset -- a larger tree is not
    // representable. Not corrupt input, so return an invalid root rather than throw (matches Builder).
    if ((int64_t)stacklessTreeBytes + soa4::LEAF_BYTES > (int64_t)soa4::LEAF_ENTRY_OFS_MASK)
      return res;
    const int64_t vertBytes = (int64_t)vertCount * vertStride;
    // Short bodies address verts through a 23-bit base: only safe when the whole [tree][verts] span fits
    // it. Gate on the stackless size (>= the SoA4 tree), byte-for-byte as Builder does with blasSize.
    shortEnabled = (int64_t)stacklessTreeBytes + soa4::LEAF_BYTES + vertBytes <= (int64_t)(soa4::SHORT_W1_FLIP - 1)
                                                                                   << QUAD_BASE_ALIGN_SHIFT;

    // SoA4 tree bytes = 16*slots + 12*Lfull + 4*Lshort, slots = L + I - P1 (see Builder::sizeTree): every
    // leaf/internal is a child slot of exactly one span except the promoted lone child of a 1-child span.
    const int Ls = shortEnabled ? shortMarkerCount : 0;
    const int P1 = oneChildInternalCount + (topLevel.size() == 1 ? 1 : 0);
    const int slots = leafCount + internalCount - P1;
    const int treeBytes = slots > 0 ? 16 * slots + 12 * (leafCount - Ls) + 4 * Ls : (int)soa4::LEAF_BYTES;
    treeBytesLimit = treeBytes;      // the emit write-bound (mirrors soa4::Builder)
    vertsOfs = (treeBytes + 7) & ~7; // 8-align the vert21 region (tree blocks are only 4-aligned)
    out->resize_noinit(size_t(vertsOfs) + size_t(vertBytes));
    if (vertsOfs > treeBytes)
      memset(out->data() + treeBytes, 0, size_t(vertsOfs - treeBytes)); // defined pad: byte-reproducible

    int cur = 0;
    uint32_t rootRef = emitSpan(topLevel.data(), (int)topLevel.size(), cur, 0);
    if (failed)
      return res;
    if (rootRef & QUAD_LEAF_FLAG)
    {
      // Degenerate whole-BLAS-is-one-leaf (never for real meshes): a lone 16B [W0 W1 W2 W3] block,
      // referenced with tag 0 and handled by the dedicated pop path in the SoA4 traversals.
      if (DAGOR_UNLIKELY(cur + soa4::LEAF_BYTES > treeBytesLimit)) // never write past the sized buffer
        return res;
      const Soa4Node &nd = nodes[(int)(rootRef & ~QUAD_LEAF_FLAG)];
      // A RAW no-hit leaf can never form a root: the boxless root block would hand its body to the
      // vertex-decoding walkers, and an empty single-no-hit-leaf BLAS is meaningless. Covers lone
      // roots and one-child chains promoted into the root; matches the stackless-path rejection.
      if (nd.marker == RAW_LEAF_MARKER)
        bvhIoBail(crd, "bvhIO: no-hit leaf as whole-BLAS root");
      const int newRelBase = (vertsOfs + (int)nd.vertIdx * vertStride) - (cur + 4);
      uint32_t *w = (uint32_t *)(out->data() + cur);
      w[0] = nd.w0;
      w[1] = (nd.w1 & ~QUAD_BASE_MASK) | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK);
      w[2] = nd.w2;
      w[3] = nd.w3;
      rootRef = (uint32_t)cur; // tag 0
      cur += soa4::LEAF_BYTES;
    }
    if (cur != treeBytes) // emit/sizing mismatch: structural corruption, refuse the buffer
      return res;
    memcpy(out->data() + vertsOfs, verts, size_t(vertBytes));

    res.root.v = (int32_t)rootRef;
    res.treeBytes = treeBytes;
    res.vertsOfs = vertsOfs;
    res.vertCount = (int)vertCount;
    return res;
  }
};
} // namespace

// counts the bytes a parse actually consumed: the on-wire tree is variable-length (marker bytes
// plus per-marker bodies), so no header field states it, and decompressor streams fatal on tell()
struct CountingLoad final : public IGenLoad
{
  IGenLoad &c;
  int64_t consumed = 0;
  CountingLoad(IGenLoad &in_c) : c(in_c) {}
  void read(void *ptr, int size) override
  {
    c.read(ptr, size);
    consumed += size;
  }
  int tryRead(void *ptr, int size) override
  {
    const int rd = c.tryRead(ptr, size);
    consumed += rd;
    return rd;
  }
  int tell() override { return (int)consumed; }
  void seekto(int) override { DAG_FATAL("CountingLoad: no seek"); }
  void seekrel(int ofs) override
  {
    G_ASSERT(ofs >= 0);
    char buf[256];
    for (; ofs > 0; ofs -= (int)sizeof(buf))
      read(buf, ofs < (int)sizeof(buf) ? ofs : (int)sizeof(buf));
  }
  const char *getTargetName() override { return c.getTargetName(); }
  int beginBlock(unsigned * = nullptr) override
  {
    DAG_FATAL("CountingLoad: no blocks");
    return 0;
  }
  void endBlock() override {}
  int getBlockLength() override { return 0; }
  int getBlockRest() override { return 0; }
  int getBlockLevel() override { return 0; }
};

Soa4DeserializeResult deserializeQuadBLASToSoA4(IGenLoad &in_crd, dag::Vector<uint8_t> &out)
{
  CountingLoad crd(in_crd);
  BlasIoHeader h;
  crd.read(&h, sizeof(h));
  if (h.magic != BVH_IO_MAGIC || h.version != BVH_IO_VERSION)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: bad BLAS magic/version", -1)); // decompressor streams fatal on tell()
  if (h.vertStride != 8 || h.leafSize != BVH_BLAS_LEAF_SIZE)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: unsupported BLAS layout", -1));

  const int vertStride = h.vertStride;
  // Reject an absurd header BEFORE allocating, in 64-bit, exactly as deserializeQuadBLAS does (the SoA4
  // tree is never larger than the stackless one this bounds).
  const int64_t treeBytes = h.treeBytes;
  const int64_t vertsOfs = (treeBytes + 7) & ~int64_t(7);
  const int64_t vertBytes = int64_t(h.vertCount) * vertStride;
  if (treeBytes <= 0 || h.vertCount == 0 || vertsOfs + vertBytes > BVH_IO_MAX_BLAS_BYTES)
    DAGOR_THROW(IGenLoad::LoadException("bvhIO: BLAS size out of range", -1));

  // Verts arrive before the tree; buffer them (a fraction of the stackless detour's peak) so leaf boxes
  // can be rebuilt during the single parse.
  dag::Vector<uint8_t> vertsBuf;
  vertsBuf.resize(size_t(vertBytes));
  crd.read(vertsBuf.data(), int(vertBytes)); // tight on disk; fits int by the cap

  Soa4Deserializer d(crd);
  d.verts = vertsBuf.data();
  d.vertCount = h.vertCount;
  d.vertStride = vertStride;
  d.stacklessTreeBytes = int(treeBytes);
  d.out = &out;
  Soa4DeserializeResult res = d.build();
  res.box.bmin = v_make_vec4f(h.bmin[0], h.bmin[1], h.bmin[2], 0.f);
  res.box.bmax = v_make_vec4f(h.bmax[0], h.bmax[1], h.bmax[2], 0.f);
  res.serializedBytes = int(crd.consumed); // header + verts + the variable-length wire tree
  return res;
}

} // namespace build_bvh
