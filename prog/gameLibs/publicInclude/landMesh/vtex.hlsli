#define TILE_WIDTH (128) // size of indirection
#define TILE_WIDTH_BITS (7)
#define TEX_MIPS (7)

#ifdef CLIPMAP_USE_RI_VTEX
  // (!) modify the dagor shader constants in rendinst_clipmap.dshl if you change it (!)
  #define MAX_RI_VTEX_CNT_BITS (3)
#else
  #define MAX_RI_VTEX_CNT_BITS (0)
#endif

#define MAX_VTEX_CNT (1 << MAX_RI_VTEX_CNT_BITS)
#define MAX_RI_VTEX_CNT (MAX_VTEX_CNT - 1) // excluding terrain

#define TILE_PACKED_X_BITS (7)
#define TILE_PACKED_Y_BITS (7)
#define TILE_PACKED_MIP_BITS (3)
#define TILE_PACKED_COUNT_BITS (32 - (TILE_PACKED_X_BITS + TILE_PACKED_Y_BITS + TILE_PACKED_MIP_BITS))

#define CLIPMAP_HIST_TOTAL_ELEMENTS (TILE_WIDTH * TILE_WIDTH * TEX_MIPS * MAX_VTEX_CNT)

#define CLIPMAP_INVALID_FEEDBACK_CLEAR 0x00000000 // invalid data, with dummy ri_offset==0 value (it can never occur in real feedback, only from clearing)

