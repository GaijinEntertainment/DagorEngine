#define MAX_DEBUG_MIP_LEVELS_DISPLAYED 11
#define MAX_TEXEL_DENSITY_DISPLAYED 10

// gbuff_dbg_mode values selecting what materials write into dbg_gbuff_tex
#define GBUFF_DBG_MODE_WORLD_TEXEL_DENSITY 2

// world texel density is stored as log2(texels per meter) in [0 .. MAX];
// dbg_gbuff_tex clears to 1.0, so values encode to [0 .. MAX/(MAX+1)]
#define DBG_WORLD_TEXEL_DENSITY_LOG2_MAX 16

// distance from the target in octaves: one octave = half or double the texels per meter
#define DBG_DENSITY_OCTAVES_ON_TARGET 0.5
#define DBG_DENSITY_OCTAVES_NEAR 1.5
// past this the scale saturates, so both ends draw as a red/green checker to read as off-scale
#define DBG_DENSITY_OCTAVES_WORST 4.0
#define DBG_DENSITY_CHECKER_TILE 8

#define DBG_DENSITY_COLOR_FAR_UNDER_R 1.0
#define DBG_DENSITY_COLOR_FAR_UNDER_G 0.0
#define DBG_DENSITY_COLOR_FAR_UNDER_B 0.0
#define DBG_DENSITY_COLOR_NEAR_UNDER_R 1.0
#define DBG_DENSITY_COLOR_NEAR_UNDER_G 0.55
#define DBG_DENSITY_COLOR_NEAR_UNDER_B 0.0
#define DBG_DENSITY_COLOR_ON_TARGET_R 1.0
#define DBG_DENSITY_COLOR_ON_TARGET_G 1.0
#define DBG_DENSITY_COLOR_ON_TARGET_B 1.0
#define DBG_DENSITY_COLOR_NEAR_OVER_R 0.0
#define DBG_DENSITY_COLOR_NEAR_OVER_G 1.0
#define DBG_DENSITY_COLOR_NEAR_OVER_B 1.0
#define DBG_DENSITY_COLOR_FAR_OVER_R 0.0
#define DBG_DENSITY_COLOR_FAR_OVER_G 1.0
#define DBG_DENSITY_COLOR_FAR_OVER_B 0.0
#define DBG_DENSITY_COLOR_CHECKER_R 0.35
#define DBG_DENSITY_COLOR_CHECKER_G 0.35
#define DBG_DENSITY_COLOR_CHECKER_B 0.35
