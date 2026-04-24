#ifndef SW_BLAS_LEAF_DEFS_HLSLI
#define SW_BLAS_LEAF_DEFS_HLSLI 1

// Quad leaf bit layout constants (shared between CPU dag_swBLAS_ray.h, GPU swBLAS.dshl, and quadBLASBuilder)
// Layout (32-bit skip word) -- unified for both single triangles and quads:
//   bit 31:    isLeaf (QUAD_LEAF_FLAG)
//   bit 30:    isFan (QUAD_FAN_FLAG) -- fan vs strip topology (only meaningful for quads)
//   bits 0-9:  o1 (10 bits)
//   bits 10-19: o2 (10 bits)
//   bits 20-29: o3 (10 bits) -- for singles, o3 == o2 (v3 == v2, degenerate second tri)
// isSingle is deduced from o3 == o2 (no dedicated bit needed)

#define QUAD_LEAF_FLAG   (1u << 31)
#define QUAD_FAN_FLAG    (1u << 30)

// Quad vertex offset encoding: 10 + 10 + 10 = 30 bits
#define QUAD_O1_BITS     10
#define QUAD_O2_BITS     10
#define QUAD_O3_BITS     10
#define QUAD_O1_MASK     ((1u << QUAD_O1_BITS) - 1)
#define QUAD_O2_SHIFT    QUAD_O1_BITS
#define QUAD_O2_MASK     ((1u << QUAD_O2_BITS) - 1)
#define QUAD_O3_SHIFT    (QUAD_O1_BITS + QUAD_O2_BITS)
#define QUAD_O3_MASK     ((1u << QUAD_O3_BITS) - 1)
#define QUAD_O1_MAX      (QUAD_O1_MASK)
#define QUAD_O2_MAX      (QUAD_O2_MASK)
#define QUAD_O3_MAX      (QUAD_O3_MASK)

#endif
