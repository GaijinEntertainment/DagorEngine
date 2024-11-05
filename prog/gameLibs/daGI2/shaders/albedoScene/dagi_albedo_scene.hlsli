#define DAGI_ALBEDO_ALLOW_NON_POW2 1
#define DAGI_MAX_ALBEDO_CLIPS 4
#define DAGI_ALBEDO_BLOCK_SHIFT 3
#define DAGI_ALBEDO_BLOCK_SIZE (1<<DAGI_ALBEDO_BLOCK_SHIFT)
#define DAGI_ALBEDO_BLOCK_MASK (DAGI_ALBEDO_BLOCK_SIZE-1)

// smooth trilinear:
// 1) wastes 33% of space in block,
// 2) requires smooth albedo field
// in reality, we dont't have albedo field, (since it is rather noise filling from rasterization and gbuffer)
// so sacrifice this quality for higher density
#define DAGI_ALBEDO_SMOOTH_TRILINEAR 0

#if DAGI_ALBEDO_SMOOTH_TRILINEAR

#define DAGI_ALBEDO_INTERNAL_BLOCK_SIZE (DAGI_ALBEDO_BLOCK_SIZE-1)
#define DAGI_ALBEDO_BORDER 0.5//half texel border, allows trilinear, doesn't allow smooth gradient
#else
#define DAGI_ALBEDO_INTERNAL_BLOCK_SIZE DAGI_ALBEDO_BLOCK_SIZE
#define DAGI_ALBEDO_BORDER 0
#endif