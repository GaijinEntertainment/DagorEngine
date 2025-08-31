#ifndef INDOOR_PROBE_CONST_HLSL
#define INDOOR_PROBE_CONST_HLSL

static const uint INVALID_CUBE_ID = 255;
static const uint GRID_SIDE = 32;
static const uint MAX_ACTIVE_PROBES = 32;
static const uint QUARTER_OF_ACTIVE_PROBES = 8;
static const uint PROBES_IN_CELL = 8;
static const uint NON_CELL_PROBES_COUNT = 4;
static const float PROBE_BOX_OFFSET_FOR_SMALL_BOX = -0.25; //(in meter)
static const float PROBE_BOX_OFFSET_FOR_BIG_BOX = 0.25; //(in meter)
static const float PROBE_BOX_PADDING = 0.2f; //(in meter) for continuous transition between outside/inside the box

static const float GRID_CELL_SIZE = 6;
static const float GRID_CELL_HEIGHT = 3;
static const int GRID_HEIGHT = 8;
static const float SKY_PROBE_FADE_DIST = 0.05; // defined as fraction of cell dimensions
static const float SKY_PROBE_FADE_DIST_WS = 4.0; // defined in world-space units
static const float PROBE_BOX_DIR_FADEOUT_DIST_START = 0;
static const float PROBE_BOX_DIR_FADEOUT_DIST_END = 0.2;

static const uint INVALID_ENVI_PROBE_TYPE = 15;
static const uint ENVI_PROBE_CUBE_TYPE = 0;
static const uint ENVI_PROBE_CYLINDER_TYPE = 1;
static const uint ENVI_PROBE_CAPSULE_TYPE = 2;
//static const uint ENVI_PROBE_HALF_CAPSULE_TYPE = 3;

#endif
