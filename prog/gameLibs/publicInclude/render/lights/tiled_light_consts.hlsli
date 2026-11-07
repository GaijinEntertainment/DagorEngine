#include "renderLightsConsts.hlsli"

static const int DIVIDE_RESOLUTION_BITS = 4;
static const int TILE_EDGE = 1L << DIVIDE_RESOLUTION_BITS;
static const int OMNI_TILES_CUBE_VERTICES = 36;
// Slack for the float error of the light center in clip space far from the origin.
static const float OMNI_TILES_NEAR_CULL_RADIUS_SCALE = 1.01;
static const int BITS_IN_UINT = 32;
static const int OMNI_DWORDS_PER_TILE = (MAX_CLUSTERED_OMNI_LIGHTS + BITS_IN_UINT - 1) / BITS_IN_UINT;
static const int SPOT_DWORDS_PER_TILE = (MAX_CLUSTERED_SPOT_LIGHTS + BITS_IN_UINT - 1) / BITS_IN_UINT;
static const int DWORDS_PER_TILE = (OMNI_DWORDS_PER_TILE + SPOT_DWORDS_PER_TILE);
static const int Z_BINS_COUNT = 4096;
static const int Z_BINNING_GROUP_SIZE = 256;
static const int Z_BINNING_LIGHTS_BLOCK_SIZE = 16;
