#ifndef SW_BLAS_LEAF_DEFS_HLSLI
#define SW_BLAS_LEAF_DEFS_HLSLI 1

// Double-quad leaf bit layout (delta-B), shared by CPU dag_swBLAS_ray.h, GPU swBLAS.dshl and the
// quadBLASBuilder. A leaf packs up to two strip quads = up to 4 triangles: 16-byte box node + W1 +
// W2 + W3 (BVH_BLAS_LEAF_SIZE bytes total).
//
// Offsets are SIGNED 13-bit vertex-index deltas from each quad's base vertex, so the decode needs a
// single strip topology -- triA = (v0,v1,v2), triB = (v1,v2,v3) -- with no fan/strip branch. The base
// vertex is the quad's apex (NOT min index); signedness is what removes the v0=min constraint. The
// 2nd triangle's winding is set by flipSecond (triB = flip ? (v2,v1,v3) : (v1,v2,v3)); it matters only
// to cull/closest-hit and is ignored by shadow/distance (any-hit is winding-independent).
//
// W0 (skip word, leaf+12) -- quad A offsets:
//   bit 31:     QUAD_LEAF_FLAG
//   bits 0-12:  o1A (13, signed)
//   bits 13-25: o2A (13, signed)
//   bits 26-30: o3A low 5
// W1 (leaf+16):
//   bits 0-23:  baseA (24, unsigned; quad A base byte offset >> 2, relative to leaf+16 = dataOffset; see below)
//   bits 24-31: o3A high 8         -- o3A = (o3A_lo5) | (o3A_hi8 << 5), 13-bit signed
// W2 (leaf+20):
//   bits 0-15:  deltaB (16, unsigned; quad B base - quad A base in vertex units, B sorted to >= A)
//   bits 16-28: o1B (13, signed)
//   bit 29:     flipA
//   bit 30:     flipB
//   bit 31:     spare (reserved)
// W3 (leaf+24):
//   bits 0-12:  o2B (13, signed)
//   bits 13-25: o3B (13, signed)
//   bits 26-31: user bits (see QUAD_LEAF_USER_*)
//
// Sentinels (no dedicated flag bits): hasB = (o1B != o2B); singleA = (o3A == o2A); singleB = (o3B == o2B).

#define QUAD_LEAF_FLAG   (1u << 31)

// Signed 13-bit offset fields: range [-4096, 4095].
#define QUAD_O_BITS      13
#define QUAD_O_MASK      ((1u << QUAD_O_BITS) - 1)         // 0x1FFF
#define QUAD_O_MAX       ((1 << (QUAD_O_BITS - 1)) - 1)    // 4095
#define QUAD_O_MIN       (-(1 << (QUAD_O_BITS - 1)))       // -4096
#define QUAD_O2_SHIFT    QUAD_O_BITS                       // W0[13:25] o2A
#define QUAD_O3_LO_SHIFT (2 * QUAD_O_BITS)                 // W0[26:30] o3A low 5
#define QUAD_O3_LO_BITS  5
#define QUAD_O3_LO_MASK  ((1u << QUAD_O3_LO_BITS) - 1)     // 0x1F
#define QUAD_O3_HI_SHIFT 24                                // W1[24:31] o3A high 8

// Quad A base: 24-bit byte offset >> 2 (4-aligned), UNSIGNED. CONTRACT: vertices always follow the tree
// in the BLAS buffer, so the base is always >= 0; storing it unsigned (rather than wasting a sign bit on
// an offset that is never negative) doubles the addressable [tree][verts] span to ~64 MB. Producers cap
// geometry so no quad base exceeds QUAD_BASE_BYTE_MAX; decoders mask the low QUAD_BASE_BITS, never sign-extend.
#define QUAD_BASE_BITS   24
#define QUAD_BASE_MASK   ((1u << QUAD_BASE_BITS) - 1)      // 0xFFFFFF
#define QUAD_BASE_ALIGN_SHIFT 2                            // stored value is byte offset >> 2
#define QUAD_BASE_BYTE_MAX (((1 << QUAD_BASE_BITS) - 1) << QUAD_BASE_ALIGN_SHIFT) // ~64 MB (24-bit unsigned << 2)

// Quad B (W2/W3).
#define QUADB_BASE_MASK  0xFFFFu                           // W2[0:15] deltaB (vert units, unsigned)
#define QUADB_BASE_MAX   QUADB_BASE_MASK
#define QUADB_O1_SHIFT   16                                // W2[16:28] o1B
#define QUAD_FLIPA_FLAG  (1u << 29)                        // W2 bit 29
#define QUAD_FLIPB_FLAG  (1u << 30)                        // W2 bit 30
#define QUADB_O3_SHIFT   QUAD_O_BITS                       // W3[13:25] o3B (W3[0:12] = o2B)

// Per-leaf user bits (W3[26:31]): 6 bits daBVH stores and returns but never interprets -- what a
// value means belongs to the BLAS owner. The builder guarantees only that one leaf carries one
// value (primitives with different values never share a leaf). Every other field masks these bits
// out, so a leaf written before this existed reads as 0, the "unset" value.
// Cost: a nonzero value keeps a single-quad leaf out of the short-body forms (SoA4 4-byte body,
// bvhIO 8-byte record), so owners should give their most common meaning the value 0.
#define QUAD_LEAF_USER_SHIFT 26
#define QUAD_LEAF_USER_BITS  6
#define QUAD_LEAF_USER_MASK  ((1u << QUAD_LEAF_USER_BITS) - 1) // 0x3F

#define BVH_BLAS_NODE_SIZE 16
#define BVH_BLAS_LEAF_SIZE 28

#endif
